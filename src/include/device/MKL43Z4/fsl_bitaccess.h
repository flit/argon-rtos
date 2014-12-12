/*
** ###################################################################
**     Version:             rev. 1.5, 2014-09-05
**     Build:               b140905
**
**     Abstract:
**         Register bit field access macros.
**
**     Copyright (c) 2014 Freescale Semiconductor, Inc.
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
**     - rev. 1.0 (2014-03-27)
**         Initial version.
**     - rev. 1.1 (2014-05-26)
**         I2S registers TCR2/RCR2 and others were changed.
**         FLEXIO register FLEXIO_VERID has now bitfields: FEATURE, MINOR, MAJOR.
**         Names of the bitfields of the FLEXIO_SHIFTBUF have been changed to the appropriate register name e.g.: FLEXIO_SHIFTBUFBBS_SHIFTBUFBBS.
**         Peripheral_BASES macros has been changed to Peripheral_BASE_PTRS, e.g.: ADC_BASES to ADC_BASE_PTRS.
**         Clock configuration for high range external oscillator has been added.
**         RFSYS module access has been added.
**     - rev. 1.2 (2014-07-10)
**         GPIO - Renamed modules PTA,PTB,PTC,PTD,PTE to GPIOA,GPIOB,GPIOC,GPIOD,GPIOE.
**         UART0 - UART0 module renamed to UART2.
**         I2S - removed MDR register.
**     - rev. 1.3 (2014-08-21)
**         UART2 - Removed ED register.
**         UART2 - Removed MODEM register.
**         UART2 - Removed IR register.
**         UART2 - Removed PFIFO register.
**         UART2 - Removed CFIFO register.
**         UART2 - Removed SFIFO register.
**         UART2 - Removed TWFIFO register.
**         UART2 - Removed TCFIFO register.
**         UART2 - Removed RWFIFO register.
**         UART2 - Removed RCFIFO register.
**         USB - Removed bitfield REG_EN in CLK_RECOVER_IRC_EN register.
**         SIM - Changed bitfield value MCGIRCLK to LIRC_CLK of bitfield CLKOUTSEL in SOPT2 register.
**         SIM - Removed bitfield DIEID in SDID register.
**     - rev. 1.4 (2014-09-01)
**         USB - USB0_CTL0 was renamed to USB0_OTGCTL register.
**         USB - USB0_CTL1 was renamed to USB0_CTL register.
**     - rev. 1.5 (2014-09-05)
**         USB - Renamed USBEN bitfield of USB0_CTL was renamed to USBENSOFEN.
**
** ###################################################################
*/

#ifndef _FSL_BITACCESS_H
#define _FSL_BITACCESS_H  1

#include <stdint.h>
#include <stdlib.h>

#define BME_AND_MASK  (1<<26)
#define BME_OR_MASK   (1<<27)
#define BME_XOR_MASK  (3<<26)
#define BME_BFI_MASK(BIT,WIDTH)   (1<<28) | (BIT<<23) | ((WIDTH-1)<<19)
#define BME_UBFX_MASK(BIT,WIDTH)  (1<<28) | (BIT<<23) | ((WIDTH-1)<<19)

/* Decorated Store: Logical AND */
#define BME_AND8(addr, wdata) (*(volatile uint8_t*)((uint32_t)addr | BME_AND_MASK) = wdata)
#define BME_AND16(addr, wdata) (*(volatile uint16_t*)((uint32_t)addr | BME_AND_MASK) = wdata)
#define BME_AND32(addr, wdata) (*(volatile uint32_t*)((uint32_t)addr | BME_AND_MASK) = wdata)

/* Decorated Store: Logical OR */
#define BME_OR8(addr, wdata) (*(volatile uint8_t*)((uint32_t)addr | BME_OR_MASK) = wdata)
#define BME_OR16(addr, wdata) (*(volatile uint16_t*)((uint32_t)addr | BME_OR_MASK) = wdata)
#define BME_OR32(addr, wdata) (*(volatile uint32_t*)((uint32_t)addr | BME_OR_MASK) = wdata)

