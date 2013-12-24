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
 * @file arkernel.h
 * @brief Header for the Argon RTOS.
 * @ingroup ar
 */

#if !defined(_AR_KERNEL_H_)
#define _AR_KERNEL_H_

#include "ar_port.h"

//! @addtogroup ar
//! @{

extern "C" {
void * ar_yield(void * topOfStack);
void ar_periodic_timer(void);
}

//! @}

//! @ingroup ar
namespace Ar {

//! @addtogroup ar
//! @{

class Thread;

// Global symbols outside of the namespace for easy access from assembler.
extern Ar::Thread * g_ar_currentThread;

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

#define AR_GLOBAL_OBJECT_LISTS (0)

// The maximum name length can be overridden.
#if !defined(AR_MAX_NAME_LENGTH)
    //! Maximum length for names.
    #define AR_MAX_NAME_LENGTH (16)
#endif

//! @brief Timeout constants.
//!
//! @ingroup ar
enum _ar_timeouts
{
    //! Return immediately if a resource cannot be acquired.
    kNoTimeout = 0,
    
    //! Pass this value to wait forever to acquire a resource.
    kInfiniteTimeout = 0xffffffffL
};

//! @brief Ar microkernel error codes.
//!
//! @ingroup ar
enum _ar_errors
{
    //! Operation was successful.
    kSuccess = 0,
    
    //! Timeout while blocked on an object.
    kTimeoutError,
    
    //! An object was deleted while a thread was blocked on it. This may be
    //! a semaphore, mutex, or queue.
    kObjectDeletedError,
    
    //! The queue is at maximum capacity and cannot accept more elements.
    kQueueFullError,
    
    //! No elements are in the queue.
    kQueueEmptyError,
};

//! @}

//------------------------------------------------------------------------------
// Classes
//------------------------------------------------------------------------------

/*!
 * @brief Base class for Ar classes that have a name associated with them.
 *
 * @ingroup ar
 *
 * This class provides functionality required by all Ar RTOS classes. It
 * keeps a name associated with each object instance. Having one copy of
 * the name handling code makes it easy to disable in order to reduce memory
 * usage, if necessary.
 *
 * For debug builds, this class manages a linked list of object instances.
 */
class NamedObject
{
public:
    NamedObject() {}
    
    virtual ~NamedObject() {}
    
    //! @brief Initialiser.
    status_t init(const char * name);
    
    //! @brief Access the object's name.
    const char * getName() const { return m_name; }
    
protected:
    char m_name[AR_MAX_NAME_LENGTH];    //!< The object's name.

#if AR_GLOBAL_OBJECT_LISTS
    NamedObject * m_nextCreated;    //!< Linked list node.
    
    //! \brief Add to linked list of all created objects of a type.
    void addToCreatedList(NamedObject * & listHead);

    //! \brief Remove from linked list of all objects.
    void removeFromCreatedList(NamedObject * & listHead);
#endif // AR_GLOBAL_OBJECT_LISTS
};

/*!
 * @brief Preemptive thread class and RTOS core.
 *
 * @ingroup ar
 *
 * This thread class implements a preemptive threading system with priorities.
 * The highest priority thread that is ready to run will always get the processor. That
 * means that if there is only one high priority thread, it can starve lower priority
 * threads if it never relinquishes control by sleeping or blocking on a resource.
 * Threads with the same priority will preempt each other in a round robin order
 * every system tick.
 *
 * Thread priorities range from 0 to 255. Higher values are higher priorities, with
 * 255 being the highest priority. Priority 0 is reserved for the idle thread.
 *
 * To create a thread, first allocate it either on the stack or with the new operator.
 * Then call the init() method passing in the name, entry point, entry parameter,
 * stack information, and priority. The entry point can be any non-member, i.e static, function
 * that matches the #thread_entry_t prototype. The init() method leaves the new thread
 * suspended. To make the new thread eligible to run you must call the resume() method
 * on it.
 *
 * If you want to fully encapsulate a thread you can create a subclass of Thread
 * that provides its own init() method which calls the original Thread::init(). You
 * can either pass a pointer to a static function to the base init() method, as
 * usual, or you can override the virtual Thread::threadEntry() method. In the latter
 * case, you can simply pass NULL for the entry point to the base init() method.
 * To pass values to the thread function, simply create member variables and set them
 * in your subclass' init() method.
 *
 * Here's an example subclass that uses a member function as the entry point:
 * @code
 *      class MySubclassThread : public Ar::Thread
 *      {
 *      public:
 *          status_t init()
 *          {
 *              // Pass NULL for the entry point. It's not needed because you are
 *              // overriding threadEntry() below.
 *              return Thread::init("my thread", NULL, this, m_stack, sizeof(m_stack), 32);
 *          }
 *
 *      protected:
 *          // Static memory for the stack.
 *          uint8_t m_stack[4096];
 *
 *          // Override the default Thread implementation.
 *          virtual void threadEntry()
 *          {
 *              // Implement your thread here.
 *          }
 *
 *      };
 * @endcode
 */
class Thread : public NamedObject
{
public:
    //! Potential thread states.
    typedef enum _thread_state {
        kThreadUnknown,     //!< Hopefully a thread is never in this state.
        kThreadSuspended,   //!< Thread is not eligible for execution.
        kThreadReady,       //!< Thread is eligible to be run.
        kThreadRunning,     //!< The thread is currently running.
        kThreadBlocked,     //!< The thread is blocked on another object.
        kThreadSleeping,    //!< Thread is sleeping.
        kThreadDone         //!< Thread has exited.
    } thread_state_t;

