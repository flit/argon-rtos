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
 * @ingroup ar
 */

#include "os/ar_kernel.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

using namespace Ar;

//------------------------------------------------------------------------------
// Defines
//------------------------------------------------------------------------------

#ifndef MU_ENABLE_IDLE_SLEEP
    //! Controls whether the idle thread puts the processor to sleep
    //! until the next interrupt. Set to 1 to enable.
    #define MU_ENABLE_IDLE_SLEEP 0
#endif

#ifndef MU_ENABLE_SYSTEM_LOAD
    //! When set to 1 the idle thread will compute the system load percentage.
    #define MU_ENABLE_SYSTEM_LOAD 1
#endif

#ifndef MU_PRINT_SYSTEM_LOAD
    //! Controls if the system load is printed.
    #define MU_PRINT_SYSTEM_LOAD 0
#endif

#if MU_PRINT_SYSTEM_LOAD
    //! Size in bytes of the idle thread's stack. Needs much more stack
    //! space in order to call printf().
    #define MU_IDLE_THREAD_STACK_SIZE (2048)
#else // MU_PRINT_SYSTEM_LOAD
    //! Size in bytes of the idle thread's stack.
    #define MU_IDLE_THREAD_STACK_SIZE (256)
#endif // MU_PRINT_SYSTEM_LOAD

//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------

bool Kernel::s_isRunning = false;
volatile uint32_t Kernel::s_tickCount = 0;
volatile uint32_t Kernel::s_irqDepth = 0;
unsigned Kernel::s_systemLoad = 0;

//! The stack for #s_idleThread.
uint8_t Kernel::s_idleThreadStack[MU_IDLE_THREAD_STACK_SIZE];

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
//! If the #MU_ENABLE_SYSTEM_LOAD define has been set to 1 then this thread will
//! also calculate the average system load once per second. The system load is
//! accessible with the Thread::getSystemLoad() static member.
//!
//! If the #MU_ENABLE_IDLE_SLEEP define is set to 1, the idle thread hits the
//! PCK bit in the SCDR register of the AT91 Power Management Controller
//! peripheral. This causes the CPU to go to sleep until the next interrupt,
//! which if nothing else will be the OS tick timer. This option can make it
//! very easy to save power.
//!
//! @param param Ignored.
void Kernel::idle_entry(void * param)
{
#if MU_ENABLE_SYSTEM_LOAD
    uint32_t start;
    uint32_t last;
    uint32_t ticks;
    uint32_t skipped = 0;
    
    start = Kernel::getTickCount();
    last = start;
#endif // MU_ENABLE_SYSTEM_LOAD
    
    while (1)
    {
#if MU_ENABLE_SYSTEM_LOAD
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
                
                #if MU_PRINT_SYSTEM_LOAD
                printf("%d%% system load\n", s_systemLoad);
                #endif // MU_PRINT_SYSTEM_LOAD
                
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
#endif // MU_ENABLE_SYSTEM_LOAD
        
#if MU_ENABLE_IDLE_SLEEP
        // Hitting this bit puts the processor to sleep until the next
        // IRQ or FIQ fires.
        *AT91C_PMC_SCDR = AT91C_PMC_PCK;
#endif // MU_ENABLE_IDLE_SLEEP
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

//! The system idle thread is created here. Its priority is set to 0, the lowest
//! possible priority. The idle thread will run when there are no other ready threads.
//!
//! At least one user thread must have been created and resumed before
//! calling run(). Usually this will be an initialisation thread that itself
//! creates the other threads of the application.
//!
//! @pre Interrupts must not be enabled prior to calling this routine.
//!
//! @note Control will never return from this method.
void Kernel::run()
{
    // Assert if there is no thread ready to run.
    assert(Thread::s_readyList);
    
    // Create the idle thread. Priority 1 is passed to init function to pass the
    // assertion and then set to the correct 0 manually.
    s_idleThread.init("idle", idle_entry, 0, s_idleThreadStack, MU_IDLE_THREAD_STACK_SIZE, 1);
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