/* Decorated Store: Logical XOR */
#define BME_XOR8(addr, wdata) (*(volatile uint8_t*)((uint32_t)addr | BME_XOR_MASK) = wdata)
#define BME_XOR16(addr, wdata) (*(volatile uint8_t*)((uint32_t)addr | BME_XOR_MASK) = wdata)
#define BME_XOR32(addr, wdata) (*(volatile uint8_t*)((uint32_t)addr | BME_XOR_MASK) = wdata)

/* Decorated Store: Bit Field Insert */
#define BME_BFI8(addr, wdata, bit, width) (*(volatile uint8_t*)((uint32_t)addr | BME_BFI_MASK(bit,width)) = wdata)
#define BME_BFI16(addr, wdata, bit, width) (*(volatile uint16_t*)((uint32_t)addr | BME_BFI_MASK(bit,width)) = wdata)
#define BME_BFI32(addr, wdata, bit, width) (*(volatile uint32_t*)((uint32_t)addr | BME_BFI_MASK(bit,width)) = wdata)

/* Decorated Load: Unsigned Bit Field Extract */
#define BME_UBFX8(addr, bit, width) (*(volatile uint8_t*)((uint32_t)addr | BME_UBFX_MASK(bit,width)))
#define BME_UBFX16(addr, bit, width) (*(volatile uint16_t*)((uint32_t)addr | BME_UBFX_MASK(bit,width)))
#define BME_UBFX32(addr, bit, width) (*(volatile uint32_t*)((uint32_t)addr | BME_UBFX_MASK(bit,width)))

/*
 * Macros for single instance registers
 */

#define BF_SET(reg, field)       HW_##reg##_SET(BM_##reg##_##field)
#define BF_CLR(reg, field)       HW_##reg##_CLR(BM_##reg##_##field)
#define BF_TOG(reg, field)       HW_##reg##_TOG(BM_##reg##_##field)

#define BF_SETV(reg, field, v)   HW_##reg##_SET(BF_##reg##_##field(v))
#define BF_CLRV(reg, field, v)   HW_##reg##_CLR(BF_##reg##_##field(v))
#define BF_TOGV(reg, field, v)   HW_##reg##_TOG(BF_##reg##_##field(v))

#define BV_FLD(reg, field, sym)  BF_##reg##_##field(BV_##reg##_##field##__##sym)
#define BV_VAL(reg, field, sym)  BV_##reg##_##field##__##sym

#define BF_RD(reg, field)        HW_##reg.B.field
#define BF_WR(reg, field, v)     BW_##reg##_##field(v)

