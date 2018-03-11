/*
 * Copyright (c) 2007-2018 Immo Software
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
// Prototypes
//------------------------------------------------------------------------------

static ar_status_t ar_mutex_get_internal(ar_mutex_t * mutex, uint32_t timeout);
static void ar_mutex_deferred_get(void * object, void * object2);
static ar_status_t ar_mutex_put_internal(ar_mutex_t * mutex);
static void ar_mutex_deferred_put(void * object, void * object2);

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
    if (ar_port_get_irq_state())
    {
        return kArNotFromInterruptError;
    }

    memset(mutex, 0, sizeof(ar_mutex_t));
    mutex->m_name = name ? name : AR_ANONYMOUS_OBJECT_NAME;

    // Set the blocked list to sort by priority.
    mutex->m_blockedList.m_predicate = ar_thread_sort_by_priority;

#if AR_GLOBAL_OBJECT_LISTS
    mutex->m_createdNode.m_obj = mutex;
    g_ar_objects.mutexes.add(&mutex->m_createdNode);
#endif // AR_GLOBAL_OBJECT_LISTS

    return kArSuccess;
}

// See ar_kernel.h for documentation of this function.
//! @todo Return error when deleting a mutex that is still locked.
ar_status_t ar_mutex_delete(ar_mutex_t * mutex)
{
    if (!mutex)
    {
        return kArInvalidParameterError;
    }
    if (ar_port_get_irq_state())
    {
        return kArNotFromInterruptError;
    }

#if AR_GLOBAL_OBJECT_LISTS
    g_ar_objects.mutexes.remove(&mutex->m_createdNode);
#endif // AR_GLOBAL_OBJECT_LISTS

    return kArSuccess;
}

static ar_status_t ar_mutex_get_internal(ar_mutex_t * mutex, uint32_t timeout)
{
    KernelLock guard;

    // If this thread already owns the mutex, just increment the count.
    if (g_ar.currentThread == mutex->m_owner)
    {
        ++mutex->m_ownerLockCount;
    }
    // Otherwise attempt to get the mutex.
    else
    {
        // Will we block?
        while (mutex->m_ownerLockCount != 0)
        {
            // Return immediately if the timeout is 0.
            if (timeout == kArNoTimeout)
            {
                return kArTimeoutError;
            }

            // Check if we need to hoist the owning thread's priority to our own.
            ar_thread_t * self = g_ar.currentThread;
            assert(mutex->m_owner);
            if (self->m_priority > mutex->m_owner->m_priority)
            {
                if (!mutex->m_originalPriority)
                {
                    mutex->m_originalPriority = mutex->m_owner->m_priority;
                }
                mutex->m_owner->m_priority = self->m_priority;
            }

            // Block this thread on the mutex.
            self->block(mutex->m_blockedList, timeout);

            // We're back from the scheduler. We'll loop and recheck the ownership counter, in case
            // a higher priority thread grabbed the lock between when we were unblocked and when we
            // actually started running.

            // Check for errors and exit early if there was one.
            if (self->m_unblockStatus != kArSuccess)
            {
                //! @todo Need to handle timeout after hoisting the owner thread.
                // Failed to gain the mutex, probably due to a timeout.
                mutex->m_blockedList.remove(&self->m_blockedNode);
                return self->m_unblockStatus;
            }
        }

        // Take ownership of the lock.
        assert(mutex->m_owner == NULL && mutex->m_ownerLockCount == 0);
        mutex->m_owner = g_ar.currentThread;
        ++mutex->m_ownerLockCount;
    }

    return kArSuccess;
}

static void ar_mutex_deferred_get(void * object, void * object2)
{
    ar_mutex_get_internal(reinterpret_cast<ar_mutex_t *>(object), kArNoTimeout);
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
        return g_ar.deferredActions.post(ar_mutex_deferred_get, mutex);
    }

    return ar_mutex_get_internal(mutex, timeout);
}

static ar_status_t ar_mutex_put_internal(ar_mutex_t * mutex)
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
        // The lock count has reached zero, so clear the owner.
        mutex->m_owner = NULL;

        // Restore this thread's priority if it had been raised.
        uint8_t original = mutex->m_originalPriority;
        if (original)
        {
            mutex->m_originalPriority = 0;
            ar_thread_set_priority(self, original);
        }

        // Unblock a waiting thread.
        if (mutex->m_blockedList.m_head)
        {
            // Unblock the head of the blocked list.
            ar_thread_t * thread = mutex->m_blockedList.getHead<ar_thread_t>();
            thread->unblockWithStatus(mutex->m_blockedList, kArSuccess);
        }
    }

    return kArSuccess;
}

static void ar_mutex_deferred_put(void * object, void * object2)
{
    ar_mutex_put_internal(reinterpret_cast<ar_mutex_t *>(object));
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
        return g_ar.deferredActions.post(ar_mutex_deferred_put, mutex);
    }

    return ar_mutex_put_internal(mutex);
}

// See ar_kernel.h for documentation of this function.
bool ar_mutex_is_locked(ar_mutex_t * mutex)
{
    return mutex ? mutex->m_ownerLockCount != 0 : false;
}

// See ar_kernel.h for documentation of this function.
ar_thread_t * ar_mutex_get_owner(ar_mutex_t * mutex)
{
    return mutex ? (ar_thread_t *)mutex->m_owner : NULL;
}

// See ar_kernel.h for documentation of this function.
const char * ar_mutex_get_name(ar_mutex_t * mutex)
{
    return mutex ? mutex->m_name : NULL;
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
