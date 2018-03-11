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
 * @brief Implementation of Ar microkernel queue.
 */

#include "ar_internal.h"
#include <string.h>

using namespace Ar;

//------------------------------------------------------------------------------
// Macros
//------------------------------------------------------------------------------

//! Returns a uint8_t * to the first byte of element @a i of the queue.
//!
//! @param q The queue object.
//! @param i Index of the queue element, base 0.
#define QUEUE_ELEMENT(q, i) (&(q)->m_elements[(q)->m_elementSize * (i)])

//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------

static ar_status_t ar_queue_send_internal(ar_queue_t * queue, const void * element, uint32_t timeout);
static void ar_queue_deferred_send(void * object, void * object2);

//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------

// See ar_kernel.h for documentation of this function.
ar_status_t ar_queue_create(ar_queue_t * queue, const char * name, void * storage, unsigned elementSize, unsigned capacity)
{
    if (!queue || !storage || !elementSize || !capacity)
    {
        return kArInvalidParameterError;
    }
    if (ar_port_get_irq_state())
    {
        return kArNotFromInterruptError;
    }

    memset(queue, 0, sizeof(ar_queue_t));

    queue->m_name = name ? name : AR_ANONYMOUS_OBJECT_NAME;
    queue->m_elements = reinterpret_cast<uint8_t *>(storage);
    queue->m_elementSize = elementSize;
    queue->m_capacity = capacity;

    queue->m_runLoopNode.m_obj = queue;

#if AR_GLOBAL_OBJECT_LISTS
    queue->m_createdNode.m_obj = queue;
    g_ar_objects.queues.add(&queue->m_createdNode);
#endif // AR_GLOBAL_OBJECT_LISTS

    return kArSuccess;
}

// See ar_kernel.h for documentation of this function.
ar_status_t ar_queue_delete(ar_queue_t * queue)
{
    if (!queue)
    {
        return kArInvalidParameterError;
    }
    if (ar_port_get_irq_state())
    {
        return kArNotFromInterruptError;
    }

    // Unblock all threads blocked on this queue.
    ar_thread_t * thread;
    while (queue->m_sendBlockedList.m_head)
    {
        thread = queue->m_sendBlockedList.getHead<ar_thread_t>();
        thread->unblockWithStatus(queue->m_sendBlockedList, kArObjectDeletedError);
    }

    while (queue->m_receiveBlockedList.m_head)
    {
        thread = queue->m_receiveBlockedList.getHead<ar_thread_t>();
        thread->unblockWithStatus(queue->m_receiveBlockedList, kArObjectDeletedError);
    }

#if AR_GLOBAL_OBJECT_LISTS
    g_ar_objects.queues.remove(&queue->m_createdNode);
#endif // AR_GLOBAL_OBJECT_LISTS

    return kArSuccess;
}

static ar_status_t ar_queue_send_internal(ar_queue_t * queue, const void * element, uint32_t timeout)
{
    KernelLock guard;

    // Check for full queue.
    while (queue->m_count >= queue->m_capacity)
    {
        // If the queue is full and a zero timeout was given, return immediately.
        if (timeout == kArNoTimeout)
        {
            return kArQueueFullError;
        }

        // Otherwise block until the queue has room.
        ar_thread_t * thread = g_ar.currentThread;
        thread->block(queue->m_sendBlockedList, timeout);

        // We're back from the scheduler.
        // Check for errors and exit early if there was one.
        if (thread->m_unblockStatus != kArSuccess)
        {
            // Probably timed out waiting for room in the queue.
            queue->m_sendBlockedList.remove(&thread->m_blockedNode);
            return thread->m_unblockStatus;
        }
    }

    // Copy queue element into place.
    uint8_t * elementSlot = QUEUE_ELEMENT(queue, queue->m_tail);
    memcpy(elementSlot, element, queue->m_elementSize);

    // Update queue tail pointer and count.
    if (++queue->m_tail >= queue->m_capacity)
    {
        queue->m_tail = 0;
    }
    ++queue->m_count;

    // Are there any threads waiting to receive?
    if (queue->m_receiveBlockedList.m_head)
    {
        // Unblock the head of the blocked list.
        ar_thread_t * thread = queue->m_receiveBlockedList.getHead<ar_thread_t>();
        thread->unblockWithStatus(queue->m_receiveBlockedList, kArSuccess);
    }
    // Is the queue associated with a runloop?
    else if (queue->m_runLoop)
    {
        // Add this queue to the list of pending queues for the runloop, but first check
        // whether the queue is already pending so we don't attempt to add it twice.
        if (!queue->m_runLoop->m_queues.contains(&queue->m_runLoopNode))
        {
            queue->m_runLoop->m_queues.add(queue);
            ar_runloop_wake(queue->m_runLoop);
        }
    }

    return kArSuccess;
}

static void ar_queue_deferred_send(void * object, void * object2)
{
    ar_queue_send_internal(reinterpret_cast<ar_queue_t *>(object), object2, kArNoTimeout);
}

// See ar_kernel.h for documentation of this function.
ar_status_t ar_queue_send(ar_queue_t * queue, const void * element, uint32_t timeout)
{
    if (!queue || !element)
    {
        return kArInvalidParameterError;
    }

    // Handle irq state by deferring the operation.
    if (ar_port_get_irq_state())
    {
        return g_ar.deferredActions.post(ar_queue_deferred_send, queue, const_cast<void *>(element));
    }

    return ar_queue_send_internal(queue, element, timeout);
}

// See ar_kernel.h for documentation of this function.
ar_status_t ar_queue_receive(ar_queue_t * queue, void * element, uint32_t timeout)
{
    if (!queue || !element)
    {
        return kArInvalidParameterError;
    }
    if (ar_port_get_irq_state())
    {
        return kArNotFromInterruptError;
    }

    {
        KernelLock guard;

        // Check for empty queue.
        while (queue->m_count == 0)
        {
            if (timeout == kArNoTimeout)
            {
                return kArQueueEmptyError;
            }

            // Otherwise block until the queue has room.
            ar_thread_t * thread = g_ar.currentThread;
            thread->block(queue->m_receiveBlockedList, timeout);

            // We're back from the scheduler.
            // Check for errors and exit early if there was one.
            if (thread->m_unblockStatus != kArSuccess)
            {
                // Timed out waiting for the queue to not be empty, or another error occurred.
                queue->m_receiveBlockedList.remove(&thread->m_blockedNode);
                return thread->m_unblockStatus;
            }
        }

        // Read out data.
        uint8_t * elementSlot = QUEUE_ELEMENT(queue, queue->m_head);
        memcpy(element, elementSlot, queue->m_elementSize);

        // Update queue head and count.
        if (++queue->m_head >= queue->m_capacity)
        {
            queue->m_head = 0;
        }
        --queue->m_count;

        // Are there any threads waiting to send?
        if (queue->m_sendBlockedList.m_head)
        {
            // Unblock the head of the blocked list.
            ar_thread_t * thread = queue->m_sendBlockedList.getHead<ar_thread_t>();
            thread->unblockWithStatus(queue->m_sendBlockedList, kArSuccess);
        }
    }

    return kArSuccess;
}

// See ar_kernel.h for documentation of this function.
const char * ar_queue_get_name(ar_queue_t * queue)
{
    return queue ? queue->m_name : NULL;
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
