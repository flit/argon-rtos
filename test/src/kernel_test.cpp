/*
 * Copyright (c) 2014 Immo Software
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
#include "argon/test/kernel_test.h"
#include <stdio.h>
#include <stdarg.h>

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

const char * KernelTest::threadIdString() const
{
    static char idString[32];
    snprintf(idString, sizeof(idString), "[%d:%s]", ar_get_tick_count(), self()->getName());
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

void KernelTest::log(const char * format...)
{
    va_list args;
    va_start(args, format);
    static char msg[128];
    vsnprintf(msg, sizeof(msg), format, args);
    printf("%s %s\n", threadIdString(), msg);
    va_end(args);
}

void KernelTest::assert_true(bool predicate, const char * msg, const char * desc, const char * file, int line)
{
    if (!predicate)
    {
        log("Assertion failed: %s (%s was not true) [%s:%d]\n", msg, desc, file, line);
    }
    else
    {
        log("Asserted passed: %s (%s) [%s:%d]\n", msg, desc, file, line);
    }
}

void KernelTest::assert_false(bool predicate, const char * msg, const char * desc, const char * file, int line)
{
    if (predicate)
    {
        log("Assertion failed: %s (%s was not false) [%s:%d]\n", msg, desc, file, line);
    }
    else
    {
        log("Asserted passed: %s !(%s) [%s:%d]\n", msg, desc, file, line);
    }
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
