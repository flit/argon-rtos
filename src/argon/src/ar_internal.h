/*
 * Copyright (c) 2013-2016 Immo Software
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
 * @file ar_internal.h
 * @ingroup ar_port
 * @brief Header for the Argon RTOS.
 */

#if !defined(_AR_INTERNAL_H_)
#define _AR_INTERNAL_H_

#include "argon/argon.h"
#include "ar_port.h"
#include "ar_config.h"

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

enum
{
    //! Signature value written to the top of each thread's the stack. The scheduler looks
    //! for this value every time it activates a thread and halts if it is missing.
    kStackCheckValue = 0xdeadbeef
};

//! @brief Types of deferred actions.
typedef enum _ar_deferred_action_type {
    kArDeferredActionValue,
    kArDeferredSemaphorePut,
    kArDeferredSemaphoreGet,
    kArDeferredMutexPut,
    kArDeferredMutexGet,
    kArDeferredChannelSend,
    kArDeferredQueueSend,
    kArDeferredTimerStart,
    kArDeferredTimerStop,
} ar_deferred_action_type_t;

//! @brief Queue containing deferred actions.
//!
//! The deferred action types and objects are stored in separate arrays in order to allow for
//! the most compact possible storage in memory. With a queue size of 8 this saves 24 bytes.
typedef struct _ar_deferred_action_queue {
    volatile int32_t m_count;   //!< Number of queue entries.
    volatile int32_t m_first;   //!< First entry index.
    volatile int32_t m_last;    //!< Last entry index.
    ar_deferred_action_type_t m_actions[AR_DEFERRED_ACTION_QUEUE_SIZE]; //!< Enqueued actions.
    void * m_objects[AR_DEFERRED_ACTION_QUEUE_SIZE];    //!< Kernel objects for enqueued actions.

    //! @brief Returns whether the queue is currently empty.
    bool isEmpty() const { return m_count == 0; }

    //! @brief Reserves room to insert a number of new entries.
    //! @return First index in queue where the requested number of entries can be inserted. If there
    //!     is not room in the queue for the requested entries, then -1 is returned.
    int32_t insert(int32_t entryCount);
} ar_deferred_action_queue_t;

/*!
 * @brief Argon kernel state.
 */
typedef struct _ar_kernel {
    ar_thread_t * currentThread;    //!< The currently running thread.
    ar_kernel_port_data_t port;     //!< Port-specific kernel data.
    ar_list_t readyList;         //!< List of threads ready to run.
    ar_list_t suspendedList;     //!< List of suspended threads.
    ar_list_t sleepingList;      //!< List of sleeping threads.
    ar_deferred_action_queue_t deferredActions; //!< Actions deferred from interrupt context.
    uint32_t isRunning:1;                 //!< True if the kernel has been started.
    uint32_t needsReschedule:1;           //!< True if we need to reschedule once the kernel is unlocked.
    uint32_t _reservedFlags:30;
    volatile int32_t lockCount;     //!< Whether the kernel is locked.
    volatile uint32_t tickCount;    //!< Current tick count.
    int32_t missedTickCount;        //!< Number of ticks that occurred while the kernel was locked.
    uint32_t nextWakeup;            //!< Time of the next wakeup event.
    volatile unsigned systemLoad;   //!< Percent of system load from 0-100. The volatile is necessary so that the IAR optimizer doesn't remove the entire load calculation loop of the idle_entry() function.
    ar_thread_t idleThread;         //!< The lowest priority thread in the system. Executes only when no other threads are ready.
} ar_kernel_t;

#if AR_GLOBAL_OBJECT_LISTS
//! Contains linked lists of all the various Ar object types that have been created during runtime.
typedef struct _ar_all_objects {
    ar_list_t threads;          //!< All existing threads.
    ar_list_t semaphores;       //!< All existing semaphores.
    ar_list_t mutexes;          //!< All existing mutexes.
    ar_list_t channels;         //!< All existing channels;
    ar_list_t queues;           //!< All existing queues.
    ar_list_t timers;           //!< All existing timers.
    ar_list_t runloops;         //!< All existing runloops.
} ar_all_objects_t;

extern ar_all_objects_t g_ar_objects;
#endif // AR_GLOBAL_OBJECT_LISTS

extern ar_kernel_t g_ar;

//------------------------------------------------------------------------------
// API
//------------------------------------------------------------------------------

//! @name Porting
//@{
void ar_port_init_system(void);
void ar_port_init_tick_timer(void);
void ar_port_set_timer_delay(bool enable, uint32_t delay_us);
uint32_t ar_port_get_timer_elapsed_us();
void ar_port_prepare_stack(ar_thread_t * thread, uint32_t stackSize, void * param);
void ar_port_service_call(void);
bool ar_port_get_irq_state(void);
//@}

