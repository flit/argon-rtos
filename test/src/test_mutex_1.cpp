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
// Code
//------------------------------------------------------------------------------

void TestMutex1::run()
{
    m_mutex.init("mutex");
    
    m_aThread.init("a", this, &TestMutex1::a_thread, 0, 60);
    m_aThread.resume();

    m_bThread.init("b", this, &TestMutex1::b_thread, 0, 70);
    m_bThread.resume();

//     m_cThread.init("c", this, &TestMutex1::c_thread, 0, 80);
//     m_cThread.resume();
// 
//     m_dThread.init("d", this, &TestMutex1::d_thread, 0, 90);
//     m_dThread.resume();
}

void TestMutex1::a_thread(void * param)
{
    printHello();
    
    while (1)
    {
//         printf("%s sleeping for 2 sec\r\n", threadIdString());
//         Ar::Thread::sleep(2000);
        
        printf("%s has priority %d\r\n", threadIdString(), self()->getPriority());
        printf("%s getting mutex\r\n", threadIdString());
        m_mutex.get();
        printf("%s got mutex\r\n", threadIdString());
        printf("%s has priority %d\r\n", threadIdString(), self()->getPriority());

        printf("%s sleeping for 5 sec\r\n", threadIdString());
        Ar::Thread::sleep(5000);
        printf("%s has priority %d\r\n", threadIdString(), self()->getPriority());

        printf("%s putting mutex\r\n", threadIdString());
        m_mutex.put();
        printf("%s has priority %d\r\n", threadIdString(), self()->getPriority());

        printf("%s sleeping for 5 sec\r\n", threadIdString());
        Ar::Thread::sleep(5000);
    }
}

void TestMutex1::b_thread(void * param)
{
    printHello();
    
    while (1)
    {
        printf("%s has priority %d\r\n", threadIdString(), self()->getPriority());
        printf("%s sleeping for 4 sec\r\n", threadIdString());
        Ar::Thread::sleep(4000);
        
        printf("%s getting mutex\r\n", threadIdString());
        m_mutex.get();
        printf("%s got mutex\r\n", threadIdString());
        m_mutex.put();
        printf("%s just put mutex\r\n", threadIdString());

        printf("%s sleeping for 5 sec\r\n", threadIdString());
        Ar::Thread::sleep(5000);
    }
}

void TestMutex1::c_thread(void * param)
{
    printHello();
    
    while (1)
    {
        Ar::Thread::sleep(2000);
    }
}

void TestMutex1::d_thread(void * param)
{
    printHello();
    
    while (1)
    {
        Ar::Thread::sleep(2000);
    }
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
