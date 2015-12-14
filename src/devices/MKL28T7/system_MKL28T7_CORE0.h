/*
** ###################################################################
**     Processors:          MKL28T512VDC7_CORE0
**                          MKL28T512VLH7_CORE0
**                          MKL28T512VLL7_CORE0
**                          MKL28T512VMP7_CORE0
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
 * @file MKL28T7_CORE0
 * @version 1.7
 * @date 2015-05-16
 * @brief Device specific configuration file for MKL28T7_CORE0 (header file)
 *
 * Provides a system configuration function and a global variable that contains
 * the system frequency. It configures the device and initializes the oscillator
 * (PLL) that is part of the microcontroller device.
 */

#ifndef _SYSTEM_MKL28T7_CORE0_H_
#define _SYSTEM_MKL28T7_CORE0_H_                 /**< Symbol preventing repeated inclusion */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>


#ifndef DISABLE_WDOG
  #define DISABLE_WDOG  1
#endif

/* SCG mode constants */

#define SCG_MODE_FIRC       0U
#define SCG_MODE_SIRC       1U
#define SCG_MODE_SOSC       2U
#define SCG_MODE_SPLL       3U

/* Predefined clock setups
   0 ... System Clock Generator (SCG) in Fast Internal Reference Clock (FIRC) mode
         Default part configuration.
         Core clock/Bus clock derived from the internal clock source 48 MHz
         Core clock = 48MHz, BusClock = 24MHz
   1 ... System Clock Generator (SCG) in Slow Internal Reference Clock (SIRC) mode
         Maximum achievable clock frequency configuration using internal clock.
         Core clock/Bus clock derived from the internal clock source 8MHz
         Core clock = 8MHz, BusClock = 4MHz
   2 ... System Clock Generator (SCG) in System Oscillator Clock (SOSC) mode
         Core clock/Bus clock derived from the internal clock source 2MHz
         The clock settings is ready for Very Low Power Run mode.
         Core clock = 2MHz, BusClock = 1MHz
   3 ... System Clock Generator (SCG) in System Phase-lock Loop (SPLL) mode
         Core clock/Bus clock derived from the internal clock source 48 MHz
         The clock settings is ready for Very Low Power Run mode.
         Core clock = 72MHz, BusClock = 24MHz
*/

/* Define clock source values */
#define CPU_XTAL_CLK_HZ                8000000u

/* Low power mode enable */
/* SMC_PMPROT: AVLP=1,AVLLS=1 */
#define SYSTEM_SMC_PMPROT_VALUE        0x2Au               /* SMC_PMPROT */
#define SYSTEM_SMC_PMCTRL_VALUE        0x0u                /* SMC_PMCTRL */

#ifdef CLOCK_SETUP
#if (CLOCK_SETUP == 0)
  #define DEFAULT_SYSTEM_CLOCK         48000000u          /* Default System clock value */
  #define CPU_INT_SLOW_CLK_HZ          24000000u
  #define SCG_MODE                     SCG_MODE_FIRC      /* Clock generator mode */
  /* SCG_RCCR: SCS=3, DIVCORE=0, DIVSLOW=1*/
  #define SCG_RCCR_VALUE               0x3000001u         /* SCG_RCCR */
  /* SCG_VCCR: SCS=2, DIVCORE=0, DIVSLOW=1*/
  #define SCG_VCCR_VALUE               0x2000001u         /* SCG_VCCR */
  /* SCG_HCCR: SCS=3, DIVCORE=0, DIVSLOW=1*/
  #define SCG_HCCR_VALUE               0x3000001u         /* SCG_VCCR */
  /* SCG_FIRCCSR: FIRCEN=1 */
  #define SCG_FIRCCSR_VALUE            0x1u               /* SCG_FIRCCSR */
  /* SCG_FIRCDIV: FIRCDIV3=1, FIRCDIV2=1, FIRCDIV1=1 */
  #define SCG_FIRCDIV_VALUE            0x10101u               /* SCG_FIRCDIV */
  /* SCG_FIRCCFG: RANGE=0 */
  #define SCG_FIRCCFG_VALUE            0x0u               /* SCG_FIRCCFG */
  /* SCG_FIRCTCFG: TRIMDIV=0 TRIMSRC=0 */
  #define SCG_FIRCTCFG_VALUE           0x0u               /* SCG_FIRCTCFG */

  /* SCG_SIRCCSR: SIRCEN=1 */
  #define SCG_SIRCCSR_VALUE            0x1u               /* SCG_SIRCCSR */
  /* SCG_SIRCDIV: SIRCDIV3=1, SIRCDIV2=1, SIRCDIV1=1 */
  #define SCG_SIRCDIV_VALUE            0x10101u           /* SCG_SIRCDIV */
  /* SCG_SIRCCFG: RANGE=0 */
  #define SCG_SIRCCFG_VALUE            0x0u               /* SCG_SIRCCFG */

  /* SCG_SOSCCSR: SOSCLPEN=1, SOSCEN=1 */
  #define SCG_SOSCCSR_VALUE            0x5u               /* SCG_SOSCCSR */
  /* SCG_SOSCDIV: SOSCDIV3=1, SOSCDIV2=1, SOSCDIV1=1 */
  #define SCG_SOSCDIV_VALUE            0x10101u           /* SCG_SOSCDIV */
  /* SCG_SOSCCFG: RANGE=2, EREFS=1 */
  #define SCG_SOSCCFG_VALUE            0x24u               /* SCG_SOSCCFG */

  /* SCG_SPLLCSR: SPLLEN=1 */
  #define SCG_SPLLCSR_VALUE            0x1u               /* SCG_SPLLCSR */
  /* SCG_SPLLDIV: SPLLDIV3=1, SPLLDIV2=1, SPLLDIV1=1 */
  #define SCG_SPLLDIV_VALUE            0x10101u               /* SCG_SPLLDIV */
  /* SCG_SPLLCFG: MULT=2, PREDIV=3, SOURCE=0 */
  #define SCG_SPLLCFG_VALUE            0x80501u           /* SCG_SPLLCFG */

