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
 * @file muthread.cpp
 * @brief Source for Ar microkernel threads.
 * @ingroup mu
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

//! 10 ms tick.
#define MU_TICK_QUANTA_MS (10)

#if MU_PRINT_SYSTEM_LOAD
    //! Size in bytes of the idle thread's stack. Needs much more stack
    //! space in order to call printf().
    #define MU_IDLE_THREAD_STACK_SIZE (2048)
#else // MU_PRINT_SYSTEM_LOAD
    //! Size in bytes of the idle thread's stack.
    #define MU_IDLE_THREAD_STACK_SIZE (256)
#endif // MU_PRINT_SYSTEM_LOAD

//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------

Thread * Ar::g_ar_currentThread;

bool Thread::s_isRunning = false;
Thread * Thread::s_readyList = NULL;
Thread * Thread::s_suspendedList = NULL;
Thread * Thread::s_sleepingList = NULL;
volatile uint32_t Thread::s_tickCount = 0;
// Thread * Thread::g_ar_currentThread = NULL;
volatile uint32_t Thread::s_irqDepth = 0;
unsigned Thread::s_systemLoad = 0;

//#pragma alignvar(8)
//! The stack for #s_idleThread.
uint8_t Thread::s_idleThreadStack[MU_IDLE_THREAD_STACK_SIZE];

//! The lowest priority thread in the system. Executes only when no other
//! threads are ready.
Thread Thread::s_idleThread;

#if AR_GLOBAL_OBJECT_LISTS
//! This global contains linked lists of all the various Ar object
//! types that have been created during runtime. This makes it much
//! easier to examine objects of interest.
ObjectLists g_muAllObjects;
#endif // AR_GLOBAL_OBJECT_LISTS

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

//! @brief System idle thread entry point.
//!
//! This thread just spins forever.
//!
//! If the #MU_ENABLE_SYSTEM_LOAD define has been set to 1 then this thread will
//! also calculate the average system load once per second. The system load is
//! accessible with the Thread::getSystemLoad() static member.
//!
//! If the #MU_ENABLE_IDLE_SLEEP define is set to 1, the idle thread hits the
//! PCK bit in the SCDR register of the AT91 Power Management Controller
//! peripheral. This causes the CPU to go to sleep until the next interrupt,
//! which if nothing else will be the OS tick timer. This option can make it
//! very easy to save power.
//!
//! @param param Ignored.
void Thread::idle_entry(void * param)
{
#if MU_ENABLE_SYSTEM_LOAD
    uint32_t start;
    uint32_t last;
    uint32_t ticks;
    uint32_t skipped = 0;
    
    start = Thread::getTickCount();
    last = start;
#endif // MU_ENABLE_SYSTEM_LOAD
    
    while (1)
    {
#if MU_ENABLE_SYSTEM_LOAD
        ticks = Thread::getTickCount();
        
        if (ticks != last)
        {
            uint32_t diff = ticks - last;
            
            if (ticks - start >= 100)
            {
                unsigned s = start + 100 - ticks;
                
                if (diff - 1 > s)
                {
                    skipped += s;
                }
                else
                {
                    skipped += diff - 1;
                }
                
                s_systemLoad = skipped;
                
                #if MU_PRINT_SYSTEM_LOAD
                printf("%d%% system load\n", s_systemLoad);
                #endif // MU_PRINT_SYSTEM_LOAD
                
                // start over counting
                if (diff - 1 > s)
                {
                    skipped = diff - 1 - s;
                }
                else
                {
                    skipped = 0;
                }
                start = ticks;
            }
            else
            {
                skipped += diff - 1;
            }
            
            last = ticks;
        }
#endif // MU_ENABLE_SYSTEM_LOAD
        
#if MU_ENABLE_IDLE_SLEEP
        // Hitting this bit puts the processor to sleep until the next
        // IRQ or FIQ fires.
        *AT91C_PMC_SCDR = AT91C_PMC_PCK;
#endif // MU_ENABLE_IDLE_SLEEP
    }
}

//#pragma mark *** NamedObject ***

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

//#pragma mark *** Thread ***

