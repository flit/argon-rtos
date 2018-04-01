Argon RTOS
==========

Small embedded RTOS with C and C++ APIs.

*Version*: 1.2.0<br/>
*Status*: Actively under development. Mostly stable API. Well tested in several real applications.

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

The core resources provided by Argon are:

- Thread
- Semaphore
- Mutex
- Queue
- Timer
- Channel
- Run Loop

A memory allocator is not provided, as every Standard C Library includes malloc. (Even though it may not be the smallest code, and doesn't support block allocation.)

Dynamic memory is not required under any circumstance. All kernel objects can be allocated statically. This lets you determine application memory requirements at link time. However, you can still use `new` or `malloc()` to allocate objects if you prefer.

There are no limits on the number of kernel objects. You may create as many threads, channels, etc., as you like at runtime via dynamic allocation.

Argon takes advantage of Cortex-M features to provide a kernel that never disables IRQs. (M0+ cores are an exception since they don't have the required ldrex/strex instructions. Even so, IRQs are disabled for only a few cycles at a time.)

### Portability

Argon is primarily targeted at ARM Cortex-M processors.

The Cortex-M porting layer only accesses common Cortex-M resources (i.e., SysTick, NVIC, and SCB). It should work without modification on any Cortex-M-based MCU.

The kernel itself is fairly core agnostic, and should be relatively easily portable to other architectures. All architecture-specific code is isolated into a separate directory.

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

