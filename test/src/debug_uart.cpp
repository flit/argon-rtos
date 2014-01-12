/*
 * Copyright (c) 2013 Immo Software
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

#include "debug_uart.h"
#include "fsl_device_registers.h"

#if defined(K60D10_SERIES) || defined(K64F12_SERIES)
#include "uart/scuart.h"
#elif defined(KL25Z4_SERIES)
#include "uart/uart0.h"
#endif

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

#if defined(KL25Z4_SERIES)
    #define UART0_RX_GPIO_PIN_NUM 1  // PIN 1 in the PTA group
    #define UART0_RX_ALT_MODE 2      // ALT mode for UART0 functionality for pin 1

    #define UART0_TX_GPIO_PIN_NUM 2  // PIN 2 in the PTA group
    #define UART0_TX_ALT_MODE 2      // ALT mode for UART0 TX functionality for pin 2
#elif defined(K64F12_SERIES)
    #define UART0_RX_GPIO_PIN_NUM 16  // PIN 1 in the PTB group
    #define UART0_RX_ALT_MODE 3      // ALT mode for UART0 functionality for pin 16

    #define UART0_TX_GPIO_PIN_NUM 17  // PIN 2 in the PTB group
    #define UART0_TX_ALT_MODE 3      // ALT mode for UART0 TX functionality for pin 17
#else
    #error "Unsupported chip family"
#endif

//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------

extern "C" size_t __write(int handle, const unsigned char *buf, size_t size);

//------------------------------------------------------------------------------
// Code
//------------------------------------------------------------------------------

void debug_init(void)
{
    SIM->SCGC5 |= ( SIM_SCGC5_PORTA_MASK
                  | SIM_SCGC5_PORTB_MASK
                  | SIM_SCGC5_PORTC_MASK
                  | SIM_SCGC5_PORTD_MASK
                  | SIM_SCGC5_PORTE_MASK );

#if defined(KL25Z4_SERIES)
    SIM->SOPT2 |= SIM_SOPT2_PLLFLLSEL_MASK // set PLLFLLSEL to select the PLL for this clock source
                | SIM_SOPT2_UART0SRC(1);   // select the PLLFLLCLK as UART0 clock source
                
    BW_PORT_PCRn_MUX(HW_PORTA, UART0_RX_GPIO_PIN_NUM, UART0_RX_ALT_MODE);   // Set UART0_RX pin to UART0_RX functionality
    BW_PORT_PCRn_MUX(HW_PORTA, UART0_TX_GPIO_PIN_NUM, UART0_TX_ALT_MODE);   // Set UART0_TX pin to UART0_TX functionality
    
    uart0_init(GetSystemMCGPLLClock(), DEBUG_UART_BAUD);
#elif defined(K64F12_SERIES)
    BW_PORT_PCRn_MUX(HW_PORTB, UART0_RX_GPIO_PIN_NUM, UART0_RX_ALT_MODE);   // Set UART0_RX pin to UART0_RX functionality
    BW_PORT_PCRn_MUX(HW_PORTB, UART0_TX_GPIO_PIN_NUM, UART0_TX_ALT_MODE);   // Set UART0_TX pin to UART0_TX functionality
    
    scuart_init(UART0, 120000000, DEBUG_UART_BAUD);
#endif
}

#if __ICCARM__

size_t __write(int handle, const unsigned char *buf, size_t size)
{
    while (size--)
    {
#if defined(KL25Z4_SERIES)
        uart0_putchar(*buf++);
#elif defined(K64F12_SERIES)
        scuart_putchar(UART0, *buf++);
#endif
    }

    return size;
}

#endif // __ICCARM__

//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
