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
// Code
//------------------------------------------------------------------------------

// See ar_kernel.h for documentation of this function.
ar_status_t ar_thread_create(ar_thread_t * thread, const char * name, ar_thread_entry_t entry, void * param, void * stack, unsigned stackSize, uint8_t priority)
{
    // Assertions.
    if (!thread)
    {
        return kArInvalidParameterError;
    }
    if (priority < kArMinThreadPriority)
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
    thread->m_name = name ? name : AR_ANONYMOUS_OBJECT_NAME;
    thread->m_stackTop = reinterpret_cast<uint8_t *>(stack) + stackSize;
    thread->m_stackSize = stackSize;
    thread->m_stackPointer = thread->m_stackTop;
    thread->m_priority = priority;
    thread->m_state = kArThreadSuspended;
    thread->m_entry = entry;

    // Set list node references back to the thread object.
    thread->m_threadNode.m_obj = thread;
    thread->m_createdNode.m_obj = thread;
    thread->m_blockedNode.m_obj = thread;

    // prepare top of stack
    ar_port_prepare_stack(thread, param);

    {
        // disable interrupts
        IrqDisableAndRestore disableIrq;

        // add to suspended list
        g_ar.suspendedList.add(thread);
    }

#if AR_GLOBAL_OBJECT_LISTS
    g_ar.allObjects.threads.add(&thread->m_createdNode);
#endif // AR_GLOBAL_OBJECT_LISTS

    return kArSuccess;
}

// See ar_kernel.h for documentation of this function.
ar_status_t ar_thread_delete(ar_thread_t * thread)
{
    if (!thread)
    {
        return kArInvalidParameterError;
    }

    if (thread->m_state != kArThreadDone)
    {
        // Remove from ready list if it's on there, and switch to another thread
        // if we're disposing of our own thread.
        ar_thread_suspend(thread);

        // Now remove from the suspended list
        {
            IrqDisableAndRestore disableIrq;
            g_ar.suspendedList.remove(thread);
        }
    }

#if AR_GLOBAL_OBJECT_LISTS
    g_ar.allObjects.threads.remove(&thread->m_createdNode);
#endif // AR_GLOBAL_OBJECT_LISTS

    return kArSuccess;
}

