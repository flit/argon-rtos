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

void Kernel::initSystem()
{
    // Init PSP.
    __set_PSP((uint32_t)s_idleThread.m_stackPointer);
}

void Kernel::initTimerInterrupt()
{
    uint32_t ticks = SystemCoreClock / 1000 * kSchedulerQuanta_ms;
    SysTick_Config(ticks);
    
    // Set priorities for the exceptions we use in the kernel.
    NVIC_SetPriority(SVCall_IRQn, kSVCallPriority);
    NVIC_SetPriority(PendSV_IRQn, kPendSVPriority);
    NVIC_SetPriority(SysTick_IRQn, kSysTickPriority);
}

//! A total of 64 bytes of stack space is required to hold the initial
//! thread context.
//!
//! The entire remainder of the stack is filled with the pattern 0xba
//! as an easy way to tell what the high watermark of stack usage is.
void Thread::prepareStack()
{
    // 8-byte align stack.
    uint32_t sp = reinterpret_cast<uint32_t>(m_stackTop);
    uint32_t delta = sp & 7;
    sp -= delta;
    m_stackTop = reinterpret_cast<uint8_t *>(sp);
    m_stackSize = (m_stackSize - delta) & ~7;
    
    // Fill the stack with a pattern.
    uint32_t * stackBottom = (uint32_t *)(sp - m_stackSize);
    memset(stackBottom, 0xba, m_stackSize);
    
    // Save new top of stack. Also, make sure stack is 8-byte aligned.
    sp -= sizeof(ThreadContext);
    m_stackPointer = reinterpret_cast<uint8_t *>(sp);
    
    // Set the initial context on stack.
    ThreadContext * context = reinterpret_cast<ThreadContext *>(sp);
    context->xpsr = kInitialxPSR;
    context->pc = reinterpret_cast<uint32_t>(thread_wrapper);
    context->lr = kInitialLR;
    context->r0 = reinterpret_cast<uint32_t>(this);
    
    // For debug builds, set registers to initial values that are easy to identify on the stack.
#if DEBUG
    context->r1 = 0x11111111;
    context->r2 = 0x22222222;
    context->r3 = 0x33333333;
    context->r4 = 0x44444444;
    context->r5 = 0x55555555;
    context->r6 = 0x66666666;
    context->r7 = 0x77777777;
    context->r8 = 0x88888888;
    context->r9 = 0x99999999;
    context->r10 = 0xaaaaaaaa;
    context->r11 = 0xbbbbbbbb;
    context->r12 = 0xcccccccc;
#endif
    
    // Write a check value to the bottom of the stack.
    *stackBottom = 0xdeadbeef;
}

void Atomic::add(uint32_t * value, uint32_t delta)
{
    IrqDisableAndRestore irqDisable;
    *value += delta;
}

bool Atomic::compareAndSwap(uint32_t * value, uint32_t expectedValue, uint32_t newValue)
{
    IrqDisableAndRestore irqDisable;
    uint32_t oldValue = *value;
    if (oldValue == expectedValue)
    {
        *value = newValue;
        return true;
    }
    return false;
}

void SysTick_Handler(void)
{
    ar_periodic_timer();
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
