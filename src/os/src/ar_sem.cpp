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
 * @file arsem.cpp
 * @brief Source for Ar microkernel semaphores.
 * @ingroup ar
 */

#include "ar_kernel.h"
#include <string.h>
#include <assert.h>

namespace Ar;

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

//! @param name Pass a name for the semaphore. If NULL is passed the name will
//!		be set to an empty string.
//! @param count The initial semaphore count. Setting this value to 0 will
//!		cause the first call to get() to block until put() is called. A value
//!		of 1 or greater will allow that many calls to get() to succeed.
//!
//! @retval kSuccess Semaphore initialised successfully.
status_t Semaphore::init(const char * name, unsigned count)
{
	NamedObject::init(name);
	
	m_count = count;

#if defined(DEBUG)
	addToCreatedList(g_muAllObjects.m_semaphores);
#endif // DEBUG
	
	return kSuccess;
}

//! Any threads on the blocked list will be unblocked immediately. Their return status
//! from the get() method will be #kObjectDeletedError.
void Semaphore::cleanup()
{
	// Unblock all threads blocked on this semaphore.
	while (m_blockedList)
	{
		m_blockedList->unblockWithStatus(m_blockedList, kObjectDeletedError);
	}
	
#if defined(DEBUG)
	removeFromCreatedList(g_muAllObjects.m_semaphores);
#endif // DEBUG
}

//! The semaphore count is decremented. If the count is 0 upon entering this
//! method then the caller thread is blocked until the count reaches 1. Threads
//! are unblocked in the order in which they were blocked. Priority is not
//! taken into consideration, so priority inversions are possible.
//!
//! @param timeout The maximum number of ticks that the caller is willing to
//!		wait in a blocked state before the semaphore can be obtained. If this
//!		value is 0, or #kNoTimeout, then this method will return immediately
//!		if the semaphore cannot be obtained. Setting the timeout to
//!		#kInfiniteTimeout will cause the thread to wait forever for a chance
//!		to get the semaphore.
//!
//! @retval kSuccess The semaphore was obtained without error.
//! @retval kTimeoutError The specified amount of time has elapsed before the
//!		semaphore could be obtained.
//! @retval kObjectDeletedError Another thread deleted the semaphore while the
//!		caller was blocked on it.
status_t Semaphore::get(uint32_t timeout)
{
	// Ensure that only 0 timeouts are specified when called from an IRQ handler.
	assert(Thread::s_irqDepth == 0 || (Thread::s_irqDepth > 0 && timeout == 0));
	
	IrqStateSetAndRestore disableIrq(false);

	if (m_count == 0)
	{
		// Count is 0, so we must block. Return immediately if the timeout is 0.
		if (timeout == kNoTimeout)
		{
			return kTimeoutError;
		}

		// Block this thread on the semaphore.
		Thread * thread = Thread::s_currentThread;
		thread->block(m_blockedList, timeout);

		// Yield to the scheduler. We'll return when a call to put()
		// wakes this thread. If another thread gains control, interrupts will be
		// set to that thread's last state.
		Thread::enterScheduler();

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

//! The semaphore count is incremented.
void Semaphore::put()
{
	IrqStateSetAndRestore disableIrq(false);

	// Increment count.
	m_count++;

	// Are there any threads waiting on this semaphore?
	if (m_blockedList)
	{
		// Unblock the head of the blocked list.
		Thread * thread = m_blockedList;
		thread->unblockWithStatus(m_blockedList, kSuccess);

		// Invoke the scheduler if the unblocked thread is higher priority than the current one.
		if (thread->m_priority > Thread::s_currentThread->m_priority)
		{
			Thread::enterScheduler();
		}
	}
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
