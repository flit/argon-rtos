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

#if !defined(_KERNEL_TESTS_QUEUE_H_)
#define _KERNEL_TESTS_QUEUE_H_

#include "argon/argon.h"
#include "argon/test/kernel_test.h"

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

/*!
 * @brief Queue test.
 */
class TestQueue1 : public KernelTest
{
public:
    TestQueue1() {}

    virtual void run();

protected:

    Ar::ThreadWithStack<512> m_producerThread;
    Ar::ThreadWithStack<512> m_consumerAThread;
    Ar::ThreadWithStack<512> m_consumerBThread;

    Ar::StaticQueue<int, 5> m_q;

    void producer_thread();
    void consumer_a_thread();
    void consumer_b_thread();

    static void _producer_thread(void * arg);
    static void _consumer_a_thread(void * arg);
    static void _consumer_b_thread(void * arg);

};

//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------

#endif // _KERNEL_TESTS_QUEUE_H_
//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
