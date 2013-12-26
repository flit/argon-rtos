/*
 * Copyright (c) 2013 Immo Software
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
 * @file arport.h
 * @ingroup ar_port
 * @brief Header for the Argon RTOS.
 */

#if !defined(_AR_PORT_H_)
#define _AR_PORT_H_

#include "fsl_platform_common.h"
#include "fsl_device_registers.h"

namespace Ar {

//! @addtogroup ar_port
//! @{

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

//! @brief Priorities for kernel exceptions.
enum _exception_priorities
{
    kSVCallPriority = 2,
    kPendSVPriority = 2,
    kSysTickPriority = 2
};

/*!
 * @brief Context for a thread saved on the stack.
 */
struct ThreadContext
{
    // Stacked manually by us
    uint32_t r4;    // Lowest address on stack
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    // Stacked automatically by Cortex-M hardware
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t xpsr;  // Highest address on stack
};

/*!
 *
 */
// template <bool E>
class IrqStateSetAndRestore
{
public:
    IrqStateSetAndRestore(bool doEnable)
    {
        m_savedPrimask = __get_PRIMASK();
        if (doEnable)
        {
            __enable_irq();
        }
        else
        {
            __disable_irq();
        }
    }
    
    ~IrqStateSetAndRestore()
    {
        __set_PRIMASK(m_savedPrimask);
    }

protected:
    uint32_t m_savedPrimask;
};

// typedef IrqStateSetAndRestore<true> IrqEnableAndRestore;
// typedef IrqStateSetAndRestore<false> IrqDisableAndRestore;

//! @brief Stop the CPU because of a serious error.
inline void _halt()
{
    asm volatile ("bkpt #0");
}

//! @brief Invoke the SVCall handler.
inline void service_call()
{
//     asm volatile ("svc #0");

    // Set PendSV pending.
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

//! @brief Set the PendSV IRQ pending.
inline void pending_service_call()
{
    // Set PendSV pending.
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

//! @}

} // namespace Ar

#endif // _AR_PORT_H_
//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
