/*
 * Copyright (c) 2007-2014 Immo Software
 * All rights reserved.
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

#include "fsl_platform_common.h"
#include "ar_port.h"

//! @addtogroup ar
//! @{

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------

#if !defined(AR_GLOBAL_OBJECT_LISTS)
//! Set to 1 to enable the lists of all created kernel objects.
#define AR_GLOBAL_OBJECT_LISTS (1)
#endif

//! @brief Timeout constants.
enum _ar_timeouts
{
    //! Return immediately if a resource cannot be acquired.
    kArNoTimeout = 0,
    
    //! Pass this value to wait forever to acquire a resource.
    kArInfiniteTimeout = 0xffffffffL
};

//! @brief Argon error codes.
enum _ar_errors
{
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
    kArTimerNotRunningError
};

//! @brief Potential thread states.
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
enum _ar_thread_priorities
{
    kArIdleThreadPriority = 0,  //!< The idle thread's priority. No other thread is allowed to have this priority.
    kArMinThreadPriority = 1,   //!< Priority value for the lowest priority user thread.
    kArMaxThreadPriority = 255  //!< Priority value for the highest priority user thread.
};

//! @brief Modes of operation for timers.
typedef enum _ar_timer_modes {
    kArOneShotTimer,      //!< Timer fires a single time.
    kArPeriodicTimer      //!< Timer repeatedly fires every time the interval elapses.
} ar_timer_mode_t;

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

// Forward declarations.
typedef struct _ar_thread ar_thread_t;
typedef struct _ar_timer ar_timer_t;
typedef struct _ar_list_node ar_list_node_t;

/*!
 * @brief Linked list node.
 */
typedef struct _ar_list_node {
    ar_list_node_t * m_next;    //!< Next node in the list.
    ar_list_node_t * m_prev;    //!< Previous node in the list.
    void * m_obj;               //!< Pointer to the object on the list.

#if defined(__cplusplus)
    //! @brief Convert the @a m_obj pointer to a particular type.
    template <typename T>
    T * getObject() { return reinterpret_cast<T *>(m_obj); }
    
    //! @brief Insert this node before another node on the list.
    void insertBefore(ar_list_node_t * node);
#endif // __cplusplus
} ar_list_node_t;

/*!
 * @brief Linked list.
 */
typedef struct _ar_list {
    ar_list_node_t * m_head;    //!< Pointer to the head of the list. Will be NULL if the list is empty.
    
#if defined(__cplusplus)
    //! Function type used for sorting object lists.
    typedef bool (*predicate_t)(ar_list_node_t * a, ar_list_node_t * b);

    //! @brief Add an item to the list.
    void add(ar_list_node_t * item, predicate_t predicate=NULL);
    
    //! @brief Remove an item from the list.
    void remove(ar_list_node_t * item);
#endif // __cplusplus
} ar_list_t;

//! Prototype for the thread entry point.
typedef void (*ar_thread_entry_t)(void * param);

/*!
 * @brief Thread.
 */
typedef struct _ar_thread {
    volatile uint8_t * m_stackPointer;  //!< Current stack pointer.
    const char * m_name;        //!< Thread name.
    uint8_t * m_stackTop;       //!< Original top of stack.
    uint32_t m_stackSize;       //!< Stack size in bytes.
    uint8_t m_priority;         //!< Thread priority. 0 is the lowest priority.
    ar_thread_state_t m_state;  //!< Current thread state.
    ar_thread_entry_t m_entry;  //!< Function pointer for the thread's entry point.
    ar_list_node_t m_threadNode;    //!< Main thread list node.
    ar_list_node_t m_createdNode;   //!< Created list node.
    ar_list_node_t m_blockedNode;   //!< Blocked list node.
    uint32_t m_wakeupTime;          //!< Tick count when a sleeping thread will awaken.
    status_t m_unblockStatus;       //!< Status code to return from a blocking function upon unblocking.
    void * m_ref;               //!< Arbitrary reference value.

#if defined(__cplusplus)
    void block(ar_list_t & blockedList, uint32_t timeout);
    void unblockWithStatus(ar_list_t & blockedList, status_t unblockStatus);
#endif // __cplusplus
} ar_thread_t;

/*!
 * @brief Counting semaphore.
 */
typedef struct _ar_semaphore {
    const char * m_name;            //!< Name of the semaphore.
    volatile unsigned m_count;      //!< Current semaphore count. Value of 0 means the semaphore is owned.
    ar_list_t m_blockedList;        //!< List of threads blocked on the semaphore.
    ar_list_node_t m_createdNode;   //!< Created list node.
} ar_semaphore_t;

