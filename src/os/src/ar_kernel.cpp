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
 * @file ar_kernel.cpp
 * @brief Source for Ar kernel.
 */

#include "os/ar_kernel.h"
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

bool Kernel::s_isRunning = false;
volatile uint32_t Kernel::s_tickCount = 0;
volatile uint32_t Kernel::s_irqDepth = 0;

//! @internal
//!
//! The volatile is necessary so that the IAR optimizer doesn't remove the entire load
//! calculation loop of the idle_entry() function.
volatile unsigned Kernel::s_systemLoad = 0;

//! The stack for the idle thread.
uint8_t Kernel::s_idleThreadStack[AR_IDLE_THREAD_STACK_SIZE];

//! The lowest priority thread in the system. Executes only when no other
//! threads are ready.
Thread Kernel::s_idleThread;

#if AR_GLOBAL_OBJECT_LISTS
//! This global contains linked lists of all the various Ar object
//! types that have been created during runtime. This makes it much
//! easier to examine objects of interest.
ObjectLists g_allObjects;
#endif // AR_GLOBAL_OBJECT_LISTS

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
void Kernel::idle_entry(void * param)
{
#if AR_ENABLE_SYSTEM_LOAD
    uint32_t start;
    uint32_t last;
    uint32_t ticks;
    uint32_t skipped = 0;
    
    start = Kernel::getTickCount();
    last = start;
#endif // AR_ENABLE_SYSTEM_LOAD
    
    while (1)
    {
        // Check if we need to handle a timer.
        Timer * timer = Timer::s_activeTimers;
        bool handledTimer = false;
        while (timer && timer->m_wakeupTime <= Kernel::s_tickCount)
        {
            // Invoke the timer callback.
            assert(timer->m_callback);
            timer->m_callback(timer, timer->m_param);
            
            switch (timer->m_mode)
            {
                case Timer::kOneShotTimer:
                    // Stop a one shot timer after it has fired.
                    timer->stop();
                    break;
                    
                case Timer::kPeriodicTimer:
                    // Restart a periodic timer.
                    timer->start();
                    break;
            }
            
            handledTimer = true;
            timer = timer->m_next;
        }
        
        // If we handled any timers, we need to set our priority back to the normal idle thread
        // priority. The tick handler will have raised our priority to the max in order to handle
        // the expired timers.
        if (handledTimer)
        {
            s_idleThread.setPriority(0);
        }
        
        // Compute system load.
#if AR_ENABLE_SYSTEM_LOAD
        ticks = Kernel::getTickCount();
        
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
                
                s_systemLoad = skipped;
                
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
void Kernel::enterScheduler()
{
    if (s_irqDepth == 0)
    {
        // In user mode we must SWI into the scheduler.
        service_call();
    }
    else
    {
        // We're in IRQ mode so just call the scheduler directly
        Thread::scheduler();
    }
}

// See ar_kernel.h for documentation of this function.
void Kernel::run()
{
    // Assert if there is no thread ready to run.
    assert(Thread::s_readyList);
    
    // Create the idle thread. Priority 1 is passed to init function to pass the
    // assertion and then set to the correct 0 manually.
    s_idleThread.init("idle", idle_entry, 0, s_idleThreadStack, AR_IDLE_THREAD_STACK_SIZE, 1);
    s_idleThread.m_priority = 0;
    s_idleThread.resume();
    
    // Set up system tick timer
    initTimerInterrupt();
    
    initSystem();
    
    // We're now ready to run
    s_isRunning = true;
    
    // Swi into the scheduler. The yieldIsr() will see that s_currentThread
    // is NULL and ignore the stack pointer it was given. After the scheduler
    // runs, we return from the swi handler to the init thread. Interrupts
    // are enabled in that switch to the init thread since all threads start
    // with a CPSR that enables IRQ and FIQ.
    enterScheduler();

    // should never reach here
    _halt();
}

void Kernel::periodicTimerIsr()
{
    // Exit immediately if the kernel isn't running.
    if (!s_isRunning)
    {
        return;
    }
    
    Thread::incrementTickCount(1);

    // Run the scheduler. It will modify s_currentThread if switching threads.
    service_call();

    // This case should never happen because of the idle thread.
    assert(Thread::getCurrent());
}

//! @param topOfStack This parameter should be the stack pointer of the thread that was
//!     current when the timer IRQ fired.
//! @return The value of the current thread's stack pointer is returned. If the scheduler
//!     changed the current thread, this will be a different value from what was passed
//!     in @a topOfStack.
uint32_t Kernel::yieldIsr(uint32_t topOfStack)
{
    // save top of stack for the thread we interrupted
    if (Thread::getCurrent())
    {
        Thread::getCurrent()->m_stackPointer = reinterpret_cast<uint8_t *>(topOfStack);
    }
    
    // Run the scheduler. It will modify s_currentThread if switching threads.
    Thread::scheduler();

    // The idle thread prevents this condition.
    assert(Thread::getCurrent());

    // return the new thread's stack pointer
    return (uint32_t)Thread::getCurrent()->m_stackPointer;
}

// See ar_kernel.h for documentation of this function.
void Kernel::enterInterrupt()
{
    ++Kernel::s_irqDepth;
}

// See ar_kernel.h for documentation of this function.
void Kernel::exitInterrupt()
{
    --Kernel::s_irqDepth;
}

void * ar_yield(void * topOfStack)
{
    return (void *)Kernel::yieldIsr((uint32_t)topOfStack);
}

void ar_periodic_timer(void)
{
    return Kernel::periodicTimerIsr();
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
