/*
 * Copyright (c) 2007-2013 Immo Software
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
/*!
 * @file
 * @brief Source for Ar base class.
 * @ingroup ar
 */

#include "os/ar_kernel.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

using namespace Ar;

//------------------------------------------------------------------------------
// Defines
//------------------------------------------------------------------------------

#ifndef MU_ENABLE_IDLE_SLEEP
    //! Controls whether the idle thread puts the processor to sleep
    //! until the next interrupt. Set to 1 to enable.
    #define MU_ENABLE_IDLE_SLEEP 0
#endif

#ifndef MU_ENABLE_SYSTEM_LOAD
    //! When set to 1 the idle thread will compute the system load percentage.
    #define MU_ENABLE_SYSTEM_LOAD 1
#endif

#ifndef MU_PRINT_SYSTEM_LOAD
    //! Controls if the system load is printed.
    #define MU_PRINT_SYSTEM_LOAD 0
#endif

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

//! @param name The object's name, a copy of which is made in the object itself. If
//!     @a name is NULL, the object's name is set to the empty string.
//!
//! @retval kSuccess Initialisation was successful.
status_t NamedObject::init(const char * name)
{
    // Copy name into the object.
    if (name)
    {
        strncpy(m_name, name, sizeof(m_name));
    }
    else
    {
        m_name[0] = 0;
    }
    
    return kSuccess;
}

#if AR_GLOBAL_OBJECT_LISTS

void NamedObject::addToCreatedList(NamedObject * & listHead)
{
    m_nextCreated = NULL;

    // handle an empty list
    if (!listHead)
    {
        listHead = this;
        return;
    }
    
    // find the end of the list
    NamedObject * thread = listHead;

    while (thread)
    {
        if (!thread->m_nextCreated)
        {
            thread->m_nextCreated = this;
            break;
        }
        thread = thread->m_nextCreated;
    }
}

void NamedObject::removeFromCreatedList(NamedObject * & listHead)
{
    // the list must not be empty
    assert(listHead != NULL);

    if (listHead == this)
    {
        // special case for removing the list head
        listHead = m_nextCreated;
    }
    else
    {
        NamedObject * item = listHead;
        while (item)
        {
            if (item->m_nextCreated == this)
            {
                item->m_nextCreated = m_nextCreated;
                return;
            }

            item = item->m_nextCreated;
        }
    }
}

#endif // AR_GLOBAL_OBJECT_LISTS

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