/*!
 * @brief Mutex.
 */
typedef struct _ar_mutex {
    ar_semaphore_t m_sem;               //!< Underlying semaphore for the mutex.
    volatile ar_thread_t * m_owner;     //!< Current owner thread of the mutex.
    volatile unsigned m_ownerLockCount; //!< Number of times the owner thread has locked the mutex.
    uint8_t m_originalPriority;         //!< Original priority of the owner thread before its priority was raised.
} ar_mutex_t;

/*!
 * @brief Queue.
 */
typedef struct _ar_queue {
    const char * m_name;    //!< Name of the queue.
    uint8_t * m_elements;   //!< Pointer to element storage.
    unsigned m_elementSize; //!< Number of bytes occupied by each element.
    unsigned m_capacity;    //!< Maximum number of elements the queue can hold.
    unsigned m_head;        //!< Index of queue head.
    unsigned m_tail;        //!< Index of queue tail.
    unsigned m_count;       //!< Current number of elements in the queue.
    ar_list_t m_sendBlockedList;    //!< List of threads blocked waiting to send.
    ar_list_t m_receiveBlockedList; //!< List of threads blocked waiting to receive data.
    ar_list_node_t m_createdNode;   //!< Created list node.
} ar_queue_t;

//! @brief Callback routine for timer expiration.
typedef void (*ar_timer_entry_t)(ar_timer_t * timer, void * param);

/*!
 * @brief Timer.
 */
typedef struct _ar_timer {
    const char * m_name;            //!< Name of the timer.
    ar_list_node_t m_activeNode;    //!< Node for the list of active timers.
    ar_list_node_t m_createdNode;   //!< Created list node.
    ar_timer_entry_t m_callback;    //!< Timer expiration callback.
    void * m_param;                 //!< Arbitrary parameter for the callback.
    ar_timer_mode_t m_mode;         //!< One-shot or periodic mode.
    uint32_t m_delay;           //!< Delay in ticks.
    uint32_t m_wakeupTime;      //!< Expiration time in ticks.
    bool m_isActive;            //!< Whether the timer is running and on the active timers list.
} ar_timer_t;

//------------------------------------------------------------------------------
// API
//------------------------------------------------------------------------------

