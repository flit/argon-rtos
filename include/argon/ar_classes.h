/*
 * Copyright (c) 2007-2016 Immo Software
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
#include <string.h>

#if defined(__cplusplus)

//! @brief The Argon RTOS namespace.
namespace Ar {

//------------------------------------------------------------------------------
// Classes
//------------------------------------------------------------------------------

/*!
 * @brief Preemptive thread class.
 *
 * @ingroup ar_thread
 *
 * This thread class implements a preemptive thread with variable priority.
 *
 * Threads may be allocated either globally or with the new operator. You can also allocate a
 * thread locally, but you must be careful to keep the stack valid as long as the thread is running.
 * There are two options for initialization. Either use one of the non-default constructors, or
 * use the default constructor and call an init() method at some later point. Both the constructors
 * and init methods take the same arguments. They accept a name, entry point, stack, and priority.
 *
 * The entry point can be a global or static function that matches the #ar_thread_entry_t
 * prototype. Alternatively, there are constructor and init() variants that let you use a member
 * function of a specific object as the entry point.
 *
 * The init() method leaves the new thread suspended. To make the new thread eligible to
 * run you must call the resume() method on it.
 */
class Thread : public _ar_thread
{
public:
    //! @brief Default constructor.
    Thread() {}

    //! @brief Constructor.
    //!
    //! @param name Name of the thread. If NULL, the thread's name is set to an empty string.
    //! @param entry Thread entry point taking one parameter and returning void.
    //! @param param Arbitrary pointer-sized value passed as the single parameter to the thread
    //!     entry point.
    //! @param stack Pointer to the start of the thread's stack. This should be the stack's bottom,
    //!     not it's top. If this parameter is NULL, the stack will be dynamically allocated.
    //! @param stackSize Number of bytes of stack space allocated to the thread. This value is
    //!     added to @a stack to get the initial top of stack address.
    //! @param priority Thread priority. The accepted range is 1 through 255. Priority 0 is
    //!     reserved for the idle thread.
    //! @param startImmediately Whether the new thread will start to run automatically. If false, the
    //!     thread will be created in a suspended state. The constants #kArStartThread and
    //!     #kArSuspendThread can be used to better document this parameter value.
    Thread(const char * name, ar_thread_entry_t entry, void * param, void * stack, unsigned stackSize, uint8_t priority, bool startImmediately=true)
    {
        init(name, entry, param, stack, stackSize, priority, startImmediately);
    }

    //! @brief Constructor to set the thread entry to a member function.
    //!
    //! @param name Name of the thread. If NULL, the thread's name is set to an empty string.
    //! @param object Pointer to an instance of class T upon which the @a entry member function
    //!     will be invoked when the thread is started.
    //! @param entry Member function of class T that will be used as the thread's entry point.
    //!     The member function must take no parameters and return void.
    //! @param stack Pointer to the start of the thread's stack. This should be the stack's bottom,
    //!     not it's top. If this parameter is NULL, the stack will be dynamically allocated.
    //! @param stackSize Number of bytes of stack space allocated to the thread. This value is
    //!     added to @a stack to get the initial top of stack address.
    //! @param priority Thread priority. The accepted range is 1 through 255. Priority 0 is
    //!     reserved for the idle thread.
    //! @param startImmediately Whether the new thread will start to run automatically. If false, the
    //!     thread will be created in a suspended state. The constants #kArStartThread and
    //!     #kArSuspendThread can be used to better document this parameter value.
    template <class T>
    Thread(const char * name, T * object, void (T::*entry)(), void * stack, unsigned stackSize, uint8_t priority, bool startImmediately=true)
    {
        init<T>(name, object, entry, stack, stackSize, priority, startImmediately);
    }

    //! @brief Constructor to dynamically allocate the stack.
    //!
    //! @param name Name of the thread. If NULL, the thread's name is set to an empty string.
    //! @param entry Thread entry point taking one parameter and returning void.
    //! @param param Arbitrary pointer-sized value passed as the single parameter to the thread
    //!     entry point.
    //! @param stackSize Number of bytes of stack space to allocate via <tt>new</tt> to the thread.
    //! @param priority Thread priority. The accepted range is 1 through 255. Priority 0 is
    //!     reserved for the idle thread.
    //! @param startImmediately Whether the new thread will start to run automatically. If false, the
    //!     thread will be created in a suspended state. The constants #kArStartThread and
    //!     #kArSuspendThread can be used to better document this parameter value.
    Thread(const char * name, ar_thread_entry_t entry, void * param, unsigned stackSize, uint8_t priority, bool startImmediately=true)
    {
        init(name, entry, param, NULL, stackSize, priority, startImmediately);
    }

