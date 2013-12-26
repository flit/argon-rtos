Argon RTOS
==========

Tiny embedded RTOS written almost entirely in C++.

Supported cores:

- ARM Cortex-M0+

Supported chips:

- Freescale Kinetis KL25

Argon is primarily targeted at ARM Cortex-M processors, though it should be fairly easily portable to other architectures. The kernel itself has no chip-specific code, and only accesses common Cortex-M resources (i.e., SysTick, NVIC, and SCB).



Copyright © 2007-2013 Immo Software. All rights reserved.  
Portions © 2013 Freescale Semiconductor, Inc.
