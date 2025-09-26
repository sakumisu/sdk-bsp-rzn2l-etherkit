/*
 * Copyright (c) 2024 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

/*  HPM example includes. */
#include <stdio.h>
#include "board.h"
#include "hpm_clock_drv.h"
#include "hpm_uart_drv.h"
#include "shell.h"
#include "hpm_gptmr_drv.h"
#include "cia402_def.h"
#include "ec_master.h"

SDK_DECLARE_EXT_ISR_M(BOARD_CONSOLE_UART_IRQ, shell_uart_isr)

#define task_start_PRIORITY (configMAX_PRIORITIES - 2U)

#define MOTOR_MODE_CSV_CSP  0
#define MOTOR_MODE_CSP      1
#define MOTOR_MODE_CSV      2

volatile uint8_t motor_mode = MOTOR_MODE_CSV;

ec_master_t g_ec_master;

static void task_start(void *param);

int main(void)
{
    board_init();

    if (pdPASS != xTaskCreate(task_start, "task_start", 1024U, NULL, task_start_PRIORITY, NULL)) {
        printf("Task start creation failed!\r\n");
        while (1) {
        };
    }

    vTaskStartScheduler();
    printf("Unexpected scheduler exit!\r\n");
    while (1) {
    };

    return 0;
}

static void task_start(void *param)
{
    (void)param;

    printf("Try to initialize the uart\r\n"
           "  if you are using the console uart as the shell uart\r\n"
           "  failure to initialize may result in no log\r\n");

    uart_config_t shell_uart_config = { 0 };
    uart_default_config(BOARD_CONSOLE_UART_BASE, &shell_uart_config);
    shell_uart_config.src_freq_in_hz = clock_get_frequency(BOARD_CONSOLE_UART_CLK_NAME);
    shell_uart_config.baudrate = 115200;

    if (status_success != uart_init(BOARD_CONSOLE_UART_BASE, &shell_uart_config)) {
        /* uart failed to be initialized */
        printf("Failed to initialize uart\r\n");
        while (1) {
        };
    }

    printf("Initialize shell uart successfully\r\n");

    /* default password is : 12345678 */
    /* shell_init() must be called in-task */
    if (0 != shell_init(BOARD_CONSOLE_UART_BASE, false)) {
        /* shell failed to be initialized */
        printf("Failed to initialize shell\r\n");
        while (1) {
        };
    }

    printf("Initialize shell successfully\r\n");

    /* irq must be enabled after shell_init() */
    uart_enable_irq(BOARD_CONSOLE_UART_BASE, uart_intr_rx_data_avail_or_timeout);
    intc_m_enable_irq_with_priority(BOARD_CONSOLE_UART_IRQ, 1);

    printf("Enable shell uart interrupt\r\n");

    ec_master_cmd_init(&g_ec_master);
    ec_master_init(&g_ec_master, 0);

    printf("Exit start task\r\n");

    vTaskDelete(NULL);
}

CSH_CMD_EXPORT(ethercat, );

unsigned char cherryecat_eepromdata[2048]; // EEPROM data buffer, please generate by esi_parse.py

static ec_pdo_entry_info_t dio_1600[] = {
    { 0x6000, 0x00, 0x20 },
};

static ec_pdo_entry_info_t dio_1a00[] = {
    { 0x7010, 0x00, 0x20 },
};

static ec_pdo_info_t dio_rxpdos[] = {
    { 0x1600, 1, &dio_1600[0] },
};

static ec_pdo_info_t dio_txpdos[] = {
    { 0x1a00, 1, &dio_1a00[0] },
};

static ec_sync_info_t dio_syncs[] = {
    { 2, EC_DIR_OUTPUT, 1, dio_rxpdos },
    { 3, EC_DIR_INPUT, 1, dio_txpdos },
};

static ec_pdo_entry_info_t coe402_1602[] = {
    { 0x6040, 0x00, 0x10 },
    { 0x60ff, 0x00, 0x20 },
    { 0x0000, 0x00, 0x10 },
};

static ec_pdo_entry_info_t coe402_1a02[] = {
    { 0x6041, 0x00, 0x10 },
    { 0x6064, 0x00, 0x20 },
    { 0x0000, 0x00, 0x10 },
};

static ec_pdo_info_t cia402_rxpdos[] = {
    { 0x1602, 3, &coe402_1602[0] },
};

static ec_pdo_info_t cia402_txpdos[] = {
    { 0x1a02, 3, &coe402_1a02[0] },
};

static ec_sync_info_t cia402_syncs[] = {
    { 2, EC_DIR_OUTPUT, 1, cia402_rxpdos },
    { 3, EC_DIR_INPUT, 1, cia402_txpdos },
};

int ec_start(int argc, const char **argv)
{
    static ec_slave_config_t slave_cia402_config;
    static ec_slave_config_t slave_dio_config;

    if (g_ec_master.slave_count == 0) {
        printf("No slave found, please check the connection\r\n");
        return -1;
    }

    if (argc < 2) {
        printf("Please input: ec_start <cyclic time in us>\r\n");
        return -1;
    }

    slave_cia402_config.dc_assign_activate = 0x300;

    slave_cia402_config.dc_sync[0].cycle_time = atoi(argv[1]) * 1000;
    slave_cia402_config.dc_sync[0].shift_time = 1000000;
    slave_cia402_config.dc_sync[1].cycle_time = 0;
    slave_cia402_config.dc_sync[1].shift_time = 0;

    slave_cia402_config.sync = cia402_syncs;
    slave_cia402_config.sync_count = sizeof(cia402_syncs) / sizeof(ec_sync_info_t);

    slave_dio_config.dc_assign_activate = 0x300;

    slave_dio_config.dc_sync[0].cycle_time = atoi(argv[1]) * 1000;
    slave_dio_config.dc_sync[0].shift_time = 1000000;
    slave_dio_config.dc_sync[1].cycle_time = 0;
    slave_dio_config.dc_sync[1].shift_time = 0;
    slave_dio_config.sync = dio_syncs;
    slave_dio_config.sync_count = sizeof(dio_syncs) / sizeof(ec_sync_info_t);

    for (uint32_t i = 0; i < g_ec_master.slave_count; i++) {
        if (g_ec_master.slaves[i].sii.vendor_id != 0x0048504D) { // HPMicro
            EC_LOG_ERR("Unsupported slave found: vendor_id=0x%08x\n", g_ec_master.slaves[i].sii.vendor_id);
            return -1;
        }

        switch (g_ec_master.slaves[i].sii.product_code) {
            case 0x00000001: // DIO
                g_ec_master.slaves[i].config = &slave_dio_config;
                break;
            case 0x00000002: // FOE
                break;
            case 0x00000003: // CIA402
                g_ec_master.slaves[i].config = &slave_cia402_config;
                break;

            default:
                break;
        }
    }

    ec_master_start(&g_ec_master, atoi(argv[1]));
    return 0;
}
CSH_CMD_EXPORT(ec_start, );

int ec_stop(int argc, const char **argv)
{
    ec_master_stop(&g_ec_master);
    return 0;
}
CSH_CMD_EXPORT(ec_stop, );