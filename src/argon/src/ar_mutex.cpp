/*
 * Copyright (c) 2007-2016 Immo Software
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

#include "ar_internal.h"
#include <string.h>
#include <assert.h>

using namespace Ar;

//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------

// See ar_kernel.h for documentation of this function.
ar_status_t ar_mutex_create(ar_mutex_t * mutex, const char * name)
{
    if (!mutex)
    {
        return kArInvalidParameterError;
    }

    ar_status_t status = ar_semaphore_create(&mutex->m_sem, name, 1);

    if (status == kArSuccess)
    {
        // Start without an owner.
        mutex->m_owner = NULL;
        mutex->m_ownerLockCount = 0;
        mutex->m_originalPriority = 0;

        // Set the blocked list to sort by priority.
        mutex->m_sem.m_blockedList.m_predicate = ar_thread_sort_by_priority;

#if AR_GLOBAL_OBJECT_LISTS
        mutex->m_sem.m_createdNode.m_obj = mutex;
        g_ar_objects.semaphores.remove(&mutex->m_sem.m_createdNode);
        g_ar_objects.mutexes.add(&mutex->m_sem.m_createdNode);
#endif // AR_GLOBAL_OBJECT_LISTS
    }

    return status;
}

// See ar_kernel.h for documentation of this function.
//! @todo Return error when deleting a mutex that is still locked.
ar_status_t ar_mutex_delete(ar_mutex_t * mutex)
{
    if (!mutex)
    {
        return kArInvalidParameterError;
    }

#if AR_GLOBAL_OBJECT_LISTS
    g_ar_objects.mutexes.remove(&mutex->m_sem.m_createdNode);
#endif // AR_GLOBAL_OBJECT_LISTS

    return kArSuccess;
}

ar_status_t ar_mutex_get_internal(ar_mutex_t * mutex, uint32_t timeout)
{
    KernelLock guard;

    // If this thread already owns the mutex, just increment the count.
    if (g_ar.currentThread == mutex->m_owner)
    {
        ++mutex->m_ownerLockCount;
        return kArSuccess;
    }
    // Otherwise attempt to get the mutex.
    else
    {
        // Will we block?
        if (mutex->m_sem.m_count == 0)
        {
            // Return immediately if the timeout is 0. No reason to call into the sem code.
            if (timeout == kArNoTimeout)
            {
                return kArTimeoutError;
            }

            // Check if we need to hoist the owning thread's priority to our own.
            ar_thread_t * self = g_ar.currentThread;
            if (self->m_priority > mutex->m_owner->m_priority)
            {
                if (!mutex->m_originalPriority)
                {
                    mutex->m_originalPriority = mutex->m_owner->m_priority;
                }
                mutex->m_owner->m_priority = self->m_priority;
            }
        }

        ar_status_t result;
        {
            KernelUnlock unlock;
            result = ar_semaphore_get_internal(&mutex->m_sem, timeout);
        }
        if (result == kArSuccess)
        {
            // Set the owner now that we own the lock.
            mutex->m_owner = g_ar.currentThread;
            ++mutex->m_ownerLockCount;
        }
        else if (result == kArTimeoutError)
        {
            //! @todo Need to handle timeout after hoisting the owner thread.

        }

        return result;
    }
}

// See ar_kernel.h for documentation of this function.
ar_status_t ar_mutex_get(ar_mutex_t * mutex, uint32_t timeout)
{
    if (!mutex)
    {
        return kArInvalidParameterError;
    }

    // Handle irq state by deferring the get.
    if (ar_port_get_irq_state())
    {
        return ar_post_deferred_action(kArDeferredMutexGet, mutex);
    }

    return ar_mutex_get_internal(mutex, timeout);
}

ar_status_t ar_mutex_put_internal(ar_mutex_t * mutex)
{
    ar_status_t result = kArSuccess;

    {
        KernelLock guard;

        // Nothing to do if the mutex is already unlocked.
        if (mutex->m_ownerLockCount == 0)
        {
            return kArAlreadyUnlockedError;
        }

        // Only the owning thread can unlock a mutex.
        ar_thread_t * self = g_ar.currentThread;
        if (self != mutex->m_owner)
        {
            return kArNotOwnerError;
        }

        // We are the owner of the mutex, so decrement its recursive lock count.
        if (--mutex->m_ownerLockCount == 0)
        {
            // The lock count has reached zero, so put the semaphore and clear the owner.
            // The owner is cleared first since putting the sem can cause us to
            // switch threads.
            mutex->m_owner = NULL;

            {
                KernelUnlock unlock;
                result = ar_semaphore_put_internal(&mutex->m_sem);
            }

            // Restore this thread's priority if it had been raised.
            uint8_t original = mutex->m_originalPriority;
            if (original)
            {
                mutex->m_originalPriority = 0;
                ar_thread_set_priority(self, original);
            }
        }
    }

    return result;
}

// See ar_kernel.h for documentation of this function.
ar_status_t ar_mutex_put(ar_mutex_t * mutex)
{
    if (!mutex)
    {
        return kArInvalidParameterError;
    }

    // Handle irq state by deferring the put.
    if (ar_port_get_irq_state())
    {
        return ar_post_deferred_action(kArDeferredMutexPut, mutex);
    }

    return ar_mutex_put_internal(mutex);
}

// See ar_kernel.h for documentation of this function.
bool ar_mutex_is_locked(ar_mutex_t * mutex)
{
    return mutex ? mutex->m_sem.m_count == 0 : false;
}

// See ar_kernel.h for documentation of this function.
ar_thread_t * ar_mutex_get_owner(ar_mutex_t * mutex)
{
    return mutex ? (ar_thread_t *)mutex->m_owner : NULL;
}

// See ar_kernel.h for documentation of this function.
const char * ar_mutex_get_name(ar_mutex_t * mutex)
{
    return mutex ? mutex->m_sem.m_name : NULL;
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
