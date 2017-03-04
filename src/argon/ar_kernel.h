/*
 * Copyright (c) 2007-2017 Immo Software
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its contributors may
 *   be used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*!
 * @file
 * @brief Header for the Argon RTOS C API.
 * @ingroup ar
 */

#if !defined(_AR_KERNEL_H_)
#define _AR_KERNEL_H_

#include "ar_port.h"
#include "src/ar_config.h"

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------

//! @brief Timeout constants.
//!
//! @ingroup ar
enum _ar_timeouts
{
    //! Return immediately if a resource cannot be acquired.
    kArNoTimeout = 0,

    //! Pass this value to wait forever to acquire a resource.
    kArInfiniteTimeout = 0xffffffffUL
};

//! @brief Argon status and error codes.
//!
//! @ingroup ar
typedef enum _ar_status {
    //! Operation was successful.
    kArSuccess = 0,

    //! Timeout while blocked on an object.
    kArTimeoutError,

    //! An object was deleted while a thread was blocked on it. This may be
    //! a semaphore, mutex, or queue.
    kArObjectDeletedError,

    //! The queue is at maximum capacity and cannot accept more elements.
    kArQueueFullError,

    //! No elements are in the queue.
    kArQueueEmptyError,

    //! The requested thread priority is invalid.
    kArInvalidPriorityError,

    //! The thread's stack size is too small.
    kArStackSizeTooSmallError,

    //! The requested operation cannot be performed from interrupt context.
    kArNotFromInterruptError,

    //! The caller is not the owning thread.
    kArNotOwnerError,

    //! The mutex is already unlocked.
    kArAlreadyUnlockedError,

    //! An invalid parameter value was passed to the function.
    kArInvalidParameterError,

    //! The timer is not running.
    kArTimerNotRunningError,

    //! The timer is not associated with a run loop.
    kArTimerNoRunLoop,

    //! Allocation failed.
    kArOutOfMemoryError,

    //! The thread is an invalid state for the given operation.
    kArInvalidStateError,
} ar_status_t;

//! @brief Options for creating a new thread.
//!
//! These constants are meant to be used for the @a startImmediately parameter.
enum _ar_thread_start {
    kArStartThread = true,      //!< Automatically run the thread.
    kArSuspendThread = false    //!< Create the thread suspended.
};

//! @brief Potential thread states.
//!
//! @ingroup ar_thread
typedef enum _ar_thread_state {
    kArThreadUnknown,     //!< Hopefully a thread is never in this state.
    kArThreadSuspended,   //!< Thread is not eligible for execution.
    kArThreadReady,       //!< Thread is eligible to be run.
    kArThreadRunning,     //!< The thread is currently running.
    kArThreadBlocked,     //!< The thread is blocked on another object.
    kArThreadSleeping,    //!< Thread is sleeping.
    kArThreadDone         //!< Thread has exited.
} ar_thread_state_t;

//! @brief Range of priorities for threads.
//!
//! @ingroup ar_thread
enum _ar_thread_priorities
{
    kArIdleThreadPriority = 0,  //!< The idle thread's priority. No other thread is allowed to have this priority.
    kArMinThreadPriority = 1,   //!< Priority value for the lowest priority user thread.
    kArMaxThreadPriority = 255  //!< Priority value for the highest priority user thread.
};

//! @brief Modes of operation for timers.
//!
//! @ingroup ar_timer
typedef enum _ar_timer_modes {
    kArOneShotTimer,      //!< Timer fires a single time.
    kArPeriodicTimer      //!< Timer repeatedly fires every time the interval elapses.
} ar_timer_mode_t;

//! @brief Runloop actions.
typedef enum _ar_runloop_status {
    kArRunLoopError,
    kArRunLoopStopped,
    kArRunLoopQueueReceived,
    kArRunLoopChannelReceived
} ar_runloop_status_t;

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

// Forward declarations.
typedef struct _ar_thread ar_thread_t;
typedef struct _ar_channel ar_channel_t;
typedef struct _ar_queue ar_queue_t;
typedef struct _ar_timer ar_timer_t;
typedef struct _ar_runloop ar_runloop_t;
typedef struct _ar_list_node ar_list_node_t;

//! @name Function types
//@{
//! Function type used for sorting object lists.
typedef bool (*ar_sort_predicate_t)(ar_list_node_t * a, ar_list_node_t * b);

//! @brief Prototype for the thread entry point.
//!
//! @ingroup ar_thread
typedef void (*ar_thread_entry_t)(void * param);

//! @brief Callback routine for timer expiration.
//!
//! @ingroup ar_timer
typedef void (*ar_timer_entry_t)(ar_timer_t * timer, void * param);

