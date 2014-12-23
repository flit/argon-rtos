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
#include "us_ticker_api.h"

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

#define TEST_CASE_CLASS TestMutex1

//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------

void main_thread(void * arg);
void x_thread(void * arg);
void y_thread(void * arg);

class Foo
{
public:

    void entry(void * param)
    {
        printf("hi from Foo!\n");
    }
};

//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------

#if !defined(__ICCARM__)
Ar::ThreadWithStack<512> g_mainThread("main", main_thread, 0, 56);
#endif

TEST_CASE_CLASS g_testCase;

Ar::ThreadWithStack<512> g_xThread("x", x_thread, 0, 30);
Ar::ThreadWithStack<512> g_yThread("y", y_thread, 0, 20);

Ar::TypedChannel<int> g_chan("c");

Ar::ThreadToMemberFunctionWithStack<512,Foo> g_fooThread;

Ar::Thread * g_dyn;

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

const char * KernelTest::threadIdString() const
{
    static char idString[32];
    snprintf(idString, sizeof(idString), "[%d:%s]", us_ticker_read(), self()->getName());
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

void x_thread(void * arg)
{
    Ar::Thread * self = Ar::Thread::getCurrent();
    const char * myName = self->getName();
    ar_status_t status;

    while (1)
    {
        printf("[%d:%s] receiving on channel\n", us_ticker_read(), myName);
        int foo = g_chan.receive();
        printf("[%d:%s] received from channel (foo=%d)\n", us_ticker_read(), myName, foo);

        printf("[%d:%s] sleeping a bit\n", us_ticker_read(), myName);
        Ar::Thread::sleep(1000);

        printf("[%d:%s] receiving on channel\n", us_ticker_read(), myName);
        foo = g_chan.receive();
        printf("[%d:%s] received from channel (foo=%d)\n", us_ticker_read(), myName, foo);

        printf("[%d:%s] receiving on channel (will timeout)\n", us_ticker_read(), myName);
        status = g_chan.receive(foo, 500);
        if (status != kArTimeoutError)
        {
            printf("[%d:%s] receiving didn't timeout!\n", us_ticker_read(), myName);
        }
        else
        {
            printf("[%d:%s] receiving timed out successfully\n", us_ticker_read(), myName);
        }

        printf("[%d:%s] sleeping a bit\n", us_ticker_read(), myName);
        Ar::Thread::sleep(4000);
    }
}

void y_thread(void * arg)
{
    Ar::Thread * self = Ar::Thread::getCurrent();
    const char * myName = self->getName();
    ar_status_t status;

    while (1)
    {
        printf("[%d:%s] sending to channel\n", us_ticker_read(), myName);
        g_chan.send(128);
        printf("[%d:%s] sent to channel\n", us_ticker_read(), myName);

        printf("[%d:%s] sending to channel\n", us_ticker_read(), myName);
        g_chan.send(256);
        printf("[%d:%s] sent to channel\n", us_ticker_read(), myName);

        printf("[%d:%s] sleeping a bit\n", us_ticker_read(), myName);
        Ar::Thread::sleep(2000);

        printf("[%d:%s] sending on channel (will timeout)\n", us_ticker_read(), myName);
        status = g_chan.send(1, 2);
        if (status != kArTimeoutError)
        {
            printf("[%d:%s] sending didn't timeout!\n", us_ticker_read(), myName);
        }
        else
        {
            printf("[%d:%s] sending timed out successfully\n", us_ticker_read(), myName);
        }
    }
}

// #define CPPSTR #__cplusplus

void dyn_test(void * arg)
{
    printf("hi from dyn_test\n");
    printf("__cplusplus = '%d'\n", __cplusplus);
}

void main_thread(void * arg)
{
    Ar::Thread * self = Ar::Thread::getCurrent();
    const char * myName = self->getName();

    printf("[%d:%s] Main thread is running\r\n", us_ticker_read(), myName);

    g_testCase.init();
    g_testCase.run();

    g_xThread.resume();
    g_yThread.resume();

    Foo * foo = new Foo;
    g_fooThread.init("foo", foo, &Foo::entry, 0, 120);
    g_fooThread.resume();

    g_dyn = new Ar::Thread("dyn", dyn_test, 0, 512, 120);
    g_dyn->resume();

    printf("[%d:%s] goodbye!\r\n", us_ticker_read(), myName);
}

int main(void)
{
    us_ticker_init();

    printf("Running test...\r\n");

#if 0
    printf("sizeof(Thread)=%d\r\n", sizeof(Ar::Thread));
    printf("sizeof(Channel)=%d\r\n", sizeof(Ar::Channel));
    printf("sizeof(Semaphore)=%d\r\n", sizeof(Ar::Semaphore));
    printf("sizeof(Mutex)=%d\r\n", sizeof(Ar::Mutex));
    printf("sizeof(Queue)=%d\r\n", sizeof(Ar::Queue));
    printf("sizeof(Timer)=%d\r\n", sizeof(Ar::Timer));
#endif

#if defined(__ICCARM__)
    main_thread(0);
    Ar::Thread::getCurrent()->suspend();
#else
//     g_mainThread.init("main", main_thread, 0, 56);
    g_mainThread.resume();
    ar_kernel_run();
    Ar::_halt();
#endif
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
