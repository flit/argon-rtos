/*
 * Copyright (c) 2007-2020 Immo Software
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
 * @file ar_kernel.cpp
 * @brief Source for Ar kernel.
 */

#include "ar_internal.h"
#include <assert.h>
#include <string.h>

using namespace Ar;

//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------

static void THREAD_STACK_OVERFLOW_DETECTED();
static void DEFERRED_ACTION_QUEUE_OVERFLOW_DETECTED();

static void idle_entry(void * param);

#if AR_ENABLE_SYSTEM_LOAD
static void ar_kernel_update_thread_loads();
#endif // AR_ENABLE_SYSTEM_LOAD

//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------

//! Global kernel state.
//!
//! @internal
//! The initializer for this struct sets the readyList and sleepingList sort
//! predicates so that those lists will be properly sorted when populated by
//! threads created through static initialization. Although static initialization
//! order is not guaranteed, we can be sure that initialization of `g_ar` from
//! .rodata will happen before static initializers are called.
//!
//! (Unfortunately we can't use C-style designated initializers until C++20.)
ar_kernel_t g_ar = {
        0,                                  // currentThread,
        {                                   // readyList
            0,                                  // m_head
            ar_thread_sort_by_priority,         // m_predicate
        },
        { 0 },                              // suspendedList
        {                                   // sleepingList
            0,                                  // m_head
            ar_thread_sort_by_wakeup,           // m_predicate
        },
        { 0 },                              // flags
        AR_VERSION,                         // version
        // the rest...
    };

#if AR_GLOBAL_OBJECT_LISTS
//! Global list of kernel objects.
ar_all_objects_t g_ar_objects = {0};
#endif // AR_GLOBAL_OBJECT_LISTS

//! The stack for the idle thread.
static uint8_t s_idleThreadStack[AR_IDLE_THREAD_STACK_SIZE];

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

//! @brief System idle thread entry point.
//!
//! This thread just spins forever.
//!
//! @param param Ignored.
#if !defined(__ICCARM__)
__attribute__((noreturn))
#endif
static void idle_entry(void * param)
{
    while (1)
    {
        while (g_ar.tickCount < g_ar.nextWakeup)
        {
        }

#if AR_ENABLE_IDLE_SLEEP
        __DSB();
        // Hitting this bit puts the processor to sleep until the next interrupt fires.
        __WFI();
        __ISB();
#endif // AR_ENABLE_IDLE_SLEEP
    }
}

//! Cause the scheduler to be run.
//!
//! If this function is called when the kernel is locked, a flag is set that
//! will cause the scheduler to be entered immediately upon the kernel being
//! unlocked.
void ar_kernel_enter_scheduler()
{
    // Do nothing if kernel isn't running yet.
    if (!g_ar.flags.isRunning)
    {
        return;
    }

    if (!g_ar.lockCount)
    {
        // Clear rescheduler.
        g_ar.flags.needsReschedule = false;

        // Call port-specific function to invoke the scheduler.
        ar_port_service_call();
    }
    else
    {
        g_ar.flags.needsReschedule = true;
    }
}

