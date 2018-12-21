/*
 * Copyright (c) 2013-2018 Immo Software
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
#include <assert.h>

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

#if !defined(ARRAY_SIZE)
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#if !defined(WEAK)
#define WEAK __attribute__((weak))
#endif

#if !defined(ALWAYS_INLINE)
#define ALWAYS_INLINE __attribute__((always_inline))
#endif

enum
{
    //! Signature value written to the top of each thread's the stack. The scheduler looks
    //! for this value every time it activates a thread and halts if it is missing.
    kStackCheckValue = 0xdeadbeef,

    //! Value to fill the stack with for detection of max stack usage. All bytes of this
    //! fill pattern must be the same.
    kStackFillValue = 0xbabababa,
};

//! All events start with a 32-bit message on ITM port 31, composed of an event ID in the
//! top 8 bits plus a 24-bit value. Certain events are also followed by a 32-bit value in
//! ITM port 30.
enum _ar_trace_events
{
    kArTraceThreadSwitch = 1,   //!< 2 value: 0=previous thread's new state, 1=new thread id
    kArTraceThreadCreated = 2,  //!< 2 value: 0=unused, 1=new thread id
    kArTraceThreadDeleted = 3,  //!< 2 value: 0=unused, 1=deleted thread id
};

//! @brief Queue containing deferred actions.
//!
//! The deferred action queue is used to postpone kernel operations performed in interrupt context
//! until the scheduler runs, in the lowest possible interrupt priority. This is part of the
//! support for never disabling interrupts on Cortex-M.
//!
//! The deferred actions enqueued in this queue are composed of a function pointer and object
//! value. The function pointer points to a very small deferred action stub function that simply
//! calls the right kernel object operation routine with the correct parameters. If a deferred
//! action requires two parameters, a special #kActionExtraValue constant can be inserted
//! in the queue to mark an entry as holding the second parameter for the previous action.
typedef struct _ar_deferred_action_queue {
    //! @brief Constant used to mark an action queue entry as containing an extra argument for the
    //!     previous action.
    static const uint32_t kActionExtraValue = 0xfeedf00d;

    //! @brief The deferred action function pointer.
    typedef void (*deferred_action_t)(void * object, void * object2);

    volatile int32_t m_count;   //!< Number of queue entries.
    volatile int32_t m_first;   //!< First entry index.
    volatile int32_t m_last;    //!< Last entry index.
    struct _ar_deferred_action_queue_entry {
        deferred_action_t action; //!< Enqueued action.
        void * object;    //!< Kernel object or parameter for enqueued action.
    } m_entries[AR_DEFERRED_ACTION_QUEUE_SIZE]; //!< The deferred action queue entries.

    //! @brief Returns whether the queue is currently empty.
    bool isEmpty() const { return m_count == 0; }

    //! @brief Enqueues a new deferred action.
    ar_status_t post(deferred_action_t action, void * object);

    //! @brief Enqueues a new deferred action with two arguments.
    ar_status_t post(deferred_action_t action, void * object, void * arg);

protected:
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
    ar_list_t readyList;            //!< List of threads ready to run.
    ar_list_t suspendedList;        //!< List of suspended threads.
    ar_list_t sleepingList;         //!< List of sleeping threads.
    struct _ar_kernel_flags {
        uint32_t isRunning:1;           //!< True if the kernel has been started.
        uint32_t needsReschedule:1;     //!< True if we need to reschedule once the kernel is unlocked.
        uint32_t isRunningDeferred:1;   //!< True if the kernel is executing deferred actions.
        uint32_t needsRoundRobin:1;     //!< True if round-robin scheduling must be used.
        uint32_t _reservedFlags:28;
    } flags;                        //!< Kernel flags.
    uint32_t version;               //!< Argon version in BCD, same as #AR_VERSION.
    ar_deferred_action_queue_t deferredActions; //!< Actions deferred from interrupt context.
    volatile int32_t lockCount;     //!< Whether the kernel is locked.
    volatile uint32_t tickCount;    //!< Current tick count.
    int32_t missedTickCount;        //!< Number of ticks that occurred while the kernel was locked.
    uint32_t nextWakeup;            //!< Time of the next wakeup event.
    uint32_t threadIdCounter;       //!< Counter for generating unique thread IDs.
#if AR_ENABLE_SYSTEM_LOAD
    uint64_t lastLoadStart;         //!< Microseconds timestamp for last load computation start.
    uint64_t lastSwitchIn;          //!< Microseconds timestamp when current thread was switched in.
    uint32_t systemLoad;            //!< Per mille of system load from 0-1000.
#endif // AR_ENABLE_SYSTEM_LOAD
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
void ar_port_init_system();
void ar_port_init_tick_timer();
void ar_port_set_timer_delay(bool enable, uint32_t delay_us);
uint32_t ar_port_get_timer_elapsed_us();
void ar_port_prepare_stack(ar_thread_t * thread, uint32_t stackSize, void * param);
void ar_port_service_call();
bool ar_port_get_irq_state();
//@}

//! @name Kernel internals
//@{
bool ar_kernel_increment_tick_count(unsigned ticks);
void ar_kernel_enter_scheduler();
void ar_kernel_run_deferred_actions();
void ar_kernel_scheduler();
void ar_kernel_update_round_robin();
uint32_t ar_kernel_get_next_wakeup_time();
void ar_kernel_run_timers(ar_list_t & timersList);
int32_t ar_kernel_atomic_queue_insert(int32_t entryCount, volatile int32_t & qCount, volatile int32_t & qTail, int32_t qSize);
void ar_runloop_wake(ar_runloop_t * runloop);
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

#if AR_ENABLE_TRACE
//! @name Kernel trace
//@{
//! @brief
WEAK void ar_trace_init();

//! @brief
WEAK void ar_trace_1(uint8_t eventID, uint32_t data);

//! @brief
WEAK void ar_trace_2(uint8_t eventID, uint32_t data0, void * data1);
//@}
#else
static inline ALWAYS_INLINE void ar_trace_init() {}
static inline ALWAYS_INLINE void ar_trace_1(uint8_t eventID, uint32_t data) {}
static inline ALWAYS_INLINE void ar_trace_2(uint8_t eventID, uint32_t data0, void * data1) {}
#endif // AR_ENABLE_TRACE

// Inline list method implementation.
inline bool _ar_list::isEmpty() const { return m_head == NULL; }
inline void _ar_list::add(ar_thread_t * item) { add(&item->m_threadNode); }
inline void _ar_list::add(ar_timer_t * item) { add(&item->m_activeNode); }
inline void _ar_list::add(ar_queue_t * item) { add(&item->m_runLoopNode); }
inline void _ar_list::remove(ar_thread_t * item) { remove(&item->m_threadNode); }
inline void _ar_list::remove(ar_timer_t * item) { remove(&item->m_activeNode); }
inline void _ar_list::remove(ar_queue_t * item) { remove(&item->m_runLoopNode); }

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
        if (E)
        {
            ar_atomic_add32(&g_ar.lockCount, 1);
        }
        else
        {
            assert(g_ar.lockCount != 0);
            ar_atomic_add32(&g_ar.lockCount, -1);
        }
    }

    //! @brief Restores previous lock state.
    ~KernelGuard()
    {
        if (E)
        {
            assert(g_ar.lockCount != 0);
            ar_atomic_add32(&g_ar.lockCount, -1);
            if (g_ar.lockCount == 0 && g_ar.flags.needsReschedule && !g_ar.flags.isRunningDeferred)
            {
                ar_kernel_enter_scheduler();
            }
        }
        else
        {
            ar_atomic_add32(&g_ar.lockCount, 1);
        }
    }
};

typedef KernelGuard<true> KernelLock;       //!< Lock kernel.
typedef KernelGuard<false> KernelUnlock;    //!< Unlock kernel.

#endif // _AR_INTERNAL_H_
//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