    //! @brief Constructor to set the thread entry to a member function, using a dynamic stack.
    //!
    //! @param name Name of the thread. If NULL, the thread's name is set to an empty string.
    //! @param object Pointer to an instance of class T upon which the @a entry member function
    //!     will be invoked when the thread is started.
    //! @param entry Member function of class T that will be used as the thread's entry point.
    //!     The member function must take no parameters and return void.
    //! @param stackSize Number of bytes of stack space to allocate via <tt>new</tt> to the thread.
    //! @param priority Thread priority. The accepted range is 1 through 255. Priority 0 is
    //!     reserved for the idle thread.
    //! @param startImmediately Whether the new thread will start to run automatically. If false, the
    //!     thread will be created in a suspended state. The constants #kArStartThread and
    //!     #kArSuspendThread can be used to better document this parameter value.
    template <class T>
    Thread(const char * name, T * object, void (T::*entry)(), unsigned stackSize, uint8_t priority, bool startImmediately=true)
    {
        init<T>(name, object, entry, NULL, stackSize, priority, startImmediately);
    }

    //! @brief Destructor.
    virtual ~Thread();

    //! @name Thread init and cleanup
    //@{
    //! @brief Base initialiser.
    //!
    //! The thread is in suspended state when this method exits. To make it eligible for
    //! execution, call the resume() method.
    //!
    //! @param name Name of the thread. If NULL, the thread's name is set to an empty string.
    //! @param entry Thread entry point taking one parameter and returning void.
    //! @param param Arbitrary pointer-sized value passed as the single parameter to the thread
    //!     entry point.
    //! @param stack Pointer to the start of the thread's stack. This should be the stack's bottom,
    //!     not it's top. If this parameter is NULL, the stack will be dynamically allocated.
    //! @param stackSize Number of bytes of stack space allocated to the thread. This value is
    //!     added to @a stack to get the initial top of stack address.
    //! @param priority Thread priority. The accepted range is 1 through 255. Priority 0 is
    //!     reserved for the idle thread.
    //! @param startImmediately Whether the new thread will start to run automatically. If false, the
    //!     thread will be created in a suspended state. The constants #kArStartThread and
    //!     #kArSuspendThread can be used to better document this parameter value.
    //!
    //! @retval #kArSuccess The thread was initialised without error.
    //! @retval #kArOutOfMemoryError Failed to dynamically allocate the stack.
    ar_status_t init(const char * name, ar_thread_entry_t entry, void * param, void * stack, unsigned stackSize, uint8_t priority, bool startImmediately=true);

    //! @brief Initializer to set the thread entry to a member function.
    //!
    //! @param name Name of the thread. If NULL, the thread's name is set to an empty string.
    //! @param object Pointer to an instance of class T upon which the @a entry member function
    //!     will be invoked when the thread is started.
    //! @param entry Member function of class T that will be used as the thread's entry point.
    //!     The member function must take no parameters and return void.
    //! @param stack Pointer to the start of the thread's stack. This should be the stack's bottom,
    //!     not it's top. If this parameter is NULL, the stack will be dynamically allocated.
    //! @param stackSize Number of bytes of stack space allocated to the thread. This value is
    //!     added to @a stack to get the initial top of stack address.
    //! @param priority Thread priority. The accepted range is 1 through 255. Priority 0 is
    //!     reserved for the idle thread.
    //! @param startImmediately Whether the new thread will start to run automatically. If false, the
    //!     thread will be created in a suspended state. The constants #kArStartThread and
    //!     #kArSuspendThread can be used to better document this parameter value.
    //!
    //! @retval #kArSuccess The thread was initialised without error.
    //! @retval #kArOutOfMemoryError Failed to dynamically allocate the stack.
    template <class T>
    ar_status_t init(const char * name, T * object, void (T::*entry)(), void * stack, unsigned stackSize, uint8_t priority, bool startImmediately=true)
    {
        return initForMemberFunction(name, object, member_thread_entry<T>, &entry, sizeof(entry), stack, stackSize, priority, startImmediately);
    }
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
    //! A sleeping thread can be woken early by calling ar_thread_resume().
    //!
    //! @param milliseconds The number of milliseconds to sleep the calling thread. A sleep time
    //!     of 0 is ignored. If the sleep time is shorter than the scheduler quanta, then the thread
    //!     will not actually sleep. If #kArInfiniteTimeout is passed for the sleep time, the thread
    //!     will simply be suspended.
    static void sleep(unsigned milliseconds) { ar_thread_sleep(milliseconds); }