    //! Prototype for the thread entry point.
    typedef void (*thread_entry_t)(void * param);

public:
    Thread() {}
    
    virtual ~Thread() {}
    
    //! @name Thread init and cleanup
    //@{
    //! @brief Base initialiser.
    status_t init(const char * name, thread_entry_t entry, void * param, void * stack, unsigned stackSize, uint8_t priority);

    //! @brief Releases any resources held by the thread.
    virtual void cleanup();
    //@}

    //! @name Thread state
    //!
    //! Control of and access to the thread state.
    //@{
    //! @brief Put thread in suspended state.
    void suspend();

    //! @brief Make the thread eligible for execution.
    void resume();

    //! @brief Return the current state of the thread.
    thread_state_t getState() const { return m_state; }

    //! @brief Put the current thread to sleep for a certain amount of time.
    static void sleep(unsigned ticks);
    
    //! @brief Block the current thread until this thread completes.
    status_t join(uint32_t timeout=kInfiniteTimeout);
    //@}

    //! @name Thread priority
    //!
    //! Accessors for the thread's priority.
    //@{
    //! @brief Return the thread's current priority.
    uint8_t getPriority() const { return m_priority; }

    //! @brief Change the thread's priority.
    void setPriority(uint8_t priority);
    //@}
    
    //! @name RTOS control
    //@{
    //! @brief Enables the tick timer and switches to a ready thread.
    static void run();
    //@}

    //! @name Accessors
    //!
    //! Static members to get system-wide information.
    //@{
    //! @brief Returns the currently running thread object.
    static Thread * getCurrent() { return g_ar_currentThread; }

    //! @brief Returns the current tick count.
    static uint32_t getTickCount() { return s_tickCount; }
    
    //! @brief Returns the current system load average.
    static unsigned getSystemLoad() { return s_systemLoad; }
    //@}

    //! @brief Handles the periodic timer tick interrupt.
    static void periodicTimerIsr();

    //! @brief Handles the software interrupt to invoke the scheduler.
    static uint32_t yieldIsr(uint32_t topOfStack);
    
    
protected:
    
    //! @brief Virtual thread entry point.
    virtual void threadEntry();

protected:
    // Instance member variables
    
    volatile uint8_t * m_stackPointer; //!< Current stack pointer.
    uint8_t * m_stackTop;  //!< Original top of stack.
    unsigned m_stackSize;   //!< Stack size in bytes.
    uint8_t m_priority; //!< Thread priority. 0 is the lowest priority.
    thread_state_t m_state; //!< Current thread state.
    thread_entry_t m_entry; //!< Function pointer for the thread's entry point.
    void * m_param; //!< Initial parameter passed to the entry point.
    Thread * m_next;    //!< Linked list node.
    Thread * m_nextBlocked; //!< Linked list node for blocked threads.
    uint32_t m_wakeupTime;  //!< Tick count when a sleeping thread will awaken.
    status_t m_unblockStatus;   //!< Status code to return from a blocking function upon unblocking.
    
    // Static member variables
    
    static bool s_isRunning;    //!< True if the kernel has been started.
    static Thread * s_readyList;    //!< Head of a linked list of ready threads.
    static Thread * s_suspendedList;    //!< Head of linked list of suspended threads.
    static Thread * s_sleepingList; //!< Head of linked list of sleeping threads.
    static volatile uint32_t s_tickCount;   //!< Current tick count.
    static Thread * s_currentThread;    //!< The currently running thread.
    static volatile uint32_t s_irqDepth;    //!< Current level of nested IRQs, or 0 if in user mode.
    static unsigned s_systemLoad;   //!< Percent of system load from 0-100.
    
