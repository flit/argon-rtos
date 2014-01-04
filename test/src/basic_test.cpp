/*
 * Copyright (c) 2013 Immo Software
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

#include "os/ar_kernel.h"
#include "debug_uart.h"
#include "kernel_tests.h"

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

#define TEST_CASE_CLASS TestMutex1

//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------

void main_thread(void * arg);

//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------

Ar::ThreadWithStack<512> g_mainThread("main", main_thread, 0, 56);

TEST_CASE_CLASS g_testCase;

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

const char * KernelTest::threadIdString() const
{
    static char idString[32];
    snprintf(idString, sizeof(idString), "[%s]", self()->getName());
    return idString;
}

void KernelTest::printHello()
{
    printf("%s running (prio=%d)\r\n", threadIdString(), self()->getPriority());
}

void KernelTest::printTicks()
{
    uint32_t ticks = ar_get_tick_count();
    printf("%s ticks=%u!\r\n", threadIdString(), ticks);
}

void main_thread(void * arg)
{
    Ar::Thread * self = Ar::Thread::getCurrent();
    const char * myName = self->getName();
    
    printf("[%s] Main thread is running\r\n", myName);
    
    g_testCase.init();
    g_testCase.run();
    
    printf("[%s] goodbye!\r\n", myName);
}

void main(void)
{
#if !defined(KL25Z4_SERIES)
    debug_init();
#endif
    
    printf("Running test...\r\n");
    
    // (const char * name, thread_entry_t entry, void * param, void * stack, unsigned stackSize, uint8_t priority);
//     g_mainThread.init("main", main_thread, 0, 56);
    g_mainThread.resume();
    
    ar_kernel_run();

    Ar::_halt();
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
