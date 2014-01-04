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
//         ar_thread_list_add(g_ar.suspendedList, thread);
        
        ar_list_add(g_ar.suspendedList, &thread->m_thread_list);
    }
    
#if AR_GLOBAL_OBJECT_LISTS
    addToCreatedList(thread, g_allObjects.m_threads);
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
    removeFromCreatedList(thread, g_allObjects.m_threads);
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
            return;
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
            return;
        }
        else
        {
            ar_thread_list_remove(g_ar.readyList, thread);
            thread->m_state = kArThreadSuspended;
            ar_thread_list_add(g_ar.suspendedList, thread);
        }
    }

    // are we suspending the current thread?
    if (g_ar.isRunning() && thread == g_ar.currentThread)
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
        return kAtInvalidPriorityError;
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
void ar_thread_sleep(unsigned ticks)
{
    // bail if there is not a running thread to put to sleep
    if (ticks == 0 || !s_currentThread)
    {
        return;
    }

    {
        IrqDisableAndRestore disableIrq;
        
        // put the current thread on the sleeping list
        g_ar.currentThread->m_wakeupTime = g_ar.tickCount() + ar_milliseconds_to_ticks(ticks);
        
//         ar_thread_list_remove(g_ar.readyList, g_ar.currentThread);
//         g_ar.currentThread->m_state = kArThreadSleeping;
//         ar_thread_list_add(g_ar.sleepingList, g_ar.currentThread);
        
        g_ar.readyList->remove(g_ar.currentThread);
        g_ar.currentThread->m_state = kArThreadSleeping;
        g_ar.sleepingList->add(g_ar.currentThread);
    }
    
    // run scheduler and switch to another thread
    ar_kernel_enter_scheduler();
}

