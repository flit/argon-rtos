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
#include <stdio.h>

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------

void main_thread(void * arg);

class Foo
{
public:

    void my_entry() //void * param)
    {
        while (1)
        {
            printf("hi from Foo!\n");
            Ar::Thread::sleep(1000);
        }
    }
};

//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------

#if !defined(__ICCARM__)
Ar::ThreadWithStack<512> g_mainThread("main", main_thread, 0, 128);
#endif

// TEST_CASE_CLASS g_testCase;

Ar::TypedChannel<int> g_chan("c");

Ar::Thread g_fooThread;

Ar::Thread * g_dyn;

Ar::Mutex g_someMutex("foobar");

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

#if defined(__GNUC__)
/* prevents the exception handling name demangling code getting pulled in */
namespace __gnu_cxx {
    void __verbose_terminate_handler() {
        Ar::_halt();
    }
}
extern "C" __attribute__((weak)) void __cxa_pure_virtual(void);
extern "C" __attribute__((weak)) void __cxa_pure_virtual(void) {
//     exit(1);
}
#endif

uint64_t us_ticker_read()
{
    return *(volatile uint64_t *)(&TSTMR0->L);
}

#if defined(__MICROLIB)
#pragma weak __aeabi_assert
extern "C" void __aeabi_assert(const char *s, const char *f, int l)
{
    Ar::_halt();
}
#endif

void dyn_test(void * arg)
{
    printf("hi from dyn_test\n");
    printf("__cplusplus = '%d'\n", __cplusplus);
}

Ar::Thread g_spinner;
// Ar::Semaphore sem("s", 0);
void sem_spin_thread(void * arg)
{
    Ar::Semaphore sem("s", 0);

    while (1)
    {
        g_someMutex.get();
        sem.put();
        sem.get();
        g_someMutex.put();
    }
}

Ar::Semaphore tsem("tsem", 0);
void ticker_thread(void * arg)
{
    while (1)
    {
        tsem.get();
    }
}

void created_thread(void * arg)
{
    Ar::Semaphore * doneSem = reinterpret_cast<Ar::Semaphore *>(arg);
    printf("hello from a created thread\n");
    doneSem->put();
}

void my_timer_fn(Ar::Timer * t, void * arg)
{
    printf("x\n");
}
Ar::Timer g_myTimer("mine", my_timer_fn, 0, kArPeriodicTimer, 1500);

void creator_thread(void * arg)
{
    while (1)
    {
        Ar::Semaphore * doneSem = new Ar::Semaphore("done", 0);
        Ar::Thread * created = new Ar::Thread("created", created_thread, doneSem, 512, 97);

        printf("creator thread: waiting on done sem\n");
        doneSem->get();
        printf("creator thread: got done sem\n");

        delete created;
        delete doneSem;

        Ar::Thread::sleep(1250);
    }
}
Ar::Thread g_creatorThread("creator", creator_thread, 0, 512, 96);

void main_thread(void * arg)
{
    Ar::Thread * self = Ar::Thread::getCurrent();
    const char * myName = self->getName();

    printf("[%d:%s] Main thread is running\r\n", us_ticker_read(), myName);

    g_myTimer.start();

//     while (1)
//     {
//         printf("hi\n");
//         Ar::Thread::sleep(1000);
//     }

//     g_testCase.init();
//     g_testCase.run();

    Foo * foo = new Foo;
    g_fooThread.init("foo", foo, &Foo::my_entry, NULL, 512, 120);

    g_dyn = new Ar::Thread("dyn", dyn_test, 0, 512, 120);

    g_spinner.init("spinner", sem_spin_thread, 0, NULL, 512, 70);

    printf("[%d:%s] goodbye!\r\n", us_ticker_read(), myName);

}

int main(void)
{
//     us_ticker_init();

    printf("Running test...\r\n");

#if 1
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
//     g_mainThread.resume();
    ar_kernel_run();
    Ar::_halt();
#endif

    return 0;
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
