/*
** ###################################################################
**     Processors:          MKL28T512VDC7_CORE1
**                          MKL28T512VLH7_CORE1
**                          MKL28T512VLL7_CORE1
**                          MKL28T512VMP7_CORE1
**
**     Compilers:           Keil ARM C/C++ Compiler
**                          Freescale C/C++ for Embedded ARM
**                          GNU C Compiler
**                          IAR ANSI C/C++ Compiler for ARM
**
**     Reference manual:    MKL28TRM, Rev. 0, Sept 30, 2015
**     Version:             rev. 1.7, 2015-05-16
**     Build:               b151010
**
**     Abstract:
**         Provides a system configuration function and a global variable that
**         contains the system frequency. It configures the device and initializes
**         the oscillator (PLL) that is part of the microcontroller device.
**
**     Copyright (c) 2015 Freescale Semiconductor, Inc.
**     All rights reserved.
**
**     Redistribution and use in source and binary forms, with or without modification,
**     are permitted provided that the following conditions are met:
**
**     o Redistributions of source code must retain the above copyright notice, this list
**       of conditions and the following disclaimer.
**
**     o Redistributions in binary form must reproduce the above copyright notice, this
**       list of conditions and the following disclaimer in the documentation and/or
**       other materials provided with the distribution.
**
**     o Neither the name of Freescale Semiconductor, Inc. nor the names of its
**       contributors may be used to endorse or promote products derived from this
**       software without specific prior written permission.
**
**     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
**     ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
**     WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
**     DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
**     ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
**     (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
**     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
**     ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
**     (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
**     SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
**     http:                 www.freescale.com
**     mail:                 support@freescale.com
**
**     Revisions:
**     - rev. 1.0 (2015-03-25)
**         Initial version for dual core.
**     - rev. 1.1 (2015-04-08)
**         Group channel registers for LPIT
**     - rev. 1.2 (2015-04-15)
**         Group channel registers for INTMUX
**     - rev. 1.3 (2015-04-23)
**         Correct memory map for different cores
**     - rev. 1.4 (2015-05-06)
**         Correct FOPT reset value
**         Correct vector table size
**         Correct ram and flash address for second core
**         Remove parts with 256K flash size
**     - rev. 1.5 (2015-05-08)
**         Reduce register for XRDC
**     - rev. 1.6 (2015-05-12)
**         Add CMP
**         DMAMUX channel count to 8
**         Add PIDR for GPIO/FGPIO
**         Rename PIT to LPIT in PCC
**         SCG, USB register update
**         SIM, TRGMUX1, TRNG, TSTMR0/1 base address update
**         Add KEY related macros for WDOG
**     - rev. 1.7 (2015-05-16)
**         Add IRQS
**
** ###################################################################
*/

/*!
 * @file MKL28T7_CORE1
 * @version 1.7
 * @date 2015-05-16
 * @brief Device specific configuration file for MKL28T7_CORE1 (implementation
 *        file)
 *
 * Provides a system configuration function and a global variable that contains
 * the system frequency. It configures the device and initializes the oscillator
 * (PLL) that is part of the microcontroller device.
 */

#include <stdint.h>
#include "fsl_device_registers.h"



/* ----------------------------------------------------------------------------
   -- Core clock
   ---------------------------------------------------------------------------- */

uint32_t SystemCoreClock = DEFAULT_SYSTEM_CLOCK;

/* ----------------------------------------------------------------------------
   -- SystemInit()
   ---------------------------------------------------------------------------- */

void SystemInit (void) {

#if (DISABLE_WDOG)
  WDOG1->CNT = WDOG_UPDATE_KEY;
  WDOG1->TOVAL = 0xFFFF;
  WDOG1->CS = (uint32_t) ((WDOG1->CS) & ~WDOG_CS_EN_MASK) | WDOG_CS_UPDATE_MASK;
#endif /* (DISABLE_WDOG) */

}

/* ----------------------------------------------------------------------------
   -- SystemCoreClockUpdate()
   ---------------------------------------------------------------------------- */

void SystemCoreClockUpdate (void) {
return;
}