//! Call the thread entry point function.
//!
//! This method can also be overridden by subclasses.
void Thread::threadEntry(void * param)
{
    // Call the entry point.
    if (m_entry)
    {
        m_entry(param);
    }
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
void Thread::thread_wrapper(Thread * thread, void * param)
{
    assert(thread);
    
    // Call the entry point.
    thread->threadEntry(param);
    
    // Thread function has finished, so clean up and terminate the thread.
    {
        IrqDisableAndRestore disableIrq;
        
        // This thread must be in the running state for this code to execute,
        // so we know it is on the ready list.
        thread->removeFromList(s_readyList);
        
        // Mark this thread as finished
        thread->m_state = kThreadDone;
    }
        
    // Switch to the scheduler to let another thread take over
    Kernel::enterScheduler();
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
bool Thread::incrementTickCount(unsigned ticks)
{
    assert(ticks > 0);
    
    // Increment tick count.
    Kernel::s_tickCount += ticks;

    // Scan list of sleeping threads to see if any should wake up.
    Thread * thread = s_sleepingList;
    bool wasThreadWoken = false;
    
    if (thread)
    {
        do {
            Thread * next = thread->m_next;
        
            // Is it time to wake this thread?
            if (Kernel::s_tickCount >= thread->m_wakeupTime)
            {
                wasThreadWoken = true;
            
                // State-specific actions
                switch (thread->m_state)
                {
                    case kThreadSleeping:
                        // The thread was just sleeping.
                        break;
                
                    case kThreadBlocked:
                        // The thread has timed out waiting for a resource.
                        thread->m_unblockStatus = kTimeoutError;
                        break;
                
                    default:
                        // Should not have threads in other states on this list!
                        _halt();
                }
            
                // Put thread in ready state.
                thread->removeFromList(s_sleepingList);
                thread->m_state = kThreadReady;
                thread->addToList(s_readyList);
            }
        
            thread = next;
        } while (s_sleepingList && thread != s_sleepingList);
    }
    
    // Check for an active timer whose wakeup time has expired.
    if (Timer::s_activeTimers)
    {
        if (Timer::s_activeTimers->m_wakeupTime <= Kernel::getTickCount())
        {
            // Raise the idle thread priority to maximum so it can execute the timer.
            // The idle thread will reduce its own priority when done.
            Kernel::s_idleThread.setPriority(255);
        }
    }
    
    return wasThreadWoken;
}

//! @todo There are many opportunities for optimising the scheduler search loop.
void Thread::scheduler()
{
    // There must always be at least one thread on the ready list.
    assert(s_readyList);
    
    // Find the next ready thread using a round-robin search algorithm.
    Thread * start;
    
    // Handle both the first time the scheduler runs and s_currentThread is NULL, and the case where
    // the current thread was suspended. For both cases we want to start searching at the beginning
    // of the ready list. Otherwise start searching at the current thread.
    if (!s_currentThread || s_currentThread->m_state != kThreadRunning)
    {
        start = s_readyList;
    }
    else
    {
        start = s_currentThread;
    }
    
    assert(start);
    Thread * next = start;
    Thread * highest = next;
    uint8_t priority = highest->m_priority;
    
    // Iterate over the ready list, finding the highest priority thread.
    do {
        if (next->m_state == kThreadReady && next->m_priority > priority)
        {
            highest = next;
            priority = next->m_priority;
        }
        
        next = next->m_next;
    } while (next != start);
    
    // Switch to newly selected thread.
    assert(highest);
    if (highest != s_currentThread)
    {
        if (s_currentThread && s_currentThread->m_state == kThreadRunning)
        {
            s_currentThread->m_state = kThreadReady;
        }
        
        highest->m_state = kThreadRunning;
        s_currentThread = highest;
    }
    
    // Check for stack overflow on current thread.
    assert(s_currentThread);
    uint32_t check = *(uint32_t *)((uint32_t)s_currentThread->m_stackTop - s_currentThread->m_stackSize);
    if (check != 0xdeadbeef)
    {
        THREAD_STACK_OVERFLOW_DETECTED();
    }
}

void ar_list_add(ar_list_node_t *& listHead, ar_list_node_t * item)
{
    // handle an empty list
    if (!listHead)
    {
        listHead = item;
        item->m_next = item;
        item->m_prev = item;
    }
    else
    {
        item->m_next = listHead;
        item->m_prev = listHead->m_prev;
        listHead->m_prev->m_next = item;
        listHead->m_prev = item;
    }
}

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

bool ar_thread_sort_by_priority(ar_list_node_t * a, ar_list_node_t * b)
{
    ar_thread_t * aThread = reinterpret_cast<ar_thread_t *>(a->m_obj);
    ar_thread_t * bThread = reinterpret_cast<ar_thread_t *>(b->m_obj);
    return (aThread->m_priority > bThread->m_priority);
}

void ar_list_add_sorted(ar_list_node_t *& listHead, ar_list_node_t * item, ar_list_predicate_t predicate)
{
    // Insert at head of list if our priority is the highest or if the list is empty.
    if (!listHead)
    {
        listHead = item;
        item->m_next = item;
        item->m_prev = item;
        return;
    }
    
    if (predicate(item, listHead))
    {
        item->m_next = listHead;
        item->m_prev = listHead->m_prev;
        listHead->m_prev->m_next = item;
        listHead->m_prev = item;
        listHead = item;
        return;
    }
    
    if (!predicate(item, listHead->m_prev))
    {
        item->m_next = listHead;
        item->m_prev = node;
        node->m_next = item;
        listHead->m_prev = item;
        return;
    }

    // Insert sorted by priority.
    ar_list_node_t * node = listHead;
    
    do {
        if (node->m_next == listHead)
        {
            item->m_next = listHead;
            item->m_prev = node;
            node->m_next = item;
            listHead->m_prev = item;
        }
        else if (predicate(item, node))
        {
            item->m_next = node;
            item->m_prev = node->m_prev;
            node->m_prev->m_next = item;
            node->m_prev = item;
            
            if (node == listHead)
            {
                listHead = node;
            }
        }
        
        node = node->m_next;
    } while (node != listHead)
}

//! The thread is added to the end of the circularly linked list.
//!
//! @param[in,out] listHead Reference to the head of the linked list. Will be
//!     NULL if the list is empty, in which case it is set to the thread instance.
void Thread::addToList(Thread * & listHead)
{
    m_next = NULL;

    // handle an empty list
    if (!listHead)
    {
        listHead = this;
        m_next = this;
        return;
    }
    
    // find the end of the list
    Thread * thread = listHead;

    while (true)
    {
        if (thread->m_next == listHead)
        {
            thread->m_next = this;
            m_next = listHead;
            break;
        }
        thread = thread->m_next;
    }
}

//! If the thread is not on the list, nothing happens. In fact, the list may be empty, indicated
//! by a NULL @a listHead.
//!
//! @param[in,out] listHead Reference to the head of the linked list. May
//!     be NULL.
void Thread::removeFromList(Thread * & listHead)
{
    // the list must not be empty
    if (listHead == NULL)
    {
        return;
    }

    Thread * item = listHead;
    do {
        if (item->m_next == this)
        {
            // De-link the item.
            item->m_next = m_next;

            // Special case for removing the list head.
            if (listHead == this)
            {
                // Handle a single item list by clearing the list head.
                if (m_next == this)
                {
                    listHead = NULL;
                }
                // Otherwise just update the list head to the second list element.
                else
                {
                    listHead = m_next;
                }
            }
            
            m_next = NULL;
            break;
        }

        item = item->m_next;
    } while (item != listHead);
}

//! The thread is inserted in sorted order by priority. The highest priority thread in the list
//! is the head of the list. If there are already one or more threads in the list with the same
//! priority as ours, we will be inserted after the existing threads.
//!
//! @param[in,out] listHead Reference to the head of the linked list. Will be
//!     NULL if the list is empty, in which case it is set to the thread instance.
void Thread::addToBlockedList(Thread * & listHead)
{
    m_nextBlocked = NULL;

    // Insert at head of list if our priority is the highest or if the list is empty.
    if (!listHead || m_priority > listHead->m_priority)
    {
        m_nextBlocked = listHead;
        listHead = this;
        return;
    }

    // Insert sorted by priority.
    Thread * thread = listHead;
    
    while (thread)
    {
        // Insert at end if we've reached the end of the list and there were no lower
        // priority threads.
        if (!thread->m_nextBlocked)
        {
            thread->m_nextBlocked = this;
            break;
        }
        // If our priority is higher than the next thread in the list, insert here.
        else if (m_priority > thread->m_nextBlocked->m_priority)
        {
            m_nextBlocked = thread->m_nextBlocked;
            thread->m_nextBlocked = this;
            break;
        }
        
        thread = thread->m_nextBlocked;
    }
}

//! The list is not allowed to be empty.
//!
//! @param[in,out] listHead Reference to the head of the linked list. May
//!     not be NULL.
void Thread::removeFromBlockedList(Thread * & listHead)
{
    // the list must not be empty
    assert(listHead != NULL);

    if (listHead == this)
    {
        // special case for removing the list head
        listHead = m_nextBlocked;
    }
    else
    {
        Thread * item = listHead;
        while (item)
        {
            if (item->m_nextBlocked == this)
            {
                item->m_nextBlocked = m_nextBlocked;
                return;
            }

            item = item->m_nextBlocked;
        }
    }
    
    m_nextBlocked = NULL;
}

//! The thread is removed from the ready list. It is placed on the blocked list
//! referenced by the @a blockedList argument and its state is set to
//! #kThreadBlocked. If the timeout is non-infinite, the thread is also
//! placed on the sleeping list with the wakeup time set to when the timeout
//! expires.
//!
//! @param[in,out] blockedList Reference to the head of the linked list of
//!     blocked threads.
//! @param timeout The maximum number of ticks that the thread can remain
//!     blocked. A value of #kInfiniteTimeout means the thread can be
//!     blocked forever. A timeout of 0 is not allowed and should be handled
//!     by the caller.
void Thread::block(Thread * & blockedList, uint32_t timeout)
{
    assert(timeout != 0);
    
    // Remove this thread from the ready list.
    removeFromList(Thread::s_readyList);
    
    // Update its state.
    m_state = kThreadBlocked;
    m_unblockStatus = kSuccess;
    
    // Add to blocked list.
    addToBlockedList(blockedList);
    
    // If a valid timeout was given, put the thread on the sleeping list.
    if (timeout != kInfiniteTimeout)
    {
        m_wakeupTime = Kernel::getTickCount() + Time::millisecondsToTicks(timeout);
        addToList(s_sleepingList);
    }
    else
    {
        // Signal to unblockWithStatus() that this thread is not on the sleeping list.
        m_wakeupTime = 0;
    }
}

//! If the thread had a valid timeout when it was blocked, it is removed from
//! the sleeping list. It is always removed from the blocked list passed in
//! @a blockedList. And finally the thread is restored to ready status by setting
//! its state to #kThreadReady and adding it back to the ready list.
//!
//! @param[in,out] blockedList Reference to the head of the linked list of
//!     blocked threads.
//! @param unblockStatus Status code to return from the function that
//!     the thread had called when it was originally blocked.
//!
//! @todo Conditionalise the removal from #Thread::s_sleepingList to when the thread
//!     is actually on that list.
void Thread::unblockWithStatus(Thread * & blockedList, status_t unblockStatus)
{
    // Remove from the sleeping list if it was on there. Won't hurt if
    // the thread is not on that list.
    if (m_wakeupTime && s_sleepingList)
    {
        removeFromList(s_sleepingList);
    }
    
    // Remove this thread from the blocked list.
    removeFromBlockedList(blockedList);

    // Put the unblocked thread back onto the ready list.
    m_state = kThreadReady;
    m_unblockStatus = unblockStatus;
    addToList(Thread::s_readyList);
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
