/*
 * Copyright (c) 2013-2014 Immo Software
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
/*!
 * @file ar_internal.h
 * @ingroup ar_port
 * @brief Header for the Argon RTOS.
 */

#if !defined(_AR_INTERNAL_H_)
#define _AR_INTERNAL_H_

#include "fsl_platform_common.h"
#include "fsl_device_registers.h"

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

typedef struct _ar_kernel {
    ar_thread_t * readyList;    //!< Head of a linked list of ready threads.
    ar_thread_t * suspendedList;    //!< Head of linked list of suspended threads.
    ar_thread_t * sleepingList; //!< Head of linked list of sleeping threads.
    ar_thread_t * currentThread;    //!< The currently running thread.
    at_timer_t * activeTimers;  //!< List of currently running timers. Sorted ascending by wakup time.
    bool isRunning;    //!< True if the kernel has been started.
    volatile uint32_t tickCount;   //!< Current tick count.
    volatile uint32_t irqDepth;    //!< Current level of nested IRQs, or 0 if in user mode.
    volatile unsigned systemLoad;   //!< Percent of system load from 0-100.
    ar_thread_t idleThread;
} ar_kernel_t;

void ar_port_init_system(void);
void ar_port_init_tick_timer(void);
void ar_port_prepare_stack(ar_thread_t * thread, void * param);
void ar_port_service_call(void);

void ar_kernel_periodic_timer_isr(void);
uint32_t ar_kernel_yield_isr(uint32_t topOfStack);
bool ar_kernel_increment_tick_count(unsigned ticks);
void ar_kernel_scheduler(void);
void ar_kernel_enter_interrupt();
void ar_kernel_exit_interrupt();

void ar_thread_wrapper(ar_thread_t * thread, void * param);

void ar_thread_list_add(ar_thread_t ** listHead, ar_thread_t * thread);
void ar_thread_list_remove(ar_thread_t ** listHead, ar_thread_t * thread);

void ar_thread_blocked_list_add(ar_thread_t ** listHead, ar_thread_t * thread);
void ar_thread_blocked_list_remove(ar_thread_t ** listHead, ar_thread_t * thread);
void ar_thread_block(ar_thread_t * thread, ar_thread_t ** blockedList, uint32_t timeout);
void ar_thread_unblock_with_status(ar_thread_t * thread, at_thread_t ** blockedList, status_t unblockStatus);

void ar_timer_list_add(ar_timer_t ** listHead, ar_timer_t * timer);
void ar_timer_list_remove(ar_timer_t ** listHead, ar_timer_t * timer);


#endif // _AR_INTERNAL_H_
//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
