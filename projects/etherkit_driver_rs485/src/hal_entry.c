/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2024-03-11     Wangyuqiang   first version
 */

#include <rtthread.h>
#include "hal_data.h"
#include <rtdevice.h>
#include <board.h>

#define DBG_TAG "rs485"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define LED_PIN_0    BSP_IO_PORT_14_PIN_3 /* Onboard LED pins */
#define LED_PIN_1    BSP_IO_PORT_14_PIN_0 /* Onboard LED pins */
#define LED_PIN_2    BSP_IO_PORT_14_PIN_1 /* Onboard LED pins */

void hal_entry(void)
{
    rt_kprintf("\nHello RT-Thread!\n");
    rt_kprintf("==================================================\n");
    rt_kprintf("This example project is an driver rs485 routine!\n");
    rt_kprintf("==================================================\n");

    while (1)
    {
        rt_pin_write(LED_PIN_0, PIN_HIGH);
        rt_pin_write(LED_PIN_1, PIN_HIGH);
        rt_pin_write(LED_PIN_2, PIN_HIGH);
        rt_thread_mdelay(1000);
        rt_pin_write(LED_PIN_0, PIN_LOW);
        rt_pin_write(LED_PIN_1, PIN_LOW);
        rt_pin_write(LED_PIN_2, PIN_LOW);
        rt_thread_mdelay(1000);
    }
}

#define RS485_NAME       "uart5"      /* 串口设备名称 */

struct rx_msg
{
    rt_device_t dev;
    rt_size_t size;
};
static rt_device_t rs485;
static struct rt_messagequeue rx_mq;

static rt_err_t rs485_input(rt_device_t dev, rt_size_t size)
{
    struct rx_msg msg;
    rt_err_t result;
    msg.dev = dev;
    msg.size = size;

    result = rt_mq_send(&rx_mq, &msg, sizeof(msg));
    if (result == -RT_EFULL)
    {
        LOG_W("message queue full!");
    }
    return result;
}

static void rs485_thread_entry(void *parameter)
{
    struct rx_msg msg;
    rt_err_t result;
    rt_uint32_t rx_length;
    static char rx_buffer[BSP_UART5_RX_BUFSIZE + 1];

    LOG_I("Start RS485 communication driver. Please open the RS485 serial port and start the functional test.");

    while (1)
    {
        rt_memset(&msg, 0, sizeof(msg));
        result = rt_mq_recv(&rx_mq, &msg, sizeof(msg), RT_WAITING_FOREVER);
        if (result > 0)
        {
            rx_length = rt_device_read(msg.dev, 0, rx_buffer, msg.size);
            rx_buffer[rx_length] = '\0';
            rt_device_write(rs485, 0, rx_buffer, rx_length);
            LOG_I("%s",rx_buffer);
        }
    }
}

static int rs485_sample(int argc, char *argv[])
{
    rt_err_t ret = RT_EOK;
    char rs485_name[RT_NAME_MAX];
    static char msg_pool[256];
    char str[] = "hello RT-Thread!\r\n";

    if (argc == 2)
    {
        rt_strncpy(rs485_name, argv[1], RT_NAME_MAX);
    }
    else
    {
        rt_strncpy(rs485_name, RS485_NAME, RT_NAME_MAX);
    }

    rs485 = rt_device_find(rs485_name);
    if (!rs485)
    {
        LOG_E("find %s failed!", rs485_name);
        return RT_ERROR;
    }

    rt_mq_init(&rx_mq, "rx_mq",
               msg_pool,
               sizeof(struct rx_msg),
               sizeof(msg_pool),
               RT_IPC_FLAG_FIFO);

    rt_device_open(rs485, RT_DEVICE_FLAG_RX_NON_BLOCKING | RT_DEVICE_FLAG_TX_BLOCKING);
    rt_device_set_rx_indicate(rs485, rs485_input);
    rt_device_write(rs485, 0, str, (sizeof(str) - 1));

    rt_thread_t thread = rt_thread_create("rs485", rs485_thread_entry, RT_NULL, 1024, 25, 10);
    if (thread != RT_NULL)
    {
        rt_thread_startup(thread);
    }
    else
    {
        ret = RT_ERROR;
    }

    return ret;
}
MSH_CMD_EXPORT(rs485_sample, rs485 device sample);