    //! @brief Put the current thread to sleep until a specific time.
    //!
    //! Does nothing if Ar is not running.
    //!
    //! A sleeping thread can be woken early by calling ar_thread_resume().
    //!
    //! @param wakeup The wakeup time in milliseconds. If the time is not in the future, i.e., less than
    //!  or equal to the current value returned by ar_get_millisecond_count(), then the sleep request is
    //!  ignored.
    static void sleepUntil(unsigned wakeup) { ar_thread_sleep_until(wakeup); }
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
    //! @retval #kArSuccess
    //! @retval #kArInvalidPriorityError
    ar_status_t setPriority(uint8_t priority) { return ar_thread_set_priority(this, priority); }
    //@}

    //! @name Info
    //@{
    //! @brief Get the thread's system load.
    uint32_t getLoad() { return ar_thread_get_load(this); }

    //! @brief Get the thread's maximum stack usage.
    uint32_t getStackUsed() { return ar_thread_get_stack_used(this); }
    //@}

    //! @name Accessors
    //!
    //! Static members to get system-wide information.
    //@{
    //! @brief Returns the currently running thread object.
    static Thread * getCurrent() { return static_cast<Thread *>(ar_thread_get_current()); }
    //@}

protected:

    uint8_t * m_allocatedStack; //!< Dynamically allocated stack.
    ar_thread_entry_t m_userEntry;  //!< User-specified thread entry point function.

    //! @brief Virtual thread entry point.
    //!
    //! This is the method that subclasses should override.
    virtual void threadEntry(void * param);

    //! @brief Static thread entry callback to invoke the virtual method.
    static void thread_entry(void * param);

    //! @brief Template function to invoke a thread entry point that is a member function.
    template <class T>
    static void member_thread_entry(void * param)
    {
        Thread * thread = getCurrent();
        T * obj = static_cast<T *>(param);
        void (T::*member)(void);
        uint32_t * storage = thread->m_stackBottom + 1; // Add 1 to skip over check value.
        memcpy((char*)&member, storage, sizeof(member));
        (obj->*member)();
    }

    //! @brief Special init method to deal with member functions.
    ar_status_t initForMemberFunction(const char * name, void * object, ar_thread_entry_t entry, void * memberPointer, uint32_t memberPointerSize, void * stack, unsigned stackSize, uint8_t priority, bool startImmediately);

private:
    //! @brief The copy constructor is disabled for thread objects.
    Thread(const Thread & other);

    //! @brief Disable assignment operator.
    Thread& operator=(const Thread & other);
};

/*!
 * @brief Template to create a thread and its stack.
 *
 * @ingroup ar_thread
 */
template <uint32_t S>
class ThreadWithStack : public Thread
{
public:
    //! @brief Default constructor.
    ThreadWithStack() {}

    //! @brief Constructor to use a normal function as entry poin.
    ThreadWithStack(const char * name, ar_thread_entry_t entry, void * param, uint8_t priority, bool startImmediately=true)
    {
        Thread::init(name, entry, param, m_stack, S, priority, startImmediately);
    }

    //! @brief Constructor to set the thread entry to a member function.
    template <class T>
    ThreadWithStack(const char * name, T * object, void (T::*entry)(), uint8_t priority, bool startImmediately=true)
    {
        Thread::init<T>(name, object, entry, m_stack, S, priority, startImmediately);
    }

    //! @brief Initializer to use a normal function as entry point.
    ar_status_t init(const char * name, ar_thread_entry_t entry, void * param, uint8_t priority, bool startImmediately=true)
    {
        return Thread::init(name, entry, param, m_stack, S, priority, startImmediately);
    }