#if defined(__cplusplus)
extern "C" {
#endif

//! @name Kernel start
//@{
void ar_kernel_run(void);
bool ar_kernel_is_running(void);
uint32_t ar_get_system_load(void);
//@}

//! @name Threads
//@{
/*!
 * @brief Create a new thread.
 *
 * The thread is in suspended state when this function exits. To make it eligible for
 * execution, call the resume() method.
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
 *
 * @return kArSuccess The thread was initialised without error.
 */
status_t ar_thread_create(ar_thread_t * thread, const char * name, ar_thread_entry_t entry, void * param, void * stack, unsigned stackSize, uint8_t priority);

/*!
 * @brief Delete a thread.
 *
 * @param thread Pointer to the thread structure.
 */
status_t ar_thread_delete(ar_thread_t * thread);

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
status_t ar_thread_suspend(ar_thread_t * thread);

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
status_t ar_thread_resume(ar_thread_t * thread);

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
 * @param priority Thread priority level from 1 to 255, where lower numbers have a lower
 *     priority. Priority number 0 is not allowed because it is reserved for the idle thread.
 *
 * @retval kArSuccess
 * @retval kArInvalidPriorityError
 */
status_t ar_thread_set_priority(ar_thread_t * thread, uint8_t newPriority);

/*!
 * @brief Returns the currently running thread object.
 *
 * @return Pointer to the current thread's structure.
 */
ar_thread_t * ar_thread_get_current(void);

/*!
 * @brief Put the current thread to sleep for a certain amount of time.
 *
 * Does nothing if Ar is not running.
 *
 * @param milliseconds The number of milliseconds to sleep the calling thread. A sleep time
 *     of 0 is ignored.
 */
void ar_thread_sleep(uint32_t milliseconds);

/*!
 * @brief Get the thread's name.
 *
 * @return Pointer to the name of the thread.
 */
const char * ar_thread_get_name(ar_thread_t * thread);
//@}

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
status_t ar_semaphore_create(ar_semaphore_t * sem, const char * name, unsigned count);

/*!
 * @brief Delete a semaphore.
 *
 * Any threads on the blocked list will be unblocked immediately. Their return status from the
 * get function will be #kArObjectDeletedError.
 *
 * @param sem Pointer to the semaphore.
 * @retval kArSuccess Semaphore deleted successfully.
 */
status_t ar_semaphore_delete(ar_semaphore_t * sem);

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
 *     blocked state before the semaphore can be obtained. If this value is 0, or #kNoTimeout,
 *     then this method will return immediately if the semaphore cannot be obtained. Setting
 *     the timeout to #kInfiniteTimeout will cause the thread to wait forever for a chance to
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
status_t ar_semaphore_get(ar_semaphore_t * sem, uint32_t timeout);

/*!
 * @brief Release the semaphore.
 *
 * The semaphore count is incremented.
 *
 * @param sem Pointer to the semaphore.
 *
 * @note This call is safe from interrupt context.
 */
status_t ar_semaphore_put(ar_semaphore_t * sem);

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
status_t ar_mutex_create(ar_mutex_t * mutex, const char * name);

/*!
 * @brief Delete a mutex.
 *
 * @param mutex Pointer to the mutex to delete.
 *
 * @retval kArSuccess
 */
status_t ar_mutex_delete(ar_mutex_t * mutex);

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
status_t ar_mutex_get(ar_mutex_t * mutex, uint32_t timeout);

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
status_t ar_mutex_put(ar_mutex_t * mutex);

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
status_t ar_queue_create(ar_queue_t * queue, const char * name, void * storage, unsigned elementSize, unsigned capacity);

/*!
 * @brief Delete an existing queue.
 *
 * @param queue The queue object.
 */
status_t ar_queue_delete(ar_queue_t * queue);

/*!
 * @brief Add an item to the queue.
 *
 * The caller will block if the queue is full.
 *
 * @param queue The queue object.
 * @param element Pointer to the element to post to the queue. The element size was specified
 *     in the init() call.
 * @param timeout The maximum number of milliseconds that the caller is willing to wait in a
 *     blocked state before the element can be sent. If this value is 0, or #ArkNoTimeout, then
 *     this method will return immediately if the queue is full. Setting the timeout to
 *     #kArInfiniteTimeout will cause the thread to wait forever for a chance to send.
 *
 * @retval kArSuccess
 * @retval kArQueueFullError
 */
status_t ar_queue_send(ar_queue_t * queue, const void * element, uint32_t timeout);

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
status_t ar_queue_receive(ar_queue_t * queue, void * element, uint32_t timeout);

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
status_t ar_timer_create(ar_timer_t * timer, const char * name, ar_timer_entry_t callback, void * param, ar_timer_mode_t timerMode, uint32_t delay);

/*!
 * @brief Delete a timer.
 *
 * @param timer The timer object.
 *
 * @retval kArSuccess The timer was deleted successfully.
 */
status_t ar_timer_delete(ar_timer_t * timer);

/*!
 * @brief Start the timer running.
 *
 * @param timer The timer object.
 *
 * @retval kArSuccess The timer was started successfully.
 */
status_t ar_timer_start(ar_timer_t * timer);

/*!
 * @brief Stop the timer.
 *
 * @param timer The timer object.
 *
 * @retval kArSuccess The timer was stopped successfully.
 */
status_t ar_timer_stop(ar_timer_t * timer);

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
status_t ar_timer_set_delay(ar_timer_t * timer, uint32_t newDelay);

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

//! @name Time
//@{
uint32_t ar_get_tick_count(void);
uint32_t ar_get_milliseconds_per_tick(void);
static inline uint32_t ar_ticks_to_milliseconds(uint32_t ticks) { return ticks * ar_get_milliseconds_per_tick(); }
static inline uint32_t ar_milliseconds_to_ticks(uint32_t milliseconds) { return milliseconds / ar_get_milliseconds_per_tick(); }
//@}

//! @name Interrupts
//@{
void ar_kernel_enter_interrupt();
void ar_kernel_exit_interrupt();
//@}

//! @name Atomic operations
//@{
//! @brief Atomic add.
void ar_atomic_add(uint32_t * value, int32_t delta);

//! @brief Atomic increment.
inline void ar_atomic_increment(uint32_t * value) { ar_atomic_add(value, 1); }

//! @brief Atomic decrement.
inline void ar_atomic_decrement(uint32_t * value) { ar_atomic_add(value, -1); }

//! @brief Atomic compare-and-swap operation.
bool ar_atomic_compare_and_swap(uint32_t * value, uint32_t expectedValue, uint32_t newValue);
//@}

#if defined(__cplusplus)
}
#endif

//! @}

#endif // _AR_KERNEL_H_
//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
