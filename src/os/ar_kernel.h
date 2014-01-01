/*
 * Copyright (c) 2013 Immo Software
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

//! @brief The Argon RTOS namespace.
namespace Ar {

//! @addtogroup ar
//! @{

class Thread;

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

#define AR_GLOBAL_OBJECT_LISTS (0)

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
    
    //! The requested thread priority is invalid.
    kInvalidPriorityError,
    
    //! The thread's stack size is too small.
    kStackSizeTooSmallError,
    
    //! The requested operation cannot be performed from interrupt context.
    kNotFromInterruptError,
    
    //! The caller is not the owning thread.
    kNotOwnerError,
    
    //! The mutex is already unlocked.
    kAlreadyUnlockedError
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
 * This class provides functionality required by all Ar RTOS classes. It keeps a name associated
 * with each object instance. Having one copy of the name handling code makes it easy to disable in
 * order to reduce memory usage, if necessary.
 *
 * For debug builds, this class manages a linked list of object instances.
 */
class NamedObject
{
public:
    //! @brief Default constructor.
    NamedObject() {}
    
    //! @brief Constructor.
    NamedObject(const char * name) { init(name); }
    
    //! @brief Destructor.
    virtual ~NamedObject() {}
    
    //! @brief Initialiser.
    //!
    //! @param name The object's name. The pointer to the name is saved in the object.
    //!
    //! @retval kSuccess Initialisation was successful.
    status_t init(const char * name);
    
    //! @brief Access the object's name.
    const char * getName() const { return m_name; }
    
protected:
    const char * m_name;    //!< The object's name.

#if AR_GLOBAL_OBJECT_LISTS
    NamedObject * m_nextCreated;    //!< Linked list node.
    
    //! \brief Add to linked list of all created objects of a type.
    void addToCreatedList(NamedObject * & listHead);

    //! \brief Remove from linked list of all objects.
    void removeFromCreatedList(NamedObject * & listHead);
#endif // AR_GLOBAL_OBJECT_LISTS
};

