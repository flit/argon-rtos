/*
 * Copyright (c) 2013, Freescale Semiconductor, Inc.
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

#ifndef __DEVICE_REGISTERS_H__
#define __DEVICE_REGISTERS_H__

#include <stdint.h>

/*
 * Include the cpu specific register header file.
 *
 * The CPU macro should be declared in the project or makefile.
 */
#if (defined(CPU_MK20DX128VMP5) || defined(CPU_MK20DN128VMP5) || defined(CPU_MK20DX64VMP5) || \
    defined(CPU_MK20DN64VMP5) || defined(CPU_MK20DX32VMP5) || defined(CPU_MK20DN32VMP5) || \
    defined(CPU_MK20DX128VLH5) || defined(CPU_MK20DN128VLH5) || defined(CPU_MK20DX64VLH5) || \
    defined(CPU_MK20DN64VLH5) || defined(CPU_MK20DX32VLH5) || defined(CPU_MK20DN32VLH5) || \
    defined(CPU_MK20DX128VFM5) || defined(CPU_MK20DN128VFM5) || defined(CPU_MK20DX64VFM5) || \
    defined(CPU_MK20DN64VFM5) || defined(CPU_MK20DX32VFM5) || defined(CPU_MK20DN32VFM5) || \
    defined(CPU_MK20DX128VFT5) || defined(CPU_MK20DN128VFT5) || defined(CPU_MK20DX64VFT5) || \
    defined(CPU_MK20DN64VFT5) || defined(CPU_MK20DX32VFT5) || defined(CPU_MK20DN32VFT5) || \
    defined(CPU_MK20DX128VLF5) || defined(CPU_MK20DN128VLF5) || defined(CPU_MK20DX64VLF5) || \
    defined(CPU_MK20DN64VLF5) || defined(CPU_MK20DX32VLF5) || defined(CPU_MK20DN32VLF5))

    #define K20D5_SERIES

    /* CMSIS-style register definitions */
    #include "device/MK20D5/MK20D5.h"
    /* Extension register definitions */
    #include "device/MK20D5/MK20D5_registers.h"
    /* CPU specific feature definitions */
    #include "device/MK20D5/MK20D5_features.h"

#elif (defined(CPU_MK22FN512CAP12) || defined(CPU_MK22FN512VDC12) || defined(CPU_MK22FN512VLH12) || \
    defined(CPU_MK22FN512VLL12) || defined(CPU_MK22FN512VMP12))

    #define K22F51212_SERIES

    /* CMSIS-style register definitions */
    #include "device/MK22F51212/MK22F51212.h"
    /* Extension register definitions */
    #include "device/MK22F51212/MK22F51212_registers.h"
    /* CPU specific feature definitions */
    #include "device/MK22F51212/MK22F51212_features.h"

#elif (defined(CPU_MK64FX512VDC12) || defined(CPU_MK64FN1M0VDC12) || defined(CPU_MK64FX512VLL12) || \
    defined(CPU_MK64FN1M0VLL12) || defined(CPU_MK64FX512VLQ12) || defined(CPU_MK64FN1M0VLQ12) || \
    defined(CPU_MK64FX512VMD12) || defined(CPU_MK64FN1M0VMD12))

    #define K64F12_SERIES

    /* CMSIS-style register definitions */
    #include "device/MK64F12/MK64F12.h"
    /* Extension register definitions */
    #include "device/MK64F12/MK64F12_registers.h"
    /* CPU specific feature definitions */
//     #include "device/MK64F12/MK64F12_features.h"

#elif (defined(CPU_MK60DN512ZVMD10))
    #define K60D10_SERIES
    #include "device/MK60DZ10/K60_uart.h"
    #include "device/MK60DZ10/K60_sim.h"
    #include "device/MK60DZ10/MK60DZ10.h"

#elif (defined(CPU_MKL25Z32VFM4) || defined(CPU_MKL25Z64VFM4) || defined(CPU_MKL25Z128VFM4) || \
    defined(CPU_MKL25Z32VFT4) || defined(CPU_MKL25Z64VFT4) || defined(CPU_MKL25Z128VFT4) || \
    defined(CPU_MKL25Z32VLH4) || defined(CPU_MKL25Z64VLH4) || defined(CPU_MKL25Z128VLH4) || \
    defined(CPU_MKL25Z32VLK4) || defined(CPU_MKL25Z64VLK4) || defined(CPU_MKL25Z128VLK4))

    #define KL25Z4_SERIES

    /* CMSIS-style register definitions */
    #include "device/MKL25Z4/MKL25Z4.h"
    /* Extension register definitions */
    #include "device/MKL25Z4/MKL25Z4_registers.h"
    /* CPU specific feature definitions */
    #include "device/MKL25Z4/MKL25Z4_features.h"

#elif (defined(CPU_MKL43Z128VLH4) || defined(CPU_MKL43Z256VLH4) || defined(CPU_MKL43Z128VMP4) || \
    defined(CPU_MKL43Z256VMP4))

    #define KL43Z4_SERIES

    /* CMSIS-style register definitions */
    #include "device/MKL43Z4/MKL43Z4.h"
    /* Extension register definitions */
    #include "device/MKL43Z4/MKL43Z4_registers.h"
    /* CPU specific feature definitions */
    #include "device/MKL43Z4/MKL43Z4_features.h"

#else
    #error "No valid CPU defined"
#endif



#endif // __DEVICE_REGISTERS_H__
////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////

