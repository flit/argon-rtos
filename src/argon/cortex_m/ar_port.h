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
/*!
 * @file arport.h
 * @ingroup ar_port
 * @brief Header for the Argon RTOS.
 */

#if !defined(_AR_PORT_H_)
#define _AR_PORT_H_

#include "fsl_device_registers.h"
#include <stdbool.h>

//! @addtogroup ar_port
//! @{

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

/*!
 * @brief ARM Cortex-M specific thread struct fields.
 */
typedef struct _ar_thread_port_data {
#if __FPU_USED
    bool m_hasExtendedFrame;    //!< Whether the thread context has an extended stack frame with saved FP registers.
#endif // __FPU_USED
} ar_thread_port_data_t;

/*!
 * @brief Cortex-M specific kernel data.
 */
typedef struct _ar_kernel_port_data {
    bool hasExtendedFrame;  //!< Used solely to pass info back to asm PendSV handler code.
} ar_kernel_port_data_t;

enum
{
    kSchedulerQuanta_ms = 10
};

//! @}

#if defined(__cplusplus)

namespace Ar {

//! @addtogroup ar_port
//! @{

/*!
 * @brief Context for a thread saved on the stack.
 */
struct ThreadContext
{
    ///! @name Stacked manually by us
    //@{
    uint32_t r4;    //!< [SP+0] Lowest address on stack
    uint32_t r5;    //!< [SP+4]
    uint32_t r6;    //!< [SP+8]
    uint32_t r7;    //!< [SP+12]
    uint32_t r8;    //!< [SP+16]
    uint32_t r9;    //!< [SP+20]
    uint32_t r10;   //!< [SP+24]
    uint32_t r11;   //!< [SP+28]
    //@}
    //! @name Stacked automatically by Cortex-M hardware
    //@{
    uint32_t r0;    //!< [SP+32]
    uint32_t r1;    //!< [SP+36]
    uint32_t r2;    //!< [SP+40]
    uint32_t r3;    //!< [SP+44]
    uint32_t r12;   //!< [SP+48]
    uint32_t lr;    //!< [SP+52]
    uint32_t pc;    //!< [SP+56]
    uint32_t xpsr;  //!< [SP+60] Highest address on stack
    //@}
};

//! @brief Stop the CPU because of a serious error.
inline void _halt()
{
    asm volatile ("bkpt #0");
}

//! @}

} // namespace Ar

#endif // defined(__cplusplus)

//! @brief
bool ar_port_set_lock(bool lockIt);

#if DEBUG
//! @brief Make the PendSV exception pending.
void ar_port_service_call();
#else // DEBUG
inline void ar_port_service_call()
{
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    __DSB();
    __ISB();
}
#endif // DEBUG

//! @brief Returns true if in IRQ state.
static inline bool ar_port_get_irq_state(void)
{
    return __get_IPSR() != 0;
}

#if __cplusplus
extern "C" inline uint32_t ar_get_milliseconds_per_tick();
#else
static inline uint32_t ar_get_milliseconds_per_tick(void);
#endif

//! @brief Returns the number of milliseconds per tick.
inline uint32_t ar_get_milliseconds_per_tick()
{
    return kSchedulerQuanta_ms;
}

#endif // _AR_PORT_H_
//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
