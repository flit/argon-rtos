/*
 * Copyright (c) 2013-2016 Immo Software
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
 * @file ar_config.h
 * @ingroup ar
 * @brief Configuration settings for the Argon RTOS.
 */

#if !defined(_AR_CONFIG_H_)
#define _AR_CONFIG_H_

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

//! @addtogroup ar_config
//! @{

//! @page Configuration
//! @ingroup ar_config
//!
//! These configuration macros are used to control features of Argon. This file can be modified
//! to change the configuration. Or compiler command line options can be used to override the
//! default values of these macros.
//! <br/><br/>
//! Some configuration macros have defaults based on the debug or release build type. For these,
//! the value of the DEBUG macro is used to determine the build type.

#if !defined(AR_ANONYMOUS_OBJECT_NAME)
    //! @brief The string to use for the name of an object that wasn't provided a name.
    #define AR_ANONYMOUS_OBJECT_NAME ("<anon>")
#endif

#if !defined(AR_GLOBAL_OBJECT_LISTS)
    //! @brief Set to 1 to enable the lists of all created kernel objects.
    //!
    //! Default is enabled for debug builds, disabled for release builds.
    #define AR_GLOBAL_OBJECT_LISTS (DEBUG)
#endif

//! @name Idle thread config
//@{

#if !defined(AR_ENABLE_IDLE_SLEEP)
    //! @brief Controls whether the idle thread puts the processor to sleep until the next interrupt.
    //!
    //! Set to 1 to enable. The default is to disable idle sleep for debug builds, enabled for
    //! release builds.
    #define AR_ENABLE_IDLE_SLEEP (!(DEBUG))
#endif

#if !defined(AR_ENABLE_SYSTEM_LOAD)
    //! @brief When set to 1, per-thread and system load will be computed.
    #define AR_ENABLE_SYSTEM_LOAD (1)
#endif

#if !defined(AR_SYSTEM_LOAD_SAMPLE_PERIOD)
    //! @brief Microsecond period over which load of system load is computed.
    //!
    //! Normally this is 1 second.
    #define AR_SYSTEM_LOAD_SAMPLE_PERIOD (1000000)
#endif

#if !defined(AR_IDLE_THREAD_STACK_SIZE)
    //! @brief Size in bytes of the idle thread's stack.
    #define AR_IDLE_THREAD_STACK_SIZE (512)
#endif // AR_IDLE_THREAD_STACK_SIZE

//@}

#if !defined(AR_THREAD_STACK_PATTERN_FILL)
    //! @brief Whether to fill a new thread's stack with a pattern.
    //!
    //! Filling the stack with a pattern enables one to determine maximum stack usage of a thread.
    //! The downside is that it takes longer to initialize a thread. Default is enabled for debug
    //! builds, disabled for release.
    #define AR_THREAD_STACK_PATTERN_FILL (DEBUG)
#endif // AR_IDLE_THREAD_STACK_SIZE

//! @name Main thread config
//@{

#if !defined(AR_ENABLE_MAIN_THREAD)
    //! @brief Set to 1 to cause main() to be run in a thread.
    //!
    //! Enabling this option will cause the kernel to automatically start prior to main() being
    //! called. A thread is created for you with the entry point set to main(). The main thread's
    //! priority is set with the #AR_MAIN_THREAD_PRIORITY macro. The stack size is determined by
    //! a combination of the linker file and #AR_SCHEDULER_STACK_SIZE.
    #define AR_ENABLE_MAIN_THREAD (1)
#endif // AR_ENABLE_MAIN_THREAD

#if !defined(AR_SCHEDULER_STACK_SIZE)
    //! @brief Size in bytes of the stack used by the scheduler and interrupts if the main thread
    //! is enabled.
    //!
    //! This size is subtracted from the C stack size specified by the linker file. The remainder
    //! is used for the main thread itself.
    #define AR_SCHEDULER_STACK_SIZE (256)
#endif // AR_SCHEDULER_STACK_SIZE

#if !defined(AR_MAIN_THREAD_PRIORITY)
    //! @brief Priority for the main thread.
    #define AR_MAIN_THREAD_PRIORITY (128)
#endif // AR_MAIN_THREAD_PRIORITY

//@}

#if !defined(AR_ENABLE_TICKLESS_IDLE)
    //! @brief Set to 1 to enable tickless idle.
    #define AR_ENABLE_TICKLESS_IDLE (1)
#endif

#if !defined(AR_DEFERRED_ACTION_QUEUE_SIZE)
    //! @brief Maximum number of actions deferred from IRQ context.
    #define AR_DEFERRED_ACTION_QUEUE_SIZE (8)
#endif

#if !defined(AR_RUNLOOP_FUNCTION_QUEUE_SIZE)
    //! @brief Maximum number of functions queued in a run loop.
    #define AR_RUNLOOP_FUNCTION_QUEUE_SIZE (8)
#endif

#if !defined(AR_ENABLE_LIST_CHECKS)
    //! @brief Enable runtime checking of linked lists.
    //!
    //! Normally not required.
    #define AR_ENABLE_LIST_CHECKS (0)
#endif

#if !defined(AR_ENABLE_TRACE)
    //! @brief Enable kernel event tracing.
    #define AR_ENABLE_TRACE (DEBUG)
#endif

//! @}

#endif // _AR_CONFIG_H_
//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