// See ar_kernel.h for documentation of this function.
void ar_kernel_run(void)
{
    // Assert if there is no thread ready to run.
    assert(g_ar.readyList.m_head);

    // Init some misc fields.
    // Note: we do _not_ init threadIdCounter since it will already have been incremented
    // some for any statically-initialized threads.
    // Note: list predicates were initialized by the g_ar initializer.
    g_ar.flags.needsReschedule = false;
    g_ar.flags.isRunningDeferred = false;
    g_ar.flags.needsRoundRobin = false;
    g_ar.nextWakeup = 0;
    g_ar.deferredActions.m_count = 0;
    g_ar.deferredActions.m_first = 0;
    g_ar.deferredActions.m_last = 0;

#if AR_ENABLE_SYSTEM_LOAD
    g_ar.lastLoadStart = ar_get_microseconds();
    g_ar.lastSwitchIn = 0;
    g_ar.systemLoad = 0;
#endif // AR_ENABLE_SYSTEM_LOAD

    // Create the idle thread. Priority 1 is passed to init function to pass the
    // assertion and then set to the correct 0 manually.
    ar_thread_create(&g_ar.idleThread, "idle", idle_entry, 0, s_idleThreadStack, sizeof(s_idleThreadStack), 1, kArSuspendThread);
    g_ar.idleThread.m_priority = 0;
    ar_thread_resume(&g_ar.idleThread);

    // Set up system tick timer
    ar_port_init_tick_timer();

    // Init port.
    ar_port_init_system();

    // Init trace.
    ar_trace_init();

    // We're now ready to run
    g_ar.flags.isRunning = true;

    // Enter into the scheduler. The yieldIsr() will see that s_currentThread
    // is NULL and ignore the stack pointer it was given. After the scheduler
    // runs, we return from the scheduler to a ready thread.
    ar_kernel_enter_scheduler();

    // should never reach here
    _halt();
}

void ar_kernel_periodic_timer_isr()
{
    // Exit immediately if the kernel isn't running.
    if (!g_ar.flags.isRunning)
    {
        return;
    }

    // If the kernel is locked, record that we missed this tick and come back as soon
    // as the kernel gets unlocked.
    if (g_ar.lockCount)
    {
        ar_atomic_add32(&g_ar.missedTickCount, 1);
        g_ar.flags.needsReschedule = true;
        return;
    }

#if AR_ENABLE_TICKLESS_IDLE
    // Get the elapsed time since the last timer tick.
    uint32_t us = ar_port_get_timer_elapsed_us();
    uint32_t elapsed_ticks = us / (kSchedulerQuanta_ms * 1000);
#else // AR_ENABLE_TICKLESS_IDLE
    uint32_t elapsed_ticks = 1;
#endif // AR_ENABLE_TICKLESS_IDLE

    // Process elapsed time. Invoke the scheduler if any threads were woken or if
    // round robin scheduling is in effect.
    if (ar_kernel_increment_tick_count(elapsed_ticks) || g_ar.flags.needsRoundRobin)
    {
        ar_port_service_call();
    }
}

//! @param topOfStack This parameter should be the stack pointer of the thread that was
//!     current when the timer IRQ fired.
//! @return The value of the current thread's stack pointer is returned. If the scheduler
//!     changed the current thread, this will be a different value from what was passed
//!     in @a topOfStack.
uint32_t ar_kernel_yield_isr(uint32_t topOfStack)
{
    assert(!g_ar.lockCount);

    // save top of stack for the thread we interrupted
    if (g_ar.currentThread)
    {
        g_ar.currentThread->m_stackPointer = reinterpret_cast<uint8_t *>(topOfStack);
    }

    // Handle any missed ticks.
    {
        uint32_t missedTicks = g_ar.missedTickCount;
        while (!ar_atomic_cas32(&g_ar.missedTickCount, missedTicks, 0))
        {
            missedTicks = g_ar.missedTickCount;
        }

        if (missedTicks)
        {
            ar_kernel_increment_tick_count(missedTicks);
        }
    }

    // Process any deferred actions.
    ar_kernel_run_deferred_actions();

#if AR_ENABLE_TICKLESS_IDLE
    // Process elapsed time to keep tick count up to date.
    uint32_t elapsed_ticks = ar_port_get_timer_elapsed_us() / (kSchedulerQuanta_ms * 1000);
    ar_kernel_increment_tick_count(elapsed_ticks);
#endif // AR_ENABLE_TICKLESS_IDLE

    // Run the scheduler. It will modify g_ar.currentThread if switching threads.
    ar_kernel_scheduler();
    g_ar.flags.needsReschedule = 0;

    // The idle thread prevents this condition.
    assert(g_ar.currentThread);

    // return the new thread's stack pointer
    return (uint32_t)g_ar.currentThread->m_stackPointer;
}

