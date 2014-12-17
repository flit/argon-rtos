Argon RTOS
==========

Tiny embedded RTOS written almost entirely in C++.

Supported cores:

- ARM Cortex-M0+
- ARM Cortex-M4

Supported chips and boards:

- Freescale Kinetis
    - KL25Z128xxx4
        - FRDM-KL25Z48 board
    - KL43Z256xxx4
        - FRDM-KL43Z48 board
    - K20DX128xxx5
        - FRDM-K20D50M board
    - K60DZ128xxx10
        - TWR-K60N512 board
    - K64F1M0xxx12
        - FRDM-K64F120 board

Argon is primarily targeted at ARM Cortex-M processors, though it should be fairly easily portable to other architectures. The kernel itself is core agnostic. All core-specific code is isolated into a separate directory. The Cortex-M port only accesses common Cortex-M resources (i.e., SysTick, NVIC, and SCB), so it should work on any other Cortex-M platform.

The resources provided by Argon are those most important for building a threaded application:

- Thread
- Semaphore
- Mutex
- Channel
- Queue
- Timer

### Licensing

The Argon RTOS is released with a BSD three-clause license. See the included LICENSE.txt file for details.

Copyright © 2007-2014 Immo Software. All rights reserved.

The repository also includes code with other copyrights and licenses. See the source file headers for licensing information of this code.<br/>
Portions copyright © 2013 Freescale Semiconductor, Inc.<br/>
Portions copyright © 2006-2013 ARM Limited

