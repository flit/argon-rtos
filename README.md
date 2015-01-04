Argon RTOS
==========

Small embedded RTOS written almost entirely in C++.

*Version*: 1.0a2</br>
*Status*: Actively under development. Not all features are fully implemented. The API is likely to change some more.

Supported cores:

- ARM Cortex-M0+
- ARM Cortex-M4
- ARM Cortex-M4F

Supported toolchains:

- IAR EWARM
- Keil MDK (armcc)
- GNU Tools for ARM Embedded Processors (gcc)

### Overview

Argon is designed to be a very clean RTOS kernel with a C++ API. Currently it also has a C API upon which the C++ API is built, but that may go away. The code is exceptionally readable and maintainable, and is well commented.

The resources provided by Argon are those most important for building a threaded application:

- Thread
- Semaphore
- Mutex
- Channel
- Queue
- Timer

A memory allocator is not provided, as every Standard C Library includes malloc.

Dynamic memory is not required under any circumstance. All kernel objects can be allocated statically. This lets you determine application memory requirements at link time. However, you can still use `new` or `malloc()` to allocate objects if you prefer.

There are no limits on the number of kernel objects. You may create as many threads, channels, etc., as you like at runtime via dynamic allocation.

Argon is primarily targeted at ARM Cortex-M processors, though it should be fairly easily portable to other architectures. The kernel itself is core agnostic. All core-specific code is isolated into a separate directory. The Cortex-M port only accesses common Cortex-M resources (i.e., SysTick, NVIC, and SCB), so it should work on any other Cortex-M platform.

### Memory requirements

Total code size with all features enabled is approximately 4 kB. Kernel objects are not explicitly disable-able, but the linker will exclude any unused code.

RAM requirements are quite small. The kernel itself consumes 128 bytes plus the idle thread stack. Timers share the idle thread stack, so you don't need an extra stack to use timers.

Kernel object sizes:

- Thread = 92 bytes + stack
- Channel = 36 bytes
- Semaphore = 32 bytes
- Mutex = 44 bytes
- Queue = 60 bytes + element storage
- Timer = 60 bytes

These sizes are as of 21 Dec 2014, and will probably change in the future. All above sizes were obtained using IAR EWARM 7.30 with full optimization enabled.

### Supported chips and boards

These are the devices that have demo projects included in the repository. The kernel itself will build for any Cortex-M device.

<table>
<tr><th>Family</th><th>Part Number</th><th>Board</th></tr>
<tr><td>Freescale Kinetis</td><td>KL25Z128xxx4</td><td>FRDM-KL25Z</td></tr>
<tr><td></td><td>KL43Z256xxx4</td><td>FRDM-KL43Z</td></tr>
<tr><td></td><td>K20DX128xxx5</td><td>FRDM-K20D50M</td></tr>
<tr><td></td><td>K22FN512xxx12</td><td>FRDM-K22F</td></tr>
<tr><td></td><td>K60DZ128xxx10</td><td>TWR-K60N512</td></tr>
<tr><td></td><td>K64F1M0xxx12</td><td>FRDM-K64F</td></tr>
</table>

### Source code

The code for the Argon kernel is in the `src/argon` directory in the repository.

The [mbed](http://mbed.org) libraries are included under `src/mbed`, to provide a simple C++ driver API for demo applications. mbed device HALs are located in `src/mbed/targets`.

### Licensing

The Argon RTOS is open source software released with a BSD three-clause license. See the included LICENSE.txt file for details.

Copyright © 2007-2015 Immo Software.

The repository also includes code with other copyrights and licenses. See the source file headers for licensing information of this code.<br/>
Portions Copyright © 2014 Freescale Semiconductor, Inc.<br/>
Portions Copyright © 2006-2014 ARM Limited