/*!
 * @brief Preemptive thread class.
 *
 * @ingroup ar
 *
 * This thread class implements a preemptive threading system with priorities. The highest priority
 * thread that is ready to run will always get the processor. That means that if there is only one
 * high priority thread, it can starve lower priority threads if it never relinquishes control by
 * sleeping or blocking on a resource. Threads with the same priority will preempt each other in a
 * round robin order every system tick.
 *
 * Thread priorities range from 0 to 255. Higher values are higher priorities, with 255 being the
 * highest priority. Priority 0 is reserved for the idle thread.
 *
 * To create a thread, first allocate it either on the stack or with the new operator. Then call the
 * init() method passing in the name, entry point, entry parameter, stack information, and priority.
 * The entry point can be any non-member, i.e static, function that matches the #thread_entry_t
 * prototype. The init() method leaves the new thread suspended. To make the new thread eligible to
 * run you must call the resume() method on it.
 *
 * If you want to fully encapsulate a thread you can create a subclass of Thread that provides its
 * own init() method which calls the original Thread::init(). You can either pass a pointer to a
 * static function to the base init() method, as usual, or you can override the virtual
 * Thread::threadEntry() method. In the latter case, you can simply pass NULL for the entry point to
 * the base init() method. To pass values to the thread function, simply create member variables and
 * set them in your subclass' init() method.
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
    //! @brief Constructor.
    Thread() {}
    
    //! @brief Constructor.
    Thread(const char * name, thread_entry_t entry, void * param, void * stack, unsigned stackSize, uint8_t priority)
    {
        init(name, entry, param, stack, stackSize, priority);
    }
    
    //! @brief Destructor.
    virtual ~Thread();
    
    //! @name Thread init and cleanup
    //@{
    //! @brief Base initialiser.
    //!
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
    status_t init(const char * name, thread_entry_t entry, void * param, void * stack, unsigned stackSize, uint8_t priority);
    //@}

    //! @name Thread state
    //!
    //! Control of and access to the thread state.
    //@{
    //! @brief Put thread in suspended state.
    //!
    //! If this method is called from the current thread then the scheduler is entered immediately
    //! after putting the thread on the suspended list. Calling suspend() on another thread will not
    //! cause the scheduler to switch threads.
    //!
    //! Does not enter the scheduler if Ar is not running. Does nothing if the thread is already
    //! suspended.
    //!
    //! @todo Deal with all thread states properly.
    void suspend();

    //! @brief Make the thread eligible for execution.
    //!
    //! If the thread being resumed has a higher priority than that of the current thread, the
    //! scheduler is called to immediately switch threads. In this case the thread being resumed
    //! will always become the new current thread. This is because the highest priority thread is
    //! always guaranteed to be running, meaning the calling thread was the previous highest
    //! priority thread.
    //!
    //! Does not enter the scheduler if Ar is not running. Does nothing if the thread is already on
    //! the ready list.
    //!
    //! @todo Deal with all thread states properly.
    void resume();

    //! @brief Return the current state of the thread.
    thread_state_t getState() const { return m_state; }

    //! @brief Put the current thread to sleep for a certain amount of time.
    //!
    //! Does nothing if Ar is not running.
    //!
    //! @param ticks The number of operating system ticks to sleep the calling thread. A sleep time
    //!     of 0 is ignored.
    static void sleep(unsigned ticks);
    
    //! @brief Block the current thread until this thread completes.
    //!
    //! The thread of the caller is put to sleep for as long as the thread referenced by this thread
    //! object is running. Once this thread completes and its state is kThreadDone, the caller's
    //! thread is woken and this method call returns. If this thread takes longer than @a timeout to
    //! finish running, then #kTimeoutError is returned.
    //!
    //! @param timeout Timeout value in operating system ticks.
    //!
    //! @retval kSuccess
    //! @retval kTimeoutError
    status_t join(uint32_t timeout=kInfiniteTimeout);
    //@}

    //! @name Thread priority
    //!
    //! Accessors for the thread's priority.
    //@{
    //! @brief Return the thread's current priority.
    uint8_t getPriority() const { return m_priority; }

    //! @brief Change the thread's priority.
    //!
    //! The scheduler is invoked after the priority is set so that the current thread can be changed
    //! to the one with the highest priority. The scheduler is invoked even if there is no new
    //! highest priority thread. In this case, control may switch to the next thread with the same
    //! priority, assuming there is one.
    //!
    //! Does not enter the scheduler if Ar is not running.
    //!
    //! @param priority Thread priority level from 1 to 255, where lower numbers have a lower
    //!     priority. Priority number 0 is not allowed because it is reserved for the idle thread.
    //!
    //! @retval kInvalidPriorityError
    status_t setPriority(uint8_t priority);
    //@}

    //! @name Accessors
    //!
    //! Static members to get system-wide information.
    //@{
    //! @brief Returns the currently running thread object.
    static Thread * getCurrent() { return s_currentThread; }
    //@}
    
protected:
    
    //! @brief Virtual thread entry point.
    virtual void threadEntry();

protected:
    
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

    static Thread * s_readyList;    //!< Head of a linked list of ready threads.
    static Thread * s_suspendedList;    //!< Head of linked list of suspended threads.
    static Thread * s_sleepingList; //!< Head of linked list of sleeping threads.
    static Thread * s_currentThread;    //!< The currently running thread.
    
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
    //! @brief Bumps the system tick count and updates sleeping threads.
    static bool incrementTickCount(unsigned ticks);
    
    //! @brief Selects the next thread to run.
    static void scheduler();

    //! @brief Function to wrap the thread entry point.
    static void thread_wrapper(Thread * param);
    //@}

    // Friends have access to all protected members for efficiency.
    friend class Kernel;
    friend class Semaphore;     //!< Needs access to protected member functions.
    friend class Mutex;     //!< Needs access to protected member functions.
    friend class Queue; //!< Needs access to protected member functions.
    friend class Timer;
    
private:
    //! @brief The copy constructor is disabled for thread objects.
    Thread(const Thread & other) {}
};

template <typename T>
class ThreadToMemberFunction : public Thread
{
public:

    typedef void (T::*thread_member_entry_t)(void * param);
    
    ThreadToMemberFunction() {}
    
    ThreadToMemberFunction(const char * name, T * obj, thread_member_entry_t entry, void * param, void * stack, unsigned stackSize, uint8_t priority)
    {
        init(name, obj, entry, param, stack, stackSize, priority);
    }

    status_t init(const char * name, T * obj, thread_member_entry_t entry, void * param, void * stack, unsigned stackSize, uint8_t priority)
    {
        m_object = obj;
        m_entryMember = entry;
        return Thread::init(name, NULL, param, stack, stackSize, priority);
    }

protected:
    T * m_object;
    thread_member_entry_t m_entryMember;
    
    virtual void threadEntry()
    {
        (m_object->*m_entryMember)(m_param);
    }
    
};

/*!
 * @brief Template to create a thread and its stack.
 */
