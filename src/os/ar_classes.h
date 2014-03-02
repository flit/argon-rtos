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
 * @brief Header for the Argon RTOS wrapper classes.
 * @ingroup ar
 */

#if !defined(_AR_CLASSES_H_)
#define _AR_CLASSES_H_

#include "ar_kernel.h"

#if defined(__cplusplus)

//! @brief The Argon RTOS namespace.
namespace Ar {

//------------------------------------------------------------------------------
// Classes
//------------------------------------------------------------------------------

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
class Thread : public _ar_thread
{
public:
    //! @brief Constructor.
    Thread() {}
    
    //! @brief Constructor.
    Thread(const char * name, ar_thread_entry_t entry, void * param, void * stack, unsigned stackSize, uint8_t priority)
    {
        init(name, entry, param, stack, stackSize, priority);
    }
    
    //! @brief Destructor.
    virtual ~Thread() { ar_thread_delete(this); }
    
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
    status_t init(const char * name, ar_thread_entry_t entry, void * param, void * stack, unsigned stackSize, uint8_t priority);
    //@}
    
    //! @brief Get the thread's name.
    const char * getName() const { return m_name; }

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
    void suspend() { ar_thread_suspend(this); }

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
    void resume() { ar_thread_resume(this); }

    //! @brief Return the current state of the thread.
    ar_thread_state_t getState() const { return m_state; }

    //! @brief Put the current thread to sleep for a certain amount of time.
    //!
    //! Does nothing if Ar is not running.
    //!
    //! @param ticks The number of milliseconds to sleep the calling thread. A sleep time
    //!     of 0 is ignored.
    static void sleep(unsigned milliseconds) { ar_thread_sleep(milliseconds); }
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
    status_t setPriority(uint8_t priority) { return ar_thread_set_priority(this, priority); }
    //@}

    //! @name Accessors
    //!
    //! Static members to get system-wide information.
    //@{
    //! @brief Returns the currently running thread object.
    static Thread * getCurrent() { return reinterpret_cast<Thread *>(ar_thread_get_current()->m_ref); }
    //@}
    
protected:

    ar_thread_entry_t m_userEntry;
    
    //! @brief Virtual thread entry point.
    virtual void threadEntry(void * param);

    static void thread_entry(void * param);

private:
    //! @brief The copy constructor is disabled for thread objects.
    Thread(const Thread & other) {}
};

/*!
 * @brief Template to create a thread whose entry point is a member of a class.
 *
 * @ingroup ar
 */
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
        return Thread::init(name, thread_entry, param, stack, stackSize, priority);
    }

protected:
    T * m_object;
    thread_member_entry_t m_entryMember;
    
    virtual void threadEntry(void * param)
    {
        (m_object->*m_entryMember)(param);
    }
    
};

/*!
 * @brief Template to create a thread and its stack.
 *
 * @ingroup ar
 */
template <uint32_t S>
class ThreadWithStack : public Thread
{
public:
    ThreadWithStack() {}
    
    ThreadWithStack(const char * name, ar_thread_entry_t entry, void * param, uint8_t priority)
    {
        Thread::init(name, entry, param, m_stack, S, priority);
    }
    
    status_t init(const char * name, ar_thread_entry_t entry, void * param, uint8_t priority)
    {
        return Thread::init(name, entry, param, m_stack, S, priority);
    }

protected:
    uint8_t m_stack[S];
};

/*!
 * @brief Template to create a thread and its stack, with a member function entry point.
 *
 * @ingroup ar
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
class Semaphore : public _ar_semaphore
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
    status_t init(const char * name, unsigned count=1) { return ar_semaphore_create(this, name, count); }

    //! @brief Destructor.
    //!
    //! Any threads on the blocked list will be unblocked immediately. Their return status  from the
    //! get() method will be #kObjectDeletedError.
    virtual ~Semaphore() { ar_semaphore_delete(this); }

    //! @brief Get the semaphore's name.
    const char * getName() const { return m_name; }

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
    status_t get(uint32_t timeout=kArInfiniteTimeout) { return ar_semaphore_get(this, timeout); }

    //! @brief Release the semaphore.
    //!
    //! The semaphore count is incremented.
    //!
    //! @note This call is safe from interrupt context.
    status_t put() { return ar_semaphore_put(this); }

    //! @brief Returns the current semaphore count.
    unsigned getCount() const { return m_count; }

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
    LockHolder(Semaphore & sem, uint32_t timeout=kArInfiniteTimeout)
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
class Mutex : public _ar_mutex
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
    status_t init(const char * name) { return ar_mutex_create(this, name); }

    //! @brief Cleanup.
    virtual ~Mutex() { ar_mutex_delete(this); }
    
    //! @brief Get the mutex's name.
    const char * getName() const { return m_sem.m_name; }

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
    status_t get(uint32_t timeout=kArInfiniteTimeout) { return ar_mutex_get(this, timeout); }
    
    //! @brief Unlock the mutex.
    //!
    //! Only the owning thread is allowed to unlock the mutex. If the owning thread has called get()
    //! multiple times, it must also call put() the same number of time before the underlying
    //! semaphore is actually released. It is illegal to call put() when the mutex is not owned by
    //! the calling thread.
    //!
    //! @retval kAlreadyUnlockedError The mutex is not locked.
    //! @retval kNotOwnerError The caller is not the thread that owns the mutex.
    status_t put() { return ar_mutex_put(this); }
    
    //! @brief Returns the current owning thread, if there is one.
    ar_thread_t * getOwner() { return (ar_thread_t *)m_owner; }

};

/*!
 * @brief A blocking queue for inter-thread messaging.
 *
 * @ingroup ar
 */
