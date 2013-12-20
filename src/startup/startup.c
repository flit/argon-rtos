/*
 * Copyright (c) 2013, Freescale Semiconductor, Inc.
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
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
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

#include "fsl_platform_common.h"
#include "device/fsl_device_registers.h"

#if (defined(__ICCARM__))
    #pragma section = ".intvec"
    #pragma section = ".data"
    #pragma section = ".data_init"
    #pragma section = ".bss"
    #pragma section = "CodeRelocate"
    #pragma section = "CodeRelocateRam"
#endif

////////////////////////////////////////////////////////////////////////////////
// Code
////////////////////////////////////////////////////////////////////////////////

void init_data_bss(void)
{
#if (defined(CW))
    extern char __START_BSS[];
    extern char __END_BSS[];
    extern uint32_t __DATA_ROM[];
    extern uint32_t __DATA_RAM[];
    extern char __DATA_END[];
#endif

    // Declare a counter we'll use in all of the copy loops
    uint32_t n;

#ifndef KEIL
    /* Declare pointers for various data sections. These pointers
     * are initialized using values pulled in from the linker file
     */
    uint8_t * data_ram, * data_rom, * data_rom_end;
    uint8_t * bss_start, * bss_end;
#endif

#if (defined(KEIL))
    extern uint32_t Image$$VECTOR_ROM$$Base[];
    extern uint32_t Image$$VECTOR_RAM$$Base[];
    #define __VECTOR_TABLE Image$$VECTOR_ROM$$Base
    #define __VECTOR_RAM Image$$VECTOR_RAM$$Base
#elif (defined(__ICCARM__))
    // Addresses for VECTOR_TABLE and VECTOR_RAM come from the linker file
    extern uint32_t __VECTOR_TABLE[];
    extern uint32_t __VECTOR_RAM[];
#elif (defined(CW))
    #define __VECTOR_TABLE __vector_table
    #define __VECTOR_RAM   __vector_ram
    extern uint32_t __VECTOR_TABLE[];
    extern uint32_t __VECTOR_RAM[];
#endif

    // Copy the vector table to RAM
//    if (__VECTOR_RAM != __VECTOR_TABLE)
//    {
//        for (n = 0; n < 0x104; n++)
//           __VECTOR_RAM[n] = __VECTOR_TABLE[n];
//    }
    // Point the VTOR to the new copy of the vector table
//     write_vtor((uint32_t)__VECTOR_RAM);
//    SCB->VTOR = (uint32_t)__VECTOR_RAM;
    SCB->VTOR = (uint32_t)__section_begin(".intvec");

    // Get the addresses for the .data section (initialized data section)
#if (defined(CW))
    data_ram = (uint8_t *)__DATA_RAM;
    data_rom = (uint8_t *)__DATA_ROM;
    data_rom_end  = (uint8_t *)__DATA_END; // This is actually a RAM address in CodeWarrior
    n = data_rom_end - data_ram;
#elif (defined(__ICCARM__))
    data_ram = __section_begin(".data");
    data_rom = __section_begin(".data_init");
    data_rom_end = __section_end(".data_init");
    n = data_rom_end - data_rom;
#endif

#ifndef __CC_ARM

    // Copy initialized data from ROM to RAM
    while (n--)
        *data_ram++ = *data_rom++;


    // Get the addresses for the .bss section (zero-initialized data)
#if (defined(CW))
    bss_start = (uint8_t *)__START_BSS;
    bss_end = (uint8_t *)__END_BSS;
#elif (defined(__ICCARM__))
    bss_start = __section_begin(".bss");
    bss_end = __section_end(".bss");
#endif


    // Clear the zero-initialized data section
    n = bss_end - bss_start;
    while(n--)
        *bss_start++ = 0;
#endif

    /* Get addresses for any code sections that need to be copied from ROM to RAM.
     * The IAR tools have a predefined keyword that can be used to mark individual
     * functions for execution from RAM. Add "__ramfunc" before the return type in
     * the function prototype for any routines you need to execute from RAM instead
     * of ROM. ex: __ramfunc void foo(void);
     */
#if (defined(__ICCARM__))
    uint8_t* code_relocate_ram = __section_begin("CodeRelocateRam");
    uint8_t* code_relocate = __section_begin("CodeRelocate");
    uint8_t* code_relocate_end = __section_end("CodeRelocate");

    // Copy functions from ROM to RAM
    n = code_relocate_end - code_relocate;
    while (n--)
        *code_relocate_ram++ = *code_relocate++;
#endif
}

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
