@page Overview

# Overview

The Argon RTOS provides the basic components required to have a useful embedded
operating system. It provides both a simple C API, plus a well-integrated C++ API. Internally, it
is written in C++, with some assembler for handling interrupts and context switching.

The seven kernel objects that are provided by Argon are:

- @ref ar_thread "Thread": thread object
- @ref ar_sem "Semaphore": counting semaphore
- @ref ar_mutex "Mutex": recursive, mutually exclusive lock with priority boosting
- @ref ar_chan "Channel": synchronized, unbuffered message passing
- @ref ar_queue "Queue": asynchronous message passing queue
- @ref ar_timer "Timer": one-shot and periodic timers
- @ref ar_runloop "Run Loop": messaging and event loop for threads, where timers are run

In addition, there are several utility helper classes and functions.

Unlike most RTOSes that support timers, there is not a timer thread. Timers are always associated
with a run loop. This allows you to control what threads timers run on, and even run different
timers at different priority levels, based on the priority of the run loop's thread.

Argon is designed such that it does not require extensive configuration in order to work correctly.
There are a relatively small set of configuration options specified in the `ar_config.h` file. Most
of the configuration settings will never need to be changed, and creating an application-specific
configuration is entirely optional. See the @ref ar_config "Configuration" module documentation for
details.


@page Startup

# Startup

There are only a couple steps involved in starting the kernel running.

-   Create at least one thread and resume it. If a thread is not created
    then only the idle thread will exist and the system will do nothing.
-   Finally call ar_kernel_run() to start the scheduler. This call will
    not return.

For certain toolchains, currently only IAR, there is special startup code that runs the `main()`
function in its own thread. There is nothing extra you need to do to startup Argon. This feature is
controlled with the #AR_ENABLE_MAIN_THREAD config macro.



@page "Memory requirements"

### Memory requirements

Total code size with all features enabled is approximately 6 kB. Kernel objects are not explicitly disable-able, but the linker will exclude any unused code.

RAM requirements are quite small. The kernel itself consumes 128 bytes plus the idle thread stack. Timers share the idle thread stack, so you don't need an extra stack to use timers.

Kernel object sizes:

- Thread = 92 bytes + stack
- Channel = 36 bytes
- Semaphore = 32 bytes
- Mutex = 44 bytes
- Queue = 60 bytes + element storage
- Timer = 60 bytes

These sizes are as of 21 Dec 2014, and will probably change in the future. All above sizes were obtained using IAR EWARM 7.30 with full optimization enabled.
