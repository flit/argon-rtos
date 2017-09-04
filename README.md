Argon RTOS
==========

Small embedded RTOS with C and C++ APIs.

*Version*: 1.2.0<br/>
*Status*: Actively under development. Stable API. Well tested in several real applications.

Tested cores:

- ARM Cortex-M0+
- ARM Cortex-M4
- ARM Cortex-M4F

Supported toolchains:

- IAR EWARM
- Keil MDK (armcc)
- GNU Tools for ARM Embedded Processors (gcc)

### Overview

Argon is designed to be a very clean RTOS kernel with a C++ API. It also has a C API upon which the C++ API is built. The code is quite readable and maintainable, and is well commented and documented.

The core resources provided by Argon are those basic building blocks most commonly used for constructing threaded applications:

- Thread
- Semaphore
- Mutex
- Queue

In addition, Argon provides a few more unique resources:

- Timer
- Channel
- Run Loop

A memory allocator is not provided, as every Standard C Library includes malloc. (Even though it may not be the smallest code, and doesn't support block allocation.)

Dynamic memory is not required under any circumstance. All kernel objects can be allocated statically. This lets you determine application memory requirements at link time. However, you can still use `new` or `malloc()` to allocate objects if you prefer.

There are no limits on the number of kernel objects. You may create as many threads, channels, etc., as you like at runtime via dynamic allocation.

Argon is primarily targeted at ARM Cortex-M processors. It takes advantage of Cortex-M features to provide a  kernel that never disables IRQs. (M0+ cores are an exception since they don't have ldrex/strex instructions. Even so, IRQs are disabled for only a couple cycles at a time.)

The kernel itself is core agnostic, and should be fairly easily portable to other architectures. All core-specific code is isolated into a separate directory. The Cortex-M port only accesses common Cortex-M resources (i.e., SysTick, NVIC, and SCB), so it should work without modification on any other Cortex-M platform.

The kernel code is written in C++, but mostly uses a C-like style to implement the C API directly. The emphasis is on as little overhead as possible for both C and C++ APIs.

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

### Source code

The code for the Argon kernel is in the `src/` directory in the repository. Public headers are in the `include/` directory.

The argon-rtos repository is intended to be usable as a submodule in other git repositories.

### Contributions

Contributions are welcome! Please create a pull request in GitHub.

### Licensing

The Argon RTOS is open source software released with a BSD three-clause license. See the included LICENSE.txt file for details.

The repository also includes code with other copyrights and licenses. See the source file headers for licensing information of this code.

Copyright © 2007-2017 Immo Software.<br/>
Portions Copyright © 2014-2017 NXP

