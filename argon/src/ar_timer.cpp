/*
 * Copyright (c) 2007-2016 Immo Software
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
 * @brief Source for Ar microkernel timers.
 */

#include "ar_internal.h"
#include <string.h>
#include <assert.h>

using namespace Ar;

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

// See ar_kernel.h for documentation of this function.
ar_status_t ar_timer_create(ar_timer_t * timer, const char * name, ar_timer_entry_t callback, void * param, ar_timer_mode_t timerMode, uint32_t delay)
{
    if (!timer || !callback || !delay)
    {
        return kArInvalidParameterError;
    }

    memset(timer, 0, sizeof(ar_timer_t));

    timer->m_name = name ? name : AR_ANONYMOUS_OBJECT_NAME;
    timer->m_callback = callback;
    timer->m_param = param;
    timer->m_mode = timerMode;
    timer->m_delay = ar_milliseconds_to_ticks(delay);
    timer->m_activeNode.m_obj = timer;

#if AR_GLOBAL_OBJECT_LISTS
    timer->m_createdNode.m_obj = timer;
    g_ar_objects.timers.add(&timer->m_createdNode);
#endif

    return kArSuccess;
}

// See ar_kernel.h for documentation of this function.
ar_status_t ar_timer_delete(ar_timer_t * timer)
{
    if (!timer)
    {
        return kArInvalidParameterError;
    }

    ar_timer_stop(timer);

#if AR_GLOBAL_OBJECT_LISTS
    g_ar_objects.timers.remove(&timer->m_createdNode);
#endif

    return kArSuccess;
}

//! @brief Handles starting or restarting a timer.
ar_status_t ar_timer_internal_start(ar_timer_t * timer, uint32_t wakeupTime)
{
    KernelLock guard;

    assert(timer->m_runLoop);

    // Handle a timer that is already active.
    if (timer->m_isActive)
    {
        timer->m_runLoop->m_timers.remove(timer);
    }

    timer->m_wakeupTime = wakeupTime;
    timer->m_isActive = true;

    timer->m_runLoop->m_timers.add(timer);

    // Wake runloop so it will recompute its sleep time.
    ar_runloop_wake(timer->m_runLoop);

    return kArSuccess;
}

// See ar_kernel.h for documentation of this function.
ar_status_t ar_timer_start(ar_timer_t * timer)
{
    if (!timer)
    {
        return kArInvalidParameterError;
    }
    if (!timer->m_runLoop)
    {
        return kArTimerNoRunLoop;
    }

    // The callback should have been verified by the create function.
    assert(timer->m_callback);

    uint32_t wakeupTime = g_ar.tickCount + timer->m_delay;

    // Handle irq state by deferring the operation.
    if (ar_port_get_irq_state())
    {
        return ar_post_deferred_action2(kArDeferredTimerStart, timer, reinterpret_cast<void *>(wakeupTime));
    }

    return ar_timer_internal_start(timer, wakeupTime);
}

ar_status_t ar_timer_stop_internal(ar_timer_t * timer)
{
    KernelLock guard;

    if (timer->m_runLoop)
    {
        timer->m_runLoop->m_timers.remove(timer);

        // Wake runloop so it will recompute its sleep time.
        ar_runloop_wake(timer->m_runLoop);
    }

    timer->m_wakeupTime = 0;
    timer->m_isActive = false;

    return kArSuccess;
}

// See ar_kernel.h for documentation of this function.
ar_status_t ar_timer_stop(ar_timer_t * timer)
{
    if (!timer)
    {
        return kArInvalidParameterError;
    }
    if (!timer->m_isActive)
    {
        return kArTimerNotRunningError;
    }
    if (!timer->m_runLoop)
    {
        return kArTimerNoRunLoop;
    }

    // Handle irq state by deferring the operation.
    if (ar_port_get_irq_state())
    {
        return ar_post_deferred_action(kArDeferredTimerStop, timer);
    }

    return ar_timer_stop_internal(timer);
}

// See ar_kernel.h for documentation of this function.
ar_status_t ar_timer_set_delay(ar_timer_t * timer, uint32_t delay)
{
    if (!timer || !delay)
    {
        return kArInvalidParameterError;
    }

    {
        KernelLock guard;

        timer->m_delay = ar_milliseconds_to_ticks(delay);

        // If the timer is running, we need to update the wakeup time and resort the list.
        if (timer->m_isActive)
        {
            // If the timer is active, it should already have a runloop set.
            assert(timer->m_runLoop);

            uint32_t wakeupTime = g_ar.tickCount + timer->m_delay;
            ar_timer_internal_start(timer, wakeupTime);
        }
    }

    return kArSuccess;
}

//! @brief Sort timers ascending by wakeup time.
bool ar_timer_sort_by_wakeup(ar_list_node_t * a, ar_list_node_t * b)
{
    ar_timer_t * timerA = a->getObject<ar_timer_t>();
    ar_timer_t * timerB = b->getObject<ar_timer_t>();
    return (timerA->m_isActive && timerA->m_wakeupTime < timerB->m_wakeupTime);
}

const char * ar_timer_get_name(ar_timer_t * timer)
{
    return timer ? timer->m_name : NULL;
}

ar_status_t Timer::init(const char * name, entry_t callback, void * param, ar_timer_mode_t timerMode, uint32_t delay)
{
    m_userCallback = callback;

    return ar_timer_create(this, name, timer_wrapper, param, timerMode, delay);
}

void Timer::timer_wrapper(ar_timer_t * timer, void * arg)
{
    Timer * _this = static_cast<Timer *>(timer);
    assert(_this);
    if (_this->m_userCallback)
    {
        _this->m_userCallback(_this, arg);
    }
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
