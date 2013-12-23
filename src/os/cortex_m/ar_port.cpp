/*
 * Copyright (c) 2013, Chris Reed
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
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
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

#include "os/ar_kernel.h"
#include "ar_port.h"

#include <string.h>

using namespace Ar;

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

enum
{
    kSchedulerQuanta_ms = 10
};

//! Initial thread register values.
enum
{
    kInitialxPSR = 0x01000000u, //!< Set T bit.
    kInitialLR = 0xfffffffeu    //!< Lockup address.
};

//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------

extern "C" void SysTick_Handler(void);

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

void Thread::initSystem()
{
    // Init PSP.
    __set_PSP((uint32_t)s_idleThread.m_stackPointer);
    
    // Switch to PSP.
//     CONTROL_Type c;
//     c.w = __get_CONTROL();
//     c.b.SPSEL = 1;
//     __set_CONTROL(c.w);
//     __ISB();
}

void Thread::initTimerInterrupt()
{
    uint32_t ticks = SystemCoreClock / (kSchedulerQuanta_ms * 1000);
    SysTick_Config(ticks);
}

//! A total of 64 bytes of stack space is required to hold the initial
//! thread context.
//!
//! The entire remainder of the stack is filled with the pattern 0xcc
//! as an easy way to tell what the high watermark of stack usage is.
void Thread::prepareStack()
{
    // 8-byte align stack.
    uint32_t sp = reinterpret_cast<uint32_t>(m_stackTop);
    uint32_t delta = sp & 7;
    if (delta)
    {
        sp -= delta;
        m_stackTop = reinterpret_cast<uint8_t *>(sp);
        m_stackSize -= delta;
    }
    
    // Fill the stack with a pattern.
    memset(m_stackTop - m_stackSize, 0xcc, m_stackSize);
    
    // Save new top of stack. Also, make sure stack is 8-byte aligned.
    sp -= sizeof(ThreadContext);
    m_stackPointer = reinterpret_cast<uint8_t *>(sp);
    
    // Set the initial context on stack.
    ThreadContext * context = reinterpret_cast<ThreadContext *>(sp);
    context->xpsr = kInitialxPSR;
    context->pc = reinterpret_cast<uint32_t>(thread_wrapper);
    context->lr = kInitialLR;
    context->r0 = reinterpret_cast<uint32_t>(this);
}

void SysTick_Handler(void)
{
    ar_periodic_timer();
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
