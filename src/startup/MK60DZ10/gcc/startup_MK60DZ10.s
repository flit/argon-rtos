/*****************************************************************************/
/* startup_MK60DZ10.s: Startup file for MK60DZ10 device series                   */
/*****************************************************************************/
/* Version: CodeSourcery Sourcery G++ Lite (with CS3)                        */
/*****************************************************************************/


/*
//*** <<< Use Configuration Wizard in Context Menu >>> ***
*/


/*
// <h> Stack Configuration
//   <o> Stack Size (in Bytes) <0x0-0xFFFFFFFF:8>
// </h>
*/

    .equ    Stack_Size, 0x00000100
    .section ".stack", "w"
    .align  3
    .globl  __cs3_stack_mem
    .globl  __cs3_stack_size
__cs3_stack_mem:
    .if     Stack_Size
    .space  Stack_Size
    .endif
    .size   __cs3_stack_mem,  . - __cs3_stack_mem
    .set    __cs3_stack_size, . - __cs3_stack_mem


/*
// <h> Heap Configuration
//   <o>  Heap Size (in Bytes) <0x0-0xFFFFFFFF:8>
// </h>
*/

    .equ    Heap_Size,  0x00001000

    .section ".heap", "w"
    .align  3
    .globl  __cs3_heap_start
    .globl  __cs3_heap_end
__cs3_heap_start:
    .if     Heap_Size
    .space  Heap_Size
    .endif
__cs3_heap_end:


/* Vector Table */

    .section ".cs3.interrupt_vector"
    .globl  __cs3_interrupt_vector_cortex_m
    .type   __cs3_interrupt_vector_cortex_m, %object

__cs3_interrupt_vector_cortex_m:
    .long   __cs3_stack                 /* Top of Stack                 */
    .long   __cs3_reset                 /* Reset Handler                */
    .long   NMI_Handler                 /* NMI Handler                  */
    .long   HardFault_Handler           /* Hard Fault Handler           */
    .long   MemManage_Handler           /* MPU Fault Handler            */
    .long   BusFault_Handler            /* Bus Fault Handler            */
    .long   UsageFault_Handler          /* Usage Fault Handler          */
    .long   0                           /* Reserved                     */
    .long   0                           /* Reserved                     */
    .long   0                           /* Reserved                     */
    .long   0                           /* Reserved                     */
    .long   SVC_Handler                 /* SVCall Handler               */
    .long   DebugMon_Handler            /* Debug Monitor Handler        */
    .long   0                           /* Reserved                     */
    .long   PendSV_Handler              /* PendSV Handler               */
    .long   SysTick_Handler             /* SysTick Handler              */

    /* External Interrupts */
    .long   DMA0_IRQHandler  /* DMA Channel 0 Transfer Complete */
    .long   DMA1_IRQHandler  /* DMA Channel 1 Transfer Complete */
    .long   DMA2_IRQHandler  /* DMA Channel 2 Transfer Complete */
    .long   DMA3_IRQHandler  /* DMA Channel 3 Transfer Complete */
    .long   DMA4_IRQHandler  /* DMA Channel 4 Transfer Complete */
    .long   DMA5_IRQHandler  /* DMA Channel 5 Transfer Complete */
    .long   DMA6_IRQHandler  /* DMA Channel 6 Transfer Complete */
    .long   DMA7_IRQHandler  /* DMA Channel 7 Transfer Complete */
    .long   DMA8_IRQHandler  /* DMA Channel 8 Transfer Complete */
    .long   DMA9_IRQHandler  /* DMA Channel 9 Transfer Complete */
    .long   DMA10_IRQHandler  /* DMA Channel 10 Transfer Complete */
    .long   DMA11_IRQHandler  /* DMA Channel 11 Transfer Complete */
    .long   DMA12_IRQHandler  /* DMA Channel 12 Transfer Complete */
    .long   DMA13_IRQHandler  /* DMA Channel 13 Transfer Complete */
    .long   DMA14_IRQHandler  /* DMA Channel 14 Transfer Complete */
    .long   DMA15_IRQHandler  /* DMA Channel 15 Transfer Complete */
    .long   DMA_Error_IRQHandler  /* DMA Error Interrupt */
    .long   MCM_IRQHandler  /* Normal Interrupt */
    .long   FTFL_IRQHandler  /* FTFL Interrupt */
    .long   Read_Collision_IRQHandler  /* Read Collision Interrupt */
    .long   LVD_LVW_IRQHandler  /* Low Voltage Detect, Low Voltage Warning */
    .long   LLW_IRQHandler  /* Low Leakage Wakeup */
    .long   Watchdog_IRQHandler  /* WDOG Interrupt */
    .long   RNG_IRQHandler  /* RNGB Interrupt */
    .long   I2C0_IRQHandler  /* I2C0 interrupt */
    .long   I2C1_IRQHandler  /* I2C1 interrupt */
    .long   SPI0_IRQHandler  /* SPI0 Interrupt */
    .long   SPI1_IRQHandler  /* SPI1 Interrupt */
    .long   SPI2_IRQHandler  /* SPI2 Interrupt */
    .long   CAN0_ORed_Message_buffer_IRQHandler  /* CAN0 OR'd Message Buffers Interrupt */
    .long   CAN0_Bus_Off_IRQHandler  /* CAN0 Bus Off Interrupt */
    .long   CAN0_Error_IRQHandler  /* CAN0 Error Interrupt */
    .long   CAN0_Tx_Warning_IRQHandler  /* CAN0 Tx Warning Interrupt */
    .long   CAN0_Rx_Warning_IRQHandler  /* CAN0 Rx Warning Interrupt */
    .long   CAN0_Wake_Up_IRQHandler  /* CAN0 Wake Up Interrupt */
    .long   Reserved51_IRQHandler  /* Reserved interrupt 51 */
    .long   Reserved52_IRQHandler  /* Reserved interrupt 52 */
    .long   CAN1_ORed_Message_buffer_IRQHandler  /* CAN1 OR'd Message Buffers Interrupt */
    .long   CAN1_Bus_Off_IRQHandler  /* CAN1 Bus Off Interrupt */
    .long   CAN1_Error_IRQHandler  /* CAN1 Error Interrupt */
    .long   CAN1_Tx_Warning_IRQHandler  /* CAN1 Tx Warning Interrupt */
    .long   CAN1_Rx_Warning_IRQHandler  /* CAN1 Rx Warning Interrupt */
    .long   CAN1_Wake_Up_IRQHandler  /* CAN1 Wake Up Interrupt */
    .long   Reserved59_IRQHandler  /* Reserved interrupt 59 */
    .long   Reserved60_IRQHandler  /* Reserved interrupt 60 */
    .long   UART0_RX_TX_IRQHandler  /* UART0 Receive/Transmit interrupt */
    .long   UART0_ERR_IRQHandler  /* UART0 Error interrupt */
    .long   UART1_RX_TX_IRQHandler  /* UART1 Receive/Transmit interrupt */
    .long   UART1_ERR_IRQHandler  /* UART1 Error interrupt */
    .long   UART2_RX_TX_IRQHandler  /* UART2 Receive/Transmit interrupt */
    .long   UART2_ERR_IRQHandler  /* UART2 Error interrupt */
    .long   UART3_RX_TX_IRQHandler  /* UART3 Receive/Transmit interrupt */
    .long   UART3_ERR_IRQHandler  /* UART3 Error interrupt */
    .long   UART4_RX_TX_IRQHandler  /* UART4 Receive/Transmit interrupt */
    .long   UART4_ERR_IRQHandler  /* UART4 Error interrupt */
    .long   UART5_RX_TX_IRQHandler  /* UART5 Receive/Transmit interrupt */
    .long   UART5_ERR_IRQHandler  /* UART5 Error interrupt */
    .long   ADC0_IRQHandler  /* ADC0 interrupt */
    .long   ADC1_IRQHandler  /* ADC1 interrupt */
    .long   CMP0_IRQHandler  /* CMP0 interrupt */
    .long   CMP1_IRQHandler  /* CMP1 interrupt */
    .long   CMP2_IRQHandler  /* CMP2 interrupt */
    .long   FTM0_IRQHandler  /* FTM0 fault, overflow and channels interrupt */
    .long   FTM1_IRQHandler  /* FTM1 fault, overflow and channels interrupt */
    .long   FTM2_IRQHandler  /* FTM2 fault, overflow and channels interrupt */
    .long   CMT_IRQHandler  /* CMT interrupt */
    .long   RTC_IRQHandler  /* RTC interrupt */
    .long   Reserved83_IRQHandler  /* Reserved interrupt 83 */
    .long   PIT0_IRQHandler  /* PIT timer channel 0 interrupt */
    .long   PIT1_IRQHandler  /* PIT timer channel 1 interrupt */
    .long   PIT2_IRQHandler  /* PIT timer channel 2 interrupt */
    .long   PIT3_IRQHandler  /* PIT timer channel 3 interrupt */
    .long   PDB0_IRQHandler  /* PDB0 Interrupt */
    .long   USB0_IRQHandler  /* USB0 interrupt */
    .long   USBDCD_IRQHandler  /* USBDCD Interrupt */
    .long   ENET_1588_Timer_IRQHandler  /* Ethernet MAC IEEE 1588 Timer Interrupt */
    .long   ENET_Transmit_IRQHandler  /* Ethernet MAC Transmit Interrupt */
    .long   ENET_Receive_IRQHandler  /* Ethernet MAC Receive Interrupt */
    .long   ENET_Error_IRQHandler  /* Ethernet MAC Error and miscelaneous Interrupt */
    .long   I2S0_IRQHandler  /* I2S0 Interrupt */
    .long   SDHC_IRQHandler  /* SDHC Interrupt */
    .long   DAC0_IRQHandler  /* DAC0 interrupt */
    .long   DAC1_IRQHandler  /* DAC1 interrupt */
    .long   TSI0_IRQHandler  /* TSI0 Interrupt */
    .long   MCG_IRQHandler  /* MCG Interrupt */
    .long   LPTimer_IRQHandler  /* LPTimer interrupt */
    .long   Reserved102_IRQHandler  /* Reserved interrupt 102 */
    .long   PORTA_IRQHandler  /* Port A interrupt */
    .long   PORTB_IRQHandler  /* Port B interrupt */
    .long   PORTC_IRQHandler  /* Port C interrupt */
    .long   PORTD_IRQHandler  /* Port D interrupt */
    .long   PORTE_IRQHandler  /* Port E interrupt */
    .long   Reserved108_IRQHandler  /* Reserved interrupt 108 */
    .long   Reserved109_IRQHandler  /* Reserved interrupt 109 */
    .long   Reserved110_IRQHandler  /* Reserved interrupt 110 */
    .long   Reserved111_IRQHandler  /* Reserved interrupt 111 */
    .long   Reserved112_IRQHandler  /* Reserved interrupt 112 */
    .long   Reserved113_IRQHandler  /* Reserved interrupt 113 */
    .long   Reserved114_IRQHandler  /* Reserved interrupt 114 */
    .long   Reserved115_IRQHandler  /* Reserved interrupt 115 */
    .long   Reserved116_IRQHandler  /* Reserved interrupt 116 */
    .long   Reserved117_IRQHandler  /* Reserved interrupt 117 */
    .long   Reserved118_IRQHandler  /* Reserved interrupt 118 */
    .long   Reserved119_IRQHandler  /* Reserved interrupt 119 */
    .long   DefaultISR  /* 120 */
    .long   DefaultISR  /* 121 */
    .long   DefaultISR  /* 122 */
    .long   DefaultISR  /* 123 */
    .long   DefaultISR  /* 124 */
    .long   DefaultISR  /* 125 */
    .long   DefaultISR  /* 126 */
    .long   DefaultISR  /* 127 */
    .long   DefaultISR  /* 128 */
    .long   DefaultISR  /* 129 */
    .long   DefaultISR  /* 130 */
    .long   DefaultISR  /* 131 */
    .long   DefaultISR  /* 132 */
    .long   DefaultISR  /* 133 */
    .long   DefaultISR  /* 134 */
    .long   DefaultISR  /* 135 */
    .long   DefaultISR  /* 136 */
    .long   DefaultISR  /* 137 */
    .long   DefaultISR  /* 138 */
    .long   DefaultISR  /* 139 */
    .long   DefaultISR  /* 140 */
    .long   DefaultISR  /* 141 */
    .long   DefaultISR  /* 142 */
    .long   DefaultISR  /* 143 */
    .long   DefaultISR  /* 144 */
    .long   DefaultISR  /* 145 */
    .long   DefaultISR  /* 146 */
    .long   DefaultISR  /* 147 */
    .long   DefaultISR  /* 148 */
    .long   DefaultISR  /* 149 */
    .long   DefaultISR  /* 150 */
    .long   DefaultISR  /* 151 */
    .long   DefaultISR  /* 152 */
    .long   DefaultISR  /* 153 */
    .long   DefaultISR  /* 154 */
    .long   DefaultISR  /* 155 */
    .long   DefaultISR  /* 156 */
    .long   DefaultISR  /* 157 */
    .long   DefaultISR  /* 158 */
    .long   DefaultISR  /* 159 */
    .long   DefaultISR  /* 160 */
    .long   DefaultISR  /* 161 */
    .long   DefaultISR  /* 162 */
    .long   DefaultISR  /* 163 */
    .long   DefaultISR  /* 164 */
    .long   DefaultISR  /* 165 */
    .long   DefaultISR  /* 166 */
    .long   DefaultISR  /* 167 */
    .long   DefaultISR  /* 168 */
    .long   DefaultISR  /* 169 */
    .long   DefaultISR  /* 170 */
    .long   DefaultISR  /* 171 */
    .long   DefaultISR  /* 172 */
    .long   DefaultISR  /* 173 */
    .long   DefaultISR  /* 174 */
    .long   DefaultISR  /* 175 */
    .long   DefaultISR  /* 176 */
    .long   DefaultISR  /* 177 */
    .long   DefaultISR  /* 178 */
    .long   DefaultISR  /* 179 */
    .long   DefaultISR  /* 180 */
    .long   DefaultISR  /* 181 */
    .long   DefaultISR  /* 182 */
    .long   DefaultISR  /* 183 */
    .long   DefaultISR  /* 184 */
    .long   DefaultISR  /* 185 */
    .long   DefaultISR  /* 186 */
    .long   DefaultISR  /* 187 */
    .long   DefaultISR  /* 188 */
    .long   DefaultISR  /* 189 */
    .long   DefaultISR  /* 190 */
    .long   DefaultISR  /* 191 */
    .long   DefaultISR  /* 192 */
    .long   DefaultISR  /* 193 */
    .long   DefaultISR  /* 194 */
    .long   DefaultISR  /* 195 */
    .long   DefaultISR  /* 196 */
    .long   DefaultISR  /* 197 */
    .long   DefaultISR  /* 198 */
    .long   DefaultISR  /* 199 */
    .long   DefaultISR  /* 200 */
    .long   DefaultISR  /* 201 */
    .long   DefaultISR  /* 202 */
    .long   DefaultISR  /* 203 */
    .long   DefaultISR  /* 204 */
    .long   DefaultISR  /* 205 */
    .long   DefaultISR  /* 206 */
    .long   DefaultISR  /* 207 */
    .long   DefaultISR  /* 208 */
    .long   DefaultISR  /* 209 */
    .long   DefaultISR  /* 210 */
    .long   DefaultISR  /* 211 */
    .long   DefaultISR  /* 212 */
    .long   DefaultISR  /* 213 */
    .long   DefaultISR  /* 214 */
    .long   DefaultISR  /* 215 */
    .long   DefaultISR  /* 216 */
    .long   DefaultISR  /* 217 */
    .long   DefaultISR  /* 218 */
    .long   DefaultISR  /* 219 */
    .long   DefaultISR  /* 220 */
    .long   DefaultISR  /* 221 */
    .long   DefaultISR  /* 222 */
    .long   DefaultISR  /* 223 */
    .long   DefaultISR  /* 224 */
    .long   DefaultISR  /* 225 */
    .long   DefaultISR  /* 226 */
    .long   DefaultISR  /* 227 */
    .long   DefaultISR  /* 228 */
    .long   DefaultISR  /* 229 */
    .long   DefaultISR  /* 230 */
    .long   DefaultISR  /* 231 */
    .long   DefaultISR  /* 232 */
    .long   DefaultISR  /* 233 */
    .long   DefaultISR  /* 234 */
    .long   DefaultISR  /* 235 */
    .long   DefaultISR  /* 236 */
    .long   DefaultISR  /* 237 */
    .long   DefaultISR  /* 238 */
    .long   DefaultISR  /* 239 */
    .long   DefaultISR  /* 240 */
    .long   DefaultISR  /* 241 */
    .long   DefaultISR  /* 242 */
    .long   DefaultISR  /* 243 */
    .long   DefaultISR  /* 244 */
    .long   DefaultISR  /* 245 */
    .long   DefaultISR  /* 246 */
    .long   DefaultISR  /* 247 */
    .long   DefaultISR  /* 248 */
    .long   DefaultISR  /* 249 */
    .long   DefaultISR  /* 250 */
    .long   DefaultISR  /* 251 */
    .long   DefaultISR  /* 252 */
    .long   DefaultISR  /* 253 */
    .long   DefaultISR  /* 254 */
    .long   DefaultISR  /* 255 */


    .size   __cs3_interrupt_vector_cortex_m, . - __cs3_interrupt_vector_cortex_m

