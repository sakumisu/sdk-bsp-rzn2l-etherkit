/* generated HAL header file - do not edit */
#ifndef HAL_DATA_H_
#define HAL_DATA_H_
#include <stdint.h>
#include "bsp_api.h"
#include "common_data.h"
#include "r_canfd.h"
#include "r_can_api.h"
#include "r_sci_uart.h"
            #include "r_uart_api.h"
FSP_HEADER
/** CANFD on CANFD Instance. */
extern const can_instance_t g_canfd1;
/** Access the CANFD instance using these structures when calling API functions directly (::p_api is not used). */
extern canfd_instance_ctrl_t g_canfd1_ctrl;
extern const can_cfg_t g_canfd1_cfg;
extern const canfd_extended_cfg_t g_canfd1_cfg_extend;

#ifndef canfd1_callback
void canfd1_callback(can_callback_args_t * p_args);
#endif

/* Global configuration (referenced by all instances) */
extern canfd_global_cfg_t g_canfd_global_cfg;
/** CANFD on CANFD Instance. */
extern const can_instance_t g_canfd0;
/** Access the CANFD instance using these structures when calling API functions directly (::p_api is not used). */
extern canfd_instance_ctrl_t g_canfd0_ctrl;
extern const can_cfg_t g_canfd0_cfg;
extern const canfd_extended_cfg_t g_canfd0_cfg_extend;

#ifndef canfd0_callback
void canfd0_callback(can_callback_args_t * p_args);
#endif

/* Global configuration (referenced by all instances) */
extern canfd_global_cfg_t g_canfd_global_cfg;
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
