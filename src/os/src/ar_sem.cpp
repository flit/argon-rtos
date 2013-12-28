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
 * @brief Source for Ar microkernel semaphores.
 */

#include "os/ar_kernel.h"
#include <string.h>
#include <assert.h>

using namespace Ar;

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

// See ar_kernel.h for documentation of this function.
status_t Semaphore::init(const char * name, unsigned count)
{
    NamedObject::init(name);
    
    m_count = count;

#if AR_GLOBAL_OBJECT_LISTS
    addToCreatedList(g_allObjects.m_semaphores);
#endif // AR_GLOBAL_OBJECT_LISTS
    
    return kSuccess;
}

// See ar_kernel.h for documentation of this function.
Semaphore::~Semaphore()
{
    // Unblock all threads blocked on this semaphore.
    while (m_blockedList)
    {
        m_blockedList->unblockWithStatus(m_blockedList, kObjectDeletedError);
    }
    
#if AR_GLOBAL_OBJECT_LISTS
    removeFromCreatedList(g_allObjects.m_semaphores);
#endif // AR_GLOBAL_OBJECT_LISTS
}

// See ar_kernel.h for documentation of this function.
status_t Semaphore::get(uint32_t timeout)
{
    // Ensure that only 0 timeouts are specified when called from an IRQ handler.
    if (Kernel::getIrqDepth() > 0 && timeout != 0)
    {
        return kNotFromInterruptError;
    }
    
    IrqDisableAndRestore disableIrq;

    if (m_count == 0)
    {
        // Count is 0, so we must block. Return immediately if the timeout is 0.
        if (timeout == kNoTimeout)
        {
            return kTimeoutError;
        }

        // Block this thread on the semaphore.
        Thread * thread = Thread::getCurrent();
        thread->block(m_blockedList, timeout);

        disableIrq.enable();
        
        // Yield to the scheduler. We'll return when a call to put()
        // wakes this thread. If another thread gains control, interrupts will be
        // set to that thread's last state.
        Kernel::enterScheduler();

        disableIrq.disable();
        
        // We're back from the scheduler. Interrupts are still disabled.
        // Check for errors and exit early if there was one.
        if (thread->m_unblockStatus != kSuccess)
        {
            // Failed to gain the semaphore, probably due to a timeout.
            thread->removeFromBlockedList(m_blockedList);
            return thread->m_unblockStatus;
        }
    }
    
    // Take ownership of the semaphore.
    m_count--;

    return kSuccess;
}

// See ar_kernel.h for documentation of this function.
status_t Semaphore::put()
{
    IrqDisableAndRestore disableIrq;

    // Increment count.
    m_count++;

    // Are there any threads waiting on this semaphore?
    if (m_blockedList)
    {
        // Unblock the head of the blocked list.
        Thread * thread = m_blockedList;
        thread->unblockWithStatus(m_blockedList, kSuccess);

        // Invoke the scheduler if the unblocked thread is higher priority than the current one.
        if (thread->m_priority > Thread::getCurrent()->m_priority)
        {
            disableIrq.enable();
        
            Kernel::enterScheduler();
        }
    }
    
    return kSuccess;
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
