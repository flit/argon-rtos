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
status_t ar_timer_create(ar_timer_t * timer, const char * name, ar_timer_entry_t callback, void * param, ar_timer_mode_t timerMode, uint32_t delay)
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
    timer->m_createdNode.m_obj = timer;

#if AR_GLOBAL_OBJECT_LISTS
    g_ar.allObjects.timers.add(&timer->m_createdNode);
#endif

    return kArSuccess;
}

// See ar_kernel.h for documentation of this function.
status_t ar_timer_delete(ar_timer_t * timer)
{
    if (!timer)
    {
        return kArInvalidParameterError;
    }
    
    ar_timer_stop(timer);
    
#if AR_GLOBAL_OBJECT_LISTS
    g_ar.allObjects.timers.remove(&timer->m_createdNode);
#endif

    return kArSuccess;
}

// See ar_kernel.h for documentation of this function.
status_t ar_timer_start(ar_timer_t * timer)
{
    if (!timer)
    {
        return kArInvalidParameterError;
    }
    
    // The callback should have been verified by the create function.
    assert(timer->m_callback);
    
    IrqDisableAndRestore irqDisable;
    
    // Handle a timer that is already active.
    if (timer->m_isActive)
    {
        g_ar.activeTimers.remove(&timer->m_activeNode);
    }
    
    timer->m_wakeupTime = g_ar.tickCount + timer->m_delay;
    timer->m_isActive = true;
    g_ar.activeTimers.add(&timer->m_activeNode, ar_timer_sort_by_wakeup);

    return kArSuccess;
}

// See ar_kernel.h for documentation of this function.
status_t ar_timer_stop(ar_timer_t * timer)
{
    if (!timer)
    {
        return kArInvalidParameterError;
    }
    if (!timer->m_isActive)
    {
        return kArTimerNotRunningError;
    }
    
    IrqDisableAndRestore irqDisable;
    
    g_ar.activeTimers.remove(&timer->m_activeNode);
    timer->m_wakeupTime = 0;
    timer->m_isActive = false;

    return kArSuccess;
}

// See ar_kernel.h for documentation of this function.
status_t ar_timer_set_delay(ar_timer_t * timer, uint32_t delay)
{
    if (!timer)
    {
        return kArInvalidParameterError;
    }
    
    IrqDisableAndRestore irqDisable;
    
    timer->m_delay = ar_milliseconds_to_ticks(delay);
    
    // If the timer is running, we need to update the wakeup time and resort the list.
    if (timer->m_isActive)
    {
        timer->m_wakeupTime = g_ar.tickCount + timer->m_delay;
        
        g_ar.activeTimers.remove(&timer->m_activeNode);
        g_ar.activeTimers.add(&timer->m_activeNode, ar_timer_sort_by_wakeup);
    }

    return kArSuccess;
}

//! @brief Sort timers ascending by wakeup time.
bool ar_timer_sort_by_wakeup(ar_list_node_t * a, ar_list_node_t * b)
{
    ar_timer_t * timerA = a->getObject<ar_timer_t>();
    ar_timer_t * timerB = b->getObject<ar_timer_t>();
    return timerA->m_wakeupTime < timerB->m_wakeupTime;
}

const char * ar_timer_get_name(ar_timer_t * timer)
{
    return timer ? timer->m_name : NULL;
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