//! @brief
typedef void (*ar_runloop_function_t)(void * param);

//! @brief
typedef void (*ar_runloop_queue_handler_t)(ar_queue_t * queue, void * param);

//! @brief
typedef void (*ar_runloop_channel_handler_t)(ar_channel_t * channel, void * param);
//@}

//! @name Linked lists
//@{
/*!
 * @brief Linked list node.
 */
struct _ar_list_node {
    ar_list_node_t * m_next;    //!< Next node in the list.
    ar_list_node_t * m_prev;    //!< Previous node in the list.
    void * m_obj;               //!< Pointer to the object on the list.

    // Internal utility methods.
#if defined(__cplusplus)
    //! @brief Convert the @a m_obj pointer to a particular type.
    template <typename T> T * getObject() { return reinterpret_cast<T *>(m_obj); }
    void insertBefore(ar_list_node_t * node);   //!< @brief Insert this node before another node on the list.
#endif // __cplusplus
};

/*!
 * @brief Linked list.
 */
typedef struct _ar_list {
    ar_list_node_t * m_head;    //!< Pointer to the head of the list. Will be NULL if the list is empty.
    ar_sort_predicate_t m_predicate;    //!< Sort predicate to use for this list. Items are added to the end if NULL.

    // Internal utility methods.
#if defined(__cplusplus)
    inline bool isEmpty() const;                //!< @brief Return whether the list is empty.
    void add(ar_list_node_t * item);            //!< @brief Add an item to the list.
    inline void add(ar_thread_t * item);        //!< @brief Add a thread to the list.
    inline void add(ar_timer_t * item);         //!< @brief Add a timer to the list.
    inline void add(ar_queue_t * item);         //!< @brief Add a queue to the list.
    inline void add(ar_channel_t * item);         //!< @brief Add a channel to the list.
    void remove(ar_list_node_t * item);         //!< @brief Remove an item from the list.
    inline void remove(ar_thread_t * item);     //!< @brief Remove a thread from the list.
    inline void remove(ar_timer_t * item);      //!< @brief Remove a timer from the list.
    inline void remove(ar_queue_t * item);      //!< @brief Remove a queue from the list.
    inline void remove(ar_channel_t * item);      //!< @brief Remove a channel from the list.
    void check();
#endif // __cplusplus
} ar_list_t;
//@}

/*!
 * @brief Thread.
 *
 * @ingroup ar_thread
 */
struct _ar_thread {
    volatile uint8_t * m_stackPointer;  //!< Current stack pointer.
    ar_thread_port_data_t m_portData; //!< Port-specific thread data.
    const char * m_name;        //!< Thread name.
    uint32_t * m_stackBottom;   //!< Beginning of stack.
    uint8_t m_priority;         //!< Thread priority. 0 is the lowest priority.
    ar_thread_state_t m_state;  //!< Current thread state.
    ar_thread_entry_t m_entry;  //!< Function pointer for the thread's entry point.
    ar_list_node_t m_threadNode;    //!< Main thread list node.
#if AR_GLOBAL_OBJECT_LISTS
    ar_list_node_t m_createdNode;   //!< Created list node.
#endif // AR_GLOBAL_OBJECT_LISTS
    ar_list_node_t m_blockedNode;   //!< Blocked list node.
    uint32_t m_wakeupTime;          //!< Tick count when a sleeping thread will awaken.
    ar_status_t m_unblockStatus;       //!< Status code to return from a blocking function upon unblocking.
    void * m_channelData;       //!< Receive or send data pointer for blocked channel.
    ar_runloop_t * m_runLoop;   //!< Run loop associated with this thread.

    // Internal utility methods.
#if defined(__cplusplus)
    void block(ar_list_t & blockedList, uint32_t timeout);
    void unblockWithStatus(ar_list_t & blockedList, ar_status_t unblockStatus);
#endif // __cplusplus
};

/*!
 * @brief Counting semaphore.
 *
 * @ingroup ar_sem
 */
typedef struct _ar_semaphore {
    const char * m_name;            //!< Name of the semaphore.
    volatile unsigned m_count;      //!< Current semaphore count. Value of 0 means the semaphore is owned.
    ar_list_t m_blockedList;        //!< List of threads blocked on the semaphore.
#if AR_GLOBAL_OBJECT_LISTS
    ar_list_node_t m_createdNode;   //!< Created list node.
#endif // AR_GLOBAL_OBJECT_LISTS
} ar_semaphore_t;

/*!
 * @brief Mutex.
 *
 * @ingroup ar_mutex
 */