    //! @brief Initializer to set the thread entry to a member function.
    template <class T>
    ar_status_t init(const char * name, T * object, void (T::*entry)(), uint8_t priority, bool startImmediately=true)
    {
        return Thread::init<T>(name, object, entry, m_stack, S, priority, startImmediately);
    }

protected:
    uint8_t m_stack[S]; //!< Stack space for the thread.

private:
    //! @brief The copy constructor is disabled for thread objects.
    ThreadWithStack(const ThreadWithStack<S> & other);

    //! @brief Disable assignment operator.
    ThreadWithStack& operator=(const ThreadWithStack<S> & other);
};

/*!
 * @brief Counting semaphore class.
 *
 * @ingroup ar_sem
 *
 * @see Semaphore::Guard
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
    //! @retval #kArSuccess Semaphore initialised successfully.
    ar_status_t init(const char * name, unsigned count=1) { return ar_semaphore_create(this, name, count); }

    //! @brief Destructor.
    //!
    //! Any threads on the blocked list will be unblocked immediately. Their return status from the
    //! get() method will be #kArObjectDeletedError.
    ~Semaphore() { ar_semaphore_delete(this); }

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
    //!     set to #kArNoTimeout (or 0).
    //!
    //! @param timeout The maximum number of milliseconds that the caller is willing to wait in a
    //!     blocked state before the semaphore can be obtained. If this value is 0, or #kArNoTimeout,
    //!     then this method will return immediately if the semaphore cannot be obtained. Setting
    //!     the timeout to #kArInfiniteTimeout will cause the thread to wait forever for a chance to
    //!     get the semaphore.
    //!
    //! @retval #kArSuccess The semaphore was obtained without error.
    //! @retval #kArTimeoutError The specified amount of time has elapsed before the semaphore could be
    //!     obtained.
    //! @retval #kArObjectDeletedError Another thread deleted the semaphore while the caller was
    //!     blocked on it.
    //! @retval #kArNotFromInterruptError A non-zero timeout is not alllowed from the interrupt
    //!     context.
    ar_status_t get(uint32_t timeout=kArInfiniteTimeout) { return ar_semaphore_get(this, timeout); }

    //! @brief Release the semaphore.
    //!
    //! The semaphore count is incremented.
    //!
    //! @note This call is safe from interrupt context.
    //!
    //! @retval #kArSuccess The semaphore was released without error.
    ar_status_t put() { return ar_semaphore_put(this); }

    //! @brief Returns the current semaphore count.
    unsigned getCount() const { return m_count; }

private:
    //! @brief Disable copy constructor.
    Semaphore(const Semaphore & other);

    //! @brief Disable assignment operator.
    Semaphore& operator=(const Semaphore & other);
};

/*!
 * @brief Mutex object.
 *
 * @ingroup ar_mutex
 *
 * Very similar to a binary semaphore, except that a single thread can lock the mutex multiple times
 * without deadlocking. In this case, the number of calls to get() and put() must be matched.
 *
 * Another difference from semaphores is that mutexes support priority inheritance. If a high-
 * priority thread blocks on a mutex held by a lower-priority thread, the thread holding the mutex
 * will have its priority temporarily raised to the priority of the thread waiting on the mutex.
 *
 * @see Mutex::Guard
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
    ar_status_t init(const char * name) { return ar_mutex_create(this, name); }

    //! @brief Cleanup.
    ~Mutex() { ar_mutex_delete(this); }

    //! @brief Get the mutex's name.
    const char * getName() const { return m_name; }

    //! @brief Lock the mutex.
    //!
    //! If the thread that already owns the mutex calls get() more than once, a count is incremented
    //! rather than attempting to decrement the underlying semaphore again. The converse is true for
    //! put(), thus allowing a thread to lock a mutex any number of times as long as there are
    //! matching get() and put() calls.
    //!
    //! @param timeout The maximum number of milliseconds that the caller is willing to wait in a
    //!     blocked state before the semaphore can be obtained. If this value is 0, or #kArNoTimeout,
    //!     then this method will return immediately if the lock cannot be obtained. Setting
    //!     the timeout to #kArInfiniteTimeout will cause the thread to wait forever for a chance to
    //!     get the lock.
    //!
    //! @retval #kArSuccess The mutex was obtained without error.
    //! @retval #kArTimeoutError The specified amount of time has elapsed before the mutex could be
    //!     obtained.
    //! @retval #kArObjectDeletedError Another thread deleted the semaphore while the caller was
    //!     blocked on it.
    ar_status_t get(uint32_t timeout=kArInfiniteTimeout) { return ar_mutex_get(this, timeout); }

    //! @brief Unlock the mutex.
    //!
    //! Only the owning thread is allowed to unlock the mutex. If the owning thread has called get()
    //! multiple times, it must also call put() the same number of time before the underlying
    //! semaphore is actually released. It is illegal to call put() when the mutex is not owned by
    //! the calling thread.
    //!
    //! @retval #kArAlreadyUnlockedError The mutex is not locked.
    //! @retval #kArNotOwnerError The caller is not the thread that owns the mutex.
    ar_status_t put() { return ar_mutex_put(this); }

    //! @brief Returns the current owning thread, if there is one.
    ar_thread_t * getOwner() { return (ar_thread_t *)m_owner; }

    //! @brief Returns whether the mutex is currently locked.
    //!
    //! @retval true The mutex is locked.
    //! @retval false The mutex is unlocked.
    bool isLocked() { return ar_mutex_is_locked(this); }

    /*!
     * @brief Utility class to automatically get and put a mutex.
     *
     * @ingroup ar_mutex
     *
     * This class is intended to be stack allocated. It gets and holds a mutex for the
     * duration of the scope in which it is declared. Once it goes out of scope, the destructor
     * automatically puts the lock.
     */
    class Guard
    {
    public:
        //! @brief Constructor which gets the mutex.
        Guard(Mutex & mutex)
        :   m_mutex(mutex)
        {
            m_mutex.get(kArInfiniteTimeout);
        }

        //! @brief Destructor that puts the mutex.
        ~Guard()
        {
            m_mutex.put();
        }

    protected:
        Mutex & m_mutex;  //!< The mutex to hold.
    };

