/* generated vector source file - do not edit */
        #include "bsp_api.h"
        /* Do not build these data structures if no interrupts are currently allocated because IAR will have build errors. */
        #if VECTOR_DATA_IRQ_COUNT > 0
        BSP_DONT_REMOVE const fsp_vector_t g_vector_table[BSP_ICU_VECTOR_MAX_ENTRIES] =
        {
                        [285] = usbfs_interrupt_handler, /* USB_FI (USB (Function) interrupt) */
            [286] = r_usb_dmaca_intDMAC0I_isr, /* USB_FDMA0 (USB (Function) DMA 0 transmit completion) */
            [287] = r_usb_dmaca_intDMAC1I_isr, /* USB_FDMA1 (USB (Function) DMA 1 transmit completion) */
            [288] = sci_uart_eri_isr, /* SCI0_ERI (SCI0 Receive error) */
            [289] = sci_uart_rxi_isr, /* SCI0_RXI (SCI0 Receive data full) */
            [290] = sci_uart_txi_isr, /* SCI0_TXI (SCI0 Transmit data empty) */
            [291] = sci_uart_tei_isr, /* SCI0_TEI (SCI0 Transmit end) */
        };
        #if (1 == BSP_FEATURE_BSP_IRQ_CR52_SEL_SUPPORTED)
        const bsp_interrupt_event_t g_interrupt_event_link_select[BSP_ICU_VECTOR_MAX_ENTRIES] =
        {
            [285] = BSP_PRV_CR52_SEL_ENUM(EVENT_USB_FI), /* USB_FI (USB (Function) interrupt) */
            [286] = BSP_PRV_CR52_SEL_ENUM(EVENT_USB_FDMA0), /* USB_FDMA0 (USB (Function) DMA 0 transmit completion) */
            [287] = BSP_PRV_CR52_SEL_ENUM(EVENT_USB_FDMA1), /* USB_FDMA1 (USB (Function) DMA 1 transmit completion) */
            [288] = BSP_PRV_CR52_SEL_ENUM(EVENT_SCI0_ERI), /* SCI0_ERI (SCI0 Receive error) */
            [289] = BSP_PRV_CR52_SEL_ENUM(EVENT_SCI0_RXI), /* SCI0_RXI (SCI0 Receive data full) */
            [290] = BSP_PRV_CR52_SEL_ENUM(EVENT_SCI0_TXI), /* SCI0_TXI (SCI0 Transmit data empty) */
            [291] = BSP_PRV_CR52_SEL_ENUM(EVENT_SCI0_TEI), /* SCI0_TEI (SCI0 Transmit end) */
        };
        #endif
        #endif