/* Flash Configuration */

  	.long	0xFFFFFFFF
  	.long	0xFFFFFFFF
  	.long	0xFFFFFFFF
  	.long	0xFFFFFFFE

    .thumb


/* Reset Handler */

    .section .cs3.reset,"x",%progbits
    .thumb_func
    .globl  __cs3_reset_cortex_m
    .type   __cs3_reset_cortex_m, %function
__cs3_reset_cortex_m:
    .fnstart
    LDR     R0, =SystemInit
    BLX     R0
    LDR     R0,=_start
    BX      R0
    .pool
    .cantunwind
    .fnend
    .size   __cs3_reset_cortex_m,.-__cs3_reset_cortex_m

    .section ".text"

/* Exception Handlers */

    .weak   NMI_Handler
    .type   NMI_Handler, %function
NMI_Handler:
    B       .
    .size   NMI_Handler, . - NMI_Handler

    .weak   HardFault_Handler
    .type   HardFault_Handler, %function
HardFault_Handler:
    B       .
    .size   HardFault_Handler, . - HardFault_Handler

    .weak   MemManage_Handler
    .type   MemManage_Handler, %function
MemManage_Handler:
    B       .
    .size   MemManage_Handler, . - MemManage_Handler

    .weak   BusFault_Handler
    .type   BusFault_Handler, %function
