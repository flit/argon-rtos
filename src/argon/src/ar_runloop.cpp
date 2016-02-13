/*
 * Copyright (c) 2016 Immo Software
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
 * @brief Source for Ar microkernel runloops.
 */

#include "ar_internal.h"
#include <string.h>
#include <assert.h>

using namespace Ar;

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

// See ar_kernel.h for documentation of this function.
ar_status_t ar_runloop_create(ar_runloop_t * runloop, const char * name, ar_thread_t * thread)
{
    if (!runloop)
    {
        return kArInvalidParameterError;
    }

    memset(runloop, 0, sizeof(ar_runloop_t));

    runloop->m_name = name ? name : AR_ANONYMOUS_OBJECT_NAME;
    runloop->m_thread = thread ? thread : ar_thread_get_current();
    runloop->m_timers.m_predicate = ar_timer_sort_by_wakeup;

#if AR_GLOBAL_OBJECT_LISTS
    runloop->m_createdNode.m_obj = runloop;
    g_ar.allObjects.runloops.add(&runloop->m_createdNode);
#endif

    return kArSuccess;
}

// See ar_kernel.h for documentation of this function.
ar_status_t ar_runloop_delete(ar_runloop_t * runloop)
{
    if (!runloop)
    {
        return kArInvalidParameterError;
    }

#if AR_GLOBAL_OBJECT_LISTS
    g_ar.allObjects.runloops.remove(&runloop->m_createdNode);
#endif

    return kArSuccess;
}

ar_runloop_status_t ar_runloop_run(ar_runloop_t * runloop, uint32_t timeout, void * object, void * value)
{
    if (!runloop)
    {
        return kArRunLoopError;
    }

    // Associate this runloop with the current thread.
    g_ar.currentThread->m_runLoop = runloop;
    runloop->m_thread = g_ar.currentThread;

    // Clear stop flag.
    runloop->m_stop = false;

    // Prepare timeout.
    uint32_t startTime = g_ar.tickCount;
    uint32_t timeoutTicks = timeout;
    if (timeout != kArInfiniteTimeout)
    {
        timeoutTicks = ar_milliseconds_to_ticks(timeout);
    }

    // Run it.
    do {
        // Invoke timers.
        ar_kernel_run_timers(runloop->m_timers);

        // Invoke queued functions.
        // TODO: one function per loop or all at once?
        uint32_t count = runloop->m_functionCount;
        while (count--)
        {
            ar_runloop_function_t fn;
            void * param;

            {
                KernelLock lock;

                ar_runloop_t::_ar_runloop_function_info * fnPtr = &runloop->m_functions[runloop->m_functionHead];
                fn = fnPtr->function;
                param = fnPtr->param;

                runloop->m_functionHead = (runloop->m_functionHead + 1) % AR_RUNLOOP_FUNCTION_QUEUE_SIZE;
                --runloop->m_functionCount;
            }

            // Call function.
            assert(fn);
            fn(param);
        }

        // Check timeout. Adjust the timeout based on how long we've run so far.
        uint32_t blockTimeoutTicks = timeoutTicks;
        if (blockTimeoutTicks != kArInfiniteTimeout)
        {
            int32_t deltaTicks = g_ar.tickCount - startTime;
            if (deltaTicks >= timeoutTicks)
            {
                break;
            }
            blockTimeoutTicks -= deltaTicks;
        }

        // Make sure we don't sleep past the next scheduled timer.
        if (runloop->m_timers.m_head)
        {
            ar_timer_t * timer = runloop->m_timers.m_head->getObject<ar_timer_t>();
            uint32_t wakeupDeltaTicks = timer->m_wakeupTime - g_ar.tickCount;
            // kArInfiniteTimeout is the max 32-bit value, so wakeupDeltaTicks will always be <=
            if (wakeupDeltaTicks < blockTimeoutTicks)
            {
                blockTimeoutTicks = wakeupDeltaTicks;
            }
        }

        // Don't sleep if there are queued functions.
        if (runloop->m_functionCount)
        {
            blockTimeoutTicks = 0;
        }

        // Sleep the runloop's thread for the adjusted timeout.
        uint32_t blockTimeout = (blockTimeoutTicks == kArInfiniteTimeout)
                                    ? kArInfiniteTimeout
                                    : ar_ticks_to_milliseconds(blockTimeoutTicks);
        if (blockTimeout)
        {
            ar_thread_sleep(blockTimeout);
        }
    } while (!runloop->m_stop);

    // Clear associated thread.
    g_ar.currentThread->m_runLoop = NULL;
    runloop->m_thread = NULL;

    return kArRunLoopStopped;
}

void ar_runloop_wake(ar_runloop_t * runloop)
{
    ar_thread_t * thread = runloop->m_thread;
    if (thread)
    {
        KernelLock lock;

        if (thread->m_state == kArThreadSleeping)
        {
            // Remove thread from the sleeping list.
            g_ar.sleepingList.remove(thread);

            // Put the thread back onto the ready list.
            thread->m_state = kArThreadReady;
            g_ar.readyList.add(thread);
        }
    }
}

ar_status_t ar_runloop_stop(ar_runloop_t * runloop)
{
    if (!runloop)
    {
        return kArInvalidParameterError;
    }

    // Set the stop flag.
    runloop->m_stop = true;

    // Wake the runloop in case it is blocked.
    ar_runloop_wake(runloop);

    return kArSuccess;
}

ar_status_t ar_runloop_perform(ar_runloop_t * runloop, ar_runloop_function_t function, void * param)
{
    if (!runloop || !function)
    {
        return kArInvalidParameterError;
    }

    if (runloop->m_functionCount >= AR_RUNLOOP_FUNCTION_QUEUE_SIZE)
    {
        return kArQueueFullError;
    }

    {
        KernelLock lock;

        ar_runloop_t::_ar_runloop_function_info * fnPtr = &runloop->m_functions[runloop->m_functionTail];
        fnPtr->function = function;
        fnPtr->param = param;

        runloop->m_functionTail = (runloop->m_functionTail + 1) % AR_RUNLOOP_FUNCTION_QUEUE_SIZE;
        ++runloop->m_functionCount;
    }

    // Wake the runloop in case it is blocked.
    ar_runloop_wake(runloop);

    return kArSuccess;
}

ar_status_t ar_runloop_add_timer(ar_runloop_t * runloop, ar_timer_t * timer)
{
    if (!runloop || !timer)
    {
        return kArInvalidParameterError;
    }

//     runloop->m_timers.add(timer);
    timer->m_runLoop = runloop;

    // Wake the runloop in case it is blocked.
    ar_runloop_wake(runloop);

    return kArSuccess;
}

ar_status_t ar_runloop_add_queue(ar_runloop_t * runloop, ar_queue_t * queue, ar_runloop_queue_handler_t callback)
{
    if (!runloop)
    {
        return kArInvalidParameterError;
    }

    runloop->m_queues.add(queue);

    // Wake the runloop in case it is blocked.
    ar_runloop_wake(runloop);

    return kArSuccess;
}

ar_status_t ar_runloop_add_channel(ar_runloop_t * runloop, ar_channel_t * channel, ar_runloop_channel_handler_t callback)
{
    if (!runloop)
    {
        return kArInvalidParameterError;
    }

    runloop->m_channels.add(channel);

    // Wake the runloop in case it is blocked.
    ar_runloop_wake(runloop);

    return kArSuccess;
}

ar_runloop_t * ar_runloop_get_current(void)
{
    return g_ar.currentThread->m_runLoop;
}

const char * ar_runloop_get_name(ar_runloop_t * runloop)
{
    return runloop ? runloop->m_name : NULL;
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------