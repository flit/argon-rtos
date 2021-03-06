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
#if (defined(CPU_MKL28T512VDC7_CORE0) || defined(CPU_MKL28T512VLH7_CORE0) || defined(CPU_MKL28T512VLL7_CORE0) || \
    defined(CPU_MKL28T512VMP7_CORE0))

    #define KL28T7_CORE0_SERIES

    #define LPI2C_INSTANCE_COUNT (3)
    #define LPI2C0_IDX (0)
    #define LPI2C1_IDX (1)
    #define LPI2C2_IDX (2)

    /* CMSIS-style register definitions */
    #include "MKL28T7/include/MKL28T7_CORE0.h"
    /* Extension register definitions */
//     #include "MKL28T7/include/MKL28T7_CORE0_extension.h"
    /* CPU specific feature definitions */
    #include "MKL28T7/include/MKL28T7_CORE0_features.h"

#elif (defined(CPU_MKL28T512VDC7_CORE1) || defined(CPU_MKL28T512VLH7_CORE1) || defined(CPU_MKL28T512VLL7_CORE1) || \
    defined(CPU_MKL28T512VMP7_CORE1))

    #define KL28T7_CORE1_SERIES

    #define LPI2C_INSTANCE_COUNT (3)
    #define LPI2C0_IDX (0)
    #define LPI2C1_IDX (1)
    #define LPI2C2_IDX (2)

    /* CMSIS-style register definitions */
    #include "MKL28T7/include/MKL28T7_CORE1.h"
    /* Extension register definitions */
//     #include "MKL28T7/include/MKL28T7_CORE1_extension.h"
    /* CPU specific feature definitions */
    #include "MKL28T7/include/MKL28T7_CORE1_features.h"

#elif (defined(CPU_MKL28Z512VDC7) || defined(CPU_MKL28Z512VLH7) || defined(CPU_MKL28Z512VLL7) || \
    defined(CPU_MKL28Z512VMP7))

    #define KL28Z7_SERIES

    #define LPI2C_INSTANCE_COUNT (3)
    #define LPI2C0_IDX (0)
    #define LPI2C1_IDX (1)
    #define LPI2C2_IDX (2)

    /* CMSIS-style register definitions */
    #include "MKL28Z7/include/MKL28Z7.h"
    /* Extension register definitions */
//     #include "MKL28Z7/include/MKL28Z7_extension.h"
    /* CPU specific feature definitions */
    #include "MKL28Z7/include/MKL28Z7_features.h"

#else
    #error "No valid CPU defined"
#endif



#endif // __DEVICE_REGISTERS_H__
////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////

