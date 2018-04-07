Argon RTOS
==========

Small embedded RTOS with clean C and C++ APIs for Arm Cortex-M-based microcontrollers.

*Version*: 1.3.0<br/>
*Status*: Actively under development. Mostly stable API. Well tested in several real applications.

### Overview

Argon is designed to be a very clean preemptive RTOS kernel with a C++ API. A plain C API upon which the C++ API is built is also available. It is primarily targeted at microcontrollers with Arm Cortex-M processors. A primary goal is for the source code to be highly readable, maintainable, and well commented.

The kernel objects provided by Argon are:

- Thread
- Semaphore
- Mutex
- Queue
- Channel
- Timer
- Run Loop

Argon takes advantage of Cortex-M features to provide a kernel that never disables IRQs (except for CM0+<a href="#fn1"><sup>1</sup></a>).

Timers run on runloops, which lets you control the thread and priority of timers. Runloops enable very efficient use of threads, including waiting on multiple queues.

Mutexes are recursive and have priority inheritance.

A memory allocator is not provided, as every Standard C Library includes malloc.<a href="#fn2"><sup>2</sup></a>

There are no limits on the number of kernel objects. You may create as many objects as you need during runtime via dynamic allocation using `new` or `malloc()`. However, dynamic memory is not required under any circumstance. All kernel objects can be allocated statically, which is often important for determining application memory requirements at link time.

### Documentation

The Argon documentation is [available online](https://flit.github.io/argon-rtos/doc/).

You can also generate it locally with Doxygen. The config file is doc/Doxyfile.

### Example

Here's a simple example of the C++ API.

~~~cpp
Ar::ThreadWithStack<2048> myThread("my", thread_fn, 0, 100, kArSuspendThread);
Ar::Semaphore theSem("s");

void thread_fn(void * param)
{
    while (theSem.get() == kArSuccess)
    {
        // do something
    }
}

void main()
{
    myThread.resume();
}
~~~

And the same example using the plain C API:

~~~c
uint8_t stack[2048];
ar_thread_t myThread;
ar_semaphore_t theSem;

void thread_fn(void * param)
{
    while (ar_semaphore_get(theSem, kArInfiniteTimeout) == kArSuccess)
    {
        // do something
    }
}

void main()
{
    ar_thread_create(&myThread, "my", thread_fn, 0, stack, sizeof(stack), 100, kArSuspendThread);
    ar_semaphore_create(&theSem, "s", 1);
    ar_thread_resume(&myThread);
}
~~~

### Requirements

Requires a Cortex-M based microcontroller.

- ARMv6-M
- ARMv7-M

Tested cores:

- Arm Cortex-M0+
- Arm Cortex-M4
- Arm Cortex-M4F

Supported toolchains:

- IAR EWARM
- Keil MDK (armcc)
- GNU Tools for Arm Embedded Processors (gcc)

Language requirements:
- C99
- C++98

### Portability

The Cortex-M porting layer only accesses standard Cortex-M resources (i.e., SysTick, NVIC, and SCB). It should work without modification on any Cortex-M-based MCU.

The kernel itself is fairly architecture-agnostic, and should be easily portable to other architectures. All architecture-specific code is isolated into a separate directory.

### Source code

The code for the Argon kernel is in the `src/` directory in the repository. Public headers are in the `include/` directory.

The argon-rtos repository is intended to be usable as a submodule in other git repositories.

The kernel code is written in C++, but mostly uses a C-like style to implement the C API directly. The emphasis is on as little overhead as possible for both C and C++ APIs.

### Contributions

Contributions are welcome! Please create a pull request in GitHub.

### Licensing

The Argon RTOS is open source software released with a BSD three-clause license. See the included LICENSE.txt file for details.

The repository also includes code with other copyrights and licenses. See the source file headers for licensing information of this code.

Copyright © 2007-2018 Immo Software.<br/>
Portions Copyright © 2014-2018 NXP

<hr width="30%" align="left" size="1"/>
<div id="fn1"><sup>1</sup> Cortex-M0+ cores are an exception since they don't have the required ldrex/strex instructions. Even so, IRQs are disabled for only a few cycles at a time.</div>
<div id="fn2"><sup>2</sup> Even though it may not be the smallest code, and doesn't support block allocation.</div>