//! Increments the system tick count and wakes any sleeping threads whose wakeup time
//! has arrived. If the thread's state is #kArThreadBlocked then its unblock status
//! is set to #kArTimeoutError.
//!
//! @param ticks The number of ticks that have elapsed. Normally this will only be 1,
//!     and must be at least 1, but may be higher if interrupts are disabled for a
//!     long time.
//! @return Flag indicating whether any threads were modified.
bool ar_kernel_increment_tick_count(unsigned ticks)
{
//     assert(ticks > 0);

    // Increment tick count.
    g_ar.tickCount += ticks;

    // Compare against next wakeup time we previously computed.
    if (g_ar.tickCount < g_ar.nextWakeup)
    {
        return false;
    }

    // Scan list of sleeping threads to see if any should wake up.
    ar_list_node_t * node = g_ar.sleepingList.m_head;
    bool wasThreadWoken = false;

    if (node)
    {
        do {
            ar_thread_t * thread = node->getObject<ar_thread_t>();
            ar_list_node_t * next = node->m_next;

            // Is it time to wake this thread?
            if (g_ar.tickCount >= thread->m_wakeupTime)
            {
                wasThreadWoken = true;

                // State-specific actions
                switch (thread->m_state)
                {
                    case kArThreadSleeping:
                        // The thread was just sleeping.
                        break;

                    case kArThreadBlocked:
                        // The thread has timed out waiting for a resource.
                        thread->m_unblockStatus = kArTimeoutError;
                        break;

                    default:
                        // Should not have threads in other states on this list!
                        _halt();
                }

                // Put thread in ready state.
                g_ar.sleepingList.remove(thread);
                thread->m_state = kArThreadReady;
                g_ar.readyList.add(thread);
                ar_kernel_update_round_robin();
            }
            // Exit loop if we hit a thread with a wakeup time in the future. The sleeping list
            // is sorted, so there will be no further threads to handle in the list.
            else
            {
                break;
            }

            node = next;
        } while (g_ar.sleepingList.m_head && node != g_ar.sleepingList.m_head);
    }

    return wasThreadWoken;
}

//! @brief Execute actions deferred from interrupt context.
void ar_kernel_run_deferred_actions()
{
    // Kernel must not be locked. However, executing deferred actions will temporarily
    // lock the kernel below.
    assert(g_ar.lockCount == 0);

    g_ar.flags.isRunningDeferred = 1;

    // Pull actions from the head of the queue and execute them.
    ar_deferred_action_queue_t & queue = g_ar.deferredActions;
    int32_t i = queue.m_first;
    while (!queue.isEmpty())
    {
        int32_t iPlusOne = i + 1;
        if (iPlusOne >= AR_DEFERRED_ACTION_QUEUE_SIZE)
        {
            iPlusOne = 0;
        }

        ar_deferred_action_queue_t::_ar_deferred_action_queue_entry & entry = queue.m_entries[i];

        // Ignore action entries that contain an extra argument value for the previous action.
        if (reinterpret_cast<uint32_t>(entry.action) != ar_deferred_action_queue_t::kActionExtraValue)
        {
            assert(entry.action);
            entry.action(entry.object, queue.m_entries[iPlusOne].object);
        }

        // Atomically remove the entry we just processed from the queue.
        // This is the only code that modifies the m_first member of the queue.
        i = iPlusOne;
        ar_atomic_add32(&queue.m_count, -1);
        queue.m_first = i;
    }

    g_ar.flags.isRunningDeferred = 0;
}

//! @brief Function to make it clear what happened.
void THREAD_STACK_OVERFLOW_DETECTED()
{
    _halt();
}

