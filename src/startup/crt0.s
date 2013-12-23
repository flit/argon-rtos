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


;         AREA   Crt0, CODE, READONLY      ; name this block of code
        SECTION .noinit : CODE

        THUMB

        import SystemInit
        import init_data_bss
        import __iar_program_start
        import main
        import CSTACK$$Limit

        EXTERN __vector_table


        REQUIRE __vector_table

#define SCS_BASE            (0xE000E000)                            /*!< System Control Space Base Address */
#define SCB_BASE            (SCS_BASE +  0x0D00)                    /*!< System Control Block Base Address */
#define SCB_VTOR_OFFSET     (0x00000008)

        PUBLIC  Reset_Handler
        EXPORT  Reset_Handler
Reset_Handler

        // Set VTOR register in SCB first thing we do.
//         ldr     r0,=__vector_table
//         ldr     r1,=SCB_BASE
//         str     r0,[r1, #SCB_VTOR_OFFSET]
// 
//         // Init the rest of the registers
//         ldr     r2,=0
//         ldr     r3,=0
//         ldr     r4,=0
//         ldr     r5,=0
//         ldr     r6,=0
//         ldr     r7,=0
//         mov     r8,r7
//         mov     r9,r7
//         mov     r10,r7
//         mov     r11,r7
//         mov     r12,r7
// 
//         // Initialize the stack pointer
//         ldr     r0,=CSTACK$$Limit
//         mov     r13,r0

        // Unmask interrupts
//         cpsie   i

        // Call the CMSIS system init routine
//         ldr     r0,=SystemInit
//         blx     r0

        // Init .data and .bss sections
//         ldr     r0,=init_data_bss
//         ldr     r0,=init_data_bss
//         blx     r0
// 
//         // Set argc and argv to NULL before calling main().
//         ldr     r0,=0
//         ldr     r1,=0
//         ldr     r2,=main
//         blx     r2

        LDR     R0, =SystemInit
        BLX     R0
        LDR     R0, =__iar_program_start
        BX      R0

__done
        B       __done



        END

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
