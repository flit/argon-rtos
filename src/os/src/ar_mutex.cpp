/*
 * Copyright (c) 2013, Chris Reed
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
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
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
 * @file mutex.cpp
 * @brief Source for Ar microkernel mutexes.
 * @ingroup mu
 */

#include "os/ar_kernel.h"
#include <string.h>
#include <assert.h>

using namespace Ar;

//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------

//! The mutex starts out unlocked.
//!
//! @param name The name of the mutex.
//!
//! @retval SUCCCESS
status_t Mutex::init(const char * name)
{
    status_t status = Semaphore::init(name);
    
    if (status == kSuccess)
    {
        // Start without an owner.
        m_owner = NULL;
        m_ownerLockCount = 0;
        
#if AR_GLOBAL_OBJECT_LISTS
        addToCreatedList(g_muAllObjects.m_mutexes);
#endif // AR_GLOBAL_OBJECT_LISTS
    }
    
    return status;
}

void Mutex::cleanup()
{
#if AR_GLOBAL_OBJECT_LISTS
    removeFromCreatedList(g_muAllObjects.m_mutexes);
#endif // AR_GLOBAL_OBJECT_LISTS
}

//! If the thread that already owns the mutex calls get() more than once, a
//! count is incremented rather than attempting to decrement the underlying
//! semaphore again. The converse is true for put(), thus allowing a
//! thread to lock a mutex any number of times as long as there are matching
//! get() and put() calls.
//!
//! @param timeout The maximum number of ticks that the caller is willing to
//!     wait in a blocked state before the lock can be obtained. If this
//!     value is 0, or #kNoTimeout, then this method will return immediately
//!     if the lock cannot be obtained. Setting the timeout to
//!     #kInfiniteTimeout will cause the thread to wait forever for a chance
//!     to get the lock.
//!
//! @retval kSuccess The mutex was obtained without error.
//! @retval ERROR_MU_TIMEOUT The specified amount of time has elapsed before the
//!     mutex could be obtained.
//! @retval ERROR_MU_OBJECT_DELETED Another thread deleted the semaphore while the
//!     caller was blocked on it.
status_t Mutex::get(uint32_t timeout)
{
    // If this thread already owns the mutex, just increment the count.
    if (g_ar_currentThread == m_owner)
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
            m_owner = g_ar_currentThread;
            m_ownerLockCount++;
        }
        
        return result;
    }
}

//! Only the owning thread is allowed to unlock the mutex. If the owning thread
//! has called get() multiple times, it must also call put() the same number of
//! time before the underlying semaphore is actually released. It is illegal
//! to call put() when the mutex is not owned by the calling thread.
void Mutex::put()
{
    assert(g_ar_currentThread == m_owner && m_ownerLockCount > 0);
    
    if (--m_ownerLockCount == 0)
    {
        // The lock count has reached zero, so put the semaphore and clear the owner.
        // The owner is cleared first since putting the sem can cause us to
        // switch threads.
        m_owner = NULL;
        Semaphore::put();
    }
}