template <uint32_t S>
class ThreadWithStack : public Thread
{
public:
    ThreadWithStack() {}
    
    ThreadWithStack(const char * name, thread_entry_t entry, void * param, uint8_t priority)
    {
        Thread::init(name, entry, param, m_stack, S, priority);
    }
    
    status_t init(const char * name, thread_entry_t entry, void * param, uint8_t priority)
    {
        return Thread::init(name, entry, param, m_stack, S, priority);
    }

protected:
    uint8_t m_stack[S];
};

/*!
 * @brief Template to create a thread and its stack.
 */
template <uint32_t S, typename T>
class ThreadToMemberFunctionWithStack : public ThreadToMemberFunction<T>
{
public:
    
//     typedef ThreadToMemberFunction<T>::thread_member_entry_t thread_member_entry_t;
    typedef void (T::*thread_member_entry_t)(void * param);
    
    ThreadToMemberFunctionWithStack() {}
    
    ThreadToMemberFunctionWithStack(const char * name, T * obj, thread_member_entry_t entry, void * param, uint8_t priority)
    {
        ThreadToMemberFunction<T>::init(name, obj, entry, param, m_stack, S, priority);
    }
    
    status_t init(const char * name, T * obj, thread_member_entry_t entry, void * param, uint8_t priority)
    {
        return ThreadToMemberFunction<T>::init(name, obj, entry, param, m_stack, S, priority);
    }

protected:
    uint8_t m_stack[S];
};

/*!
 * @brief Counting semaphore class.
 *
 * @ingroup ar
 *
 * @see LockHolder
 */
class Semaphore : public NamedObject
{
public:
    //! @brief Default constructor.
    Semaphore() {}
    
    //! @brief Constructor.
    Semaphore(const char * name, unsigned count=1)
    {
        init(name, count);
    }
    
    //! @brief Initialiser.
    //!
    //! @param name Pass a name for the semaphore. If NULL is passed the name will be set to an
    //!     empty string.
    //! @param count The initial semaphore count. Setting this value to 0 will cause the first call
    //!     to get() to block until put() is called. A value of 1 or greater will allow that many
    //!     calls to get() to succeed.
    //!
    //! @retval kSuccess Semaphore initialised successfully.
    status_t init(const char * name, unsigned count=1);

    //! @brief Destructor.
    //!
    //! Any threads on the blocked list will be unblocked immediately. Their return status  from the
    //! get() method will be #kObjectDeletedError.
    virtual ~Semaphore();

    //! @brief Acquire the semaphore.
    //!
    //! The semaphore count is decremented. If the count is 0 upon entering this method then the
    //! caller thread is blocked until the count reaches 1. Threads are unblocked in the order in
    //! which they were blocked. Priority is not taken into consideration, so priority inversions
    //! are possible.
    //!
    //! @note This function may be called from interrupt context only if the timeout parameter is
    //!     set to #Ar::kNoTimeout (or 0).
    //!
    //! @param timeout The maximum number of milliseconds that the caller is willing to wait in a
    //!     blocked state before the semaphore can be obtained. If this value is 0, or #kNoTimeout,
    //!     then this method will return immediately if the semaphore cannot be obtained. Setting
    //!     the timeout to #kInfiniteTimeout will cause the thread to wait forever for a chance to
    //!     get the semaphore.
    //!
    //! @retval kSuccess The semaphore was obtained without error.
    //! @retval kTimeoutError The specified amount of time has elapsed before the semaphore could be
    //!     obtained.
    //! @retval kObjectDeletedError Another thread deleted the semaphore while the caller was
    //!     blocked on it.
    //! @retval kNotFromInterruptError A non-zero timeout is not alllowed from the interrupt
    //!     context.
    virtual status_t get(uint32_t timeout=kInfiniteTimeout);

    //! @brief Release the semaphore.
    //!
    //! The semaphore count is incremented.
    //!
    //! @note This call is safe from interrupt context.
    virtual status_t put();