#if AR_ENABLE_SYSTEM_LOAD
//! Update the CPU load for all threads based on the current load accumulator values.
//! Also updates the total system load.
void ar_kernel_update_thread_loads()
{
    // Scan threads in all states in a way that doesn't depend on g_ar_objects.
    ar_list_t * const threadLists[] = {
#if AR_GLOBAL_OBJECT_LISTS
        &g_ar_objects.threads
#else
        &g_ar.readyList, &g_ar.suspendedList, &g_ar.sleepingList
#endif // AR_GLOBAL_OBJECT_LISTS
    };
    uint32_t i = 0;
    for (; i < ARRAY_SIZE(threadLists); ++i)
    {
        if (!threadLists[i]->isEmpty())
        {
            ar_list_node_t * node = threadLists[i]->m_head;
            do {
                ar_thread_t * thread = node->getObject<ar_thread_t>();
                thread->m_permilleCpu = 1000 * thread->m_loadAccumulator / AR_SYSTEM_LOAD_SAMPLE_PERIOD;
                thread->m_loadAccumulator = 0;
                node = node->m_next;
            } while (node != threadLists[i]->m_head);
        }
    }

    // Update total system load based on the idle thread's load.
    g_ar.systemLoad = 1000 - g_ar.idleThread.m_permilleCpu;
}
#endif // AR_ENABLE_SYSTEM_LOAD

void ar_kernel_scheduler()
{
    // There must always be at least one thread on the ready list.
    assert(g_ar.readyList.m_head);

#if AR_ENABLE_SYSTEM_LOAD
    // Update thread active time accumulator.
    if (g_ar.currentThread)
    {
        uint64_t now = ar_get_microseconds();
        uint64_t w = now - g_ar.lastLoadStart;
        if (w >= AR_SYSTEM_LOAD_SAMPLE_PERIOD)
        {
            uint64_t o = w - AR_SYSTEM_LOAD_SAMPLE_PERIOD;
            g_ar.currentThread->m_loadAccumulator += static_cast<uint32_t>(now - g_ar.lastSwitchIn - o);
            ar_kernel_update_thread_loads();
            g_ar.lastLoadStart = now - o;
            g_ar.lastSwitchIn = g_ar.lastLoadStart;
        }

        g_ar.currentThread->m_loadAccumulator += static_cast<uint32_t>(now - g_ar.lastSwitchIn);
        g_ar.lastSwitchIn = now;
    }
#endif // AR_ENABLE_SYSTEM_LOAD

    // Find the next ready thread.
    ar_list_node_t * firstNode = g_ar.readyList.m_head;
    ar_thread_t * first = firstNode->getObject<ar_thread_t>();
    ar_thread_t * highest = NULL;

    // Handle these cases by selecting the first thread in the ready list, which will have the
    // highest priority since the ready list is sorted.
    // 1. The first time the scheduler runs and g_ar.currentThread is NULL.
    // 2. The current thread was suspended.
    // 3. Higher priority thread became ready.
    if (!g_ar.currentThread
        || g_ar.currentThread->m_state != kArThreadRunning
        || first->m_priority > g_ar.currentThread->m_priority)
    {
        highest = first;
    }
    // Else handle these cases:
    // 2. We're performing round-robin scheduling.
    // 3. Shouldn't switch the thread.
    else
    {
        // Start with the current thread.
        ar_list_node_t * startNode = &g_ar.currentThread->m_threadNode;
        uint8_t startPriority = startNode->getObject<ar_thread_t>()->m_priority;

        // Pick up the next thread in the ready list.
        ar_list_node_t * nextNode = startNode->m_next;
        highest = nextNode->getObject<ar_thread_t>();

        // If the next thread is not the same priority, then go back to the start of the ready list.
        if (highest->m_priority != startPriority)
        {
            highest = first;
            assert(highest->m_priority == startPriority);
        }
    }

    // Switch to newly selected thread.
    assert(highest);
    if (highest != g_ar.currentThread)
    {
        if (g_ar.currentThread && g_ar.currentThread->m_state == kArThreadRunning)
        {
            g_ar.currentThread->m_state = kArThreadReady;
        }

        ar_trace_1(kArTraceThreadSwitch, (g_ar.currentThread->m_state << 16) | highest->m_uniqueId);

        highest->m_state = kArThreadRunning;
        g_ar.currentThread = highest;
    }

    // Check for stack overflow on the current thread.
    if (g_ar.currentThread)
    {
        uint32_t current = reinterpret_cast<uint32_t>(g_ar.currentThread->m_stackPointer);
        uint32_t bottom = reinterpret_cast<uint32_t>(g_ar.currentThread->m_stackBottom);
        uint32_t check = *(g_ar.currentThread->m_stackBottom);
        if ((current < bottom) || (check != kStackCheckValue))
        {
            THREAD_STACK_OVERFLOW_DETECTED();
        }
    }

#if AR_ENABLE_TICKLESS_IDLE
    // Compute delay until next wakeup event and adjust timer.
    uint32_t wakeup = ar_kernel_get_next_wakeup_time();
    if (wakeup != g_ar.nextWakeup)
    {
        g_ar.nextWakeup = wakeup;
        uint32_t delay = 0;
        if (g_ar.nextWakeup && g_ar.nextWakeup > g_ar.tickCount)
        {
            delay = (g_ar.nextWakeup - g_ar.tickCount) * kSchedulerQuanta_ms * 1000;
        }
        bool enable = (g_ar.nextWakeup != 0);
        ar_port_set_timer_delay(enable, delay);
    }
#endif // AR_ENABLE_TICKLESS_IDLE
}

