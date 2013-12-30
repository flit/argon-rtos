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
#include "scuart.h"
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
 *  uartch      uart channel to initialize
 *  uartclk     uart module Clock in Hz(used to calculate baud)
 *  baud        uart baud rate
 */
status_t scuart_init (UART_Type * uartch, int uartclk, int baud)
{
    uint8_t temp;
    register uint16_t sbr = (uint16_t)(uartclk/(baud * 16));
    int baud_check = uartclk / (sbr * 16);
    uint32_t baud_diff;

    if (baud_check > baud)
    {
        baud_diff = baud_check - baud;
    }
    else
    {
        baud_diff = baud - baud_check;
    }

    // If the baud rate cannot be within 3% of the passed in value
    // return a failure
    if (baud_diff > ((baud / 100) * 3))
    {
        return kStatus_Fail;
    }

    switch((unsigned int)uartch)
    {
        case (unsigned int)UART0:
            HW_SIM_SCGC4_SET(BM_SIM_SCGC4_UART0);
            break;
        case (unsigned int)UART1:
            HW_SIM_SCGC4_SET(BM_SIM_SCGC4_UART1);
            break;
        case (unsigned int)UART2:
            HW_SIM_SCGC4_SET(BM_SIM_SCGC4_UART2);
            break;
        case (unsigned int)UART3:
            HW_SIM_SCGC4_SET(BM_SIM_SCGC4_UART3);
            break;
        case (unsigned int)UART4:
            HW_SIM_SCGC1_SET(BM_SIM_SCGC1_UART4);
            break;
        case (unsigned int)UART5:
            HW_SIM_SCGC1_SET(BM_SIM_SCGC1_UART5);
            break;
    }

    //Make sure that the transmitter and receiver are disabled while we
    //change settings.
    uartch->C2 &= ~(UART_C2_TE_MASK | UART_C2_RE_MASK );

    // Configure the uart for 8-bit mode, no parity
    uartch->C1 = 0;    // We need all default settings, so entire register is cleared

    // Save off the current value of the uartx_BDH except for the SBR field
    temp = uartch->BDH & ~(UART_BDH_SBR(0x1F));

    uartch->BDH = temp |  UART_BDH_SBR(((sbr & 0x1F00) >> 8));
    uartch->BDL = (uint8_t)(sbr & UART_BDL_SBR_MASK);

    // Flush the RX and TX FIFO's
    uartch->CFIFO = UART_CFIFO_RXFLUSH_MASK | UART_CFIFO_TXFLUSH_MASK;

    // Enable receiver and transmitter
    uartch->C2 |= (UART_C2_TE_MASK | UART_C2_RE_MASK );

    return kStatus_Success;
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
char scuart_getchar (UART_Type * channel)
{
      /* Wait until character has been received */
      while (!(channel->S1 & UART_S1_RDRF_MASK));

      /* Return the 8-bit data from the receiver */
      return channel->D;
}
/********************************************************************/
/*
 * Wait for space in the uart Tx FIFO and then send a character
 *
 * Parameters:
 *  channel      uart channel to send to
 *  ch             character to send
 */
void scuart_putchar (UART_Type * channel, char ch)
{
    // Wait until space is available in the FIFO
    while(!(channel->S1 & UART_S1_TDRE_MASK));

    // Send the character
    channel->D = (uint8_t)ch;
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
int scuart_getchar_present (UART_Type * channel)
{
    return (channel->S1 & UART_S1_RDRF_MASK);
}
/********************************************************************/

/********************************************************************/
/*
 * Shutdown UART1 or UART2
 *
 * Parameters:
 *  uartch      the UART to shutdown
 *
 */
void scuart_shutdown (UART_Type * uartch)
{
    switch((unsigned int)uartch)
    {
        case (unsigned int)UART0:
            HW_SIM_SCGC4_CLR(BM_SIM_SCGC4_UART0);
            break;
        case (unsigned int)UART1:
            HW_SIM_SCGC4_CLR(BM_SIM_SCGC4_UART1);
            break;
        case (unsigned int)UART2:
            HW_SIM_SCGC4_CLR(BM_SIM_SCGC4_UART2);
            break;
        case (unsigned int)UART3:
            HW_SIM_SCGC4_CLR(BM_SIM_SCGC4_UART3);
            break;
        case (unsigned int)UART4:
            HW_SIM_SCGC1_CLR(BM_SIM_SCGC1_UART4);
            break;
        case (unsigned int)UART5:
            HW_SIM_SCGC1_CLR(BM_SIM_SCGC1_UART5);
            break;
    }
}

#if (defined(CW))
/*
** ===================================================================
**     Method      :  CsIO1___read_console (component ConsoleIO)
**
**     Description :
**         __read_console
**         This method is internal. It is used by Processor Expert only.
** ===================================================================
*/
int __read_console(__file_handle handle, unsigned char* buffer, size_t * count)
{
    (void)handle;                        /* Parameter is not used, suppress unused argument warning */

    *buffer = (unsigned char)uart_getchar((UART_Type*)REGS_UART_BASE(TERM_PORT_NUM));

    *count = 1;

    return (__no_io_error);
}

/*
** ===================================================================
**     Method      :  CsIO1___write_console (component ConsoleIO)
**
**     Description :
**         __write_console
**         This method is internal. It is used by Processor Expert only.
** ===================================================================
*/
int __write_console(__file_handle handle, unsigned char* buffer, size_t* count)
{
    size_t CharCnt = 0x00;

    (void)handle;                        // Parameter is not used, suppress unused argument warning
    for (;*count > 0x00; --*count)
    {
        // Wait until UART is ready for saving a next character into the transmit buffer
        out_char((unsigned char)*buffer);
        buffer++;                          // Increase buffer pointer
        CharCnt++;                         // Increase char counter
    }
    *count = CharCnt;

    return(__no_io_error);
}

/*
** ===================================================================
**     Method      :  CsIO1___close_console (component ConsoleIO)
**
**     Description :
**         __close_console
**         This method is internal. It is used by Processor Expert only.
** ===================================================================
*/
int __close_console(__file_handle handle)
{
    (void)handle;           // Parameter is not used, suppress unused argument warning
    return(__no_io_error);
}

#endif