#elif (CLOCK_SETUP == 1)
  #define DEFAULT_SYSTEM_CLOCK         8000000u           /* Default System clock value */
  #define CPU_INT_SLOW_CLK_HZ          4000000u
  #define SCG_MODE                     SCG_MODE_SIRC      /* Clock generator mode */
  /* SCG_RCCR: SCS=2, DIVCORE=0, DIVSLOW=1*/
  #define SCG_RCCR_VALUE               0x2000001u         /* SCG_RCCR */
  /* SCG_VCCR: SCS=2, DIVCORE=0, DIVSLOW=1*/
  #define SCG_VCCR_VALUE               0x2000001u         /* SCG_VCCR */
  /* SCG_HCCR: SCS=2, DIVCORE=0, DIVSLOW=1*/
  #define SCG_HCCR_VALUE               0x2000001u         /* SCG_VCCR */
  /* SCG_SIRCCSR: SIRCLPEN=1, SIRCEN=1 */
  #define SCG_SIRCCSR_VALUE            0x5u               /* SCG_SIRCCSR */
  /* SCG_SIRCDIV: SIRCDIV3=1, SIRCDIV2=1, SIRCDIV1=1 */
  #define SCG_SIRCDIV_VALUE            0x10101u               /* SCG_SIRCDIV */
  /* SCG_SIRCCFG: RANGE=1 */
  #define SCG_SIRCCFG_VALUE            0x1u               /* SCG_SIRCCFG */

  /* SCG_FIRCCSR: FIRCEN=1 */
  #define SCG_FIRCCSR_VALUE            0x1u               /* SCG_FIRCCSR */
  /* SCG_FIRCDIV: FIRCDIV3=1, FIRCDIV2=1, FIRCDIV1=1 */
  #define SCG_FIRCDIV_VALUE            0x10101u               /* SCG_FIRCDIV */
  /* SCG_FIRCCFG: RANGE=0 */
  #define SCG_FIRCCFG_VALUE            0x0u               /* SCG_FIRCCFG */
  /* SCG_FIRCTCFG: TRIMDIV=0 TRIMSRC=0 */
  #define SCG_FIRCTCFG_VALUE           0x0u               /* SCG_FIRCTCFG */

  /* SCG_SOSCCSR: SOSCEN=1 */
  #define SCG_SOSCCSR_VALUE            0x1u               /* SCG_SOSCCSR */
  /* SCG_SOSCDIV: SOSCDIV3=1, SOSCDIV2=1, SOSCDIV1=1 */
  #define SCG_SOSCDIV_VALUE            0x10101u           /* SCG_SOSCDIV */
  /* SCG_SOSCCFG: RANGE=2, EREFS=1 */
  #define SCG_SOSCCFG_VALUE            0x24u               /* SCG_SOSCCFG */

  /* SCG_SPLLCSR: SPLLEN=1 */
  #define SCG_SPLLCSR_VALUE            0x1u               /* SCG_SPLLCSR */
  /* SCG_SPLLDIV: SPLLDIV3=1, SPLLDIV2=1, SPLLDIV1=1 */
  #define SCG_SPLLDIV_VALUE            0x10101u               /* SCG_SPLLDIV */
  /* SCG_SPLLCFG: MULT=2, PREDIV=3, SOURCE=0 */
  #define SCG_SPLLCFG_VALUE            0x80501u           /* SCG_SPLLCFG */

