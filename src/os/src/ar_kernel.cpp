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
 * @file ar_kernel.cpp
 * @brief Source for Ar kernel.
 */

#include "ar_internal.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

using namespace Ar;

//------------------------------------------------------------------------------
// Defines
//------------------------------------------------------------------------------

#if !defined(AR_ENABLE_IDLE_SLEEP)
    //! Controls whether the idle thread puts the processor to sleep
    //! until the next interrupt. Set to 1 to enable.
    #define AR_ENABLE_IDLE_SLEEP (0)
#endif

#if !defined(AR_ENABLE_SYSTEM_LOAD)
    //! When set to 1 the idle thread will compute the system load percentage.
    #define AR_ENABLE_SYSTEM_LOAD (1)
#endif

#if !defined(AR_IDLE_THREAD_STACK_SIZE)
    //! Size in bytes of the idle thread's stack.
    #define AR_IDLE_THREAD_STACK_SIZE 256 
    //(sizeof(ThreadContext) + 32)
#endif // AR_IDLE_THREAD_STACK_SIZE

//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------

ar_kernel_t g_ar = {0};

//! The stack for the idle thread.
static uint8_t s_idleThreadStack[AR_IDLE_THREAD_STACK_SIZE];

// bool Kernel::s_isRunning = false;
// volatile uint32_t Kernel::s_tickCount = 0;
// volatile uint32_t Kernel::s_irqDepth = 0;

//! @internal
//!
//! The volatile is necessary so that the IAR optimizer doesn't remove the entire load
//! calculation loop of the idle_entry() function.
// volatile unsigned Kernel::s_systemLoad = 0;

// uint8_t Kernel::s_idleThreadStack[AR_IDLE_THREAD_STACK_SIZE];

//! The lowest priority thread in the system. Executes only when no other
//! threads are ready.
// Thread Kernel::s_idleThread;

// #if AR_GLOBAL_OBJECT_LISTS
// //! This global contains linked lists of all the various Ar object
// //! types that have been created during runtime. This makes it much
// //! easier to examine objects of interest.
// ObjectLists g_allObjects;
// #endif // AR_GLOBAL_OBJECT_LISTS

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

//! @brief System idle thread entry point.
//!
//! This thread just spins forever.
//!
//! If the #AR_ENABLE_SYSTEM_LOAD define has been set to 1 then this thread will
//! also calculate the average system load once per second. The system load is
//! accessible with the Kernel::getSystemLoad() static member.
//!
//! @param param Ignored.
void idle_entry(void * param)
{
#if AR_ENABLE_SYSTEM_LOAD
    uint32_t start;
    uint32_t last;
    uint32_t ticks;
    uint32_t skipped = 0;
    
    start = g_ar.tickCount;
    last = start;
#endif // AR_ENABLE_SYSTEM_LOAD
    
    while (1)
    {
        // Check if we need to handle a timer.
        if (g_ar.activeTimers.m_head)
        {
            ar_list_node_t * timerNode = g_ar.activeTimers.m_head;
            bool handledTimer = false;
            while (timerNode)
            {
                ar_timer_t * timer = timerNode->getObject<ar_timer_t>();
                assert(timer);
                
                if (timer->m_wakeupTime > g_ar.tickCount)
                {
                    break;
                }
                
                // Invoke the timer callback.
                assert(timer->m_callback);
                timer->m_callback(timer, timer->m_param);
            
                switch (timer->m_mode)
                {
                    case kArOneShotTimer:
                        // Stop a one shot timer after it has fired.
                        ar_timer_stop(timer);
                        break;
                    
                    case kArPeriodicTimer:
                        // Restart a periodic timer.
                        ar_timer_start(timer);
                        break;
                }
            
                handledTimer = true;
                timerNode = timerNode->m_next;
            }
        
            // If we handled any timers, we need to set our priority back to the normal idle thread
            // priority. The tick handler will have raised our priority to the max in order to handle
            // the expired timers.
            if (handledTimer)
            {
                ar_thread_set_priority(&g_ar.idleThread, 0);
            }
        }
        
        // Compute system load.
#if AR_ENABLE_SYSTEM_LOAD
        ticks = g_ar.tickCount;
        
        if (ticks != last)
        {
            uint32_t diff = ticks - last;
            
            if (ticks - start >= 100)
            {
                unsigned s = start + 100 - ticks;
                
                if (diff - 1 > s)
                {
                    skipped += s;
                }
                else
                {
                    skipped += diff - 1;
                }
                
                g_ar.systemLoad = skipped;
                
                // start over counting
                if (diff - 1 > s)
                {
                    skipped = diff - 1 - s;
                }
                else
                {
                    skipped = 0;
                }
                start = ticks;
            }
            else
            {
                skipped += diff - 1;
            }
            
            last = ticks;
        }
#endif // AR_ENABLE_SYSTEM_LOAD
        
#if AR_ENABLE_IDLE_SLEEP
        // Hitting this bit puts the processor to sleep until the next interrupt fires.
        __WFI();
#endif // AR_ENABLE_IDLE_SLEEP
    }
}