typedef struct _ar_mutex {
    const char * m_name;            //!< Name of the mutex.
    volatile ar_thread_t * m_owner;     //!< Current owner thread of the mutex.
    volatile unsigned m_ownerLockCount; //!< Number of times the owner thread has locked the mutex.
    uint8_t m_originalPriority;         //!< Original priority of the owner thread before its priority was raised.
    ar_list_t m_blockedList;        //!< List of threads blocked on the mutex.
#if AR_GLOBAL_OBJECT_LISTS
    ar_list_node_t m_createdNode;   //!< Created list node.
#endif // AR_GLOBAL_OBJECT_LISTS
} ar_mutex_t;

/*!
 * @brief Channel.
 *
 * @ingroup ar_chan
 */
struct _ar_channel {
    const char * m_name;            //!< Name of the channel.
    uint32_t m_width;               //!< Size in bytes of the channel's data.
    ar_list_t m_blockedSenders;     //!< List of blocked sender threads.
    ar_list_t m_blockedReceivers;   //!< List of blocked receiver threads.
    ar_runloop_t * m_runLoop;
    ar_list_node_t m_runLoopNode;
#if AR_GLOBAL_OBJECT_LISTS
    ar_list_node_t m_createdNode;   //!< Node on the created channels list.
#endif // AR_GLOBAL_OBJECT_LISTS
};

/*!
 * @brief Queue.
 *
 * @ingroup ar_queue
 */
struct _ar_queue {
    const char * m_name;    //!< Name of the queue.
    uint8_t * m_elements;   //!< Pointer to element storage.
    unsigned m_elementSize; //!< Number of bytes occupied by each element.
    unsigned m_capacity;    //!< Maximum number of elements the queue can hold.
    unsigned m_head;        //!< Index of queue head.
    unsigned m_tail;        //!< Index of queue tail.
    unsigned m_count;       //!< Current number of elements in the queue.
    ar_list_t m_sendBlockedList;    //!< List of threads blocked waiting to send.
    ar_list_t m_receiveBlockedList; //!< List of threads blocked waiting to receive data.
    ar_runloop_t * m_runLoop;
    ar_list_node_t m_runLoopNode;
    ar_runloop_queue_handler_t m_runLoopHandler;
    void * m_runLoopHandlerParam;
#if AR_GLOBAL_OBJECT_LISTS
    ar_list_node_t m_createdNode;   //!< Created list node.
#endif // AR_GLOBAL_OBJECT_LISTS
};

/*!
 * @brief Timer.
 *
 * @ingroup ar_timer
 */
struct _ar_timer {
    const char * m_name;            //!< Name of the timer.
    ar_list_node_t m_activeNode;    //!< Node for the list of active timers.
#if AR_GLOBAL_OBJECT_LISTS
    ar_list_node_t m_createdNode;   //!< Created list node.
#endif // AR_GLOBAL_OBJECT_LISTS
    ar_timer_entry_t m_callback;    //!< Timer expiration callback.
    void * m_param;                 //!< Arbitrary parameter for the callback.
    ar_timer_mode_t m_mode;         //!< One-shot or periodic mode.
    uint32_t m_delay;           //!< Delay in ticks.
    uint32_t m_wakeupTime;      //!< Expiration time in ticks.
    bool m_isActive;            //!< Whether the timer is running and on the active timers list.
    ar_runloop_t * m_runLoop;
    ar_list_node_t m_runLoopNode;
};

/*!
 * @brief Run loop.
 *
 * @ingroup ar_runloop
 */
struct _ar_runloop {
    const char * m_name;
    ar_thread_t * m_thread;
    ar_list_t m_timers;
    ar_list_t m_queues;
    ar_list_t m_channels;
    struct _ar_runloop_function_info {
        ar_runloop_function_t function;
        void * param;
    } m_functions[AR_RUNLOOP_FUNCTION_QUEUE_SIZE];
    uint16_t m_functionCount;
    uint16_t m_functionHead;
    uint16_t m_functionTail;
    bool m_isRunning;
    volatile bool m_stop;
#if AR_GLOBAL_OBJECT_LISTS
    ar_list_node_t m_createdNode;   //!< Created list node.
#endif // AR_GLOBAL_OBJECT_LISTS
};

//------------------------------------------------------------------------------
// API
//------------------------------------------------------------------------------