class Queue : public _ar_queue
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
    status_t init(const char * name, void * storage, unsigned elementSize, unsigned capacity)
    {
        return ar_queue_create(this, name, storage, elementSize, capacity);
    }
    
    //! @brief Queue cleanup.
    virtual ~Queue() { ar_queue_delete(this); }
    
    //! @brief Get the queue's name.
    const char * getName() const { return m_name; }

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
    status_t send(const void * element, uint32_t timeout=kArInfiniteTimeout) { return ar_queue_send(this, element, timeout); }

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
    status_t receive(void * element, uint32_t timeout=kArInfiniteTimeout) { return ar_queue_receive(this, element, timeout); }
    
    //! @brief Returns whether the queue is currently empty.
    bool isEmpty() const { return m_count == 0; }
    
    //! @brief Returns the current number of elements in the queue.
    unsigned getCount() const { return m_count; }

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
    status_t send(T element, uint32_t timeout=kArInfiniteTimeout)
    {
        return Queue::send((const void *)&element, timeout);
    }
    
    //! @copydoc Queue::receive()
    status_t receive(T * element, uint32_t timeout=kArInfiniteTimeout)
    {
        return Queue::receive((void *)element, timeout);
    }
    
    //! @brief Alternate form of typed receive.
    //!
    //! @param[out] resultStatus The status of the receive operation is placed here.
    //!     May be NULL, in which case no status is returned.
    //! @param timeout Maximum time in ticks to wait for a queue element.
    T receive(uint32_t timeout=kArInfiniteTimeout, status_t * resultStatus=NULL)
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
 *
 * @ingroup ar
 */
class Timer : public _ar_timer
{
public:
    
    //! @brief Timer callback function that takes an instance of this class.
    typedef void (*entry_t)(Timer * timer, void * param);
    
    //! @brief Default constructor.
    Timer() {}
    
    //! @brief Constructor.
    Timer(const char * name, entry_t callback, void * param, ar_timer_mode_t timerMode, uint32_t delay)
    {
        init(name, callback, param, timerMode, delay);
    }
    
    //! @brief Destructor.
    virtual ~Timer() { ar_timer_delete(this); }
    
    //! @brief Initialize the timer.
    status_t init(const char * name, entry_t callback, void * param, ar_timer_mode_t timerMode, uint32_t delay);
    
    //! @brief Get the timer's name.
    const char * getName() const { return m_name; }

    //! @brief Start the timer running.
    void start() { ar_timer_start(this); }
    
    //! @brief Stop the timer.
    void stop() { ar_timer_stop(this); }
    
    //! @brief Returns whether the timer is currently running.
    bool isActive() const { return m_isActive; }
    
    //! @brief Adjust the timer's delay.
    void setDelay(uint32_t delay) { ar_timer_set_delay(this, delay); }

    //! @brief Get the current delay for the timer.
    uint32_t getDelay() const { return m_delay; }
    
protected:

    entry_t m_userCallback;    //!< The user timer callback.

    //! @brief Converts the timer struct to an instance of this class.
    static void timer_wrapper(ar_timer_t * timer, void * arg);

};

/*!
 * @brief Helper class for managing user interrupt handlers.
 *
 * @ingroup ar
 */
class InterruptWrapper
{
public:

    //! @brief Increments interrupt depth.
    InterruptWrapper() { ar_kernel_enter_interrupt(); }
    
    //! @brief Decrements interrupt depth.
    ~InterruptWrapper() { ar_kernel_exit_interrupt(); }
    
};

} // namespace Ar

#endif // defined(__cplusplus)

#endif // _AR_CLASSES_H_
//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
