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
#include "test_queue.h"

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

void TestQueue1::run()
{
    m_q.init("q");

    m_producerThread.init("producer", _producer_thread, this, 20);
    m_consumerAThread.init("consumerA", _consumer_a_thread, this, 30);
    m_consumerBThread.init("consumerB", _consumer_b_thread, this, 31);
}

void TestQueue1::_producer_thread(void * arg)
{
    TestQueue1 * _this = (TestQueue1 *)arg;
    _this->producer_thread();
}

void TestQueue1::_consumer_a_thread(void * arg)
{
    TestQueue1 * _this = (TestQueue1 *)arg;
    _this->consumer_a_thread();
}

void TestQueue1::_consumer_b_thread(void * arg)
{
    TestQueue1 * _this = (TestQueue1 *)arg;
    _this->consumer_b_thread();
}

void TestQueue1::producer_thread()
{
    printHello();

    int counter = 0;
    while (1)
    {
        int value = ++counter;
        printf("%s sending %d\r\n", threadIdString(), value);
        m_q.send(value);

        Ar::Thread::sleep(2000);
    }
}

void TestQueue1::consumer_a_thread()
{
    printHello();

    while (1)
    {
        int value = m_q.receive();
        printf("%s received %d\r\n", threadIdString(), value);
    }
}

void TestQueue1::consumer_b_thread()
{
    printHello();

    while (1)
    {
        int value = m_q.receive();
        printf("%s received %d\r\n", threadIdString(), value);
    }
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