//! The thread is in suspended state when this method exits. The make it eligible for
//! execution, call the resume() method.
//!
//! @param name Name of the thread. If NULL, the thread's name is set to an empty string.
//! @param entry Thread entry point taking one parameter and returning void.
//! @param param Arbitrary pointer-sized value passed as the single parameter to the thread
//!     entry point.
//! @param stack Pointer to the start of the thread's stack. This should be the stack's bottom,
//!     not it's top.
//! @param stackSize Number of bytes of stack space allocated to the thread. This value is
//!     added to @a stack to get the initial top of stack address.
//! @param priority Thread priority. The accepted range is 1 through 255. Priority 0 is
//!     reserved for the idle thread.
//!
//! @return kSuccess The thread was initialised without error.
status_t Thread::init(const char * name, thread_entry_t entry, void * param, void * stack, unsigned stackSize, uint8_t priority)
{
    // Assertions.
    assert(priority != 0);
    assert(stackSize >= 64);
    
    NamedObject::init(name);
    
    // init member variables
    m_stackTop = reinterpret_cast<uint8_t *>(stack) + stackSize;
    m_stackSize = stackSize;
    m_stackPointer = m_stackTop;
    m_priority = priority;
    m_state = kThreadSuspended;
    m_entry = entry;
    m_param = param;
    m_next = NULL;
    m_wakeupTime = 0;
    m_unblockStatus = 0;

    // prepare top of stack
    prepareStack();

    {
        // disable interrupts
        IrqStateSetAndRestore disableIrq(false);
        
        // add to suspended list
        addToList(s_suspendedList);
    }
    
#if AR_GLOBAL_OBJECT_LISTS
    addToCreatedList(g_muAllObjects.m_threads);
#endif // AR_GLOBAL_OBJECT_LISTS
    
    return kSuccess;
}

void Thread::cleanup()
{
    if (m_state != kThreadDone)
    {
        // Remove from ready list if it's on there, and switch to another thread
        // if we're disposing of our own thread.
        suspend();
        
        // Now remove from the suspended list
        {
            IrqStateSetAndRestore disableIrq(false);
            removeFromList(s_suspendedList);
        }
    }
    
#if AR_GLOBAL_OBJECT_LISTS
    removeFromCreatedList(g_muAllObjects.m_threads);
#endif // AR_GLOBAL_OBJECT_LISTS
}

//! If the thread being resumed has a higher priority than that of the
//! current thread, the scheduler is called to immediately switch threads.
//! In this case the thread being resumed will always become the new
//! current thread. This is because the highest priority thread is always
//! guaranteed to be running, meaning the calling thread was the previous
//! highest priority thread.
//!
//! Does not enter the scheduler if Ar is not running. Does nothing if
//! the thread is already on the ready list.
//!
//! @todo Deal with all thread states properly.
//!
void Thread::resume()
{
    {
        IrqStateSetAndRestore disableIrq(false);
    
        if (m_state == kThreadReady)
        {
            return;
        }
        else if (m_state == kThreadSuspended)
        {
            removeFromList(s_suspendedList);
            m_state = kThreadReady;
            addToList(s_readyList);
        }
    }

    // yield to scheduler if there is not a running thread or if this thread
    // has a higher priority that the running one
    if (s_isRunning && this->m_priority > g_ar_currentThread->m_priority)
    {
        enterScheduler();
    }
}

//! If this method is called from the current thread then the scheduler is
//! entered immediately after putting the thread on the suspended list.
//! Calling suspend() on another thread will not cause the scheduler to
//! switch threads.
//!
//! Does not enter the scheduler if Ar is not running. Does nothing if
//! the thread is already suspended.
//!
//! @todo Deal with all thread states properly.
//!
void Thread::suspend()
{
    {
        IrqStateSetAndRestore disableIrq(false);
    
        if (m_state == kThreadSuspended)
        {
            // Nothing needs doing if the thread is already suspended.
            return;
        }
        else
        {
            removeFromList(s_readyList);
            m_state = kThreadSuspended;
            addToList(s_suspendedList);
        }
    }

    // are we suspending the current thread?
    if (s_isRunning && this == g_ar_currentThread)
    {
        enterScheduler();
    }
}

//! The scheduler is invoked after the priority is set so that the current thread can
//! be changed to the one with the highest priority. The scheduler is invoked even if
//! there is no new highest priority thread. In this case, control may switch to the
//! next thread with the same priority, assuming there is one.
//!
//! Does not enter the scheduler if Ar is not running.
//!
//! @param priority Thread priority level from 1 to 255, where lower number have
//!     a lower priority. Priority number 0 is not allowed because it is reserved for
//!     the idle thread.
void Thread::setPriority(uint8_t priority)
{
    assert(priority != 0);
    
    m_priority = priority;

    if (s_isRunning)
    {
        enterScheduler();
    }
}

//! The thread of the caller is put to sleep for as long as the thread
//! referenced by this thread object is running. Once this thread
//! completes and its state is kThreadDone, the caller's thread is woken
//! and this method call returns. If this thread takes longer than
//! @a timeout to finish running, then #kTimeoutError is returned.
//!
//! @param timeout Timeout value in operating system ticks.
//!
//! @retval kSuccess
//! @retval kTimeoutError
void join(uint32_t timeout=kInfiniteTimeout)
{
    // Not yet implemented!
    assert(0);
//  return kSuccess;
}

