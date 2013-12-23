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

/* offset to the m_stackPointer member of a MuThread object */
THREAD_STACK_PTR_OFFSET		equ	20

// EXC_RETURN value to return to Threa mode, while restoring state from PSP.
EXC_RETURN  equ 0xfffffffd

/* specify the section where this code belongs */
            
        section .text:CODE
        thumb

/* import external symbols */
// 			.import s_currentThread__8MuThread
// 			.import s_irqDepth__8MuThread

/*
 * Vectored IRQ handler
 *
 * Saves thread context for all IRQs, so any individual vector can safely
 * make calls to the Mu kernel. Control will not necessarily return to the
 * same thread that was executing before the interrupt happened, as Mu
 * calls may invoke the scheduler to switch threads. For instance, this
 * will happen if an ISR puts a semaphore that was blocking a thread with
 * a higher priority than the current thread.
 */
/*			
			PUBLIC irq_handler

irq_handler
			; -begin save thread context-
			
			; push r0 to save it
			stmdb		sp!, {r0}
			
			; set r0 to thread's stack pointer
			stmdb		sp, {sp}^
			sub			sp, sp, #4
			ldmia		sp!, {r0}
			
			; push return address onto the thread's stack
			stmdb		r0!, {lr}
			
			; use lr instead of r0 since lr is saved onto stack
			mov			lr, r0
			
			; pop r0 so it can be saved into system mode stack
			ldmia		sp!, {r0}
			
			; save all system mode registers onto the thread's stack
			stmdb		lr, {r0-r14}^
			sub			lr, lr, #60
			
			; save SPSR onto the thread's stack
			; can use r0 now that it has been saved
			mrs			r0, spsr
			stmdb		lr!, {r0}
			
			; -end save thread context-
			
			; Save current stack pointer directly into the current thread object.
			; The store is skipped if the current thread is NULL.
			ldr			r0, =s_currentThread__8MuThread
			ldr			r1, [r0]
			cmp			r1, #0
			strne		lr, [r1, #THREAD_STACK_PTR_OFFSET]

			; add one to the IRQ depth
			ldr			r0, =s_irqDepth__8MuThread
			ldr			r1, [r0]
			add			r1, r1, #1
			str			r1, [r0]

			; Write in the IVR to support Protect Mode
			; No effect in Normal Mode
			; De-assert the NIRQ and clear the source in Protect Mode
            ldr         r14, =AT91C_BASE_AIC
		    ldr         r0 , [r14, #AIC_IVR]
		    str         r14, [r14, #AIC_IVR]

			; Branch to the routine pointed by the AIC_IVR
            mov         lr, pc
            bx          r0

			; Mark the End of Interrupt on the AIC
            ldr         r14, =AT91C_BASE_AIC
            str         r14, [r14, #AIC_EOICR]

			; Subtract one from IRQ depth now that we're exiting
			ldr			r0, =s_irqDepth__8MuThread
			ldr			r1, [r0]
			sub			r1, r1, #1
			str			r1, [r0]

			; restore thread context
			ldr			r0, =s_currentThread__8MuThread			; get var address
			ldr			r1, [r0, 0]								; get object pointer
			ldr			r0, [r1, #THREAD_STACK_PTR_OFFSET]		; get member of object
			b			restore_context

//             .type   irq_handler,$function
//             .size   irq_handler,.-irq_handler
*/

/*
 * Restore thread context
 */

// 			PUBLIC restore_context
// 
// ; void restore_context(uint32_t tos)
// ;
// restore_context:
// 			; set lr to the thread's stack
// 			mov			lr, r0
// 		
// 			; pop spsr from thread's stack
// 			ldmfd		lr!, {r0}
// 			msr			spsr, r0
// 			
// 			; now restore system mode registers
// 			ldmfd		lr, {r0-r14}^
// 			nop
// 			
// 			; pop return address
// 			ldr			lr, [lr, #60]
// 			
// 			; now exit the isr and return to the thread
// 			; must correct offset in lr to get the right address
// 			; this instruction also sets CPSR = SPSR 
// 			subs		pc, lr, #4

// 			.type	restore_context,$function
// 			.size	restore_context,.-restore_context


/*
 * SWI instruction handler
 *
 * Used to transfer control from a thread directly to the scheduler. Usually
 * control is transferred back to another thread.
 */

// 			PUBLIC swi_handler
// 			
// swi_handler
// 			; we're in Supervisor (SVC) Mode upon entry to this handler
// 			
// 			; save thread context
// 			
// 			; push r0 to save it
// 			stmdb		sp!, {r0}
// 			
// 			; set r0 to thread's stack pointer
// 			stmdb		sp, {sp}^
// 			sub			sp, sp, #4
// 			ldmia		sp!, {r0}
// 			
// 			; push return adjusted address onto the thread's stack
// 			; lr must be adjusted because swi does not have lr offset that irq does
// 			add			lr, lr, #4
// 			stmdb		r0!, {lr}
// 			
// 			; use lr instead of r0 since lr is saved onto stack
// 			mov			lr, r0
// 			
// 			; pop r0 so it can be saved into system mode stack
// 			ldmia		sp!, {r0}
// 			
// 			; save all system mode registers onto the thread's stack
// 			stmdb		lr, {r0-r14}^
// 			sub			lr, lr, #60
// 			
// 			; save SPSR onto the thread's stack
// 			; can use r0 now that it has been saved
// 			mrs			r0, spsr
// 			stmdb		lr!, {r0}
// 			
// 			; the new top of stack is returned in r0...
// 			mov			r0, lr
// 		
// 			; ... and passed to this function, which returns the top of stack
// 			; for the new current thread in r0
// 			ldr			r1, =yieldIsr__8MuThreadSFUi
// 			mov			lr, pc
// 			bx			r1
// 
// 			; restore thread context; accepts top of stack in r0
// 			b			restore_context

// 			.type	swi_handler,$function
// 			.size	swi_handler,.-swi_handler

        import ar_yield
        import g_ar_currentThread

        public SVC_Handler
        public PendSV_Handler
        
SVC_Handler
PendSV_Handler
        
        // Get current thread.
//         ldr     r3, =g_ar_currentThread
//         ldr     r3, [r3]
        
        // Get PSP
        mrs     r0, psp
        
        // Subtract room for the registers we are going to store. We have to pre-subtract
        // and use the incrementing store multiple instruction because the CM0+ doesn't
        // have the decrementing variant.
        subs    r0, r0, #32
        
        // Save registers on the stack. This has to be done in two stages because
        // the stmia instruction cannot access the upper registers on the CM0+.
        stmia   r0!, {r4-r7}
        mov     r4,r8
        mov     r5,r9
        mov     r6,r10
        mov     r7,r10
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
        mov     r8,r4
        mov     r9,r5
        mov     r10,r6
        mov     r11,r7
        ldmia   r0!, {r4-r7}
        adds    r0, r0, #16
        
        // Update PSP with new stack pointer.
        msr     psp, r0
        
        // Exit handler. Using a bx to the special EXC_RETURN values causes the
        // processor to perform the exception return behavior.
        ldr     r0, =EXC_RETURN
        bx      r0


        end