#define BF_CS1(reg, f1, v1)  \
        (HW_##reg##_CLR(BM_##reg##_##f1),      \
         HW_##reg##_SET(BF_##reg##_##f1(v1)))

#define BF_CS2(reg, f1, v1, f2, v2)  \
        (HW_##reg##_CLR(BM_##reg##_##f1 |      \
                        BM_##reg##_##f2),      \
         HW_##reg##_SET(BF_##reg##_##f1(v1) |  \
                        BF_##reg##_##f2(v2)))

#define BF_CS3(reg, f1, v1, f2, v2, f3, v3)  \
        (HW_##reg##_CLR(BM_##reg##_##f1 |      \
                        BM_##reg##_##f2 |      \
                        BM_##reg##_##f3),      \
         HW_##reg##_SET(BF_##reg##_##f1(v1) |  \
                        BF_##reg##_##f2(v2) |  \
                        BF_##reg##_##f3(v3)))

#define BF_CS4(reg, f1, v1, f2, v2, f3, v3, f4, v4)  \
        (HW_##reg##_CLR(BM_##reg##_##f1 |      \
                        BM_##reg##_##f2 |      \
                        BM_##reg##_##f3 |      \
                        BM_##reg##_##f4),      \
         HW_##reg##_SET(BF_##reg##_##f1(v1) |  \
                        BF_##reg##_##f2(v2) |  \
                        BF_##reg##_##f3(v3) |  \
                        BF_##reg##_##f4(v4)))

#define BF_CS5(reg, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5)  \
        (HW_##reg##_CLR(BM_##reg##_##f1 |      \
                        BM_##reg##_##f2 |      \
                        BM_##reg##_##f3 |      \
                        BM_##reg##_##f4 |      \
                        BM_##reg##_##f5),      \
         HW_##reg##_SET(BF_##reg##_##f1(v1) |  \
                        BF_##reg##_##f2(v2) |  \
                        BF_##reg##_##f3(v3) |  \
                        BF_##reg##_##f4(v4) |  \
                        BF_##reg##_##f5(v5)))

#define BF_CS6(reg, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5, f6, v6)  \
        (HW_##reg##_CLR(BM_##reg##_##f1 |      \
                        BM_##reg##_##f2 |      \
                        BM_##reg##_##f3 |      \
                        BM_##reg##_##f4 |      \
                        BM_##reg##_##f5 |      \
                        BM_##reg##_##f6),      \
         HW_##reg##_SET(BF_##reg##_##f1(v1) |  \
                        BF_##reg##_##f2(v2) |  \
                        BF_##reg##_##f3(v3) |  \
                        BF_##reg##_##f4(v4) |  \
                        BF_##reg##_##f5(v5) |  \
                        BF_##reg##_##f6(v6)))

#define BF_CS7(reg, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5, f6, v6, f7, v7)  \
        (HW_##reg##_CLR(BM_##reg##_##f1 |      \
                        BM_##reg##_##f2 |      \
                        BM_##reg##_##f3 |      \
                        BM_##reg##_##f4 |      \
                        BM_##reg##_##f5 |      \
                        BM_##reg##_##f6 |      \
                        BM_##reg##_##f7),      \
         HW_##reg##_SET(BF_##reg##_##f1(v1) |  \
                        BF_##reg##_##f2(v2) |  \
                        BF_##reg##_##f3(v3) |  \
                        BF_##reg##_##f4(v4) |  \
                        BF_##reg##_##f5(v5) |  \
                        BF_##reg##_##f6(v6) |  \
                        BF_##reg##_##f7(v7)))

#define BF_CS8(reg, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5, f6, v6, f7, v7, f8, v8)  \
        (HW_##reg##_CLR(BM_##reg##_##f1 |      \
                        BM_##reg##_##f2 |      \
                        BM_##reg##_##f3 |      \
                        BM_##reg##_##f4 |      \
                        BM_##reg##_##f5 |      \
                        BM_##reg##_##f6 |      \
                        BM_##reg##_##f7 |      \
                        BM_##reg##_##f8),      \
         HW_##reg##_SET(BF_##reg##_##f1(v1) |  \
                        BF_##reg##_##f2(v2) |  \
                        BF_##reg##_##f3(v3) |  \
                        BF_##reg##_##f4(v4) |  \
                        BF_##reg##_##f5(v5) |  \
                        BF_##reg##_##f6(v6) |  \
                        BF_##reg##_##f7(v7) |  \
                        BF_##reg##_##f8(v8)))

/*
 * Macros for multiple instance registers
 */

#define BF_SETn(reg, n, field)       HW_##reg##_SET(n, BM_##reg##_##field)
#define BF_CLRn(reg, n, field)       HW_##reg##_CLR(n, BM_##reg##_##field)
#define BF_TOGn(reg, n, field)       HW_##reg##_TOG(n, BM_##reg##_##field)

#define BF_SETVn(reg, n, field, v)   HW_##reg##_SET(n, BF_##reg##_##field(v))
#define BF_CLRVn(reg, n, field, v)   HW_##reg##_CLR(n, BF_##reg##_##field(v))
#define BF_TOGVn(reg, n, field, v)   HW_##reg##_TOG(n, BF_##reg##_##field(v))

#define BV_FLDn(reg, n, field, sym)  BF_##reg##_##field(BV_##reg##_##field##__##sym)
#define BV_VALn(reg, n, field, sym)  BV_##reg##_##field##__##sym

#define BF_RDn(reg, n, field)        HW_##reg(n).B.field
#define BF_WRn(reg, n, field, v)     BW_##reg##_##field(n, v)

#define BF_CS1n(reg, n, f1, v1)  \
        (HW_##reg##_CLR(n, (BM_##reg##_##f1)),      \
         HW_##reg##_SET(n, (BF_##reg##_##f1(v1))))

#define BF_CS2n(reg, n, f1, v1, f2, v2)  \
        (HW_##reg##_CLR(n, (BM_##reg##_##f1 |       \
                            BM_##reg##_##f2)),      \
         HW_##reg##_SET(n, (BF_##reg##_##f1(v1) |   \
                            BF_##reg##_##f2(v2))))

#define BF_CS3n(reg, n, f1, v1, f2, v2, f3, v3)  \
        (HW_##reg##_CLR(n, (BM_##reg##_##f1 |       \
                            BM_##reg##_##f2 |       \
                            BM_##reg##_##f3)),      \
         HW_##reg##_SET(n, (BF_##reg##_##f1(v1) |   \
                            BF_##reg##_##f2(v2) |   \
                            BF_##reg##_##f3(v3))))

#define BF_CS4n(reg, n, f1, v1, f2, v2, f3, v3, f4, v4)  \
        (HW_##reg##_CLR(n, (BM_##reg##_##f1 |       \
                            BM_##reg##_##f2 |       \
                            BM_##reg##_##f3 |       \
                            BM_##reg##_##f4)),      \
         HW_##reg##_SET(n, (BF_##reg##_##f1(v1) |   \
                            BF_##reg##_##f2(v2) |   \
                            BF_##reg##_##f3(v3) |   \
                            BF_##reg##_##f4(v4))))

#define BF_CS5n(reg, n, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5)  \
        (HW_##reg##_CLR(n, (BM_##reg##_##f1 |       \
                            BM_##reg##_##f2 |       \
                            BM_##reg##_##f3 |       \
                            BM_##reg##_##f4 |       \
                            BM_##reg##_##f5)),      \
         HW_##reg##_SET(n, (BF_##reg##_##f1(v1) |   \
                            BF_##reg##_##f2(v2) |   \
                            BF_##reg##_##f3(v3) |   \
                            BF_##reg##_##f4(v4) |   \
                            BF_##reg##_##f5(v5))))

#define BF_CS6n(reg, n, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5, f6, v6)  \
        (HW_##reg##_CLR(n, (BM_##reg##_##f1 |       \
                            BM_##reg##_##f2 |       \
                            BM_##reg##_##f3 |       \
                            BM_##reg##_##f4 |       \
                            BM_##reg##_##f5 |       \
                            BM_##reg##_##f6)),      \
         HW_##reg##_SET(n, (BF_##reg##_##f1(v1) |   \
                            BF_##reg##_##f2(v2) |   \
                            BF_##reg##_##f3(v3) |   \
                            BF_##reg##_##f4(v4) |   \
                            BF_##reg##_##f5(v5) |   \
                            BF_##reg##_##f6(v6))))

#define BF_CS7n(reg, n, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5, f6, v6, f7, v7)  \
        (HW_##reg##_CLR(n, (BM_##reg##_##f1 |       \
                            BM_##reg##_##f2 |       \
                            BM_##reg##_##f3 |       \
                            BM_##reg##_##f4 |       \
                            BM_##reg##_##f5 |       \
                            BM_##reg##_##f6 |       \
                            BM_##reg##_##f7)),      \
         HW_##reg##_SET(n, (BF_##reg##_##f1(v1) |   \
                            BF_##reg##_##f2(v2) |   \
                            BF_##reg##_##f3(v3) |   \
                            BF_##reg##_##f4(v4) |   \
                            BF_##reg##_##f5(v5) |   \
                            BF_##reg##_##f6(v6) |   \
                            BF_##reg##_##f7(v7))))

#define BF_CS8n(reg, n, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5, f6, v6, f7, v7, f8, v8)  \
        (HW_##reg##_CLR(n, (BM_##reg##_##f1 |       \
                            BM_##reg##_##f2 |       \
                            BM_##reg##_##f3 |       \
                            BM_##reg##_##f4 |       \
                            BM_##reg##_##f5 |       \
                            BM_##reg##_##f6 |       \
                            BM_##reg##_##f7 |       \
                            BM_##reg##_##f8)),      \
         HW_##reg##_SET(n, (BF_##reg##_##f1(v1) |   \
                            BF_##reg##_##f2(v2) |   \
                            BF_##reg##_##f3(v3) |   \
                            BF_##reg##_##f4(v4) |   \
                            BF_##reg##_##f5(v5) |   \
                            BF_##reg##_##f6(v6) |   \
                            BF_##reg##_##f7(v7) |   \
                            BF_##reg##_##f8(v8))))

/*
 * Macros for single instance MULTI-BLOCK registers
 */

#define BFn_SET(reg, blk, field)       HW_##reg##_SET(blk, BM_##reg##_##field)
#define BFn_CLR(reg, blk, field)       HW_##reg##_CLR(blk, BM_##reg##_##field)
#define BFn_TOG(reg, blk, field)       HW_##reg##_TOG(blk, BM_##reg##_##field)

#define BFn_SETV(reg, blk, field, v)   HW_##reg##_SET(blk, BF_##reg##_##field(v))
#define BFn_CLRV(reg, blk, field, v)   HW_##reg##_CLR(blk, BF_##reg##_##field(v))
#define BFn_TOGV(reg, blk, field, v)   HW_##reg##_TOG(blk, BF_##reg##_##field(v))

#define BVn_FLD(reg, field, sym)  BF_##reg##_##field(BV_##reg##_##field##__##sym)
#define BVn_VAL(reg, field, sym)  BV_##reg##_##field##__##sym

#define BFn_RD(reg, blk, field)        HW_##reg(blk).B.field
#define BFn_WR(reg, blk, field, v)     BW_##reg##_##field(blk, v)

#define BFn_CS1(reg, blk, f1, v1)  \
        (HW_##reg##_CLR(blk, BM_##reg##_##f1),      \
         HW_##reg##_SET(blk, BF_##reg##_##f1(v1)))

#define BFn_CS2(reg, blk, f1, v1, f2, v2)  \
        (HW_##reg##_CLR(blk, BM_##reg##_##f1 |      \
                        BM_##reg##_##f2),      \
         HW_##reg##_SET(blk, BF_##reg##_##f1(v1) |  \
                        BF_##reg##_##f2(v2)))

#define BFn_CS3(reg, blk, f1, v1, f2, v2, f3, v3)  \
        (HW_##reg##_CLR(blk, BM_##reg##_##f1 |      \
                        BM_##reg##_##f2 |      \
                        BM_##reg##_##f3),      \
         HW_##reg##_SET(blk, BF_##reg##_##f1(v1) |  \
                        BF_##reg##_##f2(v2) |  \
                        BF_##reg##_##f3(v3)))

#define BFn_CS4(reg, blk, f1, v1, f2, v2, f3, v3, f4, v4)  \
        (HW_##reg##_CLR(blk, BM_##reg##_##f1 |      \
                        BM_##reg##_##f2 |      \
                        BM_##reg##_##f3 |      \
                        BM_##reg##_##f4),      \
         HW_##reg##_SET(blk, BF_##reg##_##f1(v1) |  \
                        BF_##reg##_##f2(v2) |  \
                        BF_##reg##_##f3(v3) |  \
                        BF_##reg##_##f4(v4)))

#define BFn_CS5(reg, blk, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5)  \
        (HW_##reg##_CLR(blk, BM_##reg##_##f1 |      \
                        BM_##reg##_##f2 |      \
                        BM_##reg##_##f3 |      \
                        BM_##reg##_##f4 |      \
                        BM_##reg##_##f5),      \
         HW_##reg##_SET(blk, BF_##reg##_##f1(v1) |  \
                        BF_##reg##_##f2(v2) |  \
                        BF_##reg##_##f3(v3) |  \
                        BF_##reg##_##f4(v4) |  \
                        BF_##reg##_##f5(v5)))

#define BFn_CS6(reg, blk, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5, f6, v6)  \
        (HW_##reg##_CLR(blk, BM_##reg##_##f1 |      \
                        BM_##reg##_##f2 |      \
                        BM_##reg##_##f3 |      \
                        BM_##reg##_##f4 |      \
                        BM_##reg##_##f5 |      \
                        BM_##reg##_##f6),      \
         HW_##reg##_SET(blk, BF_##reg##_##f1(v1) |  \
                        BF_##reg##_##f2(v2) |  \
                        BF_##reg##_##f3(v3) |  \
                        BF_##reg##_##f4(v4) |  \
                        BF_##reg##_##f5(v5) |  \
                        BF_##reg##_##f6(v6)))

#define BFn_CS7(reg, blk, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5, f6, v6, f7, v7)  \
        (HW_##reg##_CLR(blk, BM_##reg##_##f1 |      \
                        BM_##reg##_##f2 |      \
                        BM_##reg##_##f3 |      \
                        BM_##reg##_##f4 |      \
                        BM_##reg##_##f5 |      \
                        BM_##reg##_##f6 |      \
                        BM_##reg##_##f7),      \
         HW_##reg##_SET(blk, BF_##reg##_##f1(v1) |  \
                        BF_##reg##_##f2(v2) |  \
                        BF_##reg##_##f3(v3) |  \
                        BF_##reg##_##f4(v4) |  \
                        BF_##reg##_##f5(v5) |  \
                        BF_##reg##_##f6(v6) |  \
                        BF_##reg##_##f7(v7)))

#define BFn_CS8(reg, blk, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5, f6, v6, f7, v7, f8, v8)  \
        (HW_##reg##_CLR(blk, BM_##reg##_##f1 |      \
                        BM_##reg##_##f2 |      \
                        BM_##reg##_##f3 |      \
                        BM_##reg##_##f4 |      \
                        BM_##reg##_##f5 |      \
                        BM_##reg##_##f6 |      \
                        BM_##reg##_##f7 |      \
                        BM_##reg##_##f8),      \
         HW_##reg##_SET(blk, BF_##reg##_##f1(v1) |  \
                        BF_##reg##_##f2(v2) |  \
                        BF_##reg##_##f3(v3) |  \
                        BF_##reg##_##f4(v4) |  \
                        BF_##reg##_##f5(v5) |  \
                        BF_##reg##_##f6(v6) |  \
                        BF_##reg##_##f7(v7) |  \
                        BF_##reg##_##f8(v8)))

/*
 * Macros for MULTI-BLOCK multiple instance registers
 */

#define BFn_SETn(reg, blk, n, field)       HW_##reg##_SET(blk, n, BM_##reg##_##field)
#define BFn_CLRn(reg, blk, n, field)       HW_##reg##_CLR(blk, n, BM_##reg##_##field)
#define BFn_TOGn(reg, blk, n, field)       HW_##reg##_TOG(blk, n, BM_##reg##_##field)

#define BFn_SETVn(reg, blk, n, field, v)   HW_##reg##_SET(blk, n, BF_##reg##_##field(v))
#define BFn_CLRVn(reg, blk, n, field, v)   HW_##reg##_CLR(blk, n, BF_##reg##_##field(v))
#define BFn_TOGVn(reg, blk, n, field, v)   HW_##reg##_TOG(blk, n, BF_##reg##_##field(v))

#define BVn_FLDn(reg, blk, n, field, sym)  BF_##reg##_##field(BV_##reg##_##field##__##sym)
#define BVn_VALn(reg, blk, n, field, sym)  BV_##reg##_##field##__##sym

#define BFn_RDn(reg, blk, n, field)        HW_##reg(n).B.field
#define BFn_WRn(reg, blk, n, field, v)     BW_##reg##_##field(n, v)

#define BFn_CS1n(reg, blk, n, f1, v1)  \
        (HW_##reg##_CLR(blk, n, (BM_##reg##_##f1)),      \
         HW_##reg##_SET(blk, n, (BF_##reg##_##f1(v1))))

#define BFn_CS2n(reg, blk, n, f1, v1, f2, v2)  \
        (HW_##reg##_CLR(blk, n, (BM_##reg##_##f1 |       \
                            BM_##reg##_##f2)),      \
         HW_##reg##_SET(blk, n, (BF_##reg##_##f1(v1) |   \
                            BF_##reg##_##f2(v2))))

#define BFn_CS3n(reg, blk, n, f1, v1, f2, v2, f3, v3)  \
        (HW_##reg##_CLR(blk, n, (BM_##reg##_##f1 |       \
                            BM_##reg##_##f2 |       \
                            BM_##reg##_##f3)),      \
         HW_##reg##_SET(blk, n, (BF_##reg##_##f1(v1) |   \
                            BF_##reg##_##f2(v2) |   \
                            BF_##reg##_##f3(v3))))

#define BFn_CS4n(reg, blk, n, f1, v1, f2, v2, f3, v3, f4, v4)  \
        (HW_##reg##_CLR(blk, n, (BM_##reg##_##f1 |       \
                            BM_##reg##_##f2 |       \
                            BM_##reg##_##f3 |       \
                            BM_##reg##_##f4)),      \
         HW_##reg##_SET(blk, n, (BF_##reg##_##f1(v1) |   \
                            BF_##reg##_##f2(v2) |   \
                            BF_##reg##_##f3(v3) |   \
                            BF_##reg##_##f4(v4))))

#define BFn_CS5n(reg, blk, n, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5)  \
        (HW_##reg##_CLR(blk, n, (BM_##reg##_##f1 |       \
                            BM_##reg##_##f2 |       \
                            BM_##reg##_##f3 |       \
                            BM_##reg##_##f4 |       \
                            BM_##reg##_##f5)),      \
         HW_##reg##_SET(blk, n, (BF_##reg##_##f1(v1) |   \
                            BF_##reg##_##f2(v2) |   \
                            BF_##reg##_##f3(v3) |   \
                            BF_##reg##_##f4(v4) |   \
                            BF_##reg##_##f5(v5))))

#define BFn_CS6n(reg, blk, n, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5, f6, v6)  \
        (HW_##reg##_CLR(blk, n, (BM_##reg##_##f1 |       \
                            BM_##reg##_##f2 |       \
                            BM_##reg##_##f3 |       \
                            BM_##reg##_##f4 |       \
                            BM_##reg##_##f5 |       \
                            BM_##reg##_##f6)),      \
         HW_##reg##_SET(blk, n, (BF_##reg##_##f1(v1) |   \
                            BF_##reg##_##f2(v2) |   \
                            BF_##reg##_##f3(v3) |   \
                            BF_##reg##_##f4(v4) |   \
                            BF_##reg##_##f5(v5) |   \
                            BF_##reg##_##f6(v6))))

#define BFn_CS7n(reg, blk, n, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5, f6, v6, f7, v7)  \
        (HW_##reg##_CLR(blk, n, (BM_##reg##_##f1 |       \
                            BM_##reg##_##f2 |       \
                            BM_##reg##_##f3 |       \
                            BM_##reg##_##f4 |       \
                            BM_##reg##_##f5 |       \
                            BM_##reg##_##f6 |       \
                            BM_##reg##_##f7)),      \
         HW_##reg##_SET(blk, n, (BF_##reg##_##f1(v1) |   \
                            BF_##reg##_##f2(v2) |   \
                            BF_##reg##_##f3(v3) |   \
                            BF_##reg##_##f4(v4) |   \
                            BF_##reg##_##f5(v5) |   \
                            BF_##reg##_##f6(v6) |   \
                            BF_##reg##_##f7(v7))))

#define BFn_CS8n(reg, blk, n, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5, f6, v6, f7, v7, f8, v8)  \
        (HW_##reg##_CLR(blk, n, (BM_##reg##_##f1 |       \
                            BM_##reg##_##f2 |       \
                            BM_##reg##_##f3 |       \
                            BM_##reg##_##f4 |       \
                            BM_##reg##_##f5 |       \
                            BM_##reg##_##f6 |       \
                            BM_##reg##_##f7 |       \
                            BM_##reg##_##f8)),      \
         HW_##reg##_SET(blk, n, (BF_##reg##_##f1(v1) |   \
                            BF_##reg##_##f2(v2) |   \
                            BF_##reg##_##f3(v3) |   \
                            BF_##reg##_##f4(v4) |   \
                            BF_##reg##_##f5(v5) |   \
                            BF_##reg##_##f6(v6) |   \
                            BF_##reg##_##f7(v7) |   \
                            BF_##reg##_##f8(v8))))

#endif /* _FSL_BITACCESS_H */

/******************************************************************************/