#if defined(__cplusplus)
extern "C" {
#endif

//! @addtogroup ar
//! @{

//! @name Kernel
//@{
/*!
 * @brief Start the kernel running.
 *
 * Once this function is called, the kernel will begin scheduling threads.
 *
 * @note This call will not return.
 */
void ar_kernel_run(void);

/*!
 * @brief Returns whether the kernel is running or not.
 */
bool ar_kernel_is_running(void);

/*!
 * @brief Returns the current system load.
 *
 * The system load is calculated by the idle thread, if the #AR_ENABLE_SYSTEM_LOAD configuration
 * setting is enabled. If this setting is disabled, the load will always be zero.
 *
 * @return The current system load percentage from 0-100.
 */
uint32_t ar_get_system_load(void);
//@}

//! @}

//! @addtogroup ar_thread
//! @{

//! @name Threads
//@{
/*!
 * @brief Create a new thread.
 *
 * The state of the new thread is determined by the @a startImmediately parameter. If true,
 * the thread is created in the ready state. Otherwise, the thread is suspended when this function
 * exits. In this case, to make it eligible for execution you must call the ar_thread_resume()
 * function. When @a startImmediately is true, the new thread may start execution before this
 * function returns.
 *
 * @param thread Pointer to the thread structure.
 * @param name Name of the thread. If NULL, the thread's name is set to an empty string.
 * @param entry Thread entry point taking one parameter and returning void.
 * @param param Arbitrary pointer-sized value passed as the single parameter to the thread
 *     entry point.
 * @param stack Pointer to the start of the thread's stack. This should be the stack's bottom,
 *     not it's top.
 * @param stackSize Number of bytes of stack space allocated to the thread. This value is
 *     added to @a stack to get the initial top of stack address.
 * @param priority Thread priority. The accepted range is 1 through 255. Priority 0 is
 *     reserved for the idle thread.
 * @param startImmediately Whether the new thread will start to run automatically. If false, the
 *      thread will be created in a suspended state. The constants #kArStartThread and
 *      #kArSuspendThread can be used to better document this parameter value.
 *
 * @return kArSuccess The thread was initialised without error.
 */
ar_status_t ar_thread_create(ar_thread_t * thread, const char * name, ar_thread_entry_t entry, void * param, void * stack, unsigned stackSize, uint8_t priority, bool startImmediately);

/*!
 * @brief Delete a thread.
 *
 * @param thread Pointer to the thread structure.
 */
ar_status_t ar_thread_delete(ar_thread_t * thread);

/*!
 * @brief Put thread in suspended state.
 *
 * If this function is called from the current thread then the scheduler is entered immediately
 * after putting the thread on the suspended list. Calling this function on another thread will not
 * cause the scheduler to switch threads.
 *
 * Does not enter the scheduler if the kernel is not running. Does nothing if the thread is already
 * suspended.
 *
 * @param thread Pointer to the thread structure.
 */
ar_status_t ar_thread_suspend(ar_thread_t * thread);

/*!
 * @brief Make the thread eligible for execution.
 *
 * If the thread being resumed has a higher priority than that of the current thread, the
 * scheduler is called to immediately switch threads. In this case the thread being resumed
 * will always become the new current thread. This is because the highest priority thread is
 * always guaranteed to be running, meaning the calling thread was the previous highest
 * priority thread.
 *
 * Does not enter the scheduler if Ar is not running. Does nothing if the thread is already on
 * the ready list.
 *
 * @param thread Pointer to the thread structure.
 */
ar_status_t ar_thread_resume(ar_thread_t * thread);

/*!
 * @brief Return the current state of the thread.
 *
 * @param thread Pointer to the thread structure.
 */
ar_thread_state_t ar_thread_get_state(ar_thread_t * thread);

/*!
 * @brief Return the thread's current priority.
 *
 * @param thread Pointer to the thread structure.
 */
uint8_t ar_thread_get_priority(ar_thread_t * thread);

/*!
 * @brief Change a thread's priority.
 *
 * The scheduler is invoked after the priority is set so that the current thread can be changed
 * to the one with the highest priority. The scheduler is invoked even if there is no new
 * highest priority thread. In this case, control may switch to the next thread with the same
 * priority, assuming there is one.
 *
 * Does not enter the scheduler if Ar is not running.
 *
 * @param thread Pointer to the thread structure.
 * @param newPriority Thread priority level from 1 to 255, where lower numbers have a lower
 *     priority. Priority number 0 is not allowed because it is reserved for the idle thread.
 *
 * @retval kArSuccess
 * @retval kArInvalidPriorityError
 */
ar_status_t ar_thread_set_priority(ar_thread_t * thread, uint8_t newPriority);

/*!
 * @brief Returns the currently running thread object.
 *
 * @return Pointer to the current thread's structure.
 */
ar_thread_t * ar_thread_get_current(void);

/*!
 * @brief Get the runloop currently associated with the given thread.
 *
 * @return Pointer to the runloop runnning on the thread, if there is one. If there is not an
 *      associated runloop, then NULL will be returned.
 */
ar_runloop_t * ar_thread_get_runloop(ar_thread_t * thread);

/*!
 * @brief Put the current thread to sleep for a certain amount of time.
 *
 * Does nothing if Ar is not running.
 *
 * A sleeping thread can be woken early by calling ar_thread_resume().
 *
 * @param milliseconds The number of milliseconds to sleep the calling thread. A sleep time
 *     of 0 is ignored. If #kArInfiniteTimeout is passed for the sleep time, the thread will
 *     simply be suspended.
 */
void ar_thread_sleep(uint32_t milliseconds);

/*!
 * @brief Put the current thread to sleep until a specific time.
 *
 * Does nothing if Ar is not running.
 *
 * A sleeping thread can be woken early by calling ar_thread_resume().
 *
 * @param wakeup The wakeup time in milliseconds. If the time is not in the future, i.e., less than
 *  or equal to the current value returned by ar_get_millisecond_count(), then the sleep request is
 *  ignored.
 */
void ar_thread_sleep_until(uint32_t wakeup);

/*!
 * @brief Get the thread's name.
 *
 * @return Pointer to the name of the thread.
 */
const char * ar_thread_get_name(ar_thread_t * thread);
//@}

//! @}

//! @addtogroup ar_sem
//! @{

//! @name Semaphores
//@{
/*!
 * @brief Create a new semaphore.
 *
 * @param sem Pointer to storage for the semaphore.
 * @param name Pass a name for the semaphore. If NULL is passed the name will be set to an
 *     empty string.
 * @param count The initial semaphore count. Setting this value to 0 will cause the first call
 *     to get() to block until put() is called. A value of 1 or greater will allow that many
 *     calls to get() to succeed.
 *
 * @retval kArSuccess Semaphore initialised successfully.
 */
ar_status_t ar_semaphore_create(ar_semaphore_t * sem, const char * name, unsigned count);

/*!
 * @brief Delete a semaphore.
 *
 * Any threads on the blocked list will be unblocked immediately. Their return status from the
 * get function will be #kArObjectDeletedError.
 *
 * @param sem Pointer to the semaphore.
 * @retval kArSuccess Semaphore deleted successfully.
 */
ar_status_t ar_semaphore_delete(ar_semaphore_t * sem);

/*!
 * @brief Acquire the semaphore.
 *
 * The semaphore count is decremented. If the count is 0 upon entering this method then the
 * caller thread is blocked until the count reaches 1. Threads are unblocked in the order in
 * which they were blocked. Priority is not taken into consideration, so priority inversions
 * are possible.
 *
 * @note This function may be called from interrupt context only if the timeout parameter is
 *     set to #kArNoTimeout (or 0).
 *
 * @param sem Pointer to the semaphore.
 * @param timeout The maximum number of milliseconds that the caller is willing to wait in a
 *     blocked state before the semaphore can be obtained. If this value is 0, or #kArNoTimeout,
 *     then this method will return immediately if the semaphore cannot be obtained. Setting
 *     the timeout to #kArInfiniteTimeout will cause the thread to wait forever for a chance to
 *     get the semaphore.
 *
 * @retval kArSuccess The semaphore was obtained without error.
 * @retval kArTimeoutError The specified amount of time has elapsed before the semaphore could be
 *     obtained.
 * @retval kArObjectDeletedError Another thread deleted the semaphore while the caller was
 *     blocked on it.
 * @retval kArNotFromInterruptError A non-zero timeout is not alllowed from the interrupt
 *     context.
 */
ar_status_t ar_semaphore_get(ar_semaphore_t * sem, uint32_t timeout);

/*!
 * @brief Release the semaphore.
 *
 * The semaphore count is incremented.
 *
 * @param sem Pointer to the semaphore.
 *
 * @note This call is safe from interrupt context.
 */
ar_status_t ar_semaphore_put(ar_semaphore_t * sem);

/*!
 * @brief Returns the current semaphore count.
 *
 * @param sem Pointer to the semaphore.
 */
uint32_t ar_semaphore_get_count(ar_semaphore_t * sem);

/*!
 * @brief Get the semaphore's name.
 *
 * @param sem Pointer to the semaphore.
 */
const char * ar_semaphore_get_name(ar_semaphore_t * sem);
//@}

//! @}

//! @addtogroup ar_mutex
//! @{

//! @name Mutexes
//@{
/*!
 * @brief Create a new mutex object.
 *
 * The mutex starts out unlocked.
 *
 * @param mutex Pointer to storage for the mutex.
 * @param name The name of the mutex.
 *
 * @retval kArSuccess
 */
ar_status_t ar_mutex_create(ar_mutex_t * mutex, const char * name);

/*!
 * @brief Delete a mutex.
 *
 * @param mutex Pointer to the mutex to delete.
 *
 * @retval kArSuccess
 */
ar_status_t ar_mutex_delete(ar_mutex_t * mutex);

/*!
 * @brief Lock the mutex.
 *
 * If the thread that already owns the mutex calls get() more than once, a count is incremented
 * rather than attempting to decrement the underlying semaphore again. The converse is true for
 * put(), thus allowing a thread to lock a mutex any number of times as long as there are
 * matching get() and put() calls.
 *
 * @param mutex Pointer to the mutex.
 * @param timeout The maximum number of milliseconds that the caller is willing to wait in a
 *     blocked state before the semaphore can be obtained. If this value is 0, or #kArNoTimeout,
 *     then this method will return immediately if the lock cannot be obtained. Setting
 *     the timeout to #kArInfiniteTimeout will cause the thread to wait forever for a chance to
 *     get the lock.
 *
 * @retval kArSuccess The mutex was obtained without error.
 * @retval kArTimeoutError The specified amount of time has elapsed before the mutex could be
 *     obtained.
 * @retval kArObjectDeletedError Another thread deleted the semaphore while the caller was
 *     blocked on it.
 */
ar_status_t ar_mutex_get(ar_mutex_t * mutex, uint32_t timeout);

/*!
 * @brief Unlock the mutex.
 *
 * Only the owning thread is allowed to unlock the mutex. If the owning thread has called get()
 * multiple times, it must also call put() the same number of time before the underlying
 * semaphore is actually released. It is illegal to call put() when the mutex is not owned by
 * the calling thread.
 *
 * @param mutex Pointer to the mutex.
 *
 * @retval kArAlreadyUnlockedError The mutex is not locked.
 * @retval kArNotOwnerError The caller is not the thread that owns the mutex.
 */
ar_status_t ar_mutex_put(ar_mutex_t * mutex);

/*!
 * @brief Returns whether the mutex is currently locked.
 *
 * @param mutex Pointer to the mutex.
 */
bool ar_mutex_is_locked(ar_mutex_t * mutex);

/*!
 * @brief Returns the current owning thread, if there is one.
 *
 * @param mutex Pointer to the mutex.
 */
ar_thread_t * ar_mutex_get_owner(ar_mutex_t * mutex);

/*!
 * @brief Get the mutex's name.
 *
 * @param mutex Pointer to the mutex.
 */
const char * ar_mutex_get_name(ar_mutex_t * mutex);
//@}

//! @}

//! @addtogroup ar_chan
//! @{

//! @name Channels
//@{
/*!
 * @brief Create a new channel.
 *
 * @param channel Pointer to storage for the new channel.
 * @param name Optional name for the new channel.
 * @param width The size in bytes of the data passed through the channel. Set this parameter
 *      to 0 to use the default pointer-sized width.
 */
ar_status_t ar_channel_create(ar_channel_t * channel, const char * name, uint32_t width);

/*!
 * @brief Delete an existing channel.
 *
 * @param channel Pointer to the channel.
 */
ar_status_t ar_channel_delete(ar_channel_t * channel);

/*!
 * @brief Send to a channel.
 *
 * This function synchronously sends a value across the specified channel. If there is not a
 * thread waiting to receive on the other side, then this function will block.
 *
 * @param channel Pointer to the channel.
 * @param value Pointer to value to send through the channel.
 * @param timeout The maximum number of milliseconds the caller is willing to wait before the
 *      send is completed.
 */
ar_status_t ar_channel_send(ar_channel_t * channel, const void * value, uint32_t timeout);

/*!
 * @brief Receive from a channel.
 *
 * @param channel Pointer to the channel.
 * @param value Pointer to a location where the received data is written.
 * @param timeout The maximum number of milliseconds the caller is willing to wait before the
 *      receive is completed.
 */
ar_status_t ar_channel_receive(ar_channel_t * channel, void * value, uint32_t timeout);

/*!
 * @brief Get a channel's name.
 *
 * @param channel Pointer to the channel.
 */
const char * ar_channel_get_name(ar_channel_t * channel);
//@}

//! @}

//! @addtogroup ar_queue
//! @{

//! @name Queues
//@{
/*!
 * @brief Create a new queue.
 *
 * @param queue The storage for the new queue object.
 * @param name The new queue's name.
 * @param storage Pointer to a buffer used to store queue elements. The buffer must be at least
 *     @a elementSize * @a capacity bytes big.
 * @param elementSize Size in bytes of each element in the queue.
 * @param capacity The number of elements that the buffer pointed to by @a storage will hold.
 *
 * @retval kArSuccess The queue was initialised.
 */
ar_status_t ar_queue_create(ar_queue_t * queue, const char * name, void * storage, unsigned elementSize, unsigned capacity);

/*!
 * @brief Delete an existing queue.
 *
 * @param queue The queue object.
 */
ar_status_t ar_queue_delete(ar_queue_t * queue);

/*!
 * @brief Add an item to the queue.
 *
 * The caller will block if the queue is full.
 *
 * @param queue The queue object.
 * @param element Pointer to the element to post to the queue. The element size was specified
 *     in the init() call.
 * @param timeout The maximum number of milliseconds that the caller is willing to wait in a
 *     blocked state before the element can be sent. If this value is 0, or #kArNoTimeout, then
 *     this method will return immediately if the queue is full. Setting the timeout to
 *     #kArInfiniteTimeout will cause the thread to wait forever for a chance to send.
 *
 * @retval kArSuccess
 * @retval kArQueueFullError
 */
ar_status_t ar_queue_send(ar_queue_t * queue, const void * element, uint32_t timeout);

/*!
 * @brief Remove an item from the queue.
 *
 * @param queue The queue object.
 * @param[out] element
 * @param timeout The maximum number of milliseconds that the caller is willing to wait in a
 *     blocked state before an element is received. If this value is 0, or #kArNoTimeout, then
 *     this method will return immediately if the queue is empty. Setting the timeout to
 *     #kArInfiniteTimeout will cause the thread to wait forever for receive an element.
 *
 * @retval kArSuccess
 * @retval kArQueueEmptyError
 */
ar_status_t ar_queue_receive(ar_queue_t * queue, void * element, uint32_t timeout);

/*!
 * @brief Returns whether the queue is currently empty.
 *
 * @param queue The queue object.
 *
 * @return Boolean indicating whether the given queue is currently empty.
 */
bool ar_queue_is_empty(ar_queue_t * queue);

/*!
 * @brief Returns the current number of elements in the queue.
 *
 * @param queue The queue object.
 *
 * @return The number of data elements available in the queue.
 */
uint32_t ar_queue_get_count(ar_queue_t * queue);

/*!
 * @brief Get the queue's name.
 *
 * @param queue The queue object.
 *
 * @return Pointer to the queue's name.
 */
const char * ar_queue_get_name(ar_queue_t * queue);
//@}

//! @}

//! @addtogroup ar_timer
//! @{

//! @name Timers
//@{
/*!
 * @brief Create a new timer.
 *
 * @param timer Pointer to storage for the new timer.
 * @param name The name of the timer. May be NULL.
 * @param callback Callback function that will be executed when the timer expires.
 * @param param Arbitrary pointer-sized argument that is passed to the timer callback routine.
 * @param timerMode Whether to create a one-shot or periodic timer. Pass either #kArOneShotTimer or
 *      #kArPeriodicTimer.
 * @param delay The timer delay in milliseconds. The timer will fire this number of milliseconds
 *      after the ar_timer_start() API is called.
 *
 * @retval kArSuccess The timer was created successfully.
 */
ar_status_t ar_timer_create(ar_timer_t * timer, const char * name, ar_timer_entry_t callback, void * param, ar_timer_mode_t timerMode, uint32_t delay);

/*!
 * @brief Delete a timer.
 *
 * @param timer The timer object.
 *
 * @retval kArSuccess The timer was deleted successfully.
 */
ar_status_t ar_timer_delete(ar_timer_t * timer);

/*!
 * @brief Start the timer running.
 *
 * @param timer The timer object.
 *
 * @retval kArSuccess The timer was started successfully.
 */
ar_status_t ar_timer_start(ar_timer_t * timer);

/*!
 * @brief Stop the timer.
 *
 * @param timer The timer object.
 *
 * @retval kArSuccess The timer was stopped successfully.
 */
ar_status_t ar_timer_stop(ar_timer_t * timer);

/*!
 * @brief Returns whether the timer is currently running.
 *
 * @param timer The timer object.
 *
 * @retval true The timer is running.
 * @retval false The timer is stopped.
 */
bool ar_timer_is_active(ar_timer_t * timer);

/*!
 * @brief Adjust the timer's delay.
 *
 * @param timer The timer object.
 * @param newDelay New delay value for the timer in milliseconds.
 *
 * @retval kArSuccess The timer's delay was modified.
 */
ar_status_t ar_timer_set_delay(ar_timer_t * timer, uint32_t newDelay);

/*!
 * @brief Get the current delay for the timer.
 *
 * @param timer The timer object.
 *
 * @return The timer's delay in milliseconds.
 */
uint32_t ar_timer_get_delay(ar_timer_t * timer);

/*!
 * @brief Get the timer's name.
 *
 * @param timer The timer object.
 *
 * @return The name of the timer.
 */
const char * ar_timer_get_name(ar_timer_t * timer);
//@}

//! @}

//! @addtogroup ar_runloop
//! @{

//! @name Runloop
//@{

ar_status_t ar_runloop_create(ar_runloop_t * runloop, const char * name, ar_thread_t * thread);

ar_status_t ar_runloop_delete(ar_runloop_t * runloop);

ar_runloop_status_t ar_runloop_run(ar_runloop_t * runloop, uint32_t timeout, void ** object);

ar_status_t ar_runloop_stop(ar_runloop_t * runloop);

ar_status_t ar_runloop_perform(ar_runloop_t * runloop, ar_runloop_function_t function, void * param);

ar_status_t ar_runloop_add_timer(ar_runloop_t * runloop, ar_timer_t * timer);

ar_status_t ar_runloop_add_queue(ar_runloop_t * runloop, ar_queue_t * queue, ar_runloop_queue_handler_t callback, void * param);

ar_status_t ar_runloop_add_channel(ar_runloop_t * runloop, ar_channel_t * channel, ar_runloop_channel_handler_t callback, void * param);

ar_runloop_t * ar_runloop_get_current(void);

const char * ar_runloop_get_name(ar_runloop_t * runloop);

//@}
//! @}

//! @addtogroup ar_time
//! @{

//! @name Time
//@{
/*!
 * @brief Return the current time in ticks.
 */
uint32_t ar_get_tick_count(void);

/*!
 * @brief Return the current time in milliseconds.
 *
 * @return Elapsed time since the kernel was started in milliseconds. Has a resolution of one tick.
 */
uint32_t ar_get_millisecond_count(void);

/*!
 * @brief Get the number of milliseconds per tick.
 */
uint32_t ar_get_milliseconds_per_tick(void);

/*!
 * @brief Convert ticks to milliseconds.
 */
static inline uint32_t ar_ticks_to_milliseconds(uint32_t ticks) { return ticks * ar_get_milliseconds_per_tick(); }

/*!
 * @brief Convert milliseconds to ticks.
 */
static inline uint32_t ar_milliseconds_to_ticks(uint32_t milliseconds) { return milliseconds / ar_get_milliseconds_per_tick(); }
//@}

//! @}

//! @addtogroup ar_atomic
//! @{

//! @name Atomic operations
//@{
/*!
 * @brief Atomic add operation.
 *
 * A memory barrier is performed prior to the add operation.
 *
 * @param value Pointer to the word to add to.
 * @param delta Signed value to atomically add to *value.
 * @return The original value is returned.
 */
int32_t ar_atomic_add(volatile int32_t * value, int32_t delta);

/*!
 * @brief Atomically increment a value.
 *
 * @param value Pointer to the word to increment.
 * @return The original value is returned.
 */
static inline int32_t ar_atomic_inc(volatile int32_t * value) { return ar_atomic_add(value, 1); }

/*!
 * @brief Atomically decrement a value.
 *
 * @param value Pointer to the word to decrement.
 * @return The original value is returned.
 */
static inline int32_t ar_atomic_dec(volatile int32_t * value) { return ar_atomic_add(value, -1); }

/*!
 * @brief Atomic compare and swap operation.
 *
 * Tests the word pointed to by @a value for equality with @a oldValue. If they are
 * equal, the word pointed to by @a value is set to @a newValue. If *value is not
 * equal to @a oldValue, then no change is made. The return value indicates whether
 * the swap was performed. Of course, this entire operation is guaranteed to be
 * atomic even on multiprocessor platforms.
 *
 * A memory barrier is performed prior to the compare and swap operation.
 *
 * @param value Pointer to the word to compare and swap.
 * @param expectedValue Value to compare against.
 * @param newValue Value to value to swap in if *value is equal to oldValue.
 * @retval false No change was made to *value.
 * @retval true The swap was performed, and *value is now equal to newValue.
 */
bool ar_atomic_cas(volatile int32_t * value, int32_t expectedValue, int32_t newValue);
//@}

//! @}

#if defined(__cplusplus)
}
#endif

//! @}

#endif // _AR_KERNEL_H_
//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