private:
    //! @brief Disable copy constructor.
    Mutex(const Mutex & other);

    //! @brief Disable assignment operator.
    Mutex& operator=(const Mutex & other);
};

/*!
 * @brief Channel.
 *
 * @ingroup ar_chan
 */
class Channel : public _ar_channel
{
public:
    //! @brief Default constructor.
    Channel() {}

    //! @brief Constructor.
    Channel(const char * name, uint32_t width=0) { init(name, width); }

    //! @brief Destructor.
    ~Channel() { ar_channel_delete(this); }

    //! @brief Channel initialiser.
    ar_status_t init(const char * name, uint32_t width=0) { return ar_channel_create(this, name, width); }

    //! @brief Send to channel.
    ar_status_t send(const void * value, uint32_t timeout=kArInfiniteTimeout) { return ar_channel_send(this, value, timeout); }

    //! @brief Receive from channel.
    ar_status_t receive(void * value, uint32_t timeout=kArInfiniteTimeout) { return ar_channel_receive(this, value, timeout); }

private:
    //! @brief Disable copy constructor.
    Channel(const Channel & other);

    //! @brief Disable assignment operator.
    Channel& operator=(const Channel & other);
};

/*!
 * @brief Typed channel.
 *
 * @ingroup ar_chan
 */
template <typename T>
class TypedChannel : public Channel
{
public:
    //! @brief Default constructor.
    TypedChannel() {}

    //! @brief Constructor.
    TypedChannel(const char * name) : Channel(name, sizeof(T)) {}

    //! @brief Channel initialiser.
    ar_status_t init(const char * name) { return Channel::init(name, sizeof(T)); }

    //! @brief Send to channel.
    ar_status_t send(const T & value, uint32_t timeout=kArInfiniteTimeout)
    {
        return Channel::send(&value, timeout);
    }

    //! @brief Receive from channel.
    T receive(uint32_t timeout=kArInfiniteTimeout)
    {
        T temp;
        Channel::receive(&temp, timeout);
        return temp;
    }

    //! @brief Receive from channel.
    ar_status_t receive(T & value, uint32_t timeout=kArInfiniteTimeout)
    {
        return Channel::receive(&value, timeout);
    }

    //! @brief Receive from channel.
    friend T& operator <<= (T& lhs, TypedChannel<T>& rhs)
    {
        lhs = rhs.receive();
        return lhs;
    }

