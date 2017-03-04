@ingroup ar

The Argon RTOS provides the basic components required to have a useful embedded
operating system. It provides both a simple C API, plus a well-integrated C++ API. Internally, it
is written in C++, with some assembler for handling interrupts and context switching. The six
basic objects that are provided by Argon are:

- Thread: thread object
- Semaphore: counting semaphore
- Mutex: recursive, mutually exclusive lock
- Channel: synchronized message passing
- Queue: message passing queue
- Timer: one-shot and periodic timers

In addition, there are several utility helper classes and functions.

@section Startup

There are several steps involved in starting the RTOS running.

-   Create at least one thread and resume it. If a thread is not created
    then only the idle thread will exist and the system will do nothing.
-   Finally call ar_kernel_run() to start the scheduler. This call will
    not return.

For certain toolchains, currently only IAR, there is special startup code that runs the `main()`
function in its own thread. There is nothing extra you need to do to startup Argon.