//! Uses a "swi" instruction to yield to the scheduler when in user mode. If
//! the CPU is in IRQ mode then we can just call the scheduler() method
//! directly and any change will take effect when the ISR exits.
void ar_kernel_enter_scheduler(void)
{
    if (g_ar.irqDepth == 0)
    {
        // In user mode we must SWI into the scheduler.
        ar_port_service_call();
    }
    else
    {
        // We're in IRQ mode so just call the scheduler directly
        ar_kernel_scheduler();
    }
}

// See ar_kernel.h for documentation of this function.
void ar_kernel_run(void)
{
    // Assert if there is no thread ready to run.
    assert(g_ar.readyList.m_head);
    
    // Create the idle thread. Priority 1 is passed to init function to pass the
    // assertion and then set to the correct 0 manually.
    ar_thread_create(&g_ar.idleThread, "idle", idle_entry, 0, s_idleThreadStack, sizeof(s_idleThreadStack), 1);
    g_ar.idleThread.m_priority = 0;
    ar_thread_resume(&g_ar.idleThread);
    
    // Set up system tick timer
    ar_port_init_tick_timer();
    
    ar_port_init_system();
    
    // We're now ready to run
    g_ar.isRunning = true;
    
    // Enter into the scheduler. The yieldIsr() will see that s_currentThread
    // is NULL and ignore the stack pointer it was given. After the scheduler
    // runs, we return from the scheduler to a ready thread.
    ar_kernel_enter_scheduler();

    // should never reach here
    _halt();
}

void ar_kernel_periodic_timer_isr()
{
    // Exit immediately if the kernel isn't running.
    if (!g_ar.isRunning)
    {
        return;
    }
    
    ar_kernel_increment_tick_count(1);

    // Run the scheduler. It will modify s_currentThread if switching threads.
    ar_port_service_call();

    // This case should never happen because of the idle thread.
    assert(g_ar.currentThread);
}

//! @param topOfStack This parameter should be the stack pointer of the thread that was
//!     current when the timer IRQ fired.
//! @return The value of the current thread's stack pointer is returned. If the scheduler
//!     changed the current thread, this will be a different value from what was passed
//!     in @a topOfStack.
uint32_t ar_kernel_yield_isr(uint32_t topOfStack)
{
    // save top of stack for the thread we interrupted
    if (g_ar.currentThread)
    {
        g_ar.currentThread->m_stackPointer = reinterpret_cast<uint8_t *>(topOfStack);
    }
    
    // Run the scheduler. It will modify s_currentThread if switching threads.
    ar_kernel_scheduler();

    // The idle thread prevents this condition.
    assert(g_ar.currentThread);

    // return the new thread's stack pointer
    return (uint32_t)g_ar.currentThread->m_stackPointer;
}

// See ar_kernel.h for documentation of this function.
void ar_kernel_enter_interrupt()
{
    ++g_ar.irqDepth;
}

// See ar_kernel.h for documentation of this function.
void ar_kernel_exit_interrupt()
{
    --g_ar.irqDepth;
}

void * ar_yield(void * topOfStack)
{
    return (void *)ar_kernel_yield_isr((uint32_t)topOfStack);
}

void ar_periodic_timer(void)
{
    return ar_kernel_periodic_timer_isr();
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
