/*
 * Copyright (c) 2007-2013 Immo Software
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
 * @brief Source for Ar microkernel threads.
 */

#include "os/ar_kernel.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

using namespace Ar;

//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------

Thread * Thread::s_readyList = NULL;
Thread * Thread::s_suspendedList = NULL;
Thread * Thread::s_sleepingList = NULL;
Thread * Thread::s_currentThread = NULL;

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
status_t Thread::init(const char * name, thread_entry_t entry, void * param, void * stack, unsigned stackSize, uint8_t priority)
{
    // Assertions.
    if (priority == 0)
    {
        return kInvalidPriorityError;
    }
    if (stackSize < sizeof(ThreadContext))
    {
        return kStackSizeTooSmallError;
    }
    
    NamedObject::init(name);
    
    // init member variables
    m_stackTop = reinterpret_cast<uint8_t *>(stack) + stackSize;
    m_stackSize = stackSize;
    m_stackPointer = m_stackTop;
    m_priority = priority;
    m_state = kThreadSuspended;
    m_entry = entry;
    m_param = param;
    m_next = NULL;
    m_wakeupTime = 0;
    m_unblockStatus = 0;

    // prepare top of stack
    prepareStack();

    {
        // disable interrupts
        IrqDisableAndRestore disableIrq;
        
        // add to suspended list
        addToList(s_suspendedList);
    }
    
#if AR_GLOBAL_OBJECT_LISTS
    addToCreatedList(g_allObjects.m_threads);
#endif // AR_GLOBAL_OBJECT_LISTS
    
    return kSuccess;
}

// See ar_kernel.h for documentation of this function.
Thread::~Thread()
{
    if (m_state != kThreadDone)
    {
        // Remove from ready list if it's on there, and switch to another thread
        // if we're disposing of our own thread.
        suspend();
        
        // Now remove from the suspended list
        {
            IrqDisableAndRestore disableIrq;
            removeFromList(s_suspendedList);
        }
    }
    
#if AR_GLOBAL_OBJECT_LISTS
    removeFromCreatedList(g_allObjects.m_threads);
#endif // AR_GLOBAL_OBJECT_LISTS
}

// See ar_kernel.h for documentation of this function.
void Thread::resume()
{
    {
        IrqDisableAndRestore disableIrq;
    
        if (m_state == kThreadReady)
        {
            return;
        }
        else if (m_state == kThreadSuspended)
        {
            removeFromList(s_suspendedList);
            m_state = kThreadReady;
            addToList(s_readyList);
        }
    }

    // yield to scheduler if there is not a running thread or if this thread
    // has a higher priority that the running one
    if (Kernel::isRunning() && m_priority > s_currentThread->m_priority)
    {
        Kernel::enterScheduler();
    }
}

// See ar_kernel.h for documentation of this function.
void Thread::suspend()
{
    {
        IrqDisableAndRestore disableIrq;
    
        if (m_state == kThreadSuspended)
        {
            // Nothing needs doing if the thread is already suspended.
            return;
        }
        else
        {
            removeFromList(s_readyList);
            m_state = kThreadSuspended;
            addToList(s_suspendedList);
        }
    }

    // are we suspending the current thread?
    if (Kernel::isRunning() && this == s_currentThread)
    {
        Kernel::enterScheduler();
    }
}

// See ar_kernel.h for documentation of this function.
status_t Thread::setPriority(uint8_t priority)
{
    if (priority == 0)
    {
        return kInvalidPriorityError;
    }
    
    if (priority != m_priority)
    {
        m_priority = priority;

        if (Kernel::isRunning())
        {
            Kernel::enterScheduler();
        }
    }
    
    return kSuccess;
}

// See ar_kernel.h for documentation of this function.
status_t Thread::join(uint32_t timeout)
{
    // Not yet implemented!
    assert(0);
    return kSuccess;
}

// See ar_kernel.h for documentation of this function.
void Thread::sleep(unsigned ticks)
{
    // bail if there is not a running thread to put to sleep
    if (ticks == 0 || !s_currentThread)
    {
        return;
    }

    {
        IrqDisableAndRestore disableIrq;
        
        // put the current thread on the sleeping list
        s_currentThread->m_wakeupTime = Kernel::getTickCount() + Time::millisecondsToTicks(ticks);
        s_currentThread->removeFromList(s_readyList);
        s_currentThread->m_state = kThreadSleeping;
        s_currentThread->addToList(s_sleepingList);
    }
    
    // run scheduler and switch to another thread
    Kernel::enterScheduler();
}

