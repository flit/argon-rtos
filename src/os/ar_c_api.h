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
 * @brief Header for the Argon RTOS.
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

#define AR_GLOBAL_OBJECT_LISTS (0)

//! @brief Timeout constants.
enum _ar_timeouts
{
    //! Return immediately if a resource cannot be acquired.
    kArNoTimeout = 0,
    
    //! Pass this value to wait forever to acquire a resource.
    kArInfiniteTimeout = 0xffffffffL
};

//! @brief Ar microkernel error codes.
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

//! Potential thread states.
typedef enum _ar_thread_state {
    kArThreadUnknown,     //!< Hopefully a thread is never in this state.
    kArThreadSuspended,   //!< Thread is not eligible for execution.
    kArThreadReady,       //!< Thread is eligible to be run.
    kArThreadRunning,     //!< The thread is currently running.
    kArThreadBlocked,     //!< The thread is blocked on another object.
    kArThreadSleeping,    //!< Thread is sleeping.
    kArThreadDone         //!< Thread has exited.
} ar_thread_state_t;

//! Range of priorities for threads.
enum _ar_thread_priorities
{
    kArIdleThreadPriority = 0,
    kArMinThreadPriority = 1,
    kArMaxThreadPriority = 255
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

//! Prototype for the thread entry point.
typedef void (*ar_thread_entry_t)(void * param);

/*!
 * @brief Linked list node.
 */
typedef struct _ar_list_node {
    ar_list_node_t * m_next;
    ar_list_node_t * m_prev;
    void * m_obj;

#if defined(__cplusplus)
    template <typename T>
    T * getObject() { return reinterpret_cast<T *>(m_obj); }
    
    void insertBefore(ar_list_node_t * node);
#endif // __cplusplus
} ar_list_node_t;

/*!
 * @brief Linked list.
 */
typedef struct _ar_list {
    ar_list_node_t * m_head;
    
#if defined(__cplusplus)
    //! Function type used for sorting object lists.
    typedef bool (*predicate_t)(ar_list_node_t * a, ar_list_node_t * b);

    void add(ar_list_node_t * item, predicate_t predicate=NULL);
    void remove(ar_list_node_t * item);
#endif // __cplusplus
} ar_list_t;

/*!
 * @brief Thread.
 */
typedef struct _ar_thread {
    volatile uint8_t * m_stackPointer; //!< Current stack pointer.
    const char * m_name;
    uint8_t * m_stackTop;  //!< Original top of stack.
    uint32_t m_stackSize;   //!< Stack size in bytes.
    uint8_t m_priority; //!< Thread priority. 0 is the lowest priority.
    ar_thread_state_t m_state; //!< Current thread state.
    ar_thread_entry_t m_entry; //!< Function pointer for the thread's entry point.
    ar_list_node_t m_threadNode;
    ar_list_node_t m_createdNode;
    ar_list_node_t m_blockedNode;
    uint32_t m_wakeupTime;  //!< Tick count when a sleeping thread will awaken.
    status_t m_unblockStatus;   //!< Status code to return from a blocking function upon unblocking.

#if defined(__cplusplus)
    void block(ar_list_t & blockedList, uint32_t timeout);
    void unblockWithStatus(ar_list_t & blockedList, status_t unblockStatus);
#endif // __cplusplus
} ar_thread_t;

/*!
 * @brief Semaphore.
 */
typedef struct _ar_semaphore {
    const char * m_name;
    volatile unsigned m_count;  //!< Current semaphore count. Value of 0 means the semaphore is owned.
    ar_list_t m_blockedList;
    ar_list_node_t m_createdNode;
} ar_semaphore_t;

/*!
 * @brief Mutex.
 */
typedef struct _ar_mutex {
    ar_semaphore_t m_sem;
    volatile ar_thread_t * m_owner;  //!< Current owner thread of the mutex.
    volatile unsigned m_ownerLockCount; //!< Number of times the owner thread has locked the mutex.
    uint8_t m_originalPriority; //!< Original priority of the owner thread before its priority was raised.
} ar_mutex_t;

/*!
 * @brief Queue.
 */
typedef struct _ar_queue {
    const char * m_name;
    uint8_t * m_elements;   //!< Pointer to element storage.
    unsigned m_elementSize; //!< Number of bytes occupied by each element.
    unsigned m_capacity;    //!< Maximum number of elements the queue can hold.
    unsigned m_head;    //!< Index of queue head.
    unsigned m_tail;    //!< Index of queue tail.
    unsigned m_count;   //!< Current number of elements in the queue.
    ar_list_t m_sendBlockedList;
    ar_list_t m_receiveBlockedList;
    ar_list_node_t m_createdNode;
} ar_queue_t;

//! @brief Callback routine for timer expiration.
typedef void (*ar_timer_entry_t)(ar_timer_t * timer, void * param);

/*!
 * @brief Timer.
 */
typedef struct _ar_timer {
    const char * m_name;
    ar_list_node_t m_activeNode;
    ar_list_node_t m_createdNode;
    ar_timer_entry_t m_callback;   //!< Timer expiration callback.
    void * m_param;             //!< Arbitrary parameter for the callback.
    ar_timer_mode_t m_mode;        //!< One-shot or periodic mode.
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

void ar_kernel_run(void);
bool ar_kernel_is_running(void);

uint32_t ar_get_tick_count(void);
uint32_t ar_get_system_load(void);

status_t ar_thread_create(ar_thread_t * thread, const char * name, ar_thread_entry_t entry, void * param, void * stack, unsigned stackSize, uint8_t priority);
status_t ar_thread_delete(ar_thread_t * thread);
status_t ar_thread_suspend(ar_thread_t * thread);
status_t ar_thread_resume(ar_thread_t * thread);
ar_thread_state_t ar_thread_get_state(ar_thread_t * thread);
uint8_t ar_thread_get_priority(ar_thread_t * thread);
status_t ar_thread_set_priority(ar_thread_t * thread, uint8_t newPriority);
ar_thread_t * ar_thread_get_current(void);
void ar_thread_sleep(uint32_t milliseconds);
const char * ar_thread_get_name(ar_thread_t * thread);

status_t ar_semaphore_create(ar_semaphore_t * sem, const char * name, unsigned count);
status_t ar_semaphore_delete(ar_semaphore_t * sem);
status_t ar_semaphore_get(ar_semaphore_t * sem, uint32_t timeout);
status_t ar_semaphore_put(ar_semaphore_t * sem);
uint32_t ar_semaphore_get_count(ar_semaphore_t * sem);
const char * ar_semaphore_get_name(ar_semaphore_t * sem);

status_t ar_mutex_create(ar_mutex_t * mutex, const char * name);
status_t ar_mutex_delete(ar_mutex_t * mutex);
status_t ar_mutex_get(ar_mutex_t * mutex, uint32_t timeout);
status_t ar_mutex_put(ar_mutex_t * mutex);
bool ar_mutex_is_locked(ar_mutex_t * mutex);
ar_thread_t * ar_mutex_get_owner(ar_mutex_t * mutex);
const char * ar_mutex_get_name(ar_mutex_t * mutex);

status_t ar_queue_create(ar_queue_t * queue, const char * name, void * storage, unsigned elementSize, unsigned capacity);
status_t ar_queue_delete(ar_queue_t * queue);
status_t ar_queue_send(ar_queue_t * queue, const void * element, uint32_t timeout);
status_t ar_queue_receive(ar_queue_t * queue, void * element, uint32_t timeout);
bool ar_queue_is_empty(ar_queue_t * queue);
uint32_t ar_queue_get_count(ar_queue_t * queue);
const char * ar_queue_get_name(ar_queue_t * queue);

status_t ar_timer_create(ar_timer_t * timer, const char * name, ar_timer_entry_t callback, void * param, ar_timer_mode_t timerMode, uint32_t delay);
status_t ar_timer_delete(ar_timer_t * timer);
status_t ar_timer_start(ar_timer_t * timer);
status_t ar_timer_stop(ar_timer_t * timer);
bool ar_timer_is_active(ar_timer_t * timer);
status_t ar_timer_set_delay(ar_timer_t * timer, uint32_t newDelay);
uint32_t ar_timer_get_delay(ar_timer_t * timer);
const char * ar_timer_get_name(ar_timer_t * timer);

uint32_t ar_get_milliseconds_per_tick(void);
static inline uint32_t ar_ticks_to_milliseconds(uint32_t ticks) { return ticks * ar_get_milliseconds_per_tick(); }
static inline uint32_t ar_milliseconds_to_ticks(uint32_t milliseconds) { return milliseconds / ar_get_milliseconds_per_tick(); }

void ar_kernel_enter_interrupt();
void ar_kernel_exit_interrupt();

//! @brief %Atomic add.
void ar_atomic_add(uint32_t * value, int32_t delta);

//! @brief %Atomic increment.
inline void ar_atomic_increment(uint32_t * value) { ar_atomic_add(value, 1); }

//! @brief %Atomic decrement.
inline void ar_atomic_decrement(uint32_t * value) { ar_atomic_add(value, -1); }

//! @brief %Atomic compare-and-swap operation.
bool ar_atomic_compare_and_swap(uint32_t * value, uint32_t expectedValue, uint32_t newValue);

#if defined(__cplusplus)
}
#endif

//! @}

#endif // _AR_KERNEL_H_
//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
