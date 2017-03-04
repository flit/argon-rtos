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
#include "test_sleep.h"

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

void TestSleep1::run()
{
    m_aThread.init("a", _a_thread, this, 60);
    m_bThread.init("b", _b_thread, this, 70);
}

void TestSleep1::_a_thread(void * arg)
{
    TestSleep1 * _this = (TestSleep1 *)arg;
    _this->a_thread();
}

void TestSleep1::_b_thread(void * arg)
{
    TestSleep1 * _this = (TestSleep1 *)arg;
    _this->b_thread();
}

void TestSleep1::a_thread()
{
    printHello();

    while (1)
    {
        printTicks();

        Ar::Thread::sleep(2000);
    }
}

void TestSleep1::b_thread()
{
    printHello();

    while (1)
    {
        printTicks();

        Ar::Thread::sleep(3000);
    }
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
