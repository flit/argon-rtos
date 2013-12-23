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
#include "uart0.h"
#include <stdarg.h>

/********************************************************************/

/*
 * Initialize the uart for 8N1 operation, interrupts disabled, and
 * no hardware flow-control
 *
 * NOTE: Since the uarts are pinned out in multiple locations on most
 *       Kinetis devices, this driver does not enable uart pin functions.
 *       The desired pins should be enabled before calling this init function.
 *
 * Parameters:
 *  uart0clk    uart module Clock in Hz(used to calculate baud)
 *  baud        uart baud rate
 */
status_t uart0_init (uint32_t uart0clk, uint32_t baud)
{
    uint8_t i;
    uint32_t calculated_baud = 0;
    uint32_t baud_diff = ~0;
    uint32_t osr_val = 0;
    uint32_t sbr_val;
    uint32_t reg_temp = 0;
    uint32_t temp = 0;

    // Select the best OSR value
    for (i = 4; i <= 32; i++)
    {
        sbr_val = (uint32_t)(uart0clk/(baud * i));
        calculated_baud = (uart0clk / (i * sbr_val));

        if (calculated_baud > baud)
        {
            temp = calculated_baud - baud;
        }
        else
        {
            temp = baud - calculated_baud;
        }

        if (temp <= baud_diff)
        {
            baud_diff = temp;
            osr_val = i;
        }
    }

    if (baud_diff < ((baud / 100) * 3))
    {
        // Enable clocking to UART0
        SIM->SCGC4 |= SIM_SCGC4_UART0_MASK;

        // Disable UART0 before changing registers
        UART0->C2 &= ~(UART0_C2_TE_MASK | UART0_C2_RE_MASK);

        // If the OSR is between 4x and 8x then both
        // edge sampling MUST be turned on.
        if ((osr_val >3) && (osr_val < 9))
        {
            UART0->C5|= UART0_C5_BOTHEDGE_MASK;
        }

        // Setup OSR value
        reg_temp = UART0->C4;
        reg_temp &= ~UART0_C4_OSR_MASK;
        reg_temp |= UART0_C4_OSR(osr_val-1);

        // Write reg_temp to C4 register
        UART0->C4 = reg_temp;

        reg_temp = (reg_temp & UART0_C4_OSR_MASK) + 1;
        sbr_val = (uint32_t)((uart0clk)/(baud * (reg_temp)));

        // Save off the current value of the uartx_BDH except for the SBR field
        reg_temp = UART0->BDH & ~(UART0_BDH_SBR(0x1F));

        UART0->BDH = reg_temp |  UART0_BDH_SBR(((sbr_val & 0x1F00) >> 8));
        UART0->BDL = (uint8_t)(sbr_val & UART0_BDL_SBR_MASK);

        // Enable receiver and transmitter
        UART0->C2 |= (UART0_C2_TE_MASK | UART0_C2_RE_MASK );

        return kStatus_Success;
    }
    else
    {
        // Unacceptable baud rate difference
        // More than 3% difference!!
        return kStatus_Fail;
    }

}
/********************************************************************/
/*
 * Wait for a character to be received on the specified uart
 *
 * Parameters:
 *  channel      uart channel to read from
 *
 * Return Values:
 *  the received character
 */
char uart0_getchar (void)
{
      // Wait until character has been received
      while (!(UART0->S1 & UART0_S1_RDRF_MASK));

      // Return the 8-bit data from the receiver
      return UART0->D;
}
/********************************************************************/
/*
 * Wait for space in the uart Tx FIFO and then send a character
 *
 * Parameters:
 *  channel      uart channel to send to
 *  ch             character to send
 */
void uart0_putchar (char ch)
{
      // Wait until space is available in the FIFO
      while(!(UART0->S1 & UART0_S1_TDRE_MASK));

      // Send the character
      UART0->D = (uint8_t)ch;
}

/********************************************************************/
/*
 * Check to see if a character has been received
 *
 * Parameters:
 *  channel      uart channel to check for a character
 *
 * Return values:
 *  0       No character received
 *  1       Character has been received
 */
int uart0_getchar_present (void)
{
    return (UART0->S1 & UART0_S1_RDRF_MASK);
}
/********************************************************************/

/********************************************************************/
/*
 * Shuts down UART0
 *
 * Parameters:
 */
void uart0_shutdown (void)
{
    // This driver uses polling so nothing to do
    // just disable clocking to the peripheral
    HW_SIM_SCGC4_CLR(BM_SIM_SCGC4_UART0);
}
/********************************************************************/

