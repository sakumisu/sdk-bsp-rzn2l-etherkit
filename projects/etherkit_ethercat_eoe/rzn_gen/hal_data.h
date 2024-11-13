/* generated HAL header file - do not edit */
#ifndef HAL_DATA_H_
#define HAL_DATA_H_
#include <stdint.h>
#include "bsp_api.h"
#include "common_data.h"
#include "r_ethsw.h"
#include "r_ether_switch_api.h"
#include "r_gmac.h"
#include "r_ether_api.h"
#include "r_sci_uart.h"
            #include "r_uart_api.h"
FSP_HEADER
/** ether on ethsw Instance. */
extern const ether_switch_instance_t g_ethsw0;

/** Access the Ethernet PHY instance using these structures when calling API functions directly (::p_api is not used). */
extern ethsw_instance_ctrl_t g_ethsw0_ctrl;
extern const ether_switch_cfg_t g_ethsw0_cfg;
#ifndef gmac_callback_ethsw
void gmac_callback_ethsw(ether_switch_callback_args_t * const p_arg);
#endif
/** ether on ether Instance. */
extern const ether_instance_t g_ether0;

/** Access the Ethernet instance using these structures when calling API functions directly (::p_api is not used). */
extern gmac_instance_ctrl_t g_ether0_ctrl;
extern const ether_cfg_t g_ether0_cfg;

#ifndef user_ether0_callback
void user_ether0_callback(ether_callback_args_t * p_args);
#endif
/** UART on SCI Instance. */
            extern const uart_instance_t      g_uart0;

            /** Access the UART instance using these structures when calling API functions directly (::p_api is not used). */
            extern sci_uart_instance_ctrl_t     g_uart0_ctrl;
            extern const uart_cfg_t g_uart0_cfg;
            extern const sci_uart_extended_cfg_t g_uart0_cfg_extend;

            #ifndef user_uart0_callback
            void user_uart0_callback(uart_callback_args_t * p_args);
            #endif

            #define FSP_NOT_DEFINED (1)
            #if (FSP_NOT_DEFINED == FSP_NOT_DEFINED)
                #define g_uart0_P_TRANSFER_TX (NULL)
            #else
                #define g_uart0_P_TRANSFER_TX (&FSP_NOT_DEFINED)
            #endif
            #if (FSP_NOT_DEFINED == FSP_NOT_DEFINED)
                #define g_uart0_P_TRANSFER_RX (NULL)
            #else
                #define g_uart0_P_TRANSFER_RX (&FSP_NOT_DEFINED)
            #endif
            #undef FSP_NOT_DEFINED
void hal_entry(void);
void g_hal_init(void);
FSP_FOOTER
#endif /* HAL_DATA_H_ */