#elif (CLOCK_SETUP == 2)
  #define DEFAULT_SYSTEM_CLOCK         8000000u           /* Default System clock value */
  #define CPU_INT_SLOW_CLK_HZ          4000000u
  #define SCG_MODE                     SCG_MODE_SOSC      /* Clock generator mode */
  /* SCG_RCCR: SCS=2, DIVCORE=0, DIVSLOW=1*/
  #define SCG_RCCR_VALUE               0x1000001u         /* SCG_RCCR */
  /* SCG_VCCR: SCS=2, DIVCORE=0, DIVSLOW=1*/
  #define SCG_VCCR_VALUE               0x1000001u         /* SCG_VCCR */
  /* SCG_HCCR: SCS=2, DIVCORE=0, DIVSLOW=1*/
  #define SCG_HCCR_VALUE               0x1000001u         /* SCG_VCCR */
  /* SCG_SOSCCSR: SOSCLPEN=1, SOSCEN=1 */
  #define SCG_SOSCCSR_VALUE            0x5u               /* SCG_SOSCCSR */
  /* SCG_SOSCDIV: SOSCDIV3=1, SOSCDIV2=1, SOSCDIV1=1 */
  #define SCG_SOSCDIV_VALUE            0x10101u               /* SCG_SOSCDIV */
  /* SCG_SOSCCFG: RANGE=2, EREFS=1 */
  #define SCG_SOSCCFG_VALUE            0x24u               /* SCG_SOSCCFG */

  /* SCG_SIRCCSR: SIRCEN=1 */
  #define SCG_SIRCCSR_VALUE            0x1u               /* SCG_SIRCCSR */
  /* SCG_SIRCDIV: SIRCDIV3=1, SIRCDIV2=1, SIRCDIV1=1 */
  #define SCG_SIRCDIV_VALUE            0x10101u               /* SCG_SIRCDIV */
  /* SCG_SIRCCFG: RANGE=1 */
  #define SCG_SIRCCFG_VALUE            0x1u               /* SCG_SIRCCFG */

  /* SCG_FIRCCSR: FIRCEN=1 */
  #define SCG_FIRCCSR_VALUE            0x1u               /* SCG_FIRCCSR */
  /* SCG_FIRCDIV: FIRCDIV3=1, FIRCDIV2=1, FIRCDIV1=1 */
  #define SCG_FIRCDIV_VALUE            0x10101u               /* SCG_FIRCDIV */
  /* SCG_FIRCCFG: RANGE=0 */
  #define SCG_FIRCCFG_VALUE            0x0u               /* SCG_FIRCCFG */
  /* SCG_FIRCTCFG: TRIMDIV=0 TRIMSRC=0 */
  #define SCG_FIRCTCFG_VALUE           0x0u               /* SCG_FIRCTCFG */

  /* SCG_SPLLCSR: SPLLEN=1 */
  #define SCG_SPLLCSR_VALUE            0x1u               /* SCG_SPLLCSR */
  /* SCG_SPLLDIV: SPLLDIV3=1, SPLLDIV2=1, SPLLDIV1=1 */
  #define SCG_SPLLDIV_VALUE            0x10101u               /* SCG_SPLLDIV */
  /* SCG_SPLLCFG: MULT=2, PREDIV=3, SOURCE=0 */
  #define SCG_SPLLCFG_VALUE            0x80501u           /* SCG_SPLLCFG */

