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

#include "mbed.h"

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

#define TEST_CASE_CLASS TestSem1

//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------

void main_thread(void * arg);
void green_thread(void * arg);
void red_thread(void * arg);

//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------

Ar::ThreadWithStack<512> g_mainThread;
Ar::ThreadWithStack<196> g_greenThread;
Ar::ThreadWithStack<196> g_redThread;

TEST_CASE_CLASS g_testCase;

PwmOut blueLed(LED_BLUE);
PwmOut greenLed(LED_GREEN);
PwmOut redLed(LED_RED);

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

const char * KernelTest::threadIdString() const
{
    Ar::Thread * self = Ar::Thread::getCurrent();
    static char idString[32];
    snprintf(idString, sizeof(idString), "[%s]", self->getName());
    return idString;
}

void KernelTest::printHello()
{
    printf("%s running\r\n", threadIdString());
}

void KernelTest::printTicks()
{
    uint32_t ticks = Ar::Kernel::getTickCount();
    printf("%s ticks=%u!\r\n", threadIdString(), ticks);
}

void main_thread(void * arg)
{
    Ar::Thread * self = Ar::Thread::getCurrent();
    const char * myName = self->getName();
    
    printf("[%s] Main thread is running\r\n", myName);
    
    g_testCase.init();
    g_testCase.run();

    g_greenThread.init("green", green_thread, 0, 50);
    g_greenThread.resume();

    g_redThread.init("red", red_thread, 0, 50);
    g_redThread.resume();
    
    blueLed.period_ms(2);   // 500 Hz
    
    float dutyCycle = 1.0f;
    float delta = -0.05f;
    while (1)
    {
//         print_thread_ticks();

        blueLed = dutyCycle;
        
        Ar::Thread::sleep(50);
        
        dutyCycle += delta;
        if (dutyCycle < 0.01f)
        {
            dutyCycle = 0.0f;// 1.0f;
            delta = -delta;
        }
        else if (dutyCycle > 0.99f)
        {
            dutyCycle = 1.0f;
            delta = -delta;
        }
    }
}

void green_thread(void * arg)
{
    Ar::Thread::sleep(500);
    
    greenLed.period_ms(2);   // 500 Hz
    
    float dutyCycle = 1.0f;
    float delta = -0.05f;
    while (1)
    {
//         print_thread_ticks();

        greenLed = dutyCycle;
        
        Ar::Thread::sleep(50);
        
        dutyCycle += delta;
        if (dutyCycle < 0.01f)
        {
            dutyCycle = 0.0f;// 1.0f;
            delta = -delta;
        }
        else if (dutyCycle > 0.99f)
        {
            dutyCycle = 1.0f;
            delta = -delta;
        }
    }
}

void red_thread(void * arg)
{
    Ar::Thread::sleep(1250);
    
    redLed.period_ms(2);   // 500 Hz
    
    float dutyCycle = 1.0f;
    float delta = -0.05f;
    while (1)
    {
//         print_thread_ticks();

        redLed = dutyCycle;
        
        Ar::Thread::sleep(50);
        
        dutyCycle += delta;
        if (dutyCycle < 0.01f)
        {
            dutyCycle = 0.0f;// 1.0f;
            delta = -delta;
        }
        else if (dutyCycle > 0.99f)
        {
            dutyCycle = 1.0f;
            delta = -delta;
        }
    }
}

void main(void)
{
//     debug_init();
    
    printf("Running test...\r\n");
    
    // (const char * name, thread_entry_t entry, void * param, void * stack, unsigned stackSize, uint8_t priority);
    g_mainThread.init("main", main_thread, 0, 50);
    g_mainThread.resume();
    
    Ar::Kernel::run();

    Ar::_halt();
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