    //! @brief Returns the current semaphore count.
    unsigned getCount() const { return m_count; }

protected:
    volatile unsigned m_count;  //!< Current semaphore count. Value of 0 means the semaphore is owned.
    Thread * m_blockedList; //!< Linked list of threads blocked on this semaphore.
};

/*!
 * @brief Utility class to automatically get and put a semaphore or mutex.
 *
 * @ingroup ar
 *
 * This class is intended to be stack allocated. It gets and holds a semaphore or mutex for the
 * duration of the scope in which it is declared. Once it goes out of scope, the destructor
 * automatically puts the lock.
 */
class LockHolder
{
public:
    //! @brief Constructor which gets the semaphore.
    //!
    //! Like the Semaphore::get() method, this constructor takes an optional timeout value which
    //! defaults to never timeout.
    LockHolder(Semaphore & sem, uint32_t timeout=kInfiniteTimeout)
    :   m_sem(sem)
    {
        m_sem.get(timeout);
    }
    
    //! @brief Destructor that puts the semaphore.
    ~LockHolder()
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
 * Very similar to a binary semaphore, except that a single thread can lock the mutex multiple times
 * without deadlocking. In this case, the number of calls to get() and put() must be matched.
 *
 * Because Mutex is a sublcass of Semaphore, it can be used anywhere that accepts a Semaphore
 * object. For instance, LockHolder works equally well for a Mutex.
 */
class Mutex : public Semaphore
{
public:
    //! @brief Default constructor.
    Mutex() {}
    
    //! @brief Constructor.
    Mutex(const char * name)
    {
        init(name);
    }
    
    //! @brief Initialiser.
    //!
    //! The mutex starts out unlocked.
    //!
    //! @param name The name of the mutex.
    //!
    //! @retval SUCCCESS
    status_t init(const char * name);

    //! @brief Cleanup.
    virtual ~Mutex();
    
    //! @brief Lock the mutex.
    //!
    //! If the thread that already owns the mutex calls get() more than once, a count is incremented
    //! rather than attempting to decrement the underlying semaphore again. The converse is true for
    //! put(), thus allowing a thread to lock a mutex any number of times as long as there are
    //! matching get() and put() calls.
    //!
    //! @param timeout The maximum number of milliseconds that the caller is willing to wait in a
    //!     blocked state before the semaphore can be obtained. If this value is 0, or #kNoTimeout,
    //!     then this method will return immediately if the lock cannot be obtained. Setting
    //!     the timeout to #kInfiniteTimeout will cause the thread to wait forever for a chance to
    //!     get the lock.
    //!
    //! @retval kSuccess The mutex was obtained without error.
    //! @retval kTimeoutError The specified amount of time has elapsed before the mutex could be
    //!     obtained.
    //! @retval kObjectDeletedError Another thread deleted the semaphore while the caller was
    //!     blocked on it.
    virtual status_t get(uint32_t timeout=kInfiniteTimeout);
    
    //! @brief Unlock the mutex.
    //!
    //! Only the owning thread is allowed to unlock the mutex. If the owning thread has called get()
    //! multiple times, it must also call put() the same number of time before the underlying
    //! semaphore is actually released. It is illegal to call put() when the mutex is not owned by
    //! the calling thread.
    //!
    //! @retval kAlreadyUnlockedError The mutex is not locked.
    //! @retval kNotOwnerError The caller is not the thread that owns the mutex.
    virtual status_t put();
    
    //! @brief Returns the current owning thread, if there is one.
    Thread * getOwner() { return (Thread *)m_owner; }

protected:
    volatile Thread * m_owner;  //!< Current owner thread of the mutex.
    volatile unsigned m_ownerLockCount; //!< Number of times the owner thread has locked the mutex.
    uint8_t m_originalPriority; //!< Original priority of the owner thread before its priority was raised.
};

/*!
 * @brief A blocking queue for inter-thread messaging.
 *
 * @ingroup ar
 */
class Queue : public NamedObject
{
public:
    //! @brief Default constructor.
    Queue() {}
    
    //! @brief Constructor.
    Queue(const char * name, void * storage, unsigned elementSize, unsigned capacity)
    {
        init(name, storage, elementSize, capacity);
    }
    
