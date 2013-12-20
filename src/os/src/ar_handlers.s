/*
 * Copyright 2007 Chris Reed. All rights reserved.
 */
 
			.file "handlers.arm"

			.nothumb

#include "AT91SAM7S64_inc.h"

/* ARM Core Mode and Status Bits */

ARM_MODE_USER           equ     0x10
ARM_MODE_FIQ            equ		0x11
ARM_MODE_IRQ            equ     0x12
ARM_MODE_SVC            equ     0x13
ARM_MODE_ABORT          equ     0x17
ARM_MODE_UNDEF          equ     0x1B
ARM_MODE_SYS            equ     0x1F

I_BIT                   equ     0x80
F_BIT                   equ     0x40
T_BIT                   equ     0x20

/* offset to the m_stackPointer member of a MuThread object */
THREAD_STACK_PTR_OFFSET		.equ	24
			
/* constants for MuThread::s_mode */
MU_USER_MODE				.equ		0
MU_IRQ_MODE					.equ		1


/* specify the section where this code belongs */
    		.section ".reset", .text
    		.align  4

/* import external symbols */
			.import s_currentThread__8MuThread
			.import s_irqDepth__8MuThread

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
			
			.global irq_handler

irq_handler:
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

            .type   irq_handler,$function
            .size   irq_handler,.-irq_handler


/*
 * Restore thread context
 */

			.global restore_context

; void restore_context(uint32_t tos)
;
restore_context:
			; set lr to the thread's stack
			mov			lr, r0
		
			; pop spsr from thread's stack
			ldmfd		lr!, {r0}
			msr			spsr, r0
			
			; now restore system mode registers
			ldmfd		lr, {r0-r14}^
			nop
			
			; pop return address
			ldr			lr, [lr, #60]
			
			; now exit the isr and return to the thread
			; must correct offset in lr to get the right address
			; this instruction also sets CPSR = SPSR 
			subs		pc, lr, #4

			.type	restore_context,$function
			.size	restore_context,.-restore_context


/*
 * SWI instruction handler
 *
 * Used to transfer control from a thread directly to the scheduler. Usually
 * control is transferred back to another thread.
 */

			.global swi_handler
			
swi_handler:
			; we're in Supervisor (SVC) Mode upon entry to this handler
			
			; save thread context
			
			; push r0 to save it
			stmdb		sp!, {r0}
			
			; set r0 to thread's stack pointer
			stmdb		sp, {sp}^
			sub			sp, sp, #4
			ldmia		sp!, {r0}
			
			; push return adjusted address onto the thread's stack
			; lr must be adjusted because swi does not have lr offset that irq does
			add			lr, lr, #4
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
			
			; the new top of stack is returned in r0...
			mov			r0, lr
		
			; ... and passed to this function, which returns the top of stack
			; for the new current thread in r0
			ldr			r1, =yieldIsr__8MuThreadSFUi
			mov			lr, pc
			bx			r1

			; restore thread context; accepts top of stack in r0
			b			restore_context

			.type	swi_handler,$function
			.size	swi_handler,.-swi_handler