//! Does nothing if Ar is not running.
//!
//! @param ticks The number of operating system ticks to sleep the calling thread.
//!     A sleep time of 0 is not allowed.
void Thread::sleep(unsigned ticks)
{
    assert(ticks != 0);
    
    // bail if there is not a running thread to put to sleep
    if (!g_ar_currentThread)
    {
        return;
    }

    {
        IrqStateSetAndRestore disableIrq(false);
        
        // put the current thread on the sleeping list
        g_ar_currentThread->m_wakeupTime = s_tickCount + ticks;
        g_ar_currentThread->removeFromList(s_readyList);
        g_ar_currentThread->m_state = kThreadSleeping;
        g_ar_currentThread->addToList(s_sleepingList);
    }
    
    // run scheduler and switch to another thread
    enterScheduler();
}

//!
//!
void Thread::threadEntry()
{
    // Call the entry point.
    if (m_entry)
    {
        m_entry(m_param);
    }
}

//! The thread wrapper calls the thread entry function that was set in
//! the init() call. When and if the function returns, the thread is removed
//! from the ready list and its state set to #THREAD_DONE.
//!
//! This function will never actually itself return. Instead, it switches to
//! the scheduler before exiting, and the scheduler will never switch back
//! because the thread is marked as done.
//!
//! @param thread Pointer to the thread object which is starting up.
void Thread::thread_wrapper(Thread * thread)
{
    assert(thread);
    
    // Call the entry point.
    thread->threadEntry();
    
    // Thread function has finished, so clean up and terminate the thread.
    {
        IrqStateSetAndRestore disableIrq(false);
        
        // This thread must be in the running state for this code to execute,
        // so we know it is on the ready list.
        thread->removeFromList(s_readyList);
        
        // Mark this thread as finished
        thread->m_state = kThreadDone;
        
        // Switch to the scheduler to let another thread take over
        enterScheduler();
    }
}

//! Uses a "swi" instruction to yield to the scheduler when in user mode. If
//! the CPU is in IRQ mode then we can just call the scheduler() method
//! directly and any change will take effect when the ISR exits.
void Thread::enterScheduler()
{
    if (s_irqDepth == 0)
    {
        // In user mode we must SWI into the scheduler.
        service_call();
    }
    else
    {
        // We're in IRQ mode so just call the scheduler directly
        scheduler();
    }
}

//! The system idle thread is created here. Its priority is set to 0, the lowest
//! possible priority. The idle thread will run when there are no other ready threads.
//!
//! At least one user thread must have been created and resumed before
//! calling run(). Usually this will be an initialisation thread that itself
//! creates the other threads of the application.
//!
//! @pre Interrupts must not be enabled prior to calling this routine.
//!
//! @note Control will never return from this method.
void Thread::run()
{
    // Assert if there is no thread ready to run.
    assert(s_readyList);
    
    // Create the idle thread. Priority 1 is passed to init function to pass the
    // assertion and then set to the correct 0 manually.
    s_idleThread.init("idle", idle_entry, 0, s_idleThreadStack, MU_IDLE_THREAD_STACK_SIZE, 1);
    s_idleThread.m_priority = 0;
    s_idleThread.resume();
    
    // Set up system tick timer
    initTimerInterrupt();
    
    initSystem();
    
    // We're now ready to run
    s_isRunning = true;
    
    // Swi into the scheduler. The yieldIsr() will see that g_ar_currentThread
    // is NULL and ignore the stack pointer it was given. After the scheduler
    // runs, we return from the swi handler to the init thread. Interrupts
    // are enabled in that switch to the init thread since all threads start
    // with a CPSR that enables IRQ and FIQ.
    enterScheduler();

    // should never reach here
    _halt();
}

void Thread::periodicTimerIsr()
{
    // Exit immediately if the kernel isn't running.
    if (!s_isRunning)
    {
        return;
    }
    
    incrementTickCount(1);

    // Run the scheduler. It will modify g_ar_currentThread if switching threads.
//     scheduler();
//     pending_service_call();
    service_call();

    // This case should never happen because of the idle thread.
    assert(g_ar_currentThread);
}

