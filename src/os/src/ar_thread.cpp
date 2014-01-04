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
 * @brief Source for Ar threads.
 */

#include "ar_internal.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

using namespace Ar;

//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------

// Thread * Thread::s_readyList = NULL;
// Thread * Thread::s_suspendedList = NULL;
// Thread * Thread::s_sleepingList = NULL;
// Thread * Thread::s_currentThread = NULL;

//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------

static void THREAD_STACK_OVERFLOW_DETECTED();

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

//! @brief Function to make it clear what happened.
void THREAD_STACK_OVERFLOW_DETECTED()
{
    _halt();
}

// See ar_kernel.h for documentation of this function.
status_t ar_thread_create(ar_thread_t * thread, const char * name, ar_thread_entry_t entry, void * param, void * stack, unsigned stackSize, uint8_t priority)
{
    // Assertions.
    if (priority == 0)
    {
        return kArInvalidPriorityError;
    }
    if (stackSize < sizeof(ThreadContext))
    {
        return kArStackSizeTooSmallError;
    }
    
    // Clear thread structure.
    memset(thread, 0, sizeof(ar_thread_t));
    
    // init member variables
    thread->m_name = name ? name : "";
    thread->m_stackTop = reinterpret_cast<uint8_t *>(stack) + stackSize;
    thread->m_stackSize = stackSize;
    thread->m_stackPointer = thread->m_stackTop;
    thread->m_priority = priority;
    thread->m_state = kArThreadSuspended;
    thread->m_entry = entry;
    
    // Set list node references back to the thread object.
    thread->m_threadList.m_obj = thread;
    thread->m_createdList.m_obj = thread;
    thread->m_blockedList.m_obj = thread;

    // prepare top of stack
    ar_port_prepare_stack(thread, param);

    {
        // disable interrupts
        IrqDisableAndRestore disableIrq;
        
        // add to suspended list
        ar_thread_list_add(g_ar.suspendedList, thread);
    }
    
#if AR_GLOBAL_OBJECT_LISTS
    ar_list_add(g_ar.allObjects.threads, &thread->m_createdList);
#endif // AR_GLOBAL_OBJECT_LISTS
    
    return kArSuccess;
}

// See ar_kernel.h for documentation of this function.
status_t ar_thread_delete(ar_thread_t * thread)
{
    if (thread->m_state != kArThreadDone)
    {
        // Remove from ready list if it's on there, and switch to another thread
        // if we're disposing of our own thread.
        ar_thread_suspend(thread);
        
        // Now remove from the suspended list
        {
            IrqDisableAndRestore disableIrq;
            ar_thread_list_remove(g_ar.suspendedList, thread);
        }
    }
    
#if AR_GLOBAL_OBJECT_LISTS
    ar_list_remove(g_ar.allObjects.threads, &thread->m_createdList);
#endif // AR_GLOBAL_OBJECT_LISTS
    
    return kArSuccess;
}

// See ar_kernel.h for documentation of this function.
status_t ar_thread_resume(ar_thread_t * thread)
{
    {
        IrqDisableAndRestore disableIrq;
    
        if (thread->m_state == kArThreadReady)
        {
            return kArSuccess;
        }
        else if (thread->m_state == kArThreadSuspended)
        {
            ar_thread_list_remove(g_ar.suspendedList, thread);
            thread->m_state = kArThreadReady;
            ar_thread_list_add(g_ar.readyList, thread);
        }
    }

    // yield to scheduler if there is not a running thread or if this thread
    // has a higher priority that the running one
    if (g_ar.isRunning && thread->m_priority > g_ar.currentThread->m_priority)
    {
        ar_kernel_enter_scheduler();
    }
    
    return kArSuccess;
}

// See ar_kernel.h for documentation of this function.
status_t ar_thread_suspend(ar_thread_t * thread)
{
    {
        IrqDisableAndRestore disableIrq;
    
        if (thread->m_state == kArThreadSuspended)
        {
            // Nothing needs doing if the thread is already suspended.
            return kArSuccess;
        }
        else
        {
            ar_thread_list_remove(g_ar.readyList, thread);
            thread->m_state = kArThreadSuspended;
            ar_thread_list_add(g_ar.suspendedList, thread);
        }
    }

    // are we suspending the current thread?
    if (g_ar.isRunning && thread == g_ar.currentThread)
    {
        ar_kernel_enter_scheduler();
    }
    
    return kArSuccess;
}

// See ar_kernel.h for documentation of this function.
status_t ar_thread_set_priority(ar_thread_t * thread, uint8_t priority)
{
    if (priority == 0 && thread != &g_ar.idleThread)
    {
        return kArInvalidPriorityError;
    }
    
    if (priority != thread->m_priority)
    {
        thread->m_priority = priority;

        if (g_ar.isRunning)
        {
            ar_kernel_enter_scheduler();
        }
    }
    
    return kArSuccess;
}

