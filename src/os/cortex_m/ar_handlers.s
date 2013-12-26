/*
 * Copyright (c) 2007-2013 Chris Reed
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

/* offset to the m_stackPointer member of a Thread object */
// THREAD_STACK_PTR_OFFSET		equ	20

// EXC_RETURN value to return to Thread mode, while restoring state from PSP.
EXC_RETURN  equ 0xfffffffd

/* specify the section where this code belongs */
            
        section .text:CODE
        thumb

        import ar_yield
        import g_ar_currentThread

        public SVC_Handler
        public PendSV_Handler
        
SVC_Handler
PendSV_Handler
        
        // Save the EXC_RETURN value in the LR register on the MSP, temporarily,
        // while we save the thread context on the PSP. We also save r11 just to
        // maintain 8-byte alignment.
//         push    {r0, lr}
        
        // Get PSP
        mrs     r0, psp
        
        // Subtract room for the registers we are going to store. We have to pre-subtract
        // and use the incrementing store multiple instruction because the CM0+ doesn't
        // have the decrementing variant.
        subs    r0, r0, #32
        
        // Save registers on the stack. This has to be done in two stages because
        // the stmia instruction cannot access the upper registers on the CM0+.
        stmia   r0!, {r4-r7}
        mov     r4, r8
        mov     r5, r9
        mov     r6, r10
        mov     r7, r10
        stmia   r0!, {r4-r7}
        
        // Get back to the bottom of the stack before we pass the current SP to the
        // ar_yield call.
        subs    r0, r0, #32
        
        // Invoke scheduler. On return, r0 contains the stack pointer for the new thread.
        ldr     r1, =ar_yield
        blx     r1
        
        // Unstack saved registers.
        adds    r0, r0, #16
        ldmia   r0!, {r4-r7}
        subs    r0, r0, #32
        mov     r8, r4
        mov     r9, r5
        mov     r10, r6
        mov     r11, r7
        ldmia   r0!, {r4-r7}
        adds    r0, r0, #16
        
        // Update PSP with new stack pointer.
        msr     psp, r0
        
        // Pop the saved EXC_RETURN value from the main stack into r1. r0 is just there
        // to maintain 8-byte stack alignment and is unused. r0 and r1 will be restored
        // to their saved values when the core performs the exception return behavior.
//         pop     {r0, r1}
//         bx      r1
        
        // Exit handler. Using a bx to the special EXC_RETURN values causes the
        // processor to perform the exception return behavior.
        ldr     r0, =EXC_RETURN
        bx      r0


        end