//!
//!
void Thread::threadEntry()
{
    // Call the entry point.
    if (m_entry)
    {
        m_entry(m_param);
    }
}

//! The thread wrapper calls the thread entry function that was set in
//! the init() call. When and if the function returns, the thread is removed
//! from the ready list and its state set to #THREAD_DONE.
//!
//! This function will never actually itself return. Instead, it switches to
//! the scheduler before exiting, and the scheduler will never switch back
//! because the thread is marked as done.
//!
//! @param thread Pointer to the thread object which is starting up.
void Thread::thread_wrapper(Thread * thread)
{
    assert(thread);
    
    // Call the entry point.
    thread->threadEntry();
    
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
    
    while (thread)
    {
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
    }
    
    return wasThreadWoken;
}

//! @todo There are many opportunities for optimising the scheduler search loop.
void Thread::scheduler()
{
    // Find the next ready thread using a round-robin search algorithm.
    Thread * next;
    
    // Start the search either at the current thread or at the beginning of
    // the ready list.
    if (s_currentThread)
    {
        // Handle case where current thread was suspended. Here the s_currentThread is no longer
        // on the ready list so we can't start from its m_next.
        if (s_currentThread->m_state != kThreadRunning)
        {
            s_currentThread = NULL;
            next = s_readyList;
        }
        else
        {
            next = s_currentThread;
        }
    }
    else
    {
        next = s_readyList;
    }
    
    assert(next);
    Thread * highest = next;
    uint8_t priority = highest->m_priority;

    // Search the ready list starting from the current thread. The search will loop back
    // to the list head when it hits NULL. The first thread after the current one whose
    // state is kThreadReady and has the highest priority will be selected.
    do {
        if (next)
        {
            next = next->m_next;
        }
        
        // Find highest priority thread.
        if (next && next->m_state == kThreadReady && next->m_priority > priority)
        {
            highest = next;
            priority = next->m_priority;
        }

        // Handle both the case when we start with s_currentThead is 0 and when we loop
        // from the end of the ready list to the beginning.
        if (!next && s_currentThread)
        {
            next = s_readyList;
        }
    } while (next && next != s_currentThread);

    // Switch to newly selected thread.
    if (highest && highest != s_currentThread)
    {
        if (s_currentThread && s_currentThread->m_state == kThreadRunning)
        {
            s_currentThread->m_state = kThreadReady;
        }
        
        highest->m_state = kThreadRunning;
        s_currentThread = highest;
    }
    
    // Check for stack overflow on current thread.
    if (s_currentThread)
    {
        uint32_t check = *(uint32_t *)((uint32_t)s_currentThread->m_stackTop - s_currentThread->m_stackSize);
        if (check != 0xdeadbeef)
        {
            THREAD_STACK_OVERFLOW_DETECTED();
        }
    }
}

//! The thread is added to the end of the linked list.
//!
//! @param[in,out] listHead Reference to the head of the linked list. Will be
//!     NULL if the list is empty, in which case it is set to the thread instance.
void Thread::addToList(Thread * & listHead)
{
    this->m_next = NULL;

    // handle an empty list
    if (!listHead)
    {
        listHead = this;
        return;
    }
    
    // find the end of the list
    Thread * thread = listHead;

    while (thread)
    {
        if (!thread->m_next)
        {
            thread->m_next = this;
            break;
        }
        thread = thread->m_next;
    }
}

//! The list is not allowed to be empty.
//!
//! @param[in,out] listHead Reference to the head of the linked list. May
//!     not be NULL.
void Thread::removeFromList(Thread * & listHead)
{
    // the list must not be empty
    assert(listHead != NULL);

    if (listHead == this)
    {
        // special case for removing the list head
        listHead = this->m_next;
    }
    else
    {
        Thread * item = listHead;
        while (item)
        {
            if (item->m_next == this)
            {
                item->m_next = this->m_next;
                return;
            }

            item = item->m_next;
        }
    }
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
