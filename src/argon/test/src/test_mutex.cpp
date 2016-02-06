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
#include "test_mutex.h"

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

void TestMutex1::run()
{
    m_mutex.init("mutex");
    m_aThread.init("a", this, &TestMutex1::a_thread, 60);
    m_bThread.init("b", this, &TestMutex1::b_thread, 70);
}

void TestMutex1::a_thread()
{
    printHello();

    while (1)
    {
//         log("sleeping for 2 sec");
//         Ar::Thread::sleep(2000);

        log("at priority %d", self()->getPriority());
        ASSERT_TRUE(self()->getPriority() == 60, "priority check");
        log("getting mutex");
        ASSERT_EQUALS(m_mutex.getOwner(), (void *)NULL, "no mutex owner");
        m_mutex.get();
        log("got mutex");
        ASSERT_EQUALS(m_mutex.getOwner(), static_cast<ar_thread_t*>(&m_aThread), "thread a is mutex owner");
        log("at priority %d", self()->getPriority());
        ASSERT_TRUE(self()->getPriority() == 60, "priority check");

        log("sleeping for 5 sec");
        Ar::Thread::sleep(5000);
        log("at priority %d", self()->getPriority());
        ASSERT_TRUE(self()->getPriority() == 70, "priority check");

        log("putting mutex");
        m_mutex.put();
        log("at priority %d", self()->getPriority());
        ASSERT_TRUE(self()->getPriority() == 60, "priority check");

        log("sleeping for 5 sec");
        Ar::Thread::sleep(5000);
    }
}

void TestMutex1::b_thread()
{
    printHello();

    while (1)
    {
        log("at priority %d", self()->getPriority());
        ASSERT_TRUE(self()->getPriority() == 70, "priority check");
        log("sleeping for 4 sec");
        Ar::Thread::sleep(4000);

        log("getting mutex");
        m_mutex.get();
        log("got mutex");
        ASSERT_EQUALS(m_mutex.getOwner(), static_cast<ar_thread_t*>(&m_bThread), "thread a is mutex owner");
        m_mutex.put();
        log("just put mutex");
        ASSERT_EQUALS(m_mutex.getOwner(), (void *)NULL, "no mutex owner");

        log("sleeping for 5 sec");
        Ar::Thread::sleep(5000);
    }
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