//! @param topOfStack This parameter should be the stack pointer of the thread that was
//!     current when the timer IRQ fired.
//! @return The value of the current thread's stack pointer is returned. If the scheduler
//!     changed the current thread, this will be a different value from what was passed
//!     in @a topOfStack.
uint32_t Thread::yieldIsr(uint32_t topOfStack)
{
    // save top of stack for the thread we interrupted
    if (g_ar_currentThread)
    {
        g_ar_currentThread->m_stackPointer = reinterpret_cast<uint8_t *>(topOfStack);
    }
    
    // Run the scheduler. It will modify g_ar_currentThread if switching threads.
    scheduler();

    // The idle thread prevents this condition.
    assert(g_ar_currentThread);

    // return the new thread's stack pointer
    return (uint32_t)g_ar_currentThread->m_stackPointer;
}

//! Increments the system tick count and wakes any sleeping threads whose wakeup time
//! has arrived. If the thread's state is #kThreadBlocked then its unblock status
//! is set to #kTimeoutError.
//!
//! @param ticks The number of ticks that have elapsed. Normally this will only be 1, 
//!     and must be at least 1, but may be higher if interrupts are disabled for a
//!     long time.
//! @return Flag indicating whether any threads were modified.
//!
//! @todo Keep the list of sleeping threads sorted by next wakeup time.
bool Thread::incrementTickCount(unsigned ticks)
{
    assert(ticks > 0);
    
    // Increment tick count.
    s_tickCount += ticks;

    // Scan list of sleeping threads to see if any should wake up.
    Thread * thread = s_sleepingList;
    bool wasThreadWoken = false;
    
    while (thread)
    {
        Thread * next = thread->m_next;
        
        // Is it time to wake this thread?
        if (s_tickCount >= thread->m_wakeupTime)
        {
            wasThreadWoken = true;
            
            // State-specific actions
            switch (thread->m_state)
            {
                case kThreadSleeping:
                    // The thread was just sleeping.
                    break;
                
                case kThreadBlocked:
                    // The thread has timed out waiting for a resource.
                    thread->m_unblockStatus = kTimeoutError;
                    break;
                
                default:
                    // Should not have threads in other states on this list!
                    _halt();
            }
            
            // Put thread in ready state.
            thread->removeFromList(s_sleepingList);
            thread->m_state = kThreadReady;
            thread->addToList(s_readyList);
        }
        
        thread = next;
    }
    
    return wasThreadWoken;
}

//! @todo There are many opportunities for optimising the scheduler search
//! loop.
void Thread::scheduler()
{
    // Find the next ready thread using a round-robin search algorithm.
    Thread * next;
    
    // Start the search either at the current thread or at the beginning of
    // the ready list.
    if (g_ar_currentThread)
    {
        // Handle case where current thread was suspended. Here the g_ar_currentThread is no longer
        // on the ready list so we can't start from its m_next.
        if (g_ar_currentThread->m_state != kThreadRunning)
        {
            g_ar_currentThread = NULL;
            next = s_readyList;
        }
        else
        {
            next = g_ar_currentThread;
        }
    }
    else
    {
        next = s_readyList;
    }
    
    assert(next);
    Thread * highest = next;
    uint8_t priority = highest->m_priority;

    // Search the ready list starting from the current thread. The search will loop back
    // to the list head when it hits NULL. The first thread after the current one whose
    // state is THREAD_READY and has the highest priority will be selected.
    do {
        if (next)
        {
            uint32_t check = *(uint32_t *)((uint32_t)next->m_stackTop - next->m_stackSize);
            if (check != 0xdeadbeef)
            {
                _halt();
            }
            
            next = next->m_next;
        }

        // Find highest priority thread.
        if (next && next->m_state == kThreadReady && next->m_priority > priority)
        {
            highest = next;
            priority = next->m_priority;
        }

        // Handle both the case when we start with s_currentThead is 0 and when we loop
        // from the end of the ready list to the beginning.
        if (!next && g_ar_currentThread)
        {
            next = s_readyList;
        }
    } while (next && next != g_ar_currentThread);

    // Switch to newly selected thread.
    if (highest && highest != g_ar_currentThread)
    {
        if (g_ar_currentThread && g_ar_currentThread->m_state == kThreadRunning)
        {
            g_ar_currentThread->m_state = kThreadReady;
        }
        
        highest->m_state = kThreadRunning;
        g_ar_currentThread = highest;
    }
}

//! The thread is added to the end of the linked list.
//!
//! @param[in,out] listHead Reference to the head of the linked list. Will be
//!     NULL if the list is empty, in which case it is set to the thread instance.
void Thread::addToList(Thread * & listHead)
{
    this->m_next = NULL;

    // handle an empty list
    if (!listHead)
    {
        listHead = this;
        return;
    }
    
    // find the end of the list
    Thread * thread = listHead;

    while (thread)
    {
        if (!thread->m_next)
        {
            thread->m_next = this;
            break;
        }
        thread = thread->m_next;
    }
}

