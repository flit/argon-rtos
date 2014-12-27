/*
 * Copyright (c) 2013-2014 Immo Software
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

#include "argon/src/ar_internal.h"
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
    kInitialLR = 0u //!< Set to 0 to stop stack crawl by debugger.
};

//! @brief Priorities for kernel exceptions.
enum _exception_priorities
{
    //! All handlers use the same, lowest priority.
    kHandlerPriority = 0xff
};

//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------

extern "C" void SysTick_Handler(void);
extern "C" uint32_t ar_port_yield_isr(uint32_t topOfStack, uint32_t isExtendedFrame);

//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------

//! @brief Global used solely to pass info back to asm PendSV handler code.
bool g_ar_hasExtendedFrame = false;

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

void ar_port_init_system(void)
{
    // Enable FPU on Cortex-M4F.
#if __FPU_USED
    // Enable full access to coprocessors 10 and 11 to enable FPU access.
    SCB->CPACR |= (3 << 20) | (3 << 22);

    // Disable lazy FP context save.
    FPU->FPCCR |= FPU_FPCCR_ASPEN_Msk;
    FPU->FPCCR &= ~FPU_FPCCR_LSPEN_Msk;
#endif // __FPU_USED

    // Init PSP.
    __set_PSP((uint32_t)g_ar.idleThread.m_stackPointer);
}

void ar_port_init_tick_timer(void)
{
    uint32_t ticks = SystemCoreClock / 1000 * kSchedulerQuanta_ms;
    SysTick_Config(ticks);

    // Set priorities for the exceptions we use in the kernel.
    NVIC_SetPriority(SVCall_IRQn, kHandlerPriority);
    NVIC_SetPriority(PendSV_IRQn, kHandlerPriority);
    NVIC_SetPriority(SysTick_IRQn, kHandlerPriority);
}

//! A total of 64 bytes of stack space is required to hold the initial
//! thread context.
//!
//! The entire remainder of the stack is filled with the pattern 0xba
//! as an easy way to tell what the high watermark of stack usage is.
void ar_port_prepare_stack(ar_thread_t * thread, void * param)
{
#if __FPU_USED
    // Clear the extended frame flag.
    thread->m_portData.m_hasExtendedFrame = false;
#endif // __FPU_USED

    // 8-byte align stack.
    uint32_t sp = reinterpret_cast<uint32_t>(thread->m_stackTop);
    uint32_t delta = sp & 7;
    sp -= delta;
    thread->m_stackTop = reinterpret_cast<uint8_t *>(sp);
    thread->m_stackSize = (thread->m_stackSize - delta) & ~7;

#if AR_THREAD_STACK_PATTERN_FILL
    // Fill the stack with a pattern.
    uint32_t * stackBottom = (uint32_t *)(sp - thread->m_stackSize);
    memset(stackBottom, 0xba, thread->m_stackSize);
#endif // AR_THREAD_STACK_PATTERN_FILL

    // Save new top of stack. Also, make sure stack is 8-byte aligned.
    sp -= sizeof(ThreadContext);
    thread->m_stackPointer = reinterpret_cast<uint8_t *>(sp);

    // Set the initial context on stack.
    ThreadContext * context = reinterpret_cast<ThreadContext *>(sp);
    context->xpsr = kInitialxPSR;
    context->pc = reinterpret_cast<uint32_t>(ar_thread_wrapper);
    context->lr = kInitialLR;
    context->r0 = reinterpret_cast<uint32_t>(thread); // Pass pointer to Thread object as first argument.
    context->r1 = reinterpret_cast<uint32_t>(param); // Pass arbitrary parameter as second argument.

    // For debug builds, set registers to initial values that are easy to identify on the stack.
#if DEBUG
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
    *stackBottom = kStackCheckValue;
}

void ar_atomic_add(uint32_t * value, uint32_t delta)
{
    KernelLock guard;
    *value += delta;
}

bool ar_atomic_compare_and_swap(uint32_t * value, uint32_t expectedValue, uint32_t newValue)
{
    KernelLock guard;
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
    ar_kernel_periodic_timer_isr();
}

uint32_t ar_port_yield_isr(uint32_t topOfStack, uint32_t isExtendedFrame)
{
#if __FPU_USED
    // Save whether there is an extended frame.
    if (g_ar.currentThread)
    {
        g_ar.currentThread->m_portData.m_hasExtendedFrame = isExtendedFrame;
    }
#endif // __FPU_USED

    // Run the scheduler.
    uint32_t stack = ar_kernel_yield_isr(topOfStack);

#if __FPU_USED
    // Pass whether there is an extended frame back to the asm code.
    g_ar_hasExtendedFrame = g_ar.currentThread->m_portData.m_hasExtendedFrame;
#endif // __FPU_USED

    return stack;
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
