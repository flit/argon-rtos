/*
 * Copyright (c) 2010-2012, Freescale Semiconductor, Inc.
 * Copyright (c) 2015 Immo Software.
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

#include "ar_asm_macros.h"

        _CODE_SECTION(.text)
        _THUMB

        _EXPORT(ar_atomic_cas)
        _EXPORT(ar_atomic_add)

        // bool ar_atomic_cas(volatile int32_t * value, int32_t expectedValue, int32_t newValue)
        _FN_BEGIN(ar_atomic_cas)
        _FN_DECL(ar_atomic_cas)
_FN_LABEL(ar_atomic_cas)
        _FN_BEGIN_POST
        _FN_CANT_UNWIND

        dsb                             // data sync memory barrier
        ldrex       r3, [r0]            // load exclusive current value
        cmp         r3, r1              // does current value equal expected value?
        ittee       eq
        strexeq     r12, r2, [r0]       // if it does, then store exclusive the new value
        eoreq       r12, r12, #1        // invert the strex result
        clrexne                         // else, clear the exclusive load
        movne       r12, #0             // clear return value
        mov         r0, r12             // return the result of strex
        bx          lr                  // return to caller

        _FN_END(ar_atomic_cas)
        _FN_SIZE(ar_atomic_cas)

        // int32_t ar_atomic_add(volatile int32_t * value, int32_t delta)
        _FN_BEGIN(ar_atomic_add)
        _FN_DECL(ar_atomic_add)
_FN_LABEL(ar_atomic_add)
        _FN_BEGIN_POST
        _FN_CANT_UNWIND

        dsb                             // data sync barrier
        ldrex       r3, [r0]            // load exclusive original value
        add         r2, r3, r1          // add delta into original
        strex       r12, r2, [r0]       // store exclusive the new value, r12==0 if store was successful
        cmp         r12, #0             // check if the store succeeded
        bne         ar_atomic_add          // if the store failed, try again
        mov         r0, r3              // return the original value
        bx          lr

        _FN_END(ar_atomic_add)
        _FN_SIZE(ar_atomic_add)

        _ALIGN(4)

        _END

// ------------------------------------------------------------
// End of atomics.s
// ------------------------------------------------------------