// See ar_kernel.h for documentation of this function.
void ar_thread_sleep(uint32_t milliseconds)
{
    // bail if there is not a running thread to put to sleep
    if (milliseconds == 0 || !g_ar.currentThread)
    {
        return;
    }

    {
        IrqDisableAndRestore disableIrq;
        
        // put the current thread on the sleeping list
        g_ar.currentThread->m_wakeupTime = g_ar.tickCount + ar_milliseconds_to_ticks(milliseconds);
        
        ar_thread_list_remove(g_ar.readyList, g_ar.currentThread);
        g_ar.currentThread->m_state = kArThreadSleeping;
        ar_thread_list_add(g_ar.sleepingList, g_ar.currentThread);
    }
    
    // run scheduler and switch to another thread
    ar_kernel_enter_scheduler();
}

//! The thread wrapper calls the thread entry function that was set in
//! the init() call. When and if the function returns, the thread is removed
//! from the ready list and its state set to #kThreadDone.
//!
//! This function will never actually itself return. Instead, it switches to
//! the scheduler before exiting, and the scheduler will never switch back
//! because the thread is marked as done.
//!
//! @param thread Pointer to the thread object which is starting up.
//! @param param Arbitrary parameter passed to the thread entry point.
void ar_thread_wrapper(ar_thread_t * thread, void * param)
{
    assert(thread);
    
    // Call the entry point.
    if (thread->m_entry)
    {
        thread->m_entry(param);
    }
    
    // Thread function has finished, so clean up and terminate the thread.
    {
        IrqDisableAndRestore disableIrq;
        
        // This thread must be in the running state for this code to execute,
        // so we know it is on the ready list.
        ar_thread_list_remove(g_ar.readyList, thread);
        
        // Mark this thread as finished
        thread->m_state = kArThreadDone;
    }
        
    // Switch to the scheduler to let another thread take over
    ar_kernel_enter_scheduler();
}

//! Increments the system tick count and wakes any sleeping threads whose wakeup time
//! has arrived. If the thread's state is #kThreadBlocked then its unblock status
//! is set to #kTimeoutError.
//!
//! @param ticks The number of ticks that have elapsed. Normally this will only be 1, 
//!     and must be at least 1, but may be higher if interrupts are disabled for a
//!     long time.
//! @return Flag indicating whether any threads were modified.
//!
//! @todo Keep the list of sleeping threads sorted by next wakeup time.
bool ar_kernel_increment_tick_count(unsigned ticks)
{
    assert(ticks > 0);
    
    // Increment tick count.
    g_ar.tickCount += ticks;

    // Scan list of sleeping threads to see if any should wake up.
    ar_list_node_t * node = g_ar.sleepingList;
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
                ar_thread_list_remove(g_ar.sleepingList, thread);
                thread->m_state = kArThreadReady;
                ar_thread_list_add(g_ar.readyList, thread);
            }
        
            node = next;
        } while (g_ar.sleepingList && node != g_ar.sleepingList);
    }
    
    // Check for an active timer whose wakeup time has expired.
    if (g_ar.activeTimers)
    {
        if (g_ar.activeTimers->getObject<ar_timer_t>()->m_wakeupTime <= g_ar.tickCount)
        {
            // Raise the idle thread priority to maximum so it can execute the timer.
            // The idle thread will reduce its own priority when done.
            ar_thread_set_priority(&g_ar.idleThread, 255);
        }
    }
    
    return wasThreadWoken;
}

