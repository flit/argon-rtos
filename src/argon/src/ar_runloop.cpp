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
    uint32_t timeoutTicks = ar_milliseconds_to_ticks(timeout);
    uint32_t startTime = g_ar.tickCount;

    // Run it.
    do {
        ar_kernel_run_timers(runloop->m_timers);
    } while (!runloop->m_stop && (g_ar.tickCount - startTime) < timeoutTicks);

    return kArRunLoopStopped;
}

ar_status_t ar_runloop_stop(ar_runloop_t * runloop)
{
    if (!runloop)
    {
        return kArInvalidParameterError;
    }

    runloop->m_stop = true;

    return kArSuccess;
}

ar_status_t ar_runloop_perform(ar_runloop_t * runloop, ar_runloop_function_t function, void * param)
{
    if (!runloop)
    {
        return kArInvalidParameterError;
    }

    if (runloop->m_functionCount >= AR_RUNLOOP_FUNCTION_QUEUE_SIZE)
    {
        return kArQueueFullError;
    }

    runloop->m_functions[runloop->m_functionTail].function = function;
    runloop->m_functions[runloop->m_functionTail].param = param;
    runloop->m_functionTail = (runloop->m_functionTail + 1) % AR_RUNLOOP_FUNCTION_QUEUE_SIZE;
    ++runloop->m_functionCount;

    return kArSuccess;
}

ar_status_t ar_runloop_add_timer(ar_runloop_t * runloop, ar_timer_t * timer)
{
    if (!runloop || !timer)
    {
        return kArInvalidParameterError;
    }

    runloop->m_timers.add(timer);

    return kArSuccess;
}

ar_status_t ar_runloop_add_queue(ar_runloop_t * runloop, ar_queue_t * queue, ar_runloop_queue_handler_t callback)
{
    if (!runloop)
    {
        return kArInvalidParameterError;
    }

    runloop->m_queues.add(queue);

    return kArSuccess;
}

ar_status_t ar_runloop_add_channel(ar_runloop_t * runloop, ar_channel_t * channel, ar_runloop_channel_handler_t callback)
{
    if (!runloop)
    {
        return kArInvalidParameterError;
    }

    runloop->m_channels.add(channel);

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
