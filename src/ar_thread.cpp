/*
 * Copyright (c) 2007-2018 Immo Software
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
// Prototypes
//------------------------------------------------------------------------------

static ar_status_t ar_thread_resume_internal(ar_thread_t * thread);
static void ar_thread_deferred_resume(void * object, void * object2);
static ar_status_t ar_thread_suspend_internal(ar_thread_t * thread);
static void ar_thread_deferred_suspend(void * object, void * object2);

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

// See ar_kernel.h for documentation of this function.
ar_status_t ar_thread_create(ar_thread_t * thread, const char * name, ar_thread_entry_t entry, void * param, void * stack, unsigned stackSize, uint8_t priority, bool startImmediately)
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
    if (ar_port_get_irq_state())
    {
        return kArNotFromInterruptError;
    }

    // Clear thread structure.
    memset(thread, 0, sizeof(ar_thread_t));

    // init member variables
    thread->m_name = name ? name : AR_ANONYMOUS_OBJECT_NAME;
    thread->m_stackBottom = reinterpret_cast<uint32_t *>(stack);
    thread->m_priority = priority;
    thread->m_state = kArThreadSuspended;
    thread->m_entry = entry;
    thread->m_uniqueId = ++g_ar.threadIdCounter;

    // Set list node references back to the thread object.
    thread->m_threadNode.m_obj = thread;
    thread->m_blockedNode.m_obj = thread;

#if AR_GLOBAL_OBJECT_LISTS
    thread->m_createdNode.m_obj = thread;
    g_ar_objects.threads.add(&thread->m_createdNode);
#endif // AR_GLOBAL_OBJECT_LISTS

    // prepare top of stack
    ar_port_prepare_stack(thread, stackSize, param);

    {
        // disable interrupts
        KernelLock guard;

        // add to suspended list
        g_ar.suspendedList.add(thread);
    }

    ar_trace_2(kArTraceThreadCreated, 0, thread);

    // Resume thread if requested.
    if (startImmediately)
    {
        ar_thread_resume(thread);
    }

    return kArSuccess;
}

// See ar_kernel.h for documentation of this function.
ar_status_t ar_thread_delete(ar_thread_t * thread)
{
    if (!thread)
    {
        return kArInvalidParameterError;
    }
    if (ar_port_get_irq_state())
    {
        return kArNotFromInterruptError;
    }

    // Clear runloop association.
    if (thread->m_runLoop)
    {
        thread->m_runLoop->m_thread = NULL;
        thread->m_runLoop = NULL;
    }

#if AR_GLOBAL_OBJECT_LISTS
    g_ar_objects.threads.remove(&thread->m_createdNode);
#endif // AR_GLOBAL_OBJECT_LISTS

    ar_trace_2(kArTraceThreadDeleted, 0, thread);

    // Remove from whatever list the thread is on, and set state to done.
    // If we're deleting the current thread, then execution will never proceed past
    // this block.
    {
        KernelLock guard;

        switch (thread->m_state)
        {
            case kArThreadReady:
            case kArThreadRunning:
                g_ar.readyList.remove(thread);
                ar_kernel_update_round_robin();
                break;

            case kArThreadSuspended:
                g_ar.suspendedList.remove(thread);
                break;

            case kArThreadBlocked:
            case kArThreadSleeping:
                g_ar.sleepingList.remove(thread);
                break;

            case kArThreadDone:
                break;

            case kArThreadUnknown:
                return kArInvalidStateError;
        }

        // Mark thread as finished.
        thread->m_state = kArThreadDone;

        // Are we deleting ourself?
        if (thread == g_ar.currentThread)
        {
            g_ar.flags.needsReschedule = true;
        }
    }

    return kArSuccess;
}

static ar_status_t ar_thread_resume_internal(ar_thread_t * thread)
{
    KernelLock guard;

    switch (thread->m_state)
    {
        case kArThreadReady:
        case kArThreadRunning:
            return kArSuccess;

        case kArThreadSuspended:
            g_ar.suspendedList.remove(thread);
            break;

        case kArThreadSleeping:
            g_ar.sleepingList.remove(thread);
            break;

        case kArThreadBlocked:
        case kArThreadUnknown:
        case kArThreadDone:
            return kArInvalidStateError;
    }

    // Put the thread back on the ready list.
    thread->m_state = kArThreadReady;
    g_ar.readyList.add(thread);
    ar_kernel_update_round_robin();

    // yield to scheduler if there is not a running thread or if this thread
    // has a higher priority that the running one
    if (thread->m_priority > g_ar.currentThread->m_priority)
    {
        g_ar.flags.needsReschedule = true;
    }

    return kArSuccess;
}

static void ar_thread_deferred_resume(void * object, void * object2)
{
    ar_thread_resume_internal(reinterpret_cast<ar_thread_t *>(object));
}

// See ar_kernel.h for documentation of this function.
ar_status_t ar_thread_resume(ar_thread_t * thread)
{
    if (!thread)
    {
        return kArInvalidParameterError;
    }

    // Check thread state.
    switch (thread->m_state)
    {
        // Nothing to do if the thread is already ready.
        case kArThreadReady:
        case kArThreadRunning:
            return kArSuccess;

        // These are states from which we can resume the thread.
        case kArThreadSuspended:
        case kArThreadSleeping:
            break;

        // Erroneous states.
        case kArThreadBlocked:
        case kArThreadUnknown:
        case kArThreadDone:
            return kArInvalidStateError;
    }

    if (ar_port_get_irq_state())
    {
        // Handle irq state by deferring the resume.
        return g_ar.deferredActions.post(ar_thread_deferred_resume, thread);
    }
    else
    {
        return ar_thread_resume_internal(thread);
    }
}

static ar_status_t ar_thread_suspend_internal(ar_thread_t * thread)
{
    KernelLock guard;

    switch (thread->m_state)
    {
        // Move ready threads to suspended list.
        case kArThreadReady:
        case kArThreadRunning:
            g_ar.readyList.remove(thread);
            ar_kernel_update_round_robin();
            break;

        // Move sleeping threads to suspended list.
        case kArThreadSleeping:
            g_ar.sleepingList.remove(thread);
            break;

        // Nothing needs doing if the thread is already suspended.
        case kArThreadSuspended:
            return kArSuccess;

        // Erroneous states.
        case kArThreadBlocked:
        case kArThreadUnknown:
        case kArThreadDone:
            return kArInvalidStateError;
    }

    // Move the thread to the suspended list.
    thread->m_state = kArThreadSuspended;
    g_ar.suspendedList.add(thread);

    // are we suspending the current thread?
    if (thread == g_ar.currentThread)
    {
        g_ar.flags.needsReschedule = true;
    }

    return kArSuccess;
}

static void ar_thread_deferred_suspend(void * object, void * object2)
{
    ar_thread_suspend_internal(reinterpret_cast<ar_thread_t *>(object));
}

// See ar_kernel.h for documentation of this function.
ar_status_t ar_thread_suspend(ar_thread_t * thread)
{
    if (!thread)
    {
        return kArInvalidParameterError;
    }

    // Check thread state.
    switch (thread->m_state)
    {
        // Nothing to do if the thread is already ready.
        case kArThreadSuspended:
            return kArSuccess;

        // These are states from which we can suspend the thread.
        case kArThreadReady:
        case kArThreadRunning:
        case kArThreadSleeping:
            break;

        // Erroneous states.
        case kArThreadBlocked: // Would have to remove thread from the blocked list of the kernel object.
        case kArThreadUnknown:
        case kArThreadDone:
            return kArInvalidStateError;
    }

    if (ar_port_get_irq_state())
    {
        // Handle irq state by deferring the resume.
        return g_ar.deferredActions.post(ar_thread_deferred_suspend, thread);
    }
    else
    {
        return ar_thread_suspend_internal(thread);
    }
}

// See ar_kernel.h for documentation of this function.
ar_status_t ar_thread_set_priority(ar_thread_t * thread, uint8_t priority)
{
    if (!thread)
    {
        return kArInvalidParameterError;
    }
    if (ar_port_get_irq_state())
    {
        return kArNotFromInterruptError;
    }

    if (priority == kArIdleThreadPriority && thread != &g_ar.idleThread)
    {
        return kArInvalidPriorityError;
    }

    if (priority != thread->m_priority)
    {
        KernelLock guard;

        // Set new priority.
        thread->m_priority = priority;

        // Resort ready list.
        if (thread->m_state == kArThreadReady || thread->m_state == kArThreadRunning)
        {
            g_ar.readyList.remove(thread);
            g_ar.readyList.add(thread);
            ar_kernel_update_round_robin();
        }
        else if (thread->m_state == kArThreadBlocked)
        {
            //! @todo Resort blocked list and handle priority inheritence.
        }

        g_ar.flags.needsReschedule = true;
    }

    return kArSuccess;
}

// See ar_kernel.h for documentation of this function.
void ar_thread_sleep(uint32_t milliseconds)
{
    // Cannot sleep in interrup context.
    if (ar_port_get_irq_state())
    {
        return;
    }

    // bail if there is not a running thread to put to sleep
    if (milliseconds == 0 || !g_ar.currentThread)
    {
        return;
    }

    // Sleeping infinitely is equivalent to suspending the thread.
    if (milliseconds == kArInfiniteTimeout)
    {
        ar_thread_suspend(g_ar.currentThread);
    }
    else
    {
        ar_thread_sleep_until(ar_ticks_to_milliseconds(g_ar.tickCount) + milliseconds);
    }
}

// See ar_kernel.h for documentation of this function.
void ar_thread_sleep_until(uint32_t wakeup)
{
    // Cannot sleep in interrup context.
    if (ar_port_get_irq_state())
    {
        return;
    }

    // bail if there is not a running thread to put to sleep
    if (wakeup <= ar_get_millisecond_count() || !g_ar.currentThread)
    {
        return;
    }

    {
        KernelLock guard;

        // put the current thread on the sleeping list
        g_ar.currentThread->m_wakeupTime = ar_milliseconds_to_ticks(wakeup);

        g_ar.readyList.remove(g_ar.currentThread);
        g_ar.currentThread->m_state = kArThreadSleeping;
        g_ar.sleepingList.add(g_ar.currentThread);
        ar_kernel_update_round_robin();

        // run scheduler and switch to another thread
        g_ar.flags.needsReschedule = true;
    }
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
#if !defined(__ICCARM__)
__attribute__((noreturn))
#endif
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
        KernelLock guard;

        // This thread must be in the running state for this code to execute,
        // so we know it is on the ready list.
        g_ar.readyList.remove(thread);
        ar_kernel_update_round_robin();

        // Mark this thread as finished
        thread->m_state = kArThreadDone;
    }

    // Switch to the scheduler to let another thread take over
    ar_kernel_enter_scheduler();

    // The compiler will see this and know we're not returning.
    for (;;) {}
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
    ar_kernel_update_round_robin();

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

    // Enter scheduler now that this thread is blocked.
    {
        KernelUnlock guard;

        // Yield to the scheduler. We'll return when the thread is unblocked by another thread.
        ar_kernel_enter_scheduler();
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
    assert(m_state == kArThreadBlocked);

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
    ar_kernel_update_round_robin();

    // Invoke the scheduler if the unblocked thread is higher priority than the current one.
    if (m_priority > g_ar.currentThread->m_priority)
    {
        g_ar.flags.needsReschedule = true;
    }
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

ar_runloop_t * ar_thread_get_runloop(ar_thread_t * thread)
{
    return thread ? thread->m_runLoop : 0;
}

uint32_t ar_thread_get_load(ar_thread_t * thread)
{
#if AR_ENABLE_SYSTEM_LOAD
    return thread ? thread->m_permilleCpu : 0;
#else // AR_ENABLE_SYSTEM_LOAD
    return 0;
#endif // AR_ENABLE_SYSTEM_LOAD
}

// See ar_kernel.h for documentation of this function.
uint32_t ar_thread_get_stack_used(ar_thread_t * thread)
{
    // Check canary.
    if (*thread->m_stackBottom != kStackCheckValue)
    {
        return 0;
    }
    uint32_t * stack = thread->m_stackBottom + 1; // Skip canary.
    uint32_t unusedWords = 1; // Count canary.
    // Handle part of stack being used to hold member function thread entry points.
    if (*(stack + 2) == kStackFillValue)
    {
        unusedWords += 2;
        stack += 2;
    }
    for (; stack < thread->m_stackTop; ++stack, ++unusedWords)
    {
        if (*stack != kStackFillValue)
        {
            break;
        }
    }
    uint32_t stackSize = reinterpret_cast<uint32_t>(thread->m_stackTop) - reinterpret_cast<uint32_t>(thread->m_stackBottom);
    return stackSize - (unusedWords * sizeof(uint32_t));
}

// See ar_kernel.h for documentation of this function.
uint32_t ar_thread_get_report(ar_thread_status_t report[], uint32_t maxEntries)
{
    uint32_t threadCount = 0;
    ar_list_t * const threadLists[] = {
#if AR_GLOBAL_OBJECT_LISTS
        &g_ar_objects.threads
#else
        &g_ar.readyList, &g_ar.suspendedList, &g_ar.sleepingList
#endif // AR_GLOBAL_OBJECT_LISTS
    };
    uint32_t i = 0;
    for (; i < ARRAY_SIZE(threadLists) && threadCount < maxEntries; ++i)
    {
        ar_list_node_t * start = threadLists[i]->m_head;
        if (start)
        {
            ar_list_node_t * node = start;
            do {
                ar_thread_t * thread = node->getObject<ar_thread_t>();

                if (report)
                {
                    report->m_thread = thread;
                    report->m_name = thread->m_name;
                    report->m_uniqueId = thread->m_uniqueId;
#if AR_ENABLE_SYSTEM_LOAD
                    report->m_cpu = thread->m_permilleCpu;
#else // AR_ENABLE_SYSTEM_LOAD
                    report->m_cpu = 0;
#endif // AR_ENABLE_SYSTEM_LOAD
                    report->m_state = thread->m_state;
                    report->m_maxStackUsed = ar_thread_get_stack_used(thread);
                    report->m_stackSize = reinterpret_cast<uint32_t>(thread->m_stackTop) - reinterpret_cast<uint32_t>(thread->m_stackBottom);

                    ++report;
                }
                ++threadCount;

                node = node->m_next;
            } while (node != start && threadCount < maxEntries);
        }
    }

    return threadCount;
}

// See ar_classes.h for documentation of this function.
Thread::~Thread()
{
    ar_thread_delete(this);

    // Free dynamically allocated stack.
    if (m_allocatedStack)
    {
        delete [] m_allocatedStack;
    }
}

// See ar_classes.h for documentation of this function.
ar_status_t Thread::init(const char * name, ar_thread_entry_t entry, void * param, void * stack, unsigned stackSize, uint8_t priority, bool startImmediately)
{
    if (!stack)
    {
        m_allocatedStack = new uint8_t[stackSize];
        if (!m_allocatedStack)
        {
            return kArOutOfMemoryError;
        }
        stack = static_cast<void *>(m_allocatedStack);
    }
    else
    {
        m_allocatedStack = NULL;
    }
    m_userEntry = entry;
    ar_status_t result = ar_thread_create(this, name, thread_entry, param, stack, stackSize, priority, startImmediately);
    return result;
}

// See ar_classes.h for documentation of this function.
ar_status_t Thread::initForMemberFunction(const char * name, void * object, ar_thread_entry_t entry, void * memberPointer, uint32_t memberPointerSize, void * stack, unsigned stackSize, uint8_t priority, bool startImmediately)
{
    // Invoke the base initializer, passing the object pointer as the entry param.
    ar_status_t result = init(name, entry, object, stack, stackSize, priority, kArSuspendThread);
    if (result == kArSuccess)
    {
        // Save the member function pointer on the thread's stack. Add 1 to skip over check value.
        uint32_t * storage = m_stackBottom + 1;
        memcpy(storage, memberPointer, memberPointerSize);

        // Resume the thread if it's supposed to start right away.
        if (startImmediately)
        {
            ar_thread_resume(this);
        }
    }
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
    assert(thread);
    thread->threadEntry(param);
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