// See ar_kernel.h for documentation of this function.
ar_status_t ar_thread_resume(ar_thread_t * thread)
{
    if (!thread)
    {
        return kArInvalidParameterError;
    }

    {
        IrqDisableAndRestore disableIrq;

        if (thread->m_state == kArThreadReady)
        {
            return kArSuccess;
        }
        else if (thread->m_state == kArThreadSuspended)
        {
            g_ar.suspendedList.remove(thread);
            thread->m_state = kArThreadReady;
            g_ar.readyList.add(thread);
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
ar_status_t ar_thread_suspend(ar_thread_t * thread)
{
    if (!thread)
    {
        return kArInvalidParameterError;
    }

    {
        IrqDisableAndRestore disableIrq;

        if (thread->m_state == kArThreadSuspended)
        {
            // Nothing needs doing if the thread is already suspended.
            return kArSuccess;
        }
        else
        {
            g_ar.readyList.remove(thread);
            thread->m_state = kArThreadSuspended;
            g_ar.suspendedList.add(thread);
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
ar_status_t ar_thread_set_priority(ar_thread_t * thread, uint8_t priority)
{
    if (!thread)
    {
        return kArInvalidParameterError;
    }

    if (priority == 0 && thread != &g_ar.idleThread)
    {
        return kArInvalidPriorityError;
    }

    if (priority != thread->m_priority)
    {
        {
            IrqDisableAndRestore disableIrq;

            // Set new priority.
            thread->m_priority = priority;

            // Resort ready list.
            if (thread->m_state == kArThreadReady || thread->m_state == kArThreadRunning)
            {
                g_ar.readyList.remove(thread);
                g_ar.readyList.add(thread);
            }
            else if (thread->m_state == kArThreadBlocked)
            {
                //! @todo Resort blocked list and handle priority inheritence.
            }
        }

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

        g_ar.readyList.remove(g_ar.currentThread);
        g_ar.currentThread->m_state = kArThreadSleeping;
        g_ar.sleepingList.add(g_ar.currentThread);
    }

    // run scheduler and switch to another thread
    ar_kernel_enter_scheduler();
}

//! The thread wrapper calls the thread entry function that was set in
//! the init() call. When and if the function returns, the thread is removed
//! from the ready list and its state set to #kArThreadDone.
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
        g_ar.readyList.remove(thread);

        // Mark this thread as finished
        thread->m_state = kArThreadDone;
    }

    // Switch to the scheduler to let another thread take over
    ar_kernel_enter_scheduler();
}

//! @retval true The @a a thread has a higher priority than @a b.
//! @retval false The @a a thread has a lower or equal priority than @a b.
bool ar_thread_sort_by_priority(ar_list_node_t * a, ar_list_node_t * b)
{
    ar_thread_t * aThread = a->getObject<ar_thread_t>();
    ar_thread_t * bThread = b->getObject<ar_thread_t>();
    return (aThread->m_priority > bThread->m_priority);
}

//! @retval true The @a a thread has an earlier wakeup time than @a b.
//! @retval false The @a a thread has a later or equal wakeup time than @a b.
bool ar_thread_sort_by_wakeup(ar_list_node_t * a, ar_list_node_t * b)
{
    ar_thread_t * aThread = a->getObject<ar_thread_t>();
    ar_thread_t * bThread = b->getObject<ar_thread_t>();
    return (aThread->m_wakeupTime < bThread->m_wakeupTime);
}

//! The thread is removed from the ready list. It is placed on the blocked list
//! referenced by the @a blockedList argument and its state is set to
//! #kArThreadBlocked. If the timeout is non-infinite, the thread is also
//! placed on the sleeping list with the wakeup time set to when the timeout
//! expires.
//!
//! @param[in,out] blockedList Reference to the head of the linked list of
//!     blocked threads.
//! @param timeout The maximum number of ticks that the thread can remain
//!     blocked. A value of #kArInfiniteTimeout means the thread can be
//!     blocked forever. A timeout of 0 is not allowed and should be handled
//!     by the caller.
void _ar_thread::block(ar_list_t & blockedList, uint32_t timeout)
{
    assert(timeout != 0);

    // Remove this thread from the ready list.
    g_ar.readyList.remove(this);

    // Update its state.
    m_state = kArThreadBlocked;
    m_unblockStatus = kArSuccess;

    // Add to blocked list.
    blockedList.add(&m_blockedNode);

    // If a valid timeout was given, put the thread on the sleeping list.
    if (timeout != kArInfiniteTimeout)
    {
        m_wakeupTime = g_ar.tickCount + ar_milliseconds_to_ticks(timeout);
        g_ar.sleepingList.add(this);
    }
    else
    {
        // Signal when unblocking that this thread is not on the sleeping list.
        m_wakeupTime = 0;
    }
}

//! If the thread had a valid timeout when it was blocked, it is removed from
//! the sleeping list. It is always removed from the blocked list passed in
//! @a blockedList. And finally the thread is restored to ready status by setting
//! its state to #kArThreadReady and adding it back to the ready list.
//!
//! @param[in,out] blockedList Reference to the head of the linked list of
//!     blocked threads.
//! @param unblockStatus Status code to return from the function that
//!     the thread had called when it was originally blocked.
void _ar_thread::unblockWithStatus(ar_list_t & blockedList, ar_status_t unblockStatus)
{
    // Remove from the sleeping list if it was on there. Won't hurt if
    // the thread is not on that list.
    if (m_wakeupTime && g_ar.sleepingList.m_head)
    {
        g_ar.sleepingList.remove(this);
    }

    // Remove this thread from the blocked list.
    blockedList.remove(&m_blockedNode);

    // Put the unblocked thread back onto the ready list.
    m_state = kArThreadReady;
    m_unblockStatus = unblockStatus;
    g_ar.readyList.add(this);
}

// See ar_kernel.h for documentation of this function.
ar_thread_state_t ar_thread_get_state(ar_thread_t * thread)
{
    return thread ? thread->m_state : kArThreadUnknown;
}

// See ar_kernel.h for documentation of this function.
ar_thread_t * ar_thread_get_current(void)
{
    return g_ar.currentThread;
}

// See ar_kernel.h for documentation of this function.
const char * ar_thread_get_name(ar_thread_t * thread)
{
    return thread ? thread->m_name : NULL;
}

// See ar_kernel.h for documentation of this function.
uint8_t ar_thread_get_priority(ar_thread_t * thread)
{
    return thread ? thread->m_priority : 0;
}

// See ar_classes.h for documentation of this function.
ar_status_t Thread::init(const char * name, ar_thread_entry_t entry, void * param, void * stack, unsigned stackSize, uint8_t priority)
{
    ar_status_t result = ar_thread_create(this, name, thread_entry, param, stack, stackSize, priority);
    m_ref = this;
    m_userEntry = entry;
    return result;
}

// See ar_classes.h for documentation of this function.
void Thread::threadEntry(void * param)
{
    if (m_userEntry)
    {
        m_userEntry(param);
    }
}

// See ar_classes.h for documentation of this function.
void Thread::thread_entry(void * param)
{
    Thread * thread = getCurrent();
    thread->threadEntry(param);
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
