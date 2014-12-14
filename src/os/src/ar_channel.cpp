/*
 * Copyright (c) 2007-2014 Immo Software
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
 * @brief Source for Ar microkernel channels.
 */

#include "ar_internal.h"
#include <string.h>
#include <assert.h>

using namespace Ar;

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

// See ar_kernel.h for documentation of this function.
ar_status_t ar_channel_create(ar_channel_t * channel, const char * name, uint32_t width)
{
    if (!channel)
    {
        return kArInvalidParameterError;
    }

    memset(channel, 0, sizeof(ar_channel_t));
    channel->m_name = name ? name : AR_ANONYMOUS_OBJECT_NAME;
    channel->m_width = (width == 0) ? sizeof(void *) : width;
    channel->m_createdNode.m_obj = channel;

#if AR_GLOBAL_OBJECT_LISTS
    g_ar.allObjects.channels.add(&channel->m_createdNode);
#endif // AR_GLOBAL_OBJECT_LISTS

    return kArSuccess;
}

// See ar_kernel.h for documentation of this function.
ar_status_t ar_channel_delete(ar_channel_t * channel)
{
    if (!channel)
    {
        return kArInvalidParameterError;
    }

    // Unblock all threads blocked on this channel.
    ar_thread_t * thread;
    while (channel->m_blockedSenders.m_head)
    {
        thread = channel->m_blockedSenders.m_head->getObject<ar_thread_t>();
        thread->unblockWithStatus(channel->m_blockedSenders, kArObjectDeletedError);
    }

    while (channel->m_blockedReceivers.m_head)
    {
        thread = channel->m_blockedReceivers.m_head->getObject<ar_thread_t>();
        thread->unblockWithStatus(channel->m_blockedReceivers, kArObjectDeletedError);
    }

#if AR_GLOBAL_OBJECT_LISTS
    g_ar.allObjects.channels.remove(&channel->m_createdNode);
#endif // AR_GLOBAL_OBJECT_LISTS

    return kArSuccess;
}

// See ar_kernel.h for documentation of this function.
ar_status_t ar_channel_receive(ar_channel_t * channel, void * value, uint32_t timeout)
{
    if (!channel)
    {
        return kArInvalidParameterError;
    }

    // Ensure that only 0 timeouts are specified when called from an IRQ handler.
    if (g_ar.irqDepth > 0 && timeout != 0)
    {
        return kArNotFromInterruptError;
    }

    IrqDisableAndRestore disableIrq;
    ar_thread_t * thread;

    // Are there any blocked senders?
    if (channel->m_blockedSenders.isEmpty())
    {
        // Nobody waiting to send, so we must block. Return immediately if the timeout is 0.
        if (timeout == kArNoTimeout)
        {
            return kArTimeoutError;
        }

        // Block this thread on the channel.
        thread = g_ar.currentThread;
        thread->m_channelData = value;
        thread->block(channel->m_blockedReceivers, timeout);

        disableIrq.enable();

        // Yield to the scheduler. We'll return when a call to send()
        // wakes this thread. If another thread gains control, interrupts will be
        // set to that thread's last state.
        ar_kernel_enter_scheduler();

        disableIrq.disable();

        // We're back from the scheduler. Interrupts are still disabled.
        // Check for errors and exit early if there was one.
        if (thread->m_unblockStatus != kArSuccess)
        {
            channel->m_blockedReceivers.remove(&thread->m_blockedNode);
            return thread->m_unblockStatus;
        }
    }
    else
    {
        // Get the first blocked sender.
        thread = channel->m_blockedSenders.m_head->getObject<ar_thread_t>();;

        // Copy data from the sender.
        memcpy(value, thread->m_channelData, channel->m_width);

        // Unblock the sender.
        thread->unblockWithStatus(channel->m_blockedSenders, kArSuccess);

        // Invoke the scheduler if the unblocked thread is higher priority than the current one.
        if (thread->m_priority > g_ar.currentThread->m_priority)
        {
            disableIrq.enable();

            ar_kernel_enter_scheduler();
        }
    }

    return kArSuccess;
}

// See ar_kernel.h for documentation of this function.
ar_status_t ar_channel_send(ar_channel_t * channel, const void * value, uint32_t timeout)
{
    if (!channel)
    {
        return kArInvalidParameterError;
    }

    IrqDisableAndRestore disableIrq;
    ar_thread_t * thread;

    // Are there any threads waiting to receive?
    if (channel->m_blockedReceivers.isEmpty())
    {
        // Nobody waiting to receive, so we must block. Return immediately if the timeout is 0.
        if (timeout == kArNoTimeout)
        {
            return kArTimeoutError;
        }

        // Block this thread on the channel.
        thread = g_ar.currentThread;
        thread->m_channelData = const_cast<void *>(value);
        thread->block(channel->m_blockedSenders, timeout);

        disableIrq.enable();

        // Yield to the scheduler. We'll return when a call to receive()
        // wakes this thread. If another thread gains control, interrupts will be
        // set to that thread's last state.
        ar_kernel_enter_scheduler();

        disableIrq.disable();

        // We're back from the scheduler. Interrupts are still disabled.
        // Check for errors and exit early if there was one.
        if (thread->m_unblockStatus != kArSuccess)
        {
            channel->m_blockedSenders.remove(&thread->m_blockedNode);
            return thread->m_unblockStatus;
        }
    }
    else
    {
        // Get the first blocked receiver.
        thread = channel->m_blockedReceivers.m_head->getObject<ar_thread_t>();

        // Copy data to the receiver.
        memcpy(thread->m_channelData, value, channel->m_width);

        // Unblock the receiver.
        thread->unblockWithStatus(channel->m_blockedReceivers, kArSuccess);

        // Invoke the scheduler if the unblocked thread is higher priority than the current one.
        if (thread->m_priority > g_ar.currentThread->m_priority)
        {
            disableIrq.enable();

            ar_kernel_enter_scheduler();
        }
    }

    return kArSuccess;
}

// See ar_kernel.h for documentation of this function.
const char * ar_channel_get_name(ar_channel_t * channel)
{
    return channel ? channel->m_name : NULL;
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