    //! @brief Send to channel.
    friend T& operator >> (T& lhs, TypedChannel<T>& rhs)
    {
        rhs.send(lhs);
        return lhs;
    }

private:
    //! @brief Disable copy constructor.
    TypedChannel(const TypedChannel<T> & other);

    //! @brief Disable assignment operator.
    TypedChannel& operator=(const TypedChannel<T> & other);
};

/*!
 * @brief A blocking queue for inter-thread messaging.
 *
 * @ingroup ar_queue
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
    //! @retval #kArSuccess The queue was initialised.
    ar_status_t init(const char * name, void * storage, unsigned elementSize, unsigned capacity)
    {
        return ar_queue_create(this, name, storage, elementSize, capacity);
    }

    //! @brief Queue cleanup.
    ~Queue() { ar_queue_delete(this); }

    //! @brief Get the queue's name.
    const char * getName() const { return m_name; }

    //! @brief Add an item to the queue.
    //!
    //! The caller will block if the queue is full.
    //!
    //! @param element Pointer to the element to post to the queue. The element size was specified
    //!     in the init() call.
    //! @param timeout The maximum number of milliseconds that the caller is willing to wait in a
    //!     blocked state before the element can be sent. If this value is 0, or #kArNoTimeout, then
    //!     this method will return immediately if the queue is full. Setting the timeout to
    //!     #kArInfiniteTimeout will cause the thread to wait forever for a chance to send.
    //!
    //! @retval #kArSuccess
    //! @retval #kArQueueFullError
    ar_status_t send(const void * element, uint32_t timeout=kArInfiniteTimeout) { return ar_queue_send(this, element, timeout); }

    //! @brief Remove an item from the queue.
    //!
    //! @param[out] element
    //! @param timeout The maximum number of milliseconds that the caller is willing to wait in a
    //!     blocked state before an element is received. If this value is 0, or #kArNoTimeout, then
    //!     this method will return immediately if the queue is empty. Setting the timeout to
    //!     #kArInfiniteTimeout will cause the thread to wait forever for receive an element.
    //!
    //! @retval #kArSuccess
    //! @retval #kArQueueEmptyError
    ar_status_t receive(void * element, uint32_t timeout=kArInfiniteTimeout) { return ar_queue_receive(this, element, timeout); }

    //! @brief Returns whether the queue is currently empty.
    bool isEmpty() const { return m_count == 0; }

    //! @brief Returns the current number of elements in the queue.
    unsigned getCount() const { return m_count; }

private:
    //! @brief Disable copy constructor.
    Queue(const Queue & other);

    //! @brief Disable assignment operator.
    Queue& operator=(const Queue & other);
};

/*!
 * @brief Template class to help statically allocate a Queue.
 *
 * @ingroup ar_queue
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
 *      ar_status_t s;
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
    ar_status_t init(const char * name)
    {
        return Queue::init(name, m_storage, sizeof(T), N);
    }

    //! @copydoc Queue::send()
    ar_status_t send(T element, uint32_t timeout=kArInfiniteTimeout)
    {
        return Queue::send((const void *)&element, timeout);
    }

    //! @copydoc Queue::receive()
    ar_status_t receive(T * element, uint32_t timeout=kArInfiniteTimeout)
    {
        return Queue::receive((void *)element, timeout);
    }

    //! @brief Alternate form of typed receive.
    //!
    //! @param[out] resultStatus The status of the receive operation is placed here.
    //!     May be NULL, in which case no status is returned.
    //! @param timeout Maximum time in ticks to wait for a queue element.
    T receive(uint32_t timeout=kArInfiniteTimeout, ar_status_t * resultStatus=NULL)
    {
        T element;
        ar_status_t status = Queue::receive((void *)&element, timeout);
        if (resultStatus)
        {
            *resultStatus = status;
        }
        return element;
    }

protected:
    T m_storage[N]; //!< Static storage for the queue elements.

private:
    //! @brief Disable copy constructor.
    StaticQueue(const StaticQueue<T,N> & other);

    //! @brief Disable assignment operator.
    StaticQueue& operator=(const StaticQueue<T,N> & other);
};

/*!
 * @brief Timer object.
 *
 * @ingroup ar_timer
 */
