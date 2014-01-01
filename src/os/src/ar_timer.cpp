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
 * @brief Source for Ar microkernel timers.
 */

#include "os/ar_kernel.h"
#include <string.h>
#include <assert.h>

using namespace Ar;

//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------

Timer * Timer::s_activeTimers = NULL;

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

// See ar_kernel.h for documentation of this function.
Timer::~Timer()
{
    stop();
}

// See ar_kernel.h for documentation of this function.
void Timer::init(const char * name, timer_entry_t callback, void * param, timer_mode_t timerMode, uint32_t delay)
{
    NamedObject::init(name);
    
    m_callback = callback;
    m_param = param;
    m_mode = timerMode;
    m_delay = Time::millisecondsToTicks(delay);
    m_wakeupTime = 0;
    m_next = NULL;
    m_isActive = false;
}

// See ar_kernel.h for documentation of this function.
void Timer::start()
{
    if (!m_callback)
    {
        return;
    }
    
    IrqDisableAndRestore irqDisable;
    
    // Handle a timer that is already active.
    if (m_isActive)
    {
        removeFromList();
    }
    
    m_wakeupTime = Kernel::getTickCount() + m_delay;
    m_isActive = true;
    addToList();
}

// See ar_kernel.h for documentation of this function.
void Timer::start(uint32_t newDelay)
{
    if (m_isActive)
    {
        stop();
    }
    
    setDelay(newDelay);
    start();
}

// See ar_kernel.h for documentation of this function.
void Timer::stop()
{
    if (!m_isActive)
    {
        return;
    }
    
    IrqDisableAndRestore irqDisable;
    
    removeFromList();
    m_wakeupTime = 0;
    m_isActive = false;
}

// See ar_kernel.h for documentation of this function.
void Timer::setDelay(uint32_t delay)
{
    IrqDisableAndRestore irqDisable;
    
    m_delay = Time::millisecondsToTicks(delay);
    
    // If the timer is running, we need to update the wakeup time and resort the list.
    if (m_isActive)
    {
        m_wakeupTime = Kernel::getTickCount() + m_delay;
        
        removeFromList();
        addToList();
    }
}

//! The timer is added in sorted order by wakeup time.
void Timer::addToList()
{
    Timer * & listHead = s_activeTimers;
    m_next = NULL;

    // Insert at head of list if our wakeup time is the earliest or if the list is empty.
    if (!listHead || m_wakeupTime < listHead->m_wakeupTime)
    {
        m_next = listHead;
        listHead = this;
        return;
    }

    // Insert sorted by wakeup time.
    Timer * timer = listHead;
    
    while (timer)
    {
        // Insert at end if we've reached the end of the list and there were no timers with later
        // wakeup times.
        if (!timer->m_next)
        {
            timer->m_next = this;
            break;
        }
        // If our wakeup time is earlier than the next timer in the list, insert here.
        else if (m_wakeupTime < timer->m_next->m_wakeupTime)
        {
            m_next = timer->m_next;
            timer->m_next = this;
            break;
        }
        
        timer = timer->m_next;
    }
}

//! The list is not allowed to be empty.
void Timer::removeFromList()
{
    Timer * & listHead = s_activeTimers;
    
    // the list must not be empty
    assert(listHead != NULL);

    if (listHead == this)
    {
        // special case for removing the list head
        listHead = m_next;
    }
    else
    {
        Timer * item = listHead;
        while (item)
        {
            if (item->m_next == this)
            {
                item->m_next = m_next;
                return;
            }

            item = item->m_next;
        }
    }
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
