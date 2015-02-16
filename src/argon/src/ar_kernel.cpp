/*
 * Copyright (c) 2007-2015 Immo Software
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
 * @file ar_kernel.cpp
 * @brief Source for Ar kernel.
 */

#include "ar_internal.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

using namespace Ar;

//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------

static void THREAD_STACK_OVERFLOW_DETECTED();

//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------

//! Global kernel state.
ar_kernel_t g_ar = {0};

//! The stack for the idle thread.
static uint8_t s_idleThreadStack[AR_IDLE_THREAD_STACK_SIZE];

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

//! @brief System idle thread entry point.
//!
//! This thread just spins forever.
//!
//! If the #AR_ENABLE_SYSTEM_LOAD define has been set to 1 then this thread will
//! also calculate the average system load once per second. The system load is
//! accessible with the Kernel::getSystemLoad() static member.
//!
//! @param param Ignored.
void idle_entry(void * param)
{
#if AR_ENABLE_SYSTEM_LOAD
    uint32_t start;
    uint32_t last;
    uint32_t ticks;
    uint32_t skipped = 0;

    start = g_ar.tickCount;
    last = start;
#endif // AR_ENABLE_SYSTEM_LOAD

    while (1)
    {
        // Check if we need to handle a timer.
        if (g_ar.activeTimers.m_head)
        {
            ar_list_node_t * timerNode = g_ar.activeTimers.m_head;
            bool handledTimer = false;
            while (timerNode)
            {
                ar_timer_t * timer = timerNode->getObject<ar_timer_t>();
                assert(timer);

                if (timer->m_wakeupTime > g_ar.tickCount)
                {
                    break;
                }

                // Invoke the timer callback.
                assert(timer->m_callback);
                timer->m_callback(timer, timer->m_param);

                switch (timer->m_mode)
                {
                    case kArOneShotTimer:
                        // Stop a one shot timer after it has fired.
                        ar_timer_stop(timer);
                        break;

                    case kArPeriodicTimer:
                        // Restart a periodic timer.
                        ar_timer_start(timer);
                        break;
                }

                handledTimer = true;
                timerNode = timerNode->m_next;
            }

            // If we handled any timers, we need to set our priority back to the normal idle thread
            // priority. The tick handler will have raised our priority to the max in order to handle
            // the expired timers.
            if (handledTimer)
            {
                ar_thread_set_priority(&g_ar.idleThread, kArIdleThreadPriority);
            }
        }

        // Compute system load.
#if AR_ENABLE_SYSTEM_LOAD
        ticks = g_ar.tickCount;

        if (ticks != last)
        {
            uint32_t diff = ticks - last;

            if (ticks - start >= 100)
            {
                unsigned s = start + 100 - ticks;

                if (diff - 1 > s)
                {
                    skipped += s;
                }
                else
                {
                    skipped += diff - 1;
                }

                g_ar.systemLoad = skipped;

                // start over counting
                if (diff - 1 > s)
                {
                    skipped = diff - 1 - s;
                }
                else
                {
                    skipped = 0;
                }
                start = ticks;
            }
            else
            {
                skipped += diff - 1;
            }

            last = ticks;
        }
#endif // AR_ENABLE_SYSTEM_LOAD

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
//! This function must not be called when the kernel is locked.
void ar_kernel_enter_scheduler(void)
{
    // Do nothing if kernel isn't running yet.
    if (!g_ar.isRunning)
    {
        return;
    }

    if (!g_ar.lockCount)
    {
        // Clear rescheduler.
        g_ar.needsReschedule = false;

        // Call port-specific function to invoke the scheduler.
        ar_port_service_call();
    }
    else
    {
        assert(false);
    }
}

// See ar_kernel.h for documentation of this function.
void ar_kernel_run(void)
{
    // Assert if there is no thread ready to run.
    assert(g_ar.readyList.m_head);

    // Init some misc fields.
    g_ar.needsReschedule = false;
    g_ar.nextWakeup = 0;
    g_ar.deferredActions.m_count = 0;

    // Init list predicates.
    g_ar.readyList.m_predicate = ar_thread_sort_by_priority;
    g_ar.suspendedList.m_predicate = NULL;
    g_ar.sleepingList.m_predicate = ar_thread_sort_by_wakeup;
    g_ar.activeTimers.m_predicate = ar_timer_sort_by_wakeup;

    // Create the idle thread. Priority 1 is passed to init function to pass the
    // assertion and then set to the correct 0 manually.
    ar_thread_create(&g_ar.idleThread, "idle", idle_entry, 0, s_idleThreadStack, sizeof(s_idleThreadStack), 1, kArSuspendThread);
    g_ar.idleThread.m_priority = 0;
    ar_thread_resume(&g_ar.idleThread);

    // Set up system tick timer
    ar_port_init_tick_timer();

    // Init port.
    ar_port_init_system();

    // We're now ready to run
    g_ar.isRunning = true;

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
    if (!g_ar.isRunning)
    {
        return;
    }

    // Get the elapsed time since the last timer tick.
#if AR_ENABLE_TICKLESS_IDLE
    uint32_t elapsed_ms = ar_port_get_timer_elapsed_us() / 10000;

    // Process elapsed time. Invoke the scheduler if any threads were woken.
    if (ar_kernel_increment_tick_count(elapsed_ms))
    {
        if (g_ar.lockCount)
        {
            g_ar.needsReschedule = true;
        }
        else
        {
            ar_port_service_call();
        }
    }
#else // AR_ENABLE_TICKLESS_IDLE
    ar_kernel_increment_tick_count(1);
    if (g_ar.lockCount)
    {
        g_ar.needsReschedule = true;
    }
    else
    {
        ar_port_service_call();
    }
#endif // AR_ENABLE_TICKLESS_IDLE

    // This case should never happen because of the idle thread.
    assert(g_ar.currentThread);
}

//! @param topOfStack This parameter should be the stack pointer of the thread that was
//!     current when the timer IRQ fired.
//! @return The value of the current thread's stack pointer is returned. If the scheduler
//!     changed the current thread, this will be a different value from what was passed
//!     in @a topOfStack.
uint32_t ar_kernel_yield_isr(uint32_t topOfStack)
{
    // save top of stack for the thread we interrupted
    if (g_ar.currentThread)
    {
        g_ar.currentThread->m_stackPointer = reinterpret_cast<uint8_t *>(topOfStack);
    }

    // Process any deferred actions.
    ar_kernel_run_deferred_actions();

#if AR_ENABLE_TICKLESS_IDLE
    // Process elapsed time to keep tick count up to date.
    uint32_t elapsed_ms = ar_port_get_timer_elapsed_us() / 10000;
    ar_kernel_increment_tick_count(elapsed_ms);
#endif // AR_ENABLE_TICKLESS_IDLE

    // Run the scheduler. It will modify g_ar.currentThread if switching threads.
    ar_kernel_scheduler();

    // The idle thread prevents this condition.
    assert(g_ar.currentThread);

    // return the new thread's stack pointer
    return (uint32_t)g_ar.currentThread->m_stackPointer;
}

//! Increments the system tick count and wakes any sleeping threads whose wakeup time
//! has arrived. If the thread's state is #kArThreadBlocked then its unblock status
//! is set to #kArTimeoutError.
//!
//! This function also checks if any timers have expired. If so, it changes the idle thread's
//! priority to be the maximum, so it can immediately handle the timers.
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

    // Check for an active timer whose wakeup time has expired.
    if (g_ar.activeTimers.m_head)
    {
        ar_timer_t * timer = g_ar.activeTimers.m_head->getObject<ar_timer_t>();
        if (timer->m_wakeupTime <= g_ar.tickCount)
        {
            // Raise the idle thread priority to maximum so it can execute the timer.
            // The idle thread will reduce its own priority when done.
            ar_thread_set_priority(&g_ar.idleThread, kArMaxThreadPriority);
        }
    }

    return wasThreadWoken;
}

//! @brief Execute actions deferred from interrupt context.
void ar_kernel_run_deferred_actions()
{
    // Nothing to do if the queue is empty.
    if (g_ar.deferredActions.isEmpty())
    {
        return;
    }

    assert(g_ar.lockCount == 0);

    // Examine and process each action in sequence.
    int i = 0;
    ar_deferred_action_queue_t & queue = g_ar.deferredActions;
    for (; i < queue.m_count; ++i)
    {
        void * object = queue.m_objects[i];
        switch (queue.m_actions[i])
        {
            case kArDeferredActionValue:
                // This entry contained the value for the previous action.
                break;
            case kArDeferredSemaphorePut:
                ar_semaphore_put(reinterpret_cast<ar_semaphore_t *>(object));
                break;
            case kArDeferredSemaphoreGet:
                ar_semaphore_get(reinterpret_cast<ar_semaphore_t *>(object), kArNoTimeout);
                break;
            case kArDeferredMutexPut:
                ar_mutex_put(reinterpret_cast<ar_mutex_t *>(object));
                break;
            case kArDeferredMutexGet:
                ar_mutex_get(reinterpret_cast<ar_mutex_t *>(object), kArNoTimeout);
                break;
            case kArDeferredChannelSend:
                assert(i + 1 < queue.m_count);
                ar_channel_send(reinterpret_cast<ar_channel_t *>(object), queue.m_objects[i + 1], kArNoTimeout);
                break;
            case kArDeferredQueueSend:
                assert(i + 1 < queue.m_count);
                ar_queue_send(reinterpret_cast<ar_queue_t *>(object), queue.m_objects[i + 1], kArNoTimeout);
                break;
            case kArDeferredTimerStart:
                ar_timer_start(reinterpret_cast<ar_timer_t *>(object));
                break;
            case kArDeferredTimerStop:
                ar_timer_stop(reinterpret_cast<ar_timer_t *>(object));
                break;
        }
    }

    // Clear the queue.
    g_ar.deferredActions.m_count = 0;
}

//! @brief Function to make it clear what happened.
void THREAD_STACK_OVERFLOW_DETECTED()
{
    _halt();
}

void ar_kernel_scheduler()
{
    // There must always be at least one thread on the ready list.
    assert(g_ar.readyList.m_head);

    // Find the next ready thread using a round-robin search algorithm.
    ar_list_node_t * start;

    // Handle both the first time the scheduler runs and g_ar.currentThread is NULL, and the case where
    // the current thread was suspended. For both cases we want to start searching at the beginning
    // of the ready list. Otherwise start searching at the current thread.
    if (!g_ar.currentThread || g_ar.currentThread->m_state != kArThreadRunning)
    {
        start = g_ar.readyList.m_head;
    }
    else
    {
        start = &g_ar.currentThread->m_threadNode;
    }

    assert(start);
    ar_list_node_t * next = start;
    ar_thread_t * highest = next->getObject<ar_thread_t>();
    uint8_t priority = highest->m_priority;

    // Iterate over the ready list, finding the highest priority thread.
    do {
        ar_thread_t * nextThread = next->getObject<ar_thread_t>();
        if (nextThread->m_state == kArThreadReady && nextThread->m_priority > priority)
        {
            highest = nextThread;
            priority = nextThread->m_priority;
        }

        next = next->m_next;
    } while (next != start);

    // Switch to newly selected thread.
    assert(highest);
    if (highest != g_ar.currentThread)
    {
        if (g_ar.currentThread && g_ar.currentThread->m_state == kArThreadRunning)
        {
            g_ar.currentThread->m_state = kArThreadReady;
        }

        highest->m_state = kArThreadRunning;
        g_ar.currentThread = highest;
    }

    // Check for stack overflow on the selected thread.
    assert(g_ar.currentThread);
    uint32_t check = *(g_ar.currentThread->m_stackBottom);
    if (check != kStackCheckValue)
    {
        THREAD_STACK_OVERFLOW_DETECTED();
    }

#if AR_ENABLE_TICKLESS_IDLE
    // Compute delay until next wakeup event and adjust timer.
    g_ar.nextWakeup = ar_kernel_get_next_wakeup_time();
    uint32_t delay = 0;
    if (g_ar.nextWakeup && g_ar.nextWakeup > g_ar.tickCount)
    {
        delay = (g_ar.nextWakeup - g_ar.tickCount) * 10000;
        if (delay == 0)
        {
            printf("delay is 0\n");
        }
    }
    bool enable = (g_ar.nextWakeup != 0);
    ar_port_set_timer_delay(enable, delay);
#endif // AR_ENABLE_TICKLESS_IDLE
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
    ar_list_node_t * node;
    ar_thread_t * thread;

    // See if round-robin needs to be used. Round-robin is required if there are multiple ready
    // threads with the same priority. Since the ready list is sorted by priority, we can just
    // check the first two nodes to see if they are the same priority.
    node = g_ar.readyList.m_head;
    assert(node);
    uint8_t pri1 = node->getObject<ar_thread_t>()->m_priority;
    node = node->m_next;
    if (node)
    {
        uint8_t pri2 = node->getObject<ar_thread_t>()->m_priority;

        // Check
        if (pri1 == pri2)
        {
            wakeup = g_ar.tickCount + 1;
        }
    }

    // Check for a sleeping thread. The sleeping list is sorted by wakeup time, so we only
    // need to look at the list head.
    node = g_ar.sleepingList.m_head;
    if (node)
    {
        thread = node->getObject<ar_thread_t>();
        uint32_t sleepWakeup = thread->m_wakeupTime;
        if (wakeup == 0 || sleepWakeup < wakeup)
        {
            wakeup = sleepWakeup;
        }
    }

    // Check for an active timer. Again, the list is sorted by wakeup time.
    node = g_ar.activeTimers.m_head;
    if (node)
    {
        ar_timer_t * timer = node->getObject<ar_timer_t>();
        uint32_t timerWakeup = timer->m_wakeupTime;

        if (wakeup == 0 || timerWakeup < wakeup)
        {
            wakeup = timerWakeup;
        }
    }

    return wakeup;
}

// See ar_kernel.h for documentation of this function.
bool ar_kernel_is_running(void)
{
    return g_ar.isRunning;
}

// See ar_kernel.h for documentation of this function.
uint32_t ar_get_system_load(void)
{
    return g_ar.systemLoad;
}

// See ar_kernel.h for documentation of this function.
uint32_t ar_get_tick_count(void)
{
    return g_ar.tickCount;
}

// See ar_kernel.h for documentation of this function.
uint32_t ar_get_millisecond_count(void)
{
    return g_ar.tickCount * ar_get_milliseconds_per_tick();
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
}

//! If the specified item is not on the list, nothing happens. In fact, the list may be empty,
//! indicated by a NULL @a m_head. Items are compared only by pointer value.
//!
//! @param item The item to remove from the list.
void _ar_list::remove(ar_list_node_t * item)
{
    // the list must not be empty
    if (m_head == NULL)
    {
        return;
    }

    ar_list_node_t * node = m_head;
    do {
        if (node == item)
        {
            node->m_prev->m_next = node->m_next;
            node->m_next->m_prev = node->m_prev;

            // Special case for removing the list head.
            if (m_head == node)
            {
                // Handle a single item list by clearing the list head.
                if (node->m_next == m_head)
                {
                    m_head = NULL;
                }
                // Otherwise just update the list head to the second list element.
                else
                {
                    m_head = node->m_next;
                }
            }

            item->m_next = NULL;
            item->m_prev = NULL;
            break;
        }

        node = node->m_next;
    } while (node != m_head);
}

ar_status_t ar_post_deferred_action(ar_deferred_action_type_t action, void * object)
{
    if (g_ar.deferredActions.m_count >= AR_DEFERRED_ACTION_QUEUE_SIZE)
    {
        assert(false);
        return kArQueueFullError;
    }

    int index = ar_atomic_increment(&g_ar.deferredActions.m_count);

    g_ar.deferredActions.m_actions[index] = action;
    g_ar.deferredActions.m_objects[index] = object;

    return kArSuccess;
}

ar_status_t ar_post_deferred_action2(ar_deferred_action_type_t action, void * object, void * arg)
{
    if (g_ar.deferredActions.m_count >= AR_DEFERRED_ACTION_QUEUE_SIZE)
    {
        assert(false);
        return kArQueueFullError;
    }

    int index = ar_atomic_add(&g_ar.deferredActions.m_count, 2);

    g_ar.deferredActions.m_actions[index] = action;
    g_ar.deferredActions.m_objects[index] = object;
    g_ar.deferredActions.m_actions[index+1] = kArDeferredActionValue;
    g_ar.deferredActions.m_objects[index+1] = arg;

    return kArSuccess;
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