//! @name Kernel internals
//@{
bool ar_kernel_increment_tick_count(unsigned ticks);
void ar_kernel_enter_scheduler(void);
void ar_kernel_run_deferred_actions();
void ar_kernel_scheduler(void);
uint32_t ar_kernel_get_next_wakeup_time();
bool ar_kernel_run_timers(ar_list_t & timersList);
void ar_runloop_wake(ar_runloop_t * runloop);
//@}

//! @name Deferred actions
//@{
ar_status_t ar_post_deferred_action(ar_deferred_action_type_t action, void * object);
ar_status_t ar_post_deferred_action2(ar_deferred_action_type_t action, void * object, void * arg);
//@}

//! @name Internal routines
//@{
ar_status_t ar_semaphore_get_internal(ar_semaphore_t * sem, uint32_t timeout);
ar_status_t ar_semaphore_put_internal(ar_semaphore_t * sem);
ar_status_t ar_mutex_get_internal(ar_mutex_t * mutex, uint32_t timeout);
ar_status_t ar_mutex_put_internal(ar_mutex_t * mutex);
ar_status_t ar_timer_internal_start(ar_timer_t * timer, uint32_t wakeupTime);
ar_status_t ar_timer_stop_internal(ar_timer_t * timer);
ar_status_t ar_queue_send_internal(ar_queue_t * queue, const void * element, uint32_t timeout);
ar_status_t ar_channel_send_receive_internal(ar_channel_t * channel, bool isSending, ar_list_t & myDirList, ar_list_t & otherDirList, void * value, uint32_t timeout);
//@}

//! @name Thread entry point wrapper
//@{
//! @brief Thread entry point.
void ar_thread_wrapper(ar_thread_t * thread, void * param);
//@}

//! @name List sorting predicates
//@{
//! @brief Sort thread list by descending priority.
bool ar_thread_sort_by_priority(ar_list_node_t * a, ar_list_node_t * b);

//! @brief Sort thread list by ascending wakeup time.
bool ar_thread_sort_by_wakeup(ar_list_node_t * a, ar_list_node_t * b);

//! @brief Sort timer list by ascending wakeup time.
bool ar_timer_sort_by_wakeup(ar_list_node_t * a, ar_list_node_t * b);
//@}

//! @name Interrupt handlers
//@{
extern "C" void ar_kernel_periodic_timer_isr(void);
extern "C" uint32_t ar_kernel_yield_isr(uint32_t topOfStack);
//@}

// Inline list method implementation.
inline bool _ar_list::isEmpty() const { return m_head == NULL; }
inline void _ar_list::add(ar_thread_t * item) { add(&item->m_threadNode); }
inline void _ar_list::add(ar_timer_t * item) { add(&item->m_activeNode); }
inline void _ar_list::add(ar_queue_t * item) { add(&item->m_runLoopNode); }
inline void _ar_list::add(ar_channel_t * item) { add(&item->m_runLoopNode); }
inline void _ar_list::remove(ar_thread_t * item) { remove(&item->m_threadNode); }
inline void _ar_list::remove(ar_timer_t * item) { remove(&item->m_activeNode); }
inline void _ar_list::remove(ar_queue_t * item) { remove(&item->m_runLoopNode); }
inline void _ar_list::remove(ar_channel_t * item) { remove(&item->m_runLoopNode); }

/*!
 * @brief Utility class to temporarily lock or unlock the kernel.
 *
 * @param E The desired lock state, true for locked and false for unlocked.
 */
template <bool E>
class KernelGuard
{
public:
    //! @brief Saves lock state then modifies it.
    KernelGuard()
    {
//         m_savedMask = ar_port_set_lock(E);
        if (E)
        {
            ar_atomic_inc(&g_ar.lockCount);
        }
        else
        {
            if (g_ar.lockCount == 0)
            {
                Ar::_halt();
            }
            ar_atomic_dec(&g_ar.lockCount);
        }
    }

    //! @brief Restores previous lock state.
    ~KernelGuard()
    {
        if (E)
        {
            if (g_ar.lockCount == 0)
            {
                Ar::_halt();
            }
            ar_atomic_dec(&g_ar.lockCount);
//             ar_port_set_lock(!E);
            if (g_ar.lockCount == 0 && g_ar.needsReschedule)
            {
                ar_kernel_enter_scheduler();
            }
        }
        else
        {
            ar_atomic_inc(&g_ar.lockCount);
//             ar_port_set_lock(!E);
        }
    }

private:
//     bool m_savedMask;    //!< The lock state saved by the constructor.
};

typedef KernelGuard<true> KernelLock;  //!< Lock kernel.
typedef KernelGuard<false> KernelUnlock;    //!< Unlock kernel.

#endif // _AR_INTERNAL_H_
//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