    //! @name Idle thread members
    //@{
    
//  #pragma alignvar(8)
    //! The stack for #s_idleThread.
    static uint8_t s_idleThreadStack[];
    
    //! The lowest priority thread in the system. Executes only when no other
    //! threads are ready.
    static Thread s_idleThread;
    
    //@}
    
protected:
    //
    // Protected member functions
    //
    
    //! @name Thread setup
    //!
    //! Utilities for getting a thread ready to run.
    //@{
    //! @brief Fill the thread's stack with it's initial contents.
    void prepareStack();
    //@}

    //! @name List management
    //!
    //! Methods to manipulate the thread object as a linked list node.
    //@{
    //! @brief Utility to add this thread to a null terminated linked list.
    void addToList(Thread * & listHead);

    //! @brief Removes this thread from a linked list.
    void removeFromList(Thread * & listHead);
    
    //! @brief Utility to add this thread to a null terminated linked list.
    void addToBlockedList(Thread * & listHead);

    //! @brief Removes this thread from a linked list.
    void removeFromBlockedList(Thread * & listHead);
    //@}

    //! @name Blocking
    //!
    //! Methods to manage the blocking and unblocking of a thread on some
    //! sort of resource. These methods are called by friend classes such
    //! as Queue or Mutex.
    //@{
    //! @brief Blocks the thread with a timeout.
    void block(Thread * & blockedList, uint32_t timeout);
    
    //! @brief Restores a thread to ready status.
    void unblockWithStatus(Thread * & blockedList, status_t unblockStatus);
    //@}
    
    //
    // Protected static member functions
    //
    
    //! @name Operating system internals
    //!
    //! Functions that implement the low-level parts of the operating system.
    //@{
    //! @brief Function to wrap the thread entry point.
    static void thread_wrapper(Thread * param);
    
    //! @brief Idle thread entry point.
    static void idle_entry(void * param);

    //! @brief Force entry into the scheduler with a swi instruction.
    static void enterScheduler();
    
    //! @brief Sets up the periodic system tick timer.
    static void initTimerInterrupt();
    
    //! @brief
    static void initSystem();
    
    //! @brief Bumps the system tick count and updates sleeping threads.
    static void incrementTickCount(unsigned ticks);
    
    //! @brief Selects the next thread to run.
    static void scheduler();
    //@}

    // Friends have access to all protected members for efficiency.
    friend class Semaphore;     //!< Needs access to protected member functions.
    friend class Mutex;     //!< Needs access to protected member functions.
    friend class Queue; //!< Needs access to protected member functions.
    
private:
    //! @brief The copy constructor is disabled for thread objects.
    Thread(const Thread & other) {}
};

/*!
 * @brief Counting semaphore class.
 *
 * @ingroup ar
 *
 * @see SemaphoreHolder
 */
class Semaphore : public NamedObject
{
public:
    //! @brief Initialiser.
    status_t init(const char * name, unsigned count=1);

    //! @brief
    virtual void cleanup();

    //! @brief Acquire the semaphore.
    virtual status_t get(uint32_t timeout=kInfiniteTimeout);

    //! @brief Release the semaphore.
    virtual void put();

    //! @brief Returns the current semaphore count.
    unsigned getCount() const { return m_count; }

protected:
    volatile unsigned m_count;  //!< Current semaphore count. Value of 0 means the semaphore is owned.
    Thread * m_blockedList; //!< Linked list of threads blocked on this semaphore.
};

/*!
 * @brief Utility class to automatically get and put a semaphore.
 *
 * @ingroup ar
 *
 * This class is intended to be stack allocated. It gets and holds a semaphore
 * for the duration of the scope in which it is declared. Once it goes out of
 * scope, the destructor automatically puts the semaphore.
 */
class SemaphoreHolder
{
public:
    //! @brief Constructor which gets the semaphore.
    //!
    //! Like the Semaphore::get() method, this constructor takes an optional timeout
    //! value which defaults to never timeout.
    inline SemaphoreHolder(Semaphore & sem, uint32_t timeout=kInfiniteTimeout)
    :   m_sem(sem)
    {
        m_sem.get(timeout);
    }
    
    //! @brief Destructor that puts the semaphore.
    inline ~SemaphoreHolder()
    {
        m_sem.put();
    }

protected:
    Semaphore & m_sem;  //!< The semaphore to hold.
};

/*!
 * @brief Mutex object.
 *
 * @ingroup ar
 *
 * Very similar to a binary semaphore, except that a single thread can
 * lock the mutex multiple times without deadlocking. In this case, the
 * number of calls to get() and put() must be matched.
 *
 * Because Mutex is a sublcass of Semaphore, it can be used anywhere that
 * accepts a Semaphore object. For instance, SemaphoreHolder works equally well
 * for a Mutex.
 */
class Mutex : public Semaphore
{
public:
    //! @brief Initialiser.
    status_t init(const char * name);