    //! @brief Queue initialiser.
    //!
    //! @param name The new queue's name.
    //! @param storage Pointer to a buffer used to store queue elements. The buffer must be at least
    //!     @a elementSize * @a capacity bytes big.
    //! @param elementSize Size in bytes of each element in the queue.
    //! @param capacity The number of elements that the buffer pointed to by @a storage will hold.
    //!
    //! @retval kSuccess The queue was initialised.
    status_t init(const char * name, void * storage, unsigned elementSize, unsigned capacity);
    
    //! @brief Queue cleanup.
    virtual ~Queue();
    
    //! @brief Add an item to the queue.
    //!
    //! The caller will block if the queue is full.
    //!
    //! @param element Pointer to the element to post to the queue. The element size was specified
    //!     in the init() call.
    //! @param timeout The maximum number of milliseconds that the caller is willing to wait in a
    //!     blocked state before the element can be sent. If this value is 0, or #kNoTimeout, then
    //!     this method will return immediately if the queue is full. Setting the timeout to
    //!     #kInfiniteTimeout will cause the thread to wait forever for a chance to send.
    //!
    //! @retval kSuccess
    //! @retval kQueueFullError
    status_t send(const void * element, uint32_t timeout=kInfiniteTimeout);

    //! @brief Remove an item from the queue.
    //!
    //! @param[out] element
    //! @param timeout The maximum number of milliseconds that the caller is willing to wait in a
    //!     blocked state before an element is received. If this value is 0, or #kNoTimeout, then
    //!     this method will return immediately if the queue is empty. Setting the timeout to
    //!     #kInfiniteTimeout will cause the thread to wait forever for receive an element.
    //!
    //! @retval kSuccess
    //! @retval kQueueEmptyError
    status_t receive(void * element, uint32_t timeout=kInfiniteTimeout);
    
    //! @brief Returns whether the queue is currently empty.
    bool isEmpty() const { return m_count == 0; }
    
    //! @brief Returns the current number of elements in the queue.
    unsigned getCount() const { return m_count; }

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
 * This template class helps create a Queue instance by defining a static array of queue elements.
 * The array length is one of the template parameters.
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
    //! @brief Default constructor.
    StaticQueue() {}
    
    //! @brief Constructor.
    StaticQueue(const char * name)
    {
        Queue::init(name, m_storage, sizeof(T), N);
    }
    
    //! @brief Initialiser method.
    status_t init(const char * name)
    {
        return Queue::init(name, m_storage, sizeof(T), N);
    }
    
    //! @copydoc Queue::send()
    status_t send(T element, uint32_t timeout=kInfiniteTimeout)
    {
        return Queue::send((const void *)&element, timeout);
    }
    
    //! @copydoc Queue::receive()
    status_t receive(T * element, uint32_t timeout=kInfiniteTimeout)
    {
        return Queue::receive((void *)element, timeout);
    }
    
    //! @brief Alternate form of typed receive.
    //!
    //! @param[out] resultStatus The status of the receive operation is placed here.
    //!     May be NULL, in which case no status is returned.
    //! @param timeout Maximum time in ticks to wait for a queue element.
    T receive(uint32_t timeout=kInfiniteTimeout, status_t * resultStatus=NULL)
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

/*!
 * @brief Timer object.
 */
class Timer : public NamedObject
{
public:
    
    //! @brief Modes of operation for timers.
    typedef enum _timer_modes {
        kOneShotTimer,
        kPeriodicTimer
    } timer_mode_t;
    
    //! @brief
    typedef void (*timer_entry_t)(Timer * timer, void * param);
    
    //! @brief Default constructor.
    Timer() {}
    
    //! @brief Constructor.
    Timer(const char * name, timer_entry_t callback, void * param, timer_mode_t timerMode, uint32_t delay)
    {
        init(name, callback, param, timerMode, delay);
    }
    
    //! @brief Destructor.
    virtual ~Timer() {}
    
    //! @brief
    void init(const char * name, timer_entry_t callback, void * param, timer_mode_t timerMode, uint32_t delay);
    
    //! @brief
    void start();
    
    //! @brief
    void stop();
    
    //! @brief
    void setDelay(uint32_t delay);

    //! @brief
    uint32_t getDelay() const { return m_delay; }
    
protected:
    
    timer_entry_t m_callback;
    Timer * m_next;
    void * m_param;
    timer_mode_t m_mode;
    uint32_t m_delay;
    uint32_t m_wakeupTime;
    bool m_isActive;
    
