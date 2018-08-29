/*
 * Copyright (c) 2013-2014 Immo Software
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

#include "argon/ar_classes.h"
#include "ar_config.h"

#if defined(__CC_ARM)
#include <rt_misc.h>
#endif // defined(__CC_ARM)

#if AR_ENABLE_MAIN_THREAD

namespace Ar {
//! @brief Thread in which main() executes.
Thread g_mainThread;
} // namespace Ar

#if defined (__ICCARM__)

#include <stdlib.h>

#pragma segment="CSTACK"

extern "C" {
extern void* __vector_table;
extern int  __low_level_init(void);
extern void __iar_data_init3(void);
extern __weak void __iar_init_core( void );
extern __weak void __iar_init_vfp( void );
extern void __iar_dynamic_initialization(void);
void _call_main( void );
void __iar_program_start( void );
}

#pragma required=__vector_table
__noreturn void __iar_program_start(void)
{
    __iar_init_core();
    __iar_init_vfp();

    if (__low_level_init() != 0)
    {
        __iar_data_init3();
    }

    uint32_t mainStack = (uint32_t)__section_begin("CSTACK");
    uint32_t mainStackSize = __section_size("CSTACK") - AR_SCHEDULER_STACK_SIZE;
    Ar::g_mainThread.init("main", (ar_thread_entry_t)_call_main, 0, (void *)mainStack, mainStackSize, AR_MAIN_THREAD_PRIORITY, kArStartThread);
    ar_kernel_run();
    exit(0);
}

#elif defined(__CC_ARM)

#if 0

extern uint32_t Image$$ARM_LIB_STACK$$ZI$$Base[];
extern uint32_t Image$$ARM_LIB_STACK$$ZI$$Limit[];
extern uint32_t Image$$ARM_LIB_HEAP$$ZI$$Base[];
extern uint32_t Image$$ARM_LIB_HEAP$$ZI$$Limit[];

extern int main(void);

#if defined(__MICROLIB)

extern "C" void _main_init (void) __attribute__((section(".ARM.Collect$$$$000000FF")));

void _main_init (void)
{
    uint32_t mainStack = (uint32_t)Image$$ARM_LIB_STACK$$ZI$$Limit;
    uint32_t mainStackSize = (uint32_t)Image$$ARM_LIB_STACK$$ZI$$Limit - (uint32_t)Image$$ARM_LIB_STACK$$ZI$$Base;
    Ar::g_mainThread.init("main", (ar_thread_entry_t)x._main, 0, (void *)mainStack, mainStackSize, AR_MAIN_THREAD_PRIORITY);
    Ar::g_mainThread.resume();
    ar_kernel_run();
    for (;;) {}
}

#else // defined(__MICROLIB)

/* The single memory model is checking for stack collision at run time, verifing
   that the heap pointer is underneath the stack pointer.

   With the RTOS there is not only one stack above the heap, there are multiple
   stacks and some of them are underneath the heap pointer.
*/
#pragma import(__use_two_region_memory)

extern "C" {

// __value_in_regs struct __initial_stackheap
// __user_initial_stackheap(unsigned /*R0*/, unsigned /*SP*/,
//                          unsigned /*R2*/, unsigned /*SL*/)
// {
//     struct __initial_stackheap x;
//     x.heap_base = (unsigned)Image$$ARM_LIB_HEAP$$ZI$$Base;
//     x.stack_base = (unsigned)Image$$ARM_LIB_STACK$$ZI$$Base;
//     x.heap_limit = (unsigned)Image$$ARM_LIB_HEAP$$ZI$$Limit;
//     x.stack_limit = (unsigned)Image$$ARM_LIB_STACK$$ZI$$Limit;
//     return x;
// }

__asm void __user_setup_stackheap(void)
{
    IMPORT  |Image$$ARM_LIB_HEAP$$ZI$$Base|
    IMPORT  |Image$$ARM_LIB_HEAP$$ZI$$Limit|

    mov     r0,sp
    mov     r2,#8
    subs    r0,r0,r2
    mov     sp,r0
    ldr     r0,=|Image$$ARM_LIB_HEAP$$ZI$$Base|
    ldr     r2,=|Image$$ARM_LIB_HEAP$$ZI$$Limit|
    bx      lr

    align
}

extern "C" void start_argon (ar_thread_entry_t _main)
{
    uint32_t mainStack = (uint32_t)Image$$ARM_LIB_STACK$$ZI$$Limit;
    uint32_t mainStackSize = (uint32_t)Image$$ARM_LIB_STACK$$ZI$$Limit - (uint32_t)Image$$ARM_LIB_STACK$$ZI$$Base;
    Ar::g_mainThread.init("main", _main, 0, (void *)mainStack, mainStackSize, AR_MAIN_THREAD_PRIORITY);
    Ar::g_mainThread.resume();
    ar_kernel_run();
    for (;;) {}
}

__asm void __rt_entry(void)
{
//     IMPORT  __user_setup_stackheap
    IMPORT  __rt_lib_init
    IMPORT  exit
    IMPORT  main
    IMPORT  start_argon

    bl      __user_setup_stackheap
    movs    r1,r2
    bl      __rt_lib_init
    ldr     r0,=main
    bl      start_argon
    bl      exit

    ALIGN
}

} // extern "C"

#endif // defined(__MICROLIB)

#endif // 0

#endif // defined (__ICCARM__)

#endif // AR_ENABLE_MAIN_THREAD

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
