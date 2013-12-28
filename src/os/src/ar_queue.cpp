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
 * @brief Implementation of Ar microkernel queue.
 */

#include "os/ar_kernel.h"
#include <string.h>

using namespace Ar;

//------------------------------------------------------------------------------
// Macros
//------------------------------------------------------------------------------

//! Returns a uint8_t * to the first byte of element @a i of the queue.
//!
//! @param i Index of the queue element, base 0.
#define QUEUE_ELEMENT(i) (&m_elements[m_elementSize * (i)])

//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------

// See ar_kernel.h for documentation of this function.
status_t Queue::init(const char * name, void * storage, unsigned elementSize, unsigned capacity)
{
    NamedObject::init(name);
    
    m_elements = reinterpret_cast<uint8_t *>(storage);
    m_elementSize = elementSize;
    m_capacity = capacity;
    
    m_head = 0;
    m_tail = 0;
    m_count = 0;
    
    m_sendBlockedList = NULL;
    m_receiveBlockedList = NULL;
    
#if AR_GLOBAL_OBJECT_LISTS
    addToCreatedList(g_allObjects.m_queues);
#endif // AR_GLOBAL_OBJECT_LISTS

    return kSuccess;
}

// See ar_kernel.h for documentation of this function.
Queue::~Queue()
{
#if AR_GLOBAL_OBJECT_LISTS
    removeFromCreatedList(g_allObjects.m_queues);
#endif // AR_GLOBAL_OBJECT_LISTS
}

// See ar_kernel.h for documentation of this function.
status_t Queue::send(const void * element, uint32_t timeout)
{
    IrqDisableAndRestore disableIrq;
    
    // Check for full queue
    if (m_count >= m_capacity)
    {
        // If the queue is full and a zero timeout was given, return immediately.
        if (timeout == kNoTimeout)
        {
            return kQueueFullError;
        }
        
        // Otherwise block until the queue has room.
        Thread * thread = Thread::getCurrent();
        thread->block(m_sendBlockedList, timeout);
        
        // Reenable interrupts to allow switching contexts.
        disableIrq.enable();
        
        // Yield to the scheduler. While other threads are executing, interrupts
        // will be restored to the state on those threads. When we come back to
        // this thread, interrupts will still be disabled.
        Kernel::enterScheduler();
        
        disableIrq.disable();
        
        // We're back from the scheduler. Interrupts are still disabled.
        // Check for errors and exit early if there was one.
        if (thread->m_unblockStatus != kSuccess)
        {
            // Failed to gain the semaphore, probably due to a timeout.
            thread->removeFromBlockedList(m_sendBlockedList);
            return thread->m_unblockStatus;
        }
    }
    
    // fill element
    uint8_t * elementSlot = QUEUE_ELEMENT(m_tail);
    memcpy(elementSlot, element, m_elementSize);
    
    // Update queue pointers
    m_tail = (m_tail + 1) % m_capacity;
    m_count++;
    
    // Are there any threads waiting to receive?
    if (m_receiveBlockedList)
    {
        // Unblock the head of the blocked list.
        Thread * thread = m_receiveBlockedList;
        thread->unblockWithStatus(m_receiveBlockedList, kSuccess);

        // Invoke the scheduler if the unblocked thread is higher priority than the current one.
        if (thread->m_priority > Thread::getCurrent()->m_priority)
        {
            // Reenable interrupts to allow switching contexts.
            disableIrq.enable();
        
            Kernel::enterScheduler();
        }
    }
    
    return kSuccess;
}

// See ar_kernel.h for documentation of this function.
status_t Queue::receive(void * element, uint32_t timeout)
{
    IrqDisableAndRestore disableIrq;
    
    // Check for empty queue
    if (m_count == 0)
    {
        if (timeout == kNoTimeout)
        {
            return kQueueEmptyError;
        }
        
        // Otherwise block until the queue has room.
        Thread * thread = Thread::getCurrent();
        thread->block(m_receiveBlockedList, timeout);
        
        // Reenable interrupts to allow switching contexts.
        disableIrq.enable();
        
        // Yield to the scheduler. While other threads are executing, interrupts
        // will be restored to the state on those threads. When we come back to
        // this thread, interrupts will still be disabled.
        Kernel::enterScheduler();
        
        disableIrq.disable();
        
        // We're back from the scheduler. Interrupts are still disabled.
        // Check for errors and exit early if there was one.
        if (thread->m_unblockStatus != kSuccess)
        {
            // Failed to gain the semaphore, probably due to a timeout.
            thread->removeFromBlockedList(m_receiveBlockedList);
            return thread->m_unblockStatus;
        }
    }
    
    // read out data
    uint8_t * elementSlot = QUEUE_ELEMENT(m_head);
    memcpy(element, elementSlot, m_elementSize);
    
    // update queue
    m_head = (m_head + 1) % m_capacity;
    m_count--;
    
    // Are there any threads waiting to send?
    if (m_sendBlockedList)
    {
        // Unblock the head of the blocked list.
        Thread * thread = m_sendBlockedList;
        thread->unblockWithStatus(m_sendBlockedList, kSuccess);

        // Invoke the scheduler if the unblocked thread is higher priority than the current one.
        if (thread->m_priority > Thread::getCurrent()->m_priority)
        {
            // Reenable interrupts to allow switching contexts.
            disableIrq.enable();
        
            Kernel::enterScheduler();
        }
    }
    
    return kSuccess;
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
