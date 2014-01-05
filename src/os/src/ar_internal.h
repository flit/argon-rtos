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

#include "os/ar_c_api.h"
#include "os/ar_kernel.h"
#include "ar_port.h"

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

//! The string to use for the name of an object that wasn't provided a name.
#define AR_ANONYMOUS_OBJECT_NAME ("<anon>")

/*!
 * @brief Argon kernel state.
 */
typedef struct _ar_kernel {
    ar_thread_t * currentThread;    //!< The currently running thread.
    ar_list_t readyList;            //!< List of threads ready to run.
    ar_list_t suspendedList;        //!< List of suspended threads.
    ar_list_t sleepingList;         //!< List of sleeping threads.
    ar_list_t activeTimers;         //!< List of running timers
    bool isRunning;                 //!< True if the kernel has been started.
    volatile uint32_t tickCount;    //!< Current tick count.
    volatile uint32_t irqDepth;     //!< Current level of nested IRQs, or 0 if in user mode.
    volatile unsigned systemLoad;   //!< Percent of system load from 0-100. The volatile is necessary so that the IAR optimizer doesn't remove the entire load calculation loop of the idle_entry() function.
    ar_thread_t idleThread;         //!< The lowest priority thread in the system. Executes only when no other threads are ready.
#if AR_GLOBAL_OBJECT_LISTS
    //! Contains linked lists of all the various Ar object types that have been created during runtime.
    struct {
        ar_list_t threads;          //!< All existing threads.
        ar_list_t semaphores;       //!< All existing semaphores.
        ar_list_t mutexes;          //!< All existing mutexes.
        ar_list_t queues;           //!< All existing queues.
        ar_list_t timers;           //!< All existing timers.
    } allObjects;
#endif // AR_GLOBAL_OBJECT_LISTS
} ar_kernel_t;

extern ar_kernel_t g_ar;

//------------------------------------------------------------------------------
// API
//------------------------------------------------------------------------------

void ar_port_init_system(void);
void ar_port_init_tick_timer(void);
void ar_port_prepare_stack(ar_thread_t * thread, void * param);
void ar_port_service_call(void);

void ar_kernel_periodic_timer_isr(void);
uint32_t ar_kernel_yield_isr(uint32_t topOfStack);
bool ar_kernel_increment_tick_count(unsigned ticks);
void ar_kernel_enter_scheduler(void);
void ar_kernel_scheduler(void);

void ar_thread_wrapper(ar_thread_t * thread, void * param);

bool ar_thread_sort_by_priority(ar_list_node_t * a, ar_list_node_t * b);
bool ar_timer_sort_by_wakeup(ar_list_node_t * a, ar_list_node_t * b);

#if defined(__cplusplus)
extern "C" {
#endif

void * ar_yield(void * topOfStack);
void ar_periodic_timer(void);

#if defined(__cplusplus)
}
#endif

#endif // _AR_INTERNAL_H_
//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