BusFault_Handler:
    B       .
    .size   BusFault_Handler, . - BusFault_Handler

    .weak   UsageFault_Handler
    .type   UsageFault_Handler, %function
UsageFault_Handler:
    B       .
    .size   UsageFault_Handler, . - UsageFault_Handler

    .weak   SVC_Handler
    .type   SVC_Handler, %function
SVC_Handler:
    B       .
    .size   SVC_Handler, . - SVC_Handler

    .weak   DebugMon_Handler
    .type   DebugMon_Handler, %function
DebugMon_Handler:
    B       .
    .size   DebugMon_Handler, . - DebugMon_Handler

    .weak   PendSV_Handler
    .type   PendSV_Handler, %function
PendSV_Handler:
    B       .
    .size   PendSV_Handler, . - PendSV_Handler

    .weak   SysTick_Handler
    .type   SysTick_Handler, %function
SysTick_Handler:
    B       .
    .size   SysTick_Handler, . - SysTick_Handler


/* IRQ Handlers */

    .globl  Default_Handler
    .type   Default_Handler, %function
Default_Handler:
    B       .
    .size   Default_Handler, . - Default_Handler

    .macro  IRQ handler
    .weak   \handler
    .set    \handler, Default_Handler
    .endm

    IRQ     DMA0_IRQHandler
    IRQ     DMA1_IRQHandler
    IRQ     DMA2_IRQHandler
    IRQ     DMA3_IRQHandler
    IRQ     DMA4_IRQHandler
    IRQ     DMA5_IRQHandler
    IRQ     DMA6_IRQHandler
    IRQ     DMA7_IRQHandler
    IRQ     DMA8_IRQHandler
    IRQ     DMA9_IRQHandler
    IRQ     DMA10_IRQHandler
    IRQ     DMA11_IRQHandler
    IRQ     DMA12_IRQHandler
    IRQ     DMA13_IRQHandler
    IRQ     DMA14_IRQHandler
    IRQ     DMA15_IRQHandler
    IRQ     DMA_Error_IRQHandler
    IRQ     MCM_IRQHandler
    IRQ     FTFL_IRQHandler
    IRQ     Read_Collision_IRQHandler
    IRQ     LVD_LVW_IRQHandler
    IRQ     LLW_IRQHandler
    IRQ     Watchdog_IRQHandler
    IRQ     RNG_IRQHandler
    IRQ     I2C0_IRQHandler
    IRQ     I2C1_IRQHandler
    IRQ     SPI0_IRQHandler
    IRQ     SPI1_IRQHandler
    IRQ     SPI2_IRQHandler
    IRQ     CAN0_ORed_Message_buffer_IRQHandler
    IRQ     CAN0_Bus_Off_IRQHandler
    IRQ     CAN0_Error_IRQHandler
    IRQ     CAN0_Tx_Warning_IRQHandler
    IRQ     CAN0_Rx_Warning_IRQHandler
    IRQ     CAN0_Wake_Up_IRQHandler
    IRQ     Reserved51_IRQHandler
    IRQ     Reserved52_IRQHandler
    IRQ     CAN1_ORed_Message_buffer_IRQHandler
    IRQ     CAN1_Bus_Off_IRQHandler
    IRQ     CAN1_Error_IRQHandler
    IRQ     CAN1_Tx_Warning_IRQHandler
    IRQ     CAN1_Rx_Warning_IRQHandler
    IRQ     CAN1_Wake_Up_IRQHandler
    IRQ     Reserved59_IRQHandler
    IRQ     Reserved60_IRQHandler
    IRQ     UART0_RX_TX_IRQHandler
    IRQ     UART0_ERR_IRQHandler
    IRQ     UART1_RX_TX_IRQHandler
    IRQ     UART1_ERR_IRQHandler
    IRQ     UART2_RX_TX_IRQHandler
    IRQ     UART2_ERR_IRQHandler
    IRQ     UART3_RX_TX_IRQHandler
    IRQ     UART3_ERR_IRQHandler
    IRQ     UART4_RX_TX_IRQHandler
    IRQ     UART4_ERR_IRQHandler
    IRQ     UART5_RX_TX_IRQHandler
    IRQ     UART5_ERR_IRQHandler
    IRQ     ADC0_IRQHandler
    IRQ     ADC1_IRQHandler
    IRQ     CMP0_IRQHandler
    IRQ     CMP1_IRQHandler
    IRQ     CMP2_IRQHandler
    IRQ     FTM0_IRQHandler
    IRQ     FTM1_IRQHandler
    IRQ     FTM2_IRQHandler
    IRQ     CMT_IRQHandler
    IRQ     RTC_IRQHandler
    IRQ     Reserved83_IRQHandler
    IRQ     PIT0_IRQHandler
    IRQ     PIT1_IRQHandler
    IRQ     PIT2_IRQHandler
    IRQ     PIT3_IRQHandler
    IRQ     PDB0_IRQHandler
    IRQ     USB0_IRQHandler
    IRQ     USBDCD_IRQHandler
    IRQ     ENET_1588_Timer_IRQHandler
    IRQ     ENET_Transmit_IRQHandler
    IRQ     ENET_Receive_IRQHandler
    IRQ     ENET_Error_IRQHandler
    IRQ     I2S0_IRQHandler
    IRQ     SDHC_IRQHandler
    IRQ     DAC0_IRQHandler
    IRQ     DAC1_IRQHandler
    IRQ     TSI0_IRQHandler
    IRQ     MCG_IRQHandler
    IRQ     LPTimer_IRQHandler
    IRQ     Reserved102_IRQHandler
    IRQ     PORTA_IRQHandler
    IRQ     PORTB_IRQHandler
    IRQ     PORTC_IRQHandler
    IRQ     PORTD_IRQHandler
    IRQ     PORTE_IRQHandler
    IRQ     Reserved108_IRQHandler
    IRQ     Reserved109_IRQHandler
    IRQ     Reserved110_IRQHandler
    IRQ     Reserved111_IRQHandler
    IRQ     Reserved112_IRQHandler
    IRQ     Reserved113_IRQHandler
    IRQ     Reserved114_IRQHandler
    IRQ     Reserved115_IRQHandler
    IRQ     Reserved116_IRQHandler
    IRQ     Reserved117_IRQHandler
    IRQ     Reserved118_IRQHandler
    IRQ     Reserved119_IRQHandler
    IRQ     DefaultISR

    .end
