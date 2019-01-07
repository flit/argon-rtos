/*
 * Copyright (c) 2016-2018 Immo Software
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
ar_status_t ar_runloop_create(ar_runloop_t * runloop, const char * name)
{
    if (!runloop)
    {
        return kArInvalidParameterError;
    }
    if (ar_port_get_irq_state())
    {
        return kArNotFromInterruptError;
    }

    memset(runloop, 0, sizeof(ar_runloop_t));

    runloop->m_name = name ? name : AR_ANONYMOUS_OBJECT_NAME;
    runloop->m_timers.m_predicate = ar_timer_sort_by_wakeup;

#if AR_GLOBAL_OBJECT_LISTS
    runloop->m_createdNode.m_obj = runloop;
    g_ar_objects.runloops.add(&runloop->m_createdNode);
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
    if (ar_port_get_irq_state())
    {
        return kArNotFromInterruptError;
    }
    if (runloop->m_isRunning)
    {
        return kArInvalidStateError;
    }

#if AR_GLOBAL_OBJECT_LISTS
    g_ar_objects.runloops.remove(&runloop->m_createdNode);
#endif

    return kArSuccess;
}

ar_status_t ar_runloop_run(ar_runloop_t * runloop, uint32_t timeout, ar_runloop_result_t * object)
{
    if (!runloop)
    {
        return kArInvalidParameterError;
    }
    // It's ok to nest runs of the runloop, but only on a single thread.
    if (runloop->m_isRunning && runloop->m_thread != g_ar.currentThread)
    {
        return kArRunLoopAlreadyRunningError;
    }
    // Another check to make sure no other runloop is already running on the current thread.
    if (g_ar.currentThread->m_runLoop && g_ar.currentThread->m_runLoop != runloop)
    {
        return kArRunLoopAlreadyRunningError;
    }
    // Disallow running from interrupt context.
    if (ar_port_get_irq_state())
    {
        return kArNotFromInterruptError;
    }

    // Clear returned object.
    if (object)
    {
        object->m_queue = NULL;
    }

    bool isNested = runloop->m_isRunning;
    if (!isNested)
    {
        // Associate this runloop with the current thread.
        g_ar.currentThread->m_runLoop = runloop;
        runloop->m_thread = g_ar.currentThread;

        // Clear stop flag.
        runloop->m_stop = false;

        // Set running flag.
        runloop->m_isRunning = true;
    }

    // Prepare timeout.
    uint32_t startTime = g_ar.tickCount;
    uint32_t timeoutTicks = timeout;
    if (timeout != kArInfiniteTimeout)
    {
        timeoutTicks = ar_milliseconds_to_ticks(timeout);
    }

    ar_status_t returnStatus = kArRunLoopStopped;

    // Run it.
    do {
        // Invoke timers.
        ar_kernel_run_timers(runloop->m_timers);

        // Invoke one queued function.
        if (runloop->m_functionCount)
        {
            uint16_t i = runloop->m_functionHead;
            uint16_t iPlusOne = (i + 1) % AR_RUNLOOP_FUNCTION_QUEUE_SIZE;

            ar_runloop_t::_ar_runloop_function_info fn = runloop->m_functions[i];

            ar_atomic_add32(&runloop->m_functionCount, -1);
            // This is the only line that modifies m_functionHead.
            runloop->m_functionHead = iPlusOne;

            assert(fn.function);
            fn.function(fn.param);
        }

        // Check pending queues.
        if (!runloop->m_queues.isEmpty())
        {
            ar_queue_t * queue = runloop->m_queues.getHead<ar_queue_t>();
            assert(queue);
            if (queue->m_count < 2)
            {
                runloop->m_queues.remove(queue);
            }

            if (queue->m_count > 0)
            {
                if (queue->m_runLoopHandler)
                {
                    // Call out to run loop queue source handler.
                    queue->m_runLoopHandler(queue, queue->m_runLoopHandlerParam);
                }
                else
                {
                    // No handler associated with this queue, so exit the run loop.
                    if (object)
                    {
                        object->m_queue = queue;
                    }

                    returnStatus = kArRunLoopQueueReceived;
                    break;
                }
            }
        }

        // Check timeout. Adjust the timeout based on how long we've run so far.
        uint32_t blockTimeoutTicks = timeoutTicks;
        if (blockTimeoutTicks != kArInfiniteTimeout)
        {
            uint32_t deltaTicks = g_ar.tickCount - startTime;
            if (deltaTicks >= timeoutTicks)
            {
                // Timed out, exit runloop.
                returnStatus = kArTimeoutError;
                break;
            }
            blockTimeoutTicks -= deltaTicks;
        }

        // Make sure we don't sleep past the next scheduled timer.
        if (!runloop->m_timers.isEmpty())
        {
            ar_timer_t * timer = runloop->m_timers.getHead<ar_timer_t>();

            // Timers should always have a wakeup time in the future.
            assert (timer->m_wakeupTime >= g_ar.tickCount);

            uint32_t wakeupDeltaTicks = timer->m_wakeupTime - g_ar.tickCount;
            // kArInfiniteTimeout is the max 32-bit value, so wakeupDeltaTicks will always be <=
            if (wakeupDeltaTicks < blockTimeoutTicks)
            {
                blockTimeoutTicks = wakeupDeltaTicks;
            }
        }

        // Don't sleep if there are queued functions or sources.
        if (!runloop->m_functionCount && runloop->m_queues.isEmpty())
        {
            // Sleep the runloop's thread for the adjusted timeout.
            uint32_t blockTimeout = (blockTimeoutTicks == kArInfiniteTimeout)
                                        ? kArInfiniteTimeout
                                        : ar_ticks_to_milliseconds(blockTimeoutTicks);
            if (blockTimeout)
            {
                ar_thread_sleep(blockTimeout);
            }
        }
    } while (!runloop->m_stop);

    if (!isNested)
    {
        // Clear associated thread.
        g_ar.currentThread->m_runLoop = NULL;
        runloop->m_thread = NULL;

        // Clear running flag.
        runloop->m_isRunning = false;
    }

    // TODO return different status for timed out versus stopped?
    return returnStatus;
}

void ar_runloop_wake(ar_runloop_t * runloop)
{
    ar_thread_t * thread = runloop->m_thread;
    if (runloop->m_isRunning && thread)
    {
        ar_thread_resume(thread);
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

    // TODO block if queue is full?
    int32_t tail = ar_kernel_atomic_queue_insert(1, runloop->m_functionCount, runloop->m_functionTail, AR_RUNLOOP_FUNCTION_QUEUE_SIZE);
    if (tail == -1)
    {
        return kArQueueFullError;
    }
    ar_runloop_t::_ar_runloop_function_info & fn = runloop->m_functions[tail];
    fn.function = function;
    fn.param = param;

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

    timer->m_runLoop = runloop;

    return kArSuccess;
}

ar_status_t ar_runloop_add_queue(ar_runloop_t * runloop, ar_queue_t * queue, ar_runloop_queue_handler_t callback, void * param)
{
    if (!runloop || !queue)
    {
        return kArInvalidParameterError;
    }

    // A queue can only be added to one runloop at a time.
    if (queue->m_runLoop != NULL && queue->m_runLoop != runloop)
    {
        return kArAlreadyAttachedError;
    }

    // Set runloop on the queue.
    queue->m_runLoopHandler = callback;
    queue->m_runLoopHandlerParam = param;
    queue->m_runLoop = runloop;

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
