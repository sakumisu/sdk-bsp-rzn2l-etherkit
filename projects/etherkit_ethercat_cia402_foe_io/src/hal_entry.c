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

void eth_delay(ether_phy_instance_ctrl_t * p_instance_ctrl)
{
    rt_thread_mdelay(500);
    rt_kprintf("eth_delay!\n");
}

void hal_entry(void)
{
    rt_kprintf("\nHello RT-Thread!\n");
    rt_kprintf("==================================================\n");
    rt_kprintf("This example project is an ethercat CIA402_FOE_IO routine!\n");
    rt_kprintf("==================================================\n");
}
