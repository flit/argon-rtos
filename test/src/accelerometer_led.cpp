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

#include "argon/argon.h"
#include "debug_uart.h"
#include "kernel_tests.h"
#include "MMA8451Q/MMA8451Q.h"

#include "mbed.h"

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

#define TEST_CASE_CLASS TestSem1

#define MMA8451_I2C_ADDRESS (0x1d<<1)

enum
{
    kLedFadeRate = 100
};

class Fader
{
private:
    float m_value;
    float m_delta;

public:
    Fader(float delta);

    float next();

    void setDelta(float delta) { m_delta = delta; }

    operator float () { return next(); }
};

//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------

void main_thread(void * arg);
void blue_thread(void * arg);
void green_thread(void * arg);
void red_thread(void * arg);
void x_thread(void * arg);
void y_thread(void * arg);
void z_thread(void * arg);

//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------

Ar::ThreadWithStack<512> g_mainThread;

Ar::ThreadWithStack<512> g_blueThread;
Ar::ThreadWithStack<512> g_greenThread;
Ar::ThreadWithStack<512> g_redThread;

Ar::ThreadWithStack<512> g_xThread;
Ar::ThreadWithStack<256> g_yThread;
Ar::ThreadWithStack<256> g_zThread;

TEST_CASE_CLASS g_testCase;

Ar::StaticQueue<int16_t, 4> g_x;
Ar::StaticQueue<int16_t, 4> g_y;
Ar::StaticQueue<int16_t, 4> g_z;

MMA8451Q acc(PTE25, PTE24, MMA8451_I2C_ADDRESS);
Ar::Mutex g_accLock;

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

Fader::Fader(float delta)
:   m_value(0.0),
    m_delta(delta)
{
}

float Fader::next()
{
    m_value += m_delta;
    if (m_value < 0.000001f)
    {
        m_value = 0.0f;
        m_delta = -m_delta;
    }
    else if (m_value > 0.99999f)
    {
        m_value = 1.0f;
        m_delta = -m_delta;
    }

    return m_value;
}

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

    g_accLock.init("accLock");
    g_x.init("x");
    g_y.init("y");
    g_z.init("z");

    g_xThread.init("x", x_thread, 0, 55);
    g_xThread.resume();

    g_yThread.init("y", y_thread, 0, 55);
    g_yThread.resume();

    g_zThread.init("z", z_thread, 0, 55);
    g_zThread.resume();

    g_blueThread.init("blue", blue_thread, 0, 50);
    g_blueThread.resume();

    g_greenThread.init("green", green_thread, 0, 50);
    g_greenThread.resume();

    g_redThread.init("red", red_thread, 0, 50);
    g_redThread.resume();

    printf("[%s] goodbye!\r\n", myName);
}

void x_thread(void * arg)
{
    g_accLock.get();
    printf("MMA8451Q ID: 0x%2x\r\n", acc.getWhoAmI());
    g_accLock.put();

    while (true)
    {
        g_accLock.get();
        int16_t x = acc.getAccX();
        g_accLock.put();
        g_x.send(x);

        Ar::Thread::sleep(50);
    }
}

void y_thread(void * arg)
{
    while (true)
    {
        g_accLock.get();
        int16_t y = acc.getAccY();
        g_accLock.put();
        g_y.send(y);

        Ar::Thread::sleep(50);
    }
}

void z_thread(void * arg)
{
    while (true)
    {
        g_accLock.get();
        int16_t z = acc.getAccZ();
        g_accLock.put();
        g_z.send(z);

        Ar::Thread::sleep(50);
    }
}

void blue_thread(void * arg)
{
    PwmOut blueLed(LED_BLUE);
    blueLed.period_ms(2);   // 500 Hz

//     Fader dutyCycle(0.05f);
    while (1)
    {
//         print_thread_ticks();

        int16_t a = g_x.receive();
        float dutyCycle = (float)(4096 - (a < 0 ? -a : a)) / 4096.0f;
        printf("x=%d\r\n", a);

        blueLed = dutyCycle;

//         Ar::Thread::sleep(kLedFadeRate);
    }
}

void green_thread(void * arg)
{
//     Ar::Thread::sleep(500);

    PwmOut greenLed(LED_GREEN);
    greenLed.period_ms(2);   // 500 Hz

//     Fader dutyCycle(0.025f);
    while (1)
    {
//         print_thread_ticks();

        int16_t a = -g_y.receive();
        float dutyCycle = (float)(a < 0 ? -a : a) / 4096.0f;
        printf("y=%d\r\n", a);

        greenLed = dutyCycle;

//         Ar::Thread::sleep(kLedFadeRate);
    }
}

void red_thread(void * arg)
{
//     Ar::Thread::sleep(1250);

    PwmOut redLed(LED_RED);
    redLed.period_ms(2);   // 500 Hz

//     Fader dutyCycle(0.035f);
    while (1)
    {
//         print_thread_ticks();

        int16_t a = g_z.receive();
        float dutyCycle = (float)(4096 - (a < 0 ? -a : a)) / 4096.0f;
        printf("z=%d\r\n", a);

        redLed = dutyCycle;

//         Ar::Thread::sleep(kLedFadeRate);
    }
}

void main(void)
{
//     debug_init();

    printf("Running test...\r\n");

    // (const char * name, thread_entry_t entry, void * param, void * stack, unsigned stackSize, uint8_t priority);
    g_mainThread.init("main", main_thread, 0, 56);
    g_mainThread.resume();

    Ar::Kernel::run();

    Ar::_halt();
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
