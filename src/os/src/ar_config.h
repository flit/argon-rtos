/*
 * Copyright (c) 2013-2014 Immo Software
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
 * @file ar_config.h
 * @ingroup ar_port
 * @brief Configuration settings for the Argon RTOS.
 */

#if !defined(_AR_CONFIG_H_)
#define _AR_CONFIG_H_

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

// Determine whether this is a debug or release build.
#if !defined(AR_DEBUG_ENABLED)
    #if DEBUG
        #define AR_DEBUG_ENABLED (1)
    #else
        #define AR_DEBUG_ENABLED (0)
    #endif
#endif

#if !defined(AR_ANONYMOUS_OBJECT_NAME)
    //! The string to use for the name of an object that wasn't provided a name.
    #define AR_ANONYMOUS_OBJECT_NAME ("<anon>")
#endif

#if !defined(AR_GLOBAL_OBJECT_LISTS)
    //! Set to 1 to enable the lists of all created kernel objects. Default is enabled for
    //! debug builds, disabled for release builds.
    #define AR_GLOBAL_OBJECT_LISTS (AR_DEBUG_ENABLED)
#endif

#if !defined(AR_ENABLE_IDLE_SLEEP)
    //! Controls whether the idle thread puts the processor to sleep until the next interrupt. Set
    //! to 1 to enable. The default is to disable idle sleep for debug builds, enabled for release
    //! builds.
    #define AR_ENABLE_IDLE_SLEEP (!(AR_DEBUG_ENABLED))
#endif

#if !defined(AR_ENABLE_SYSTEM_LOAD)
    //! When set to 1 the idle thread will compute the system load percentage.
    #define AR_ENABLE_SYSTEM_LOAD (1)
#endif

#if !defined(AR_IDLE_THREAD_STACK_SIZE)
    //! Size in bytes of the idle and timer thread's stack.
    #define AR_IDLE_THREAD_STACK_SIZE (512)
#endif // AR_IDLE_THREAD_STACK_SIZE

#if !defined(AR_ENABLE_MAIN_THREAD)
    //! Set to 1 to cause main() to be run in a thread.
    #define AR_ENABLE_MAIN_THREAD (1)
#endif // AR_ENABLE_MAIN_THREAD

#if !defined(AR_SCHEDULER_STACK_SIZE)
    //! Size in bytes of the stack used by the scheduler and interrupts if the main thread
    //! is enabled.
    #define AR_SCHEDULER_STACK_SIZE (256)
#endif // AR_SCHEDULER_STACK_SIZE

#if !defined(AR_MAIN_THREAD_PRIORITY)
    //! Priority for the main thread.
    #define AR_MAIN_THREAD_PRIORITY (128)
#endif // AR_MAIN_THREAD_PRIORITY

#endif // _AR_CONFIG_H_
//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