    //! @brief Cleanup.
    virtual void cleanup();
    
    //! @brief Lock the mutex.
    virtual status_t get(uint32_t timeout=kInfiniteTimeout);
    
    //! @brief Unlock the mutex.
    virtual void put();
    
    //! @brief Returns the current owning thread, if there is one.
    inline Thread * getOwner() { return (Thread *)m_owner; }

protected:
    volatile Thread * m_owner;  //!< Current owner thread of the mutex.
    volatile unsigned m_ownerLockCount; //!< Number of times the owner thread has locked the mutex.
};

/*!
 * @brief A blocking queue for inter-thread messaging.
 *
 * @ingroup ar
 */
class Queue : public NamedObject
{
public:
    //! @brief Queue initialiser.
    status_t init(const char * name, void * storage, unsigned elementSize, unsigned capacity);
    
    //! @brief Queue cleanup.
    virtual void cleanup();
    
    //! @brief Add an item to the queue.
    status_t send(const void * element, uint32_t timeout=kInfiniteTimeout);

    //! @brief Remove an item from the queue.
    status_t receive(void * element, uint32_t timeout=kInfiniteTimeout);
    
    //! @brief Returns whether the queue is currently empty.
    inline bool isEmpty() const { return m_count == 0; }
    
    //! @brief Returns the current number of elements in the queue.
    inline unsigned getCount() const { return m_count; }

protected:
    uint8_t * m_elements;   //!< Pointer to element storage.
    unsigned m_elementSize; //!< Number of bytes occupied by each element.
    unsigned m_capacity;    //!< Maximum number of elements the queue can hold.
    unsigned m_head;    //!< Index of queue head.
    unsigned m_tail;    //!< Index of queue tail.
    unsigned m_count;   //!< Current number of elements in the queue.
    Thread * m_sendBlockedList; //!< Linked list of threads blocked waiting to send.
    Thread * m_receiveBlockedList;  //!< Linked list of threads blocked waiting to receive.
};

/*!
 * @brief Template class to help statically allocate a Queue.
 *
 * @ingroup ar
 *
 * This template class helps create a Queue instance by defining a
 * static array of queue elements. The array length is one of the template
 * parameters.
 *
 * Example of creating a queue and adding an element:
 * @code
 *      // MyQueueType holds up to five uint32_t elements.
 *      typedef StaticQueue<uint32_t, 5> MyQueueType;
 *
 *      MyQueueType q; // Can statically allocate MyQueueType!
 *      q.init("my queue");
 *
 *      uint32_t element = 512;
 *      q.send(element);
 *
 *      q.send(1024);
 *
 *      status_t s;
 *
 *      s = q.receive(&element);
 *      
 *      element = q.receive(&s);
 * @endcode
 *
 * @param T The queue element type.
 * @param N Maximum number of elements the queue will hold.
 */
template <typename T, unsigned N>
class StaticQueue : public Queue
{
public:
    //! @brief Initialiser method.
    inline status_t init(const char * name)
    {
        return Queue::init(name, m_storage, sizeof(T), N);
    }
    
    //! @brief
    inline status_t send(T element, uint32_t timeout=kInfiniteTimeout)
    {
        return Queue::send((const void *)&element, timeout);
    }
    
    //! @brief
    inline status_t receive(T * element, uint32_t timeout=kInfiniteTimeout)
    {
        return Queue::receive((void *)element, timeout);
    }
    
    //! @brief Alternate form of typed receive.
    //!
    //! @param[out] resultStatus The status of the receive operation is placed here.
    //!     May be NULL, in which case no status is returned.
    //! @param timeout Maximum time in ticks to wait for a queue element.
    T receive(status_t * resultStatus, uint32_t timeout=kInfiniteTimeout)
    {
        T element;
        status_t status = Queue::receive((void *)&element, timeout);
        if (resultStatus)
        {
            *resultStatus = status;
        }
        return element;
    }
    
protected:
    T m_storage[N]; //!< Static storage for the queue elements.
};

#if AR_GLOBAL_OBJECT_LISTS

/*!
 * \brief Linked lists of each Ar object type.
 */
struct ObjectLists
{
    Thread * m_threads;
    Semaphore * m_semaphores;
    Mutex * m_mutexes;
    Queue * m_queues;
};

//! \brief Global containing lists of all Ar objects.
extern ObjectLists g_muAllObjects;

#endif // AR_GLOBAL_OBJECT_LISTS

} // namespace Ar

#endif // _AR_KERNEL_H_
//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