#elif (CLOCK_SETUP == 3)
  #define DEFAULT_SYSTEM_CLOCK         72000000u          /* Default System clock value */
  #define CPU_INT_SLOW_CLK_HZ          24000000u
  #define SCG_MODE                     SCG_MODE_SPLL      /* Clock generator mode */
  /* SCG_RCCR: SCS=6, DIVCORE=0, DIVSLOW=2*/
  #define SCG_RCCR_VALUE               0x6000002u         /* SCG_RCCR */
  /* SCG_VCCR: SCS=2, DIVCORE=0, DIVSLOW=2*/
  #define SCG_VCCR_VALUE               0x2000002u         /* SCG_VCCR */
  /* SCG_HCCR: SCS=6, DIVCORE=0, DIVSLOW=2*/
  #define SCG_HCCR_VALUE               0x6000002u         /* SCG_VCCR */

  /* SCG_SPLLCSR: SPLLEN=1 */
  #define SCG_SPLLCSR_VALUE            0x1u               /* SCG_SPLLCSR */
  /* SCG_SPLLDIV: SPLLDIV3=1, SPLLDIV2=1, SPLLDIV1=1 */
  #define SCG_SPLLDIV_VALUE            0x10101u               /* SCG_SPLLDIV */
  /* SCG_SPLLCFG: MULT=2, PREDIV=3, SOURCE=0 */
  #define SCG_SPLLCFG_VALUE            0x80501u           /* SCG_SPLLCFG */

  /* SCG_SOSCCSR: SOSCLPEN=1, SOSCEN=1 */
  #define SCG_SOSCCSR_VALUE            0x5u               /* SCG_SOSCCSR */
  /* SCG_SOSCDIV: SOSCDIV3=1, SOSCDIV2=1, SOSCDIV1=1 */
  #define SCG_SOSCDIV_VALUE            0x10101u               /* SCG_SOSCDIV */
  /* SCG_SOSCCFG: RANGE=2, EREFS=1 */
  #define SCG_SOSCCFG_VALUE            0x24u               /* SCG_SOSCCFG */

  /* SCG_SIRCCSR: SIRCEN=1 */
  #define SCG_SIRCCSR_VALUE            0x1u               /* SCG_SIRCCSR */
  /* SCG_SIRCDIV: SIRCDIV3=1, SIRCDIV2=1, SIRCDIV1=1 */
  #define SCG_SIRCDIV_VALUE            0x10101u               /* SCG_SIRCDIV */
  /* SCG_SIRCCFG: RANGE=1 */
  #define SCG_SIRCCFG_VALUE            0x1u               /* SCG_SIRCCFG */

  /* SCG_FIRCCSR: FIRCEN=1 */
  #define SCG_FIRCCSR_VALUE            0x1u               /* SCG_FIRCCSR */
  /* SCG_FIRCDIV: FIRCDIV3=1, FIRCDIV2=1, FIRCDIV1=1 */
  #define SCG_FIRCDIV_VALUE            0x10101u               /* SCG_FIRCDIV */
  /* SCG_FIRCCFG: RANGE=0 */
  #define SCG_FIRCCFG_VALUE            0x0u               /* SCG_FIRCCFG */
  /* SCG_FIRCTCFG: TRIMDIV=0 TRIMSRC=0 */
  #define SCG_FIRCTCFG_VALUE           0x0u               /* SCG_FIRCTCFG */

#else
  #error The selected clock setup is not supported.
#endif /* (CLOCK_SETUP == 3) */
#else
  /* TODO */
  #define DEFAULT_SYSTEM_CLOCK         8000000u            /* Default System clock value */
  #define CPU_INT_SLOW_CLK_HZ          4000000u            /* Value of the slow internal oscillator clock frequency in Hz  */
#endif


/**
 * @brief System clock frequency (core clock)
 *
 * The system clock frequency supplied to the SysTick timer and the processor
 * core clock. This variable can be used by the user application to setup the
 * SysTick timer or configure other parameters. It may also be used by debugger to
 * query the frequency of the debug timer or configure the trace clock speed
 * SystemCoreClock is initialized with a correct predefined value.
 */
extern uint32_t SystemCoreClock;

/**
 * @brief Setup the microcontroller system.
 *
 * Typically this function configures the oscillator (PLL) that is part of the
 * microcontroller device. For systems with variable clock speed it also updates
 * the variable SystemCoreClock. SystemInit is called from startup_device file.
 */
void SystemInit (void);

/**
 * @brief Updates the SystemCoreClock variable.
 *
 * It must be called whenever the core clock is changed during program
 * execution. SystemCoreClockUpdate() evaluates the clock register settings and calculates
 * the current core clock.
 */
void SystemCoreClockUpdate (void);

#ifdef __cplusplus
}
#endif

#endif  /* _SYSTEM_MKL28T7_CORE0_H_ */