//! The list is not allowed to be empty.
//!
//! @param[in,out] listHead Reference to the head of the linked list. May
//!     not be NULL.
void Thread::removeFromList(Thread * & listHead)
{
    // the list must not be empty
    assert(listHead != NULL);

    if (listHead == this)
    {
        // special case for removing the list head
        listHead = this->m_next;
    }
    else
    {
        Thread * item = listHead;
        while (item)
        {
            if (item->m_next == this)
            {
                item->m_next = this->m_next;
                return;
            }

            item = item->m_next;
        }
    }
}

//! The thread is added to the end of the linked list.
//!
//! @param[in,out] listHead Reference to the head of the linked list. Will be
//!     NULL if the list is empty, in which case it is set to the thread instance.
void Thread::addToBlockedList(Thread * & listHead)
{
    this->m_nextBlocked = NULL;

    // handle an empty list
    if (!listHead)
    {
        listHead = this;
        return;
    }
    
    // find the end of the list
    Thread * thread = listHead;

    while (thread)
    {
        if (!thread->m_nextBlocked)
        {
            thread->m_nextBlocked = this;
            break;
        }
        thread = thread->m_nextBlocked;
    }
}

//! The list is not allowed to be empty.
//!
//! @param[in,out] listHead Reference to the head of the linked list. May
//!     not be NULL.
void Thread::removeFromBlockedList(Thread * & listHead)
{
    // the list must not be empty
    assert(listHead != NULL);

    if (listHead == this)
    {
        // special case for removing the list head
        listHead = this->m_nextBlocked;
    }
    else
    {
        Thread * item = listHead;
        while (item)
        {
            if (item->m_nextBlocked == this)
            {
                item->m_nextBlocked = this->m_nextBlocked;
                return;
            }

            item = item->m_nextBlocked;
        }
    }
}

//! The thread is removed from the ready list. It is placed on the blocked list
//! referenced by the @a blockedList argument and its state is set to
//! #THREAD_BLOCKED. If the timeout is non-infinite, the thread is also
//! placed on the sleeping list with the wakeup time set to when the timeout
//! expires.
//!
//! @param[in,out] blockedList Reference to the head of the linked list of
//!     blocked threads.
//! @param timeout The maximum number of ticks that the thread can remain
//!     blocked. A value of #kInfiniteTimeout means the thread can be
//!     blocked forever. A timeout of 0 is not allowed and should be handled
//!     by the caller.
void Thread::block(Thread * & blockedList, uint32_t timeout)
{
    assert(timeout != 0);
    
    // Remove this thread from the ready list.
    removeFromList(Thread::s_readyList);
    
    // Update its state.
    m_state = kThreadBlocked;
    m_unblockStatus = kSuccess;
    
    // Add to blocked list.
    addToBlockedList(blockedList);
    
    // If a valid timeout was given, put the thread on the sleeping list.
    if (timeout != kInfiniteTimeout)
    {
        m_wakeupTime = s_tickCount + timeout;
        addToList(s_sleepingList);
    }
    else
    {
        // Signal to unblockWithStatus() that this thread is not on the sleeping list.
        m_wakeupTime = 0;
    }
}

//! If the thread had a valid timeout when it was blocked, it is removed from
//! the sleeping list. It is always removed from the blocked list passed in
//! @a blockedList. And finally the thread is restored to ready status by setting
//! its state to #THREAD_READY and adding it back to the ready list.
//!
//! @param[in,out] blockedList Reference to the head of the linked list of
//!     blocked threads.
//! @param unblockStatus Status code to return from the function that
//!     the thread had called when it was originally blocked.
//!
//! @todo Conditionalise the removal from #Thread::s_sleepingList to when the thread
//!     is actually on that list.
void Thread::unblockWithStatus(Thread * & blockedList, status_t unblockStatus)
{
    // Remove from the sleeping list if it was on there. Won't hurt if
    // the thread is not on that list.
    if (m_wakeupTime && s_sleepingList)
    {
        removeFromList(s_sleepingList);
    }
    
    // Remove this thread from the blocked list.
    removeFromBlockedList(blockedList);

    // Put the unblocked thread back onto the ready list.
    m_state = kThreadReady;
    m_unblockStatus = unblockStatus;
    addToList(Thread::s_readyList);
}

void * ar_yield(void * topOfStack)
{
    return (void *)Thread::yieldIsr((uint32_t)topOfStack);
}

void ar_periodic_timer(void)
{
    return Thread::periodicTimerIsr();
}

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