class Timer : public _ar_timer
{
public:

    //! @brief Timer callback function that takes an instance of this class.
    typedef void (*callback_t)(Timer * timer, void * param);

    //! @brief Default constructor.
    Timer() {}

    //! @brief Constructor.
    Timer(const char * name, callback_t callback, void * param, ar_timer_mode_t timerMode, uint32_t delay)
    {
        init(name, callback, param, timerMode, delay);
    }

    //! @brief Destructor.
    ~Timer() { ar_timer_delete(this); }

    //! @brief Initialize the timer.
    ar_status_t init(const char * name, callback_t callback, void * param, ar_timer_mode_t timerMode, uint32_t delay);

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

    callback_t m_userCallback;    //!< The user timer callback.

    //! @brief Converts the timer struct to an instance of this class.
    static void timer_wrapper(ar_timer_t * timer, void * arg);

private:
    //! @brief Disable copy constructor.
    Timer(const Timer & other);

    //! @brief Disable assignment operator.
    Timer& operator=(const Timer & other);
};

/*!
 * @brief Timer object taking a member function callback.
 *
 * @ingroup ar_timer
 */
template <class T>
class TimerWithMemberCallback : public Timer
{
public:

    //! @brief Timer callback function that takes an instance of this class.
    typedef void (T::*callback_t)(Timer * timer);

    //! @brief Default constructor.
    TimerWithMemberCallback<T>() {}

    //! @brief Constructor taking a member function callback.
    TimerWithMemberCallback<T>(const char * name, T * object, callback_t callback, ar_timer_mode_t timerMode, uint32_t delay)
    {
        init(name, object, callback, timerMode, delay);
    }

    //! @brief Destructor.
    ~TimerWithMemberCallback<T>() {}

    //! @brief Initialize the timer with a member function callback.
    ar_status_t init(const char * name, T * object, callback_t callback, ar_timer_mode_t timerMode, uint32_t delay)
    {
        m_memberCallback = callback;
        return Timer::init(name, member_callback, object, timerMode, delay);
    }

protected:

    callback_t m_memberCallback;    //!< The user timer callback.

    //! @brief Call the member function.
    void invoke(T * obj)
    {
        (obj->*m_memberCallback)(this);
    }

    //! @brief Template function to invoke a callback that is a member function.
    static void member_callback(Timer * timer, void * param)
    {
        TimerWithMemberCallback<T> * _this = static_cast<TimerWithMemberCallback<T> *>(timer);
        T * obj = static_cast<T *>(param);
        _this->invoke(obj);
    }

private:
    //! @brief Disable copy constructor.
    TimerWithMemberCallback<T>(const TimerWithMemberCallback<T> & other);

    //! @brief Disable assignment operator.
    TimerWithMemberCallback<T>& operator=(const TimerWithMemberCallback<T> & other);
};

/*!
 * @brief Run loop.
 *
 * @ingroup ar_runloop
 */
class RunLoop : public _ar_runloop
{
public:
    //! @brief Default constructor.
    RunLoop() {}

    //! @brief Constructor to init the runloop.
    //! @param The runloop's name. May be NULL.
    RunLoop(const char * name)
    {
        init(name);
    }

    //! @brief Runloop destructor.
    ~RunLoop() { ar_runloop_delete(this); }

    /*!
     * @brief Create a new runloop.
     *
     * A new runloop is not associated with any thread. This association will happen when the runloop
     * is run.
     *
     * @param name The name of the runloop. May be NULL.
     *
     * @retval #kArSuccess The runloop was created successfully.
     * @retval #kArNotFromInterruptError Cannot call this API from interrupt context.
     */
    ar_status_t init(const char * name) { return ar_runloop_create(this, name); }

    //! @brief Get the run loop's name.
    const char * getName() const { return m_name; }

