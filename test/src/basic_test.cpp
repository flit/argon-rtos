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

//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------

void main_thread(void * arg);
void a_thread(void * arg);
void b_thread(void * arg);

//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------

// uint32_t dummy;
uint8_t g_mainThreadStack[1024];
Ar::Thread g_mainThread; //(main_thread, g_mainThreadStack, sizeof(g_mainThreadStack));

uint8_t g_aThreadStack[1024];
Ar::Thread g_aThread;

uint8_t g_bThreadStack[1024];
Ar::Thread g_bThread;

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

void main_thread(void * arg)
{
    const char * myName = Ar::Thread::getCurrent()->getName();
    printf("Thread '%s' is running\r\n", myName);
    
    g_aThread.init("a", a_thread, 0, g_aThreadStack, sizeof(g_aThreadStack), 60);
    g_aThread.resume();

    g_bThread.init("b", b_thread, 0, g_bThreadStack, sizeof(g_bThreadStack), 70);
    g_bThread.resume();

    while (1)
    {
        printf("Hello from thread '%s' (ticks=%u)!\r\n", myName, Ar::Thread::getTickCount());
        
        Ar::Thread::sleep(100);
    }
}

void a_thread(void * arg)
{
    const char * myName = Ar::Thread::getCurrent()->getName();
    printf("Thread '%s' is running\r\n", myName);
    
    while (1)
    {
        printf("Hello from thread '%s' (ticks=%u)!\r\n", myName, Ar::Thread::getTickCount());
        
        Ar::Thread::sleep(200);
    }
}

void b_thread(void * arg)
{
    const char * myName = Ar::Thread::getCurrent()->getName();
    printf("Thread '%s' is running\r\n", myName);
    
    while (1)
    {
        printf("Hello from thread '%s' (ticks=%u)!\r\n", myName, Ar::Thread::getTickCount());
        
        Ar::Thread::sleep(300);
    }
}

void main(void)
{
    debug_init();
//     dummy = 1234;
    
    printf("Running test...\r\n");
    
    // (const char * name, thread_entry_t entry, void * param, void * stack, unsigned stackSize, uint8_t priority);
    g_mainThread.init("main", main_thread, 0, g_mainThreadStack, sizeof(g_mainThreadStack), 50);
    g_mainThread.resume();
    
    Ar::Thread::run();

    Ar::_halt();
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
