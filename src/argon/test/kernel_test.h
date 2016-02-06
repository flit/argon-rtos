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

#if !defined(_KERNEL_TEST_H_)
#define _KERNEL_TEST_H_

#include "argon/argon.h"
#include <stdio.h>

#define _STRINGIFY_B(x) #x
#define _STRINGIFY(x) _STRINGIFY_B(x)

#define ASSERT_TRUE(p, m) do { assert_true((p), m, _STRINGIFY(p), __FILE__, __LINE__); } while (0)
#define ASSERT_FALSE(p, m) do { assert_false((p), m, _STRINGIFY(p), __FILE__, __LINE__); } while (0)
#define ASSERT_EQUALS(v, e, m) do { assert_equals((v), (e), m, _STRINGIFY(v), _STRINGIFY(e), __FILE__, __LINE__); } while (0)

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

/*!
 * @brief Abstract kernel test class.
 */
class KernelTest
{
public:
    KernelTest() {}

    virtual ~KernelTest() {}

    virtual void init() {}
    virtual void run()=0;
    virtual void teardown() {}

    Ar::Thread * self() const { return Ar::Thread::getCurrent(); }
    const char * threadIdString() const;

    void log(const char * format...);

    void assert_true(bool predicate, const char * msg, const char * desc, const char * file, int line);
    void assert_false(bool predicate, const char * msg, const char * desc, const char * file, int line);

    template <typename V, typename E>
    void assert_equals(V value, E expected, const char * msg, const char * vdesc, const char * edesc, const char * file, int line) {
        if ((value) != (expected))
        {
            log("Assertion failed: %s (actual %s != expected %s) [%s:%d]\n", msg, vdesc, edesc, file, line);
        }
        else
        {
            log("Asserted passed: %s (%s == %s) [%s:%d]\n", msg, vdesc, edesc, file, line);
        }
    }

protected:

    void printHello();
    void printTicks();

};

#endif // _KERNEL_TEST_H_
//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