    /*!
     * @brief Run a runloop for a period of time.
     *
     * Starts the runloop running for a specified amount of time. It will sleep the thread until an
     * associated timer or source is pending. If the timeout expires, the API will return. To force the
     * runloop to exit, call ar_runloop_stop().
     *
     * It's ok to nest runs of the runloop, but only on a single thread.
     *
     * If a queue is associated with the runloop (via ar_runloop_add_queue()) and the queue receives
     * an item, then the runloop will either invoke the queue handler callback or exit. If it exits,
     * it will return #kArRunLoopQueueReceived and the @a object parameter will be filled in with the
     * queue object that received the item. If @a object is NULL, then the runloop will still exit but
     * you cannot tell which queue received. This is acceptable if only one queue is associated with
     * the runloop.
     *
     * @param timeout The maximum number of milliseconds to run the runloop. If this value is 0, or
     *      #kArNoTimeout, then the call will exit after handling pending sources. Setting the timeout to
     *      #kArInfiniteTimeout will cause the runloop to run until stopped or a queue receives an item.
     * @param[out] object Optional structure that will be filled in when the return value is #kArRunLoopQueueReceived.
     *      May be NULL, in which case the receiving queue cannot be indicated.
     *
     * @retval #kArRunLoopStopped The runloop exited due to a timeout or explict call to ar_runloop_stop().
     * @retval #kArRunLoopQueueReceived A queue associated with the runloop received an item.
     * @retval #kArTimeoutError The runloop timed out.
     * @retval #kArRunLoopAlreadyRunningError The runloop is already running on another thread, or another
     *      runloop is already running on the current thread.
     * @retval #kArNotFromInterruptError Cannot run a runloop from interrupt context.
     */
    ar_status_t run(uint32_t timeout=kArInfiniteTimeout, ar_runloop_result_t * object=0) { return ar_runloop_run(this, timeout, object); }

    /*!
     * @brief Stop a runloop.
     *
     * Use this function to stop a running runloop. It may be called from any execution context, including
     * from within the runloop itself, another thread, or interrupt context. When the runloop stops, it
     * will return #kArRunLoopStopped from the run() API. If multiple runs of the runloop
     * are nested, only the innermost will be stopped. If the runloop is calling out to a perform function
     * or a handler callback, it will only be stopped when the callback returns.
     *
     * @retval #kArSuccess The runloop was stopped, or was already stopped.
     */
    ar_status_t stop() { return ar_runloop_stop(this); }

    /*!
     * @brief Invoke a function on a runloop.
     *
     * The function will be called in FIFO order the next time the runloop runs. If the runloop is
     * asleep, it will be woken and run immediately.
     *
     * This API can be called from any execution context.
     *
     * @param function The function to invoke on the runloop.
     * @param param Arbitrary parameter passed to the function when it is called.
     *
     * @retval #kArSuccess The runloop was stopped, or was already stopped.
     * @retval #kArInvalidParameterError The _function_ parameter was NULL.
     * @retval #kArQueueFullError No room to enqueue the function.
     */
    ar_status_t perform(ar_runloop_function_t function, void * param=0) { return ar_runloop_perform(this, function, param); }

    /*!
     * @brief Associate a timer with a runloop.
     *
     * If the timer is already associated with another runloop, its association will be changed.
     *
     * @param timer The timer to associate with the runloop.
     *
     * @retval #kArSuccess The timer was added to the runloop.
     * @retval #kArInvalidParameterError The _timer_ parameter was NULL.
     */
    ar_status_t addTimer(ar_timer_t * timer) { return ar_runloop_add_timer(this, timer); }

    /*!
     * @brief Add a queue to a runloop.
     *
     * If the queue is already associated with another runloop, the #kArAlreadyAttachedError error
     * is returned.
     *
     * @param queue The queue to associate with the runloop.
     * @param callback Optional callback to handler an item received on the queue. May be NULL.
     * @param param Arbitrary parameter passed to the _callback_ when it is called.
     *
     * @retval #kArSuccess The timer was added to the runloop.
     * @retval #kArAlreadyAttachedError A queue can only be added to one runloop at a time.
     * @retval #kArInvalidParameterError The _queue_ parameter was NULL.
     */
    ar_status_t addQueue(ar_queue_t * queue, ar_runloop_queue_handler_t callback=NULL, void * param=NULL) { return ar_runloop_add_queue(this, queue, callback, param); }

    /*!
     * @brief Return the current runloop.
     *
     * @return The runloop currently running on the active thread. If no runloop is running, then NULL
     *      will be returned.
     */
    static ar_runloop_t * getCurrent(void) { return ar_runloop_get_current(); }
};

} // namespace Ar

#endif // defined(__cplusplus)

#endif // _AR_CLASSES_H_
//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