//! @todo There are many opportunities for optimising the scheduler search loop.
void ar_kernel_scheduler()
{
    // There must always be at least one thread on the ready list.
    assert(g_ar.readyList);
    
    // Find the next ready thread using a round-robin search algorithm.
    ar_list_node_t * start;
    
    // Handle both the first time the scheduler runs and g_ar.currentThread is NULL, and the case where
    // the current thread was suspended. For both cases we want to start searching at the beginning
    // of the ready list. Otherwise start searching at the current thread.
    if (!g_ar.currentThread || g_ar.currentThread->m_state != kArThreadRunning)
    {
        start = g_ar.readyList;
    }
    else
    {
        start = &g_ar.currentThread->m_threadList;
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
    
    // Check for stack overflow on current thread.
    assert(g_ar.currentThread);
    uint32_t check = *(uint32_t *)((uint32_t)g_ar.currentThread->m_stackTop - g_ar.currentThread->m_stackSize);
    if (check != 0xdeadbeef)
    {
        THREAD_STACK_OVERFLOW_DETECTED();
    }
}

bool ar_thread_sort_by_priority(ar_list_node_t * a, ar_list_node_t * b)
{
    ar_thread_t * aThread = a->getObject<ar_thread_t>();
    ar_thread_t * bThread = b->getObject<ar_thread_t>();
    return (aThread->m_priority > bThread->m_priority);
}

void _ar_list_node::insertBefore(ar_list_node_t * node)
{
    m_next = node;
    m_prev = node->m_prev;
    node->m_prev->m_next = this;
    node->m_prev = this;
}

//! The thread is inserted in sorted order. The highest priority thread in the list
//! is the head of the list. If there are already one or more threads in the list with the same
//! priority as ours, we will be inserted after the existing threads.
//!
//! @param[in,out] listHead Reference to the head of the linked list. Will be
//!     NULL if the list is empty, in which case it is set to the thread instance.
void ar_list_add(ar_list_node_t *& listHead, ar_list_node_t * item, ar_list_predicate_t predicate)
{
    // Handle an empty list.
    if (!listHead)
    {
        listHead = item;
        item->m_next = item;
        item->m_prev = item;
    }
    // Insert at end of list if there is no sorting predicate, or if the item sorts after the
    // last item in the list.
    else if (!predicate || !predicate(item, listHead->m_prev))
    {
        item->insertBefore(listHead);
    }
    // Otherwise, search for the sorted position in the list for the item to be inserted.
    else
    {
        // Insert sorted by priority.
        ar_list_node_t * node = listHead;
    
        do {
//             if (node->m_next == listHead)
//             {
//                 item->m_next = listHead;
//                 item->m_prev = node;
//                 node->m_next = item;
//                 listHead->m_prev = item;
//             }
//             else
            if (predicate(item, node))
            {
                item->insertBefore(node);
            
                if (node == listHead)
                {
                    listHead = node;
                }
                
                break;
            }
        
            node = node->m_next;
        } while (node != listHead);
    }
}

//! If the thread is not on the list, nothing happens. In fact, the list may be empty, indicated
//! by a NULL @a listHead.
//!
//! @param[in,out] listHead Reference to the head of the linked list. May
//!     be NULL.
void ar_list_remove(ar_list_node_t *& listHead, ar_list_node_t * item)
{
    // the list must not be empty
    if (listHead == NULL)
    {
        return;
    }

    ar_list_node_t * node = listHead;
    do {
        if (node == item)
        {
            node->m_prev->m_next = node->m_next;
            node->m_next->m_prev = node->m_prev;
            
            // Special case for removing the list head.
            if (listHead == node)
            {
                // Handle a single item list by clearing the list head.
                if (node->m_next == listHead)
                {
                    listHead = NULL;
                }
                // Otherwise just update the list head to the second list element.
                else
                {
                    listHead = node->m_next;
                }
            }
            
            item->m_next = NULL;
            item->m_prev = NULL;
            break;
        }

        node = node->m_next;
    } while (node != listHead);
}

//! The thread is removed from the ready list. It is placed on the blocked list
//! referenced by the @a blockedList argument and its state is set to
//! #kArThreadBlocked. If the timeout is non-infinite, the thread is also
//! placed on the sleeping list with the wakeup time set to when the timeout
//! expires.
//!
//! @param thread The thread to block.
//! @param[in,out] blockedList Reference to the head of the linked list of
//!     blocked threads.
//! @param timeout The maximum number of ticks that the thread can remain
//!     blocked. A value of #kArInfiniteTimeout means the thread can be
//!     blocked forever. A timeout of 0 is not allowed and should be handled
//!     by the caller.
void ar_thread_block(ar_thread_t * thread, ar_list_node_t * & blockedList, uint32_t timeout)
{
    assert(timeout != 0);
    
    // Remove this thread from the ready list.
    ar_thread_list_remove(g_ar.readyList, thread);
    
    // Update its state.
    thread->m_state = kArThreadBlocked;
    thread->m_unblockStatus = kArSuccess;
    
    // Add to blocked list.
    ar_list_add(blockedList, &thread->m_blockedList);
    
    // If a valid timeout was given, put the thread on the sleeping list.
    if (timeout != kArInfiniteTimeout)
    {
        thread->m_wakeupTime = g_ar.tickCount + ar_milliseconds_to_ticks(timeout);
        ar_thread_list_add(g_ar.sleepingList, thread);
    }
    else
    {
        // Signal when unblocking that this thread is not on the sleeping list.
        thread->m_wakeupTime = 0;
    }
}

//! If the thread had a valid timeout when it was blocked, it is removed from
//! the sleeping list. It is always removed from the blocked list passed in
//! @a blockedList. And finally the thread is restored to ready status by setting
//! its state to #kArThreadReady and adding it back to the ready list.
//!
//! @param thread The thread to unblock.
//! @param[in,out] blockedList Reference to the head of the linked list of
//!     blocked threads.
//! @param unblockStatus Status code to return from the function that
//!     the thread had called when it was originally blocked.
//!
//! @todo Conditionalise the removal from the sleeping list to when the thread
//!     is actually on that list.
void ar_thread_unblock_with_status(ar_thread_t * thread, ar_list_node_t * & blockedList, status_t unblockStatus)
{
    // Remove from the sleeping list if it was on there. Won't hurt if
    // the thread is not on that list.
    if (thread->m_wakeupTime && g_ar.sleepingList)
    {
        ar_thread_list_remove(g_ar.sleepingList, thread);
    }
    
    // Remove this thread from the blocked list.
    ar_list_remove(blockedList, &thread->m_blockedList);

    // Put the unblocked thread back onto the ready list.
    thread->m_state = kArThreadReady;
    thread->m_unblockStatus = unblockStatus;
    ar_thread_list_add(g_ar.readyList, thread);
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
