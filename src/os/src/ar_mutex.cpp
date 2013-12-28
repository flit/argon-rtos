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
 * @brief Source for Ar microkernel mutexes.
 */

#include "os/ar_kernel.h"
#include <string.h>
#include <assert.h>

using namespace Ar;

//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------

// See ar_kernel.h for documentation of this function.
status_t Mutex::init(const char * name)
{
    status_t status = Semaphore::init(name);
    
    if (status == kSuccess)
    {
        // Start without an owner.
        m_owner = NULL;
        m_ownerLockCount = 0;
        
#if AR_GLOBAL_OBJECT_LISTS
        addToCreatedList(g_allObjects.m_mutexes);
#endif // AR_GLOBAL_OBJECT_LISTS
    }
    
    return status;
}

// See ar_kernel.h for documentation of this function.
Mutex::~Mutex()
{
#if AR_GLOBAL_OBJECT_LISTS
    removeFromCreatedList(g_allObjects.m_mutexes);
#endif // AR_GLOBAL_OBJECT_LISTS
}

// See ar_kernel.h for documentation of this function.
status_t Mutex::get(uint32_t timeout)
{
    // If this thread already owns the mutex, just increment the count.
    if (Thread::getCurrent() == m_owner)
    {
        m_ownerLockCount++;
        return kSuccess;
    }
    // Otherwise attempt to get the mutex.
    else
    {
        status_t result = Semaphore::get(timeout);
        if (result == kSuccess)
        {
            // Set the owner now that we own the lock.
            m_owner = Thread::getCurrent();
            m_ownerLockCount++;
        }
        
        return result;
    }
}

// See ar_kernel.h for documentation of this function.
status_t Mutex::put()
{
    if (m_ownerLockCount == 0)
    {
        return kAlreadyUnlockedError;
    }
    if (Thread::getCurrent() != m_owner)
    {
        return kNotOwnerError;
    }
    
    if (--m_ownerLockCount == 0)
    {
        // The lock count has reached zero, so put the semaphore and clear the owner.
        // The owner is cleared first since putting the sem can cause us to
        // switch threads.
        m_owner = NULL;
        return Semaphore::put();
    }
    
    return kSuccess;
}