    static Timer * s_activeTimers;

    void addToList();
    void removeFromList();
    
    friend class Thread;
    friend class Kernel;
    
};

/*!
 * @brief RTOS core.
 *
 * @ingroup ar
 */
class Kernel
{
public:
    
    //! @name RTOS control
    //@{
    //! @brief Enables the tick timer and switches to a ready thread.
    //!
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
    static void run();
    //@}

    //! @name Accessors
    //@{
    //! @brief Returns the current tick count.
    static uint32_t getTickCount() { return s_tickCount; }
    
    //! @brief Returns the current system load average.
    static unsigned getSystemLoad() { return s_systemLoad; }
    
    //! @brief Whether the scheduler is running.
    static bool isRunning() { return s_isRunning; }
    //@}
    
    //! @name Interrupt handling
    //!
    //! Routines to inform the kernel of user interrupt handler execution.
    //@{
    //! @brief Inform the kernel that an interrupt handler is running.
    static void enterInterrupt();
    
    //! @brief Inform the kernel that an interrupt handler is exiting.
    static void exitInterrupt();
    //@}

    //! @brief Handles the periodic timer tick interrupt.
    static void periodicTimerIsr();

    //! @brief Handles the software interrupt to invoke the scheduler.
    static uint32_t yieldIsr(uint32_t topOfStack);

protected:

    static bool s_isRunning;    //!< True if the kernel has been started.
    static volatile uint32_t s_tickCount;   //!< Current tick count.
    static volatile uint32_t s_irqDepth;    //!< Current level of nested IRQs, or 0 if in user mode.
    static volatile unsigned s_systemLoad;   //!< Percent of system load from 0-100.

    //! @name Idle thread members
    //@{
    //! The stack for the idle thread.
    static uint8_t s_idleThreadStack[];
    
    //! The lowest priority thread in the system. Executes only when no other
    //! threads are ready.
    static Thread s_idleThread;
    //@}
    
    static uint32_t getIrqDepth() { return s_irqDepth; }

    //! @brief Force entry into the scheduler.
    static void enterScheduler();
    
    //! @brief Sets up the periodic system tick timer.
    static void initTimerInterrupt();
    
    //! @brief
    static void initSystem();

    //! @brief Idle thread entry point.
    static void idle_entry(void * param);

    friend class Thread;
    friend class Semaphore;
    friend class Queue;
    friend class Timer;
    friend class InterruptWrapper;

private:

    //! @brief The constructor is private so nobody can create an instance.
    Kernel() {}

    //! @brief The copy constructor is disabled by being private.
    Kernel(const Kernel & other) {}
    
};

//! @brief %Time related utilities.
namespace Time {

//! @brief Get the number of milliseconds per tick.
//!
//! @ingroup ar
uint32_t getMillisecondsPerTick();

//! @name Time conversion
//@{
//! @brief Convert ticks to milliseconds.
//!
//! @ingroup ar
inline uint32_t ticksToMilliseconds(uint32_t ticks) { return ticks * getMillisecondsPerTick(); }

//! @brief Convert milliseconds to ticks.
//!
//! @ingroup ar
inline uint32_t millisecondsToTicks(uint32_t millisecs) { return millisecs / getMillisecondsPerTick(); }
//@}

} // namespace Time

//! @brief %Atomic operations.
namespace Atomic {

//! @brief %Atomic add.
void add(uint32_t * value, uint32_t delta);

//! @brief %Atomic increment.
inline void increment(uint32_t * value) { add(value, 1); }

//! @brief %Atomic decrement.
inline void decrement(uint32_t * value) { add(value, -1); }

//! @brief %Atomic compare-and-swap operation.
bool compareAndSwap(uint32_t * value, uint32_t expectedValue, uint32_t newValue);

} // namespace Atomic

/*!
 * @brief Helper class for managing user interrupt handlers.
 */
class InterruptWrapper
{
public:

    //! @brief Increments interrupt depth.
    InterruptWrapper() { Kernel::enterInterrupt(); }
    
    //! @brief Decrements interrupt depth.
    ~InterruptWrapper() { Kernel::exitInterrupt(); }
    
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
extern ObjectLists g_allObjects;

#endif // AR_GLOBAL_OBJECT_LISTS

} // namespace Ar

#endif // _AR_KERNEL_H_
//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
