/*
 * Copyright (c) 2013-2018 Immo Software
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

#include "ar_asm_macros.h"

// EXC_RETURN value to return to Thread mode, while restoring state from PSP.
_EQU(EXC_RETURN, 0xfffffffd)

// Return to Thread mode, use PSP, unstack extended FP frame.
_EQU(FP_EXC_RETURN, 0xffffffed)

// Bit 4 of LR indicates whether the extended stack frame was used.
_EQU(EXTENDED_FRAME, 0x10)

        /* specify the section where this code belongs */
        _CODE_SECTION(.text)
        _THUMB

        _IMPORT(ar_port_yield_isr)
        _IMPORT(g_ar_hasExtendedFrame)

        _EXPORT(SVC_Handler)
        _EXPORT(PendSV_Handler)

        _FN_BEGIN(PendSV_Handler)
        _FN_DECL(PendSV_Handler)
_FN_LABEL(PendSV_Handler)
        _FN_BEGIN_POST
        _FN_CANT_UNWIND

        // Get PSP
        mrs     r0, psp

        // Check FP context bit on LR. Extended frame is present if 0. If the core doesn't
        // implement the FPU extension, this will always be 1.
        tst     lr, #EXTENDED_FRAME
        bne     _save_no_ext_frame
        vstmdb  r0!, {s16-s31}
        mov     r1, #1
        b       _continue_save
_LABEL(_save_no_ext_frame)
        mov     r1, #0
_LABEL(_continue_save)
        // Save registers on the stack and update the stack pointer (r0).
        stmdb   r0!, {r4-r11}

        // Invoke scheduler. r0 is the sp, r1 is whether there was an extended frame.
        // On return, r0 contains the stack pointer for the new thread.
        ldr     r12, =ar_port_yield_isr
        blx     r12

        // Unstack saved registers.
        ldmia   r0!, {r4-r11}

        // Set default exception return value.
        mvn     lr, #~EXC_RETURN

        // Get extended frame flag.
        ldr     r1, =g_ar_hasExtendedFrame
        ldrb    r1, [r1]
        cbz     r1, _exit_no_ext_frame

        // Restore extended frame
        vldmia  r0!, {s16-s31}
        mvn     lr, #~FP_EXC_RETURN

_LABEL(_exit_no_ext_frame)
        // Update PSP with new stack pointer.
        msr     psp, r0

        // Exit handler. Using a bx to the special EXC_RETURN values causes the
        // processor to perform the exception return behavior.
        bx      lr

        _FN_END(PendSV_Handler)
        _FN_SIZE(PendSV_Handler)

        _FN_BEGIN(SVC_Handler)
        _FN_DECL(SVC_Handler)
_FN_LABEL(SVC_Handler)
        _FN_BEGIN_POST
        _FN_CANT_UNWIND

        // Exit handler.
        bx      lr

        _FN_END(SVC_Handler)
        _FN_SIZE(SVC_Handler)

        _ALIGN(4)

        _END