//! @brief Cache whether round-robin scheduling needs to be used.
//!
//! Round-robin is required if there are multiple ready threads with the same priority. Since the
//! ready list is sorted by priority, we can just check the first two nodes to see if they are the
//! same priority.
void ar_kernel_update_round_robin()
{
    ar_list_node_t * node = g_ar.readyList.m_head;
    assert(node);
    uint8_t pri1 = node->getObject<ar_thread_t>()->m_priority;
    if (node->m_next != node)
    {
        node = node->m_next;
        uint8_t pri2 = node->getObject<ar_thread_t>()->m_priority;

        g_ar.flags.needsRoundRobin = (pri1 == pri2);
    }
    else
    {
        g_ar.flags.needsRoundRobin = false;
    }
}

//! @brief Determine the delay to the next wakeup event.
//!
//! Wakeup events are either sleeping threads that are scheduled to wake, or a timer that is
//! scheduled to fire.
//!
//! @return The number of ticks until the next wakup event. If the result is 0, then there are no
//!     wakeup events pending.
uint32_t ar_kernel_get_next_wakeup_time()
{
    uint32_t wakeup = 0;

    // See if round-robin needs to be used.
    if (g_ar.flags.needsRoundRobin)
    {
        // No need to check sleeping threads!
        return g_ar.tickCount + 1;
    }

    // Check for a sleeping thread. The sleeping list is sorted by wakeup time, so we only
    // need to look at the list head.
    ar_list_node_t * node = g_ar.sleepingList.m_head;
    if (node)
    {
        ar_thread_t * thread = node->getObject<ar_thread_t>();
        uint32_t sleepWakeup = thread->m_wakeupTime;
        if (wakeup == 0 || sleepWakeup < wakeup)
        {
            wakeup = sleepWakeup;
        }
    }

    return wakeup;
}

// See ar_kernel.h for documentation of this function.
bool ar_kernel_is_running(void)
{
    return g_ar.flags.isRunning;
}

// See ar_kernel.h for documentation of this function.
uint32_t ar_get_system_load(void)
{
#if AR_ENABLE_SYSTEM_LOAD
    return g_ar.systemLoad;
#else // AR_ENABLE_SYSTEM_LOAD
    return 0;
#endif // AR_ENABLE_SYSTEM_LOAD
}

// See ar_kernel.h for documentation of this function.
uint32_t ar_get_tick_count(void)
{
#if AR_ENABLE_TICKLESS_IDLE
    uint32_t elapsed_ticks = ar_port_get_timer_elapsed_us() / (kSchedulerQuanta_ms * 1000);
    return g_ar.tickCount + elapsed_ticks;
#else
    return g_ar.tickCount;
#endif // AR_ENABLE_TICKLESS_IDLE
}

// See ar_kernel.h for documentation of this function.
uint32_t ar_get_millisecond_count(void)
{
#if AR_ENABLE_TICKLESS_IDLE
    uint32_t elapsed_ms = ar_port_get_timer_elapsed_us() / 1000;
    return g_ar.tickCount * ar_get_milliseconds_per_tick() + elapsed_ms;
#else
    return g_ar.tickCount * ar_get_milliseconds_per_tick();
#endif // AR_ENABLE_TICKLESS_IDLE
}

//! Updates the list node's links and those of @a node so that the object is inserted before
//! @node in the list.
//!
//! @note This method does not update the list head. To actually insert a node into the list you
//!     need to use _ar_list::add().
void _ar_list_node::insertBefore(ar_list_node_t * node)
{
    m_next = node;
    m_prev = node->m_prev;
    node->m_prev->m_next = this;
    node->m_prev = this;
}

//! Performs a linear search of the linked list.
bool _ar_list::contains(ar_list_node_t * item)
{
    // Check if the node is even on a list.
    if (item->m_next == NULL || item->m_prev == NULL)
    {
        return false;
    }

    if (m_head)
    {
        ar_list_node_t * node = m_head;
        do {
            if (node == item)
            {
                // Matching node was found in the list.
                return true;
            }
            node = node->m_next;
        } while (node != m_head);
    }

    return false;
}

//! The item is inserted in either FIFO or sorted order, depending on whether the predicate
//! member is set. If the predicate is NULL, the new list item will be inserted at the
//! end of the list, maintaining FIFO order.
//!
//! If the predicate function is provided, then it is used to search for the insert position on the
//! list. The new item will be inserted before the first existing list item for which the predicate
//! function returns true when called with its first parameter set to the new item and second
//! parameter set to the existing item.
//!
//! The list is maintained as a doubly-linked circular list. The last item in the list has its
//! next link set to the head of the list, and vice versa for the head's previous link. If there is
//! only one item in the list, both its next and previous links point to itself.
//!
//! @param item The item to insert into the list. The item must not already be on the list.
void _ar_list::add(ar_list_node_t * item)
{
    assert(item->m_next == NULL && item->m_prev == NULL);

    // Handle an empty list.
    if (!m_head)
    {
        m_head = item;
        item->m_next = item;
        item->m_prev = item;
    }
    // Insert at end of list if there is no sorting predicate, or if the item sorts after the
    // last item in the list.
    else if (!m_predicate || !m_predicate(item, m_head->m_prev))
    {
        item->insertBefore(m_head);
    }
    // Otherwise, search for the sorted position in the list for the item to be inserted.
    else
    {
        // Insert sorted by priority.
        ar_list_node_t * node = m_head;

        do {
            if (m_predicate(item, node))
            {
                item->insertBefore(node);

                if (node == m_head)
                {
                    m_head = item;
                }

                break;
            }

            node = node->m_next;
        } while (node != m_head);
    }

#if AR_ENABLE_LIST_CHECKS
    check();
#endif // AR_ENABLE_LIST_CHECKS
}

//! If the specified item is not on the list, nothing happens. Items are compared only by pointer
//! value, *not* by using the predicate function.
//!
//! @param item The item to remove from the list.
void _ar_list::remove(ar_list_node_t * item)
{
    // the list must not be empty
    assert(m_head != NULL);
    assert(item->m_next);
    assert(item->m_prev);
#if AR_ENABLE_LIST_CHECKS
    assert(contains(item));
#endif // AR_ENABLE_LIST_CHECKS

    // Adjust other nodes' links.
    item->m_prev->m_next = item->m_next;
    item->m_next->m_prev = item->m_prev;

    // Special case for removing the list head.
    if (m_head == item)
    {
        // Handle a single item list by clearing the list head.
        if (item->m_next == m_head)
        {
            m_head = NULL;
        }
        // Otherwise just update the list head to the second list element.
        else
        {
            m_head = item->m_next;
        }
    }

    // Clear links.
    item->m_next = NULL;
    item->m_prev = NULL;

#if AR_ENABLE_LIST_CHECKS
    check();
#endif // AR_ENABLE_LIST_CHECKS
}

#if AR_ENABLE_LIST_CHECKS
void _ar_list::check()
{
    const uint32_t kNumNodes = 20;
    ar_list_node_t * nodes[kNumNodes] = {0};
    uint32_t count = 0;
    uint32_t i;

    // Handle empty list.
    if (isEmpty())
    {
        return;
    }

    // Build array of all nodes in the list.
    ar_list_node_t * node = m_head;
    bool loop = true;
    while (loop)
    {
        // Save this node in the list.
        nodes[count] = node;
        ++count;
        if (count == kNumNodes - 1)
        {
            // More nodes than we have room for!
            _halt();
        }

        node = node->m_next;

        // Compare the next ptr against every node we've seen so far. If we find
        // a match, exit the loop.
        for (i = 0; i < count; ++i)
        {
            if (node == nodes[i])
            {
                loop = false;
                break;
            }
        }
    }

    // Scan the nodes array and verify all links.
    for (i = 0; i < count; ++i)
    {
        uint32_t prev = (i == 0) ? (count - 1) : (i  - 1);
        uint32_t next = (i == count - 1) ? 0 : (i + 1);

        node = nodes[i];
        if (node->m_next != nodes[next])
        {
            _halt();
        }
        if (node->m_prev != nodes[prev])
        {
            _halt();
        }
    }
}
#endif // AR_ENABLE_LIST_CHECKS

//! @brief Atomically allocate entries at the end of a queue.
int32_t ar_kernel_atomic_queue_insert(int32_t entryCount, volatile int32_t & qCount, volatile int32_t & qTail, int32_t qSize)
{
    // Increment queue entry count.
    int32_t count;
    do {
        count = qCount;

        // Check if queue is full.
        if (count + entryCount > qSize)
        {
            return -1;
        }
    } while (!ar_atomic_cas32(&qCount, count, count + entryCount));

    // Increment last entry index.
    int32_t last;
    int32_t newLast;
    do {
        last = qTail;
        newLast = (last + entryCount) % qSize;
    } while (!ar_atomic_cas32(&qTail, last, newLast));

    return last;
}

//! @brief There is no more room available in the deferred action queue.
void DEFERRED_ACTION_QUEUE_OVERFLOW_DETECTED()
{
    _halt();
}

int32_t _ar_deferred_action_queue::insert(int32_t entryCount)
{
    int32_t last = ar_kernel_atomic_queue_insert(entryCount, m_count, m_last, AR_DEFERRED_ACTION_QUEUE_SIZE);
    if (last == -1)
    {
        DEFERRED_ACTION_QUEUE_OVERFLOW_DETECTED();
    }
    return last;
}

ar_status_t _ar_deferred_action_queue::post(deferred_action_t action, void * object)
{
    int32_t index = insert(1);
    if (index < 0)
    {
        assert(false);
        return kArQueueFullError;
    }

    m_entries[index].action = action;
    m_entries[index].object = object;

    ar_kernel_enter_scheduler();

    return kArSuccess;
}

ar_status_t _ar_deferred_action_queue::post(deferred_action_t action, void * object, void * arg)
{
    int32_t index = insert(2);
    if (index < 0)
    {
        assert(false);
        return kArQueueFullError;
    }

    m_entries[index].action = action;
    m_entries[index].object = object;

    ++index;
    if (index >= AR_DEFERRED_ACTION_QUEUE_SIZE)
    {
        index = 0;
    }
    m_entries[index].action = reinterpret_cast<deferred_action_t>(kActionExtraValue);
    m_entries[index].object = arg;

    ar_kernel_enter_scheduler();

    return kArSuccess;
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
