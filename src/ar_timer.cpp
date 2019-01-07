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
 * @brief Source for Ar microkernel timers.
 */

#include "ar_internal.h"
#include <string.h>
#include <assert.h>

using namespace Ar;

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

static ar_status_t ar_timer_start_internal(ar_timer_t * timer, uint32_t wakeupTime);
static void ar_timer_deferred_start(void * object, void * object2);
static ar_status_t ar_timer_stop_internal(ar_timer_t * timer);
static void ar_timer_deferred_stop(void * object, void * object2);

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
    if (ar_port_get_irq_state())
    {
        return kArNotFromInterruptError;
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
    if (ar_port_get_irq_state())
    {
        return kArNotFromInterruptError;
    }

    ar_timer_stop(timer);

#if AR_GLOBAL_OBJECT_LISTS
    g_ar_objects.timers.remove(&timer->m_createdNode);
#endif

    return kArSuccess;
}

//! @brief Handles starting or restarting a timer.
static ar_status_t ar_timer_start_internal(ar_timer_t * timer, uint32_t wakeupTime)
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

static void ar_timer_deferred_start(void * object, void * object2)
{
    ar_timer_start_internal(reinterpret_cast<ar_timer_t *>(object), reinterpret_cast<uint32_t>(object2));
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
        return g_ar.deferredActions.post(ar_timer_deferred_start, timer, reinterpret_cast<void *>(wakeupTime));
    }

    return ar_timer_start_internal(timer, wakeupTime);
}

static ar_status_t ar_timer_stop_internal(ar_timer_t * timer)
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

static void ar_timer_deferred_stop(void * object, void * object2)
{
    ar_timer_stop_internal(reinterpret_cast<ar_timer_t *>(object));
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
        return g_ar.deferredActions.post(ar_timer_deferred_stop, timer);
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

    timer->m_delay = ar_milliseconds_to_ticks(delay);

    // If the timer is running, we need to restart it, unless it is a periodic
    // timer whose callback is currently executing. In that case, the timer will
    // automatically be rescheduled for us, so if we did it here then it would be
    // rescheduled twice causing a double delay.
    if (timer->m_isActive && !(timer->m_isRunning && timer->m_mode == kArPeriodicTimer))
    {
        ar_timer_start(timer);
    }

    return kArSuccess;
}

//! @brief Execute callbacks for all expired timers.
//!
//! While a timer callback is running, the m_isRunning flag on the timer is set to true.
//! When the callback returns, a one shot timer is stopped while a periodic timer is
//! rescheduled based on its delay. If a periodic timer's callback runs so long that the next
//! wakeup time is in the past, it will be rescheduled to a time in the future that is aligned
//! with the period.
void ar_kernel_run_timers(ar_list_t & timersList)
{
    // Check if we need to handle a timer.
    if (timersList.m_head)
    {
        ar_list_node_t * timerNode = timersList.m_head;
        do {
            ar_timer_t * timer = timerNode->getObject<ar_timer_t>();
            assert(timer);

            // Exit loop if all remaining timers on the list wake up in the future.
            if (timer->m_wakeupTime > g_ar.tickCount)
            {
                break;
            }

            // Invoke the timer callback.
            assert(timer->m_callback);
            timer->m_isRunning = true;
            timer->m_callback(timer, timer->m_param);
            timer->m_isRunning = false;

            // Check that the timer wasn't stopped in its callback.
            if (timer->m_isActive)
            {
                switch (timer->m_mode)
                {
                    case kArOneShotTimer:
                        // Stop a one shot timer after it has fired.
                        ar_timer_stop(timer);
                        break;

                    case kArPeriodicTimer:
                    {
                        // Restart a periodic timer without introducing (much) jitter. Also handle
                        // the cases where the timer callback ran longer than the next wakeup.
                        uint32_t wakeupTime = timer->m_wakeupTime + timer->m_delay;
                        if (wakeupTime == g_ar.tickCount)
                        {
                            // Push the wakeup out another period into the future.
                            wakeupTime += timer->m_delay;
                        }
                        else if (wakeupTime < g_ar.tickCount)
                        {
                            // Compute the delay to the next wakeup in the future that is aligned
                            // to the timer's period.
                            uint32_t delta = (g_ar.tickCount - timer->m_wakeupTime + timer->m_delay - 1)
                                                / timer->m_delay * timer->m_delay;
                            wakeupTime = timer->m_wakeupTime + delta;
                        }
                        ar_timer_start_internal(timer, wakeupTime);
                        break;
                    }
                }
            }

            timerNode = timerNode->m_next;
        } while (timerNode != timersList.m_head);
    }
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

ar_status_t Timer::init(const char * name, callback_t callback, void * param, ar_timer_mode_t timerMode, uint32_t delay)
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
