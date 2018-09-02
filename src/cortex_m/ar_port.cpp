/*
 * Copyright (c) 2013-2015 Immo Software
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

#include "../ar_internal.h"
#include "ar_port.h"
#include <assert.h>
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

void ar_port_init_system()
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

    // Set priorities for the exceptions we use in the kernel.
    NVIC_SetPriority(SVCall_IRQn, kHandlerPriority);
    NVIC_SetPriority(PendSV_IRQn, kHandlerPriority);
    NVIC_SetPriority(SysTick_IRQn, kHandlerPriority);
}

void ar_port_init_tick_timer()
{
    // Set SysTick clock source to processor clock.
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk;

    // Clear any pending SysTick IRQ.
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk;

#if AR_ENABLE_TICKLESS_IDLE
    ar_port_set_timer_delay(false, 0);
#else // AR_ENABLE_TICKLESS_IDLE
    ar_port_set_timer_delay(true, kSchedulerQuanta_ms * 1000);
#endif // AR_ENABLE_TICKLESS_IDLE
}

void ar_port_set_timer_delay(bool enable, uint32_t delay_us)
{
    // Disable SysTick while we adjust it.
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk; //&= ~(SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk);

    if (enable)
    {
        // If the delay is 0, just make the SysTick interrupt pending.
        if (delay_us == 0)
        {
            // Clear reload and counter so the elapsed time reads as 0 in ar_port_get_timer_elapsed_us().
            SysTick->LOAD = 0;
            SysTick->VAL = 0;

            // Pend SysTick.
            SCB->ICSR = SCB_ICSR_PENDSTSET_Msk;
            return;
        }

        // Calculate SysTick reload value. If the desired delay overflows the SysTick counter,
        // we just use the max delay (24 bits for SysTick).
        uint32_t ticks = SystemCoreClock / 1000000 * delay_us - 1;
        if (ticks > SysTick_LOAD_RELOAD_Msk)
        {
            ticks = SysTick_LOAD_RELOAD_Msk;
        }

        // Update SysTick reload value.
        SysTick->LOAD = ticks;

        // Reset the timer to count from the new load value. This also clears COUNTFLAG.
        SysTick->VAL = 0;

        // Enable SysTick and its IRQ.
        SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
    }
    else
    {
        // Enable SysTick for maximum count but disable IRQ.
        SysTick->LOAD = SysTick_LOAD_RELOAD_Msk;
        SysTick->VAL = 0;
        SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
    }
}

uint32_t ar_port_get_timer_elapsed_us()
{
    uint32_t max = SysTick->LOAD;
    uint32_t counter = max - SysTick->VAL;
    if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
    {
        counter += max;
    }
    return counter / (SystemCoreClock / 1000000);
}


//! A total of 64 bytes of stack space is required to hold the initial
//! thread context.
//!
//! The entire remainder of the stack is filled with the pattern 0xba
//! as an easy way to tell what the high watermark of stack usage is.
void ar_port_prepare_stack(ar_thread_t * thread, uint32_t stackSize, void * param)
{
#if __FPU_USED
    // Clear the extended frame flag.
    thread->m_portData.m_hasExtendedFrame = false;
#endif // __FPU_USED

    // 8-byte align stack.
    uint32_t sp = reinterpret_cast<uint32_t>(thread->m_stackBottom) + stackSize;
    uint32_t delta = sp & 7;
    sp -= delta;
    stackSize = (stackSize - delta) & ~7;
    thread->m_stackTop = reinterpret_cast<uint32_t *>(sp);
    thread->m_stackBottom = reinterpret_cast<uint32_t *>(sp - stackSize);

#if AR_THREAD_STACK_PATTERN_FILL
    // Fill the stack with a pattern. We just take the low byte of the fill pattern since
    // memset() is a byte fill. This assumes each byte of the fill pattern is the same.
    memset(thread->m_stackBottom, kStackFillValue & 0xff, stackSize);
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
    *thread->m_stackBottom = kStackCheckValue;
}

// Provide atomic operations for Cortex-M0+ that doesn't have load/store exclusive
// instructions. We just have to disable interrupts.
#if (__CORTEX_M < 3)
int8_t ar_atomic_add8(volatile int8_t * value, int8_t delta)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    __DSB();
    int8_t originalValue = *value;
    *value += delta;
    __set_PRIMASK(primask);
    return originalValue;
}

int16_t ar_atomic_add16(volatile int16_t * value, int16_t delta)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    __DSB();
    int16_t originalValue = *value;
    *value += delta;
    __set_PRIMASK(primask);
    return originalValue;
}

int32_t ar_atomic_add32(volatile int32_t * value, int32_t delta)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    __DSB();
    int32_t originalValue = *value;
    *value += delta;
    __set_PRIMASK(primask);
    return originalValue;
}

bool ar_atomic_cas8(volatile int8_t * value, int8_t expectedValue, int8_t newValue)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    __DSB();
    int8_t oldValue = *value;
    if (oldValue == expectedValue)
    {
        *value = newValue;
        __set_PRIMASK(primask);
        return true;
    }
    __set_PRIMASK(primask);
    return false;
}

bool ar_atomic_cas16(volatile int16_t * value, int16_t expectedValue, int16_t newValue)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    __DSB();
    int16_t oldValue = *value;
    if (oldValue == expectedValue)
    {
        *value = newValue;
        __set_PRIMASK(primask);
        return true;
    }
    __set_PRIMASK(primask);
    return false;
}

bool ar_atomic_cas32(volatile int32_t * value, int32_t expectedValue, int32_t newValue)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    __DSB();
    int32_t oldValue = *value;
    if (oldValue == expectedValue)
    {
        *value = newValue;
        __set_PRIMASK(primask);
        return true;
    }
    __set_PRIMASK(primask);
    return false;
}
#endif // (__CORTEX_M < 3)

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

#if DEBUG
void ar_port_service_call()
{
    assert(g_ar.lockCount == 0);
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    __DSB();
    __ISB();
}
#endif // DEBUG

WEAK uint64_t ar_get_microseconds()
{
    return static_cast<uint64_t>(ar_get_millisecond_count()) * 1000ull;
}

#if AR_ENABLE_TRACE
void ar_trace_init()
{
}

//! @brief Send one trace event word via ITM port 31.
void ar_trace_1(uint8_t eventID, uint32_t data)
{
    if (ITM->TCR & ITM_TCR_ITMENA_Msk)
    {
        // Wait until we can send the event.
        while (!ITM->PORT[31].u32)
        {
        }

        // Event consists of 8-bit event ID plus 24-bits of event data.
        ITM->PORT[31].u32 = (static_cast<uint32_t>(eventID) << 24) | (data & 0x00ffffff);
    }
}

//! @brief Send a 2-word trace event via ITM ports 31 and 30.
void ar_trace_2(uint8_t eventID, uint32_t data0, void * data1)
{
    if (ITM->TCR & ITM_TCR_ITMENA_Msk)
    {
        // Wait until we can send the event.
        while (!ITM->PORT[31].u32)
        {
        }

        // Event consists of 8-bit event ID plus 24-bits of event data.
        ITM->PORT[31].u32 = (static_cast<uint32_t>(eventID) << 24) | (data0 & 0x00ffffff);

        // Wait until we can send the event.
        while (!ITM->PORT[30].u32)
        {
        }

        // Send second data on port 30.
        ITM->PORT[30].u32 = reinterpret_cast<uint32_t>(data1);
    }
}
#endif // AR_ENABLE_TRACE

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
