/* generated vector source file - do not edit */
        #include "bsp_api.h"
        /* Do not build these data structures if no interrupts are currently allocated because IAR will have build errors. */
        #if VECTOR_DATA_IRQ_COUNT > 0
        BSP_DONT_REMOVE const fsp_vector_t g_vector_table[BSP_ICU_VECTOR_MAX_ENTRIES] =
        {
                        [288] = sci_uart_eri_isr, /* SCI0_ERI (SCI0 Receive error) */
            [289] = sci_uart_rxi_isr, /* SCI0_RXI (SCI0 Receive data full) */
            [290] = sci_uart_txi_isr, /* SCI0_TXI (SCI0 Transmit data empty) */
            [291] = sci_uart_tei_isr, /* SCI0_TEI (SCI0 Transmit end) */
            [316] = canfd_rx_fifo_isr, /* CAN_RXF (CANFD RX FIFO interrupt) */
            [317] = canfd_error_isr, /* CAN_GLERR (CANFD Global error interrupt) */
            [318] = canfd_channel_tx_isr, /* CAN0_TX (CANFD0 Channel TX interrupt) */
            [319] = canfd_error_isr, /* CAN0_CHERR (CANFD0 Channel CAN error interrupt) */
            [321] = canfd_channel_tx_isr, /* CAN1_TX (CANFD1 Channel TX interrupt) */
            [322] = canfd_error_isr, /* CAN1_CHERR (CANFD1 Channel CAN error interrupt) */
        };
        #endif