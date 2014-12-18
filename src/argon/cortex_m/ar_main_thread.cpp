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

#include "argon/ar_classes.h"
#include "argon/src/ar_config.h"

#if AR_ENABLE_MAIN_THREAD

namespace Ar {
//! @brief Thread in which main() executes.
Thread g_mainThread;
} // namespace Ar

#if defined (__ICCARM__)

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
    Ar::g_mainThread.init("main", (ar_thread_entry_t)_call_main, 0, (void *)mainStack, mainStackSize, AR_MAIN_THREAD_PRIORITY);
    Ar::g_mainThread.resume();
    ar_kernel_run();
    exit(0);
}

#endif // defined (__ICCARM__)

#endif // AR_ENABLE_MAIN_THREAD

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
