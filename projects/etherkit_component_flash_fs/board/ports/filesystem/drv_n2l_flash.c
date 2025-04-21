/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-02-06     newfl       the first version
 */
#define DBG_TAG "drv.flash"
//#define DBG_LVL DBG_LOG
#define DBG_LVL DBG_WARNING
#include <rtdbg.h>

#include <fal.h>
#include <hal_data.h>

// RZ/N2L Group User’s Manual: Hardware TABLE36.3 https://www.renesas.cn/zh/document/mah/rzn2l-group-users-manual-hardware?language=en&r=1622651
// and do not us cache space 0x40000000, but use direct space 0x60000000
#define FLASH_START_XSPI0_CS0   0x60000000
// 64M space，GD25Q64: 8MB
#define FLASH_SIZE_GD25Q64      (8 * 1024 * 1024)
#define FLASH_END_XSPI0_CS0     ((uint32_t)(N2L_FLASH_START_XSPI0_CS0 + N2L_FLASH_SIZE_XSPI0_CS0))
// GD25Q64 datasheep Page7, https://item.szlcsc.com/80213.html
#define FLASH_PAGESIZE_GD25Q64      256
#define FLASH_SECTORSIZE_GD25Q64    4096

#define FLASH_START_ADDR    FLASH_START_XSPI0_CS0
#define FLASH_SIZE          FLASH_SIZE_GD25Q64
#define FLASH_END_ADDR      ((uint32_t)(FLASH_START_ADDR + FLASH_SIZE))
// minimum write size
#define FLASH_PAGESIZE      FLASH_PAGESIZE_GD25Q64
// minimum erase size
#define FLASH_SECTORSIZE    FLASH_SECTORSIZE_GD25Q64

#define FLASH_WRITE_MASK    (~(FLASH_PAGESIZE - 1))
#define FLASH_ERASE_MASK    (~(FLASH_SECTORSIZE - 1))

static int n2l_flash_init(void) {
    fsp_err_t ret = R_XSPI_QSPI_Open(&g_qspi0_ctrl, &g_qspi0_cfg);
    if (ret != FSP_SUCCESS) {
        return RT_ERROR;
    }
    LOG_I("QSPI open");
    return RT_EOK;
}

static int n2l_flash_read(rt_uint32_t addr, uint8_t* buf, size_t size) {
    size_t i;

    if ((addr + size) > FLASH_END_ADDR) {
        LOG_E("read outrange flash size! addr is (0x%p)", (void * ) (addr + size));
        return -RT_EINVAL;
    }

    LOG_D("red: 0x%08x, %d", addr, size);
    for (i = 0; i < size; i++) {
        buf[i] = *(rt_uint8_t *)(addr + i);
    }

    return size;
}

static int n2l_flash_erase(rt_uint32_t addr, size_t size) {
    uint32_t addr_start, pos, unit;
    uint32_t addr_end;
    spi_flash_status_t status_erase;

    if ((addr + size) > FLASH_END_ADDR) {
        LOG_E("ERROR: erase outrange flash size! addr is (0x%p)\n", (void * ) (addr + size));
        return -RT_EINVAL;
    }

    addr_start = addr & FLASH_ERASE_MASK;
    addr_end   = (addr + size + FLASH_SECTORSIZE - 1) & FLASH_ERASE_MASK;
    pos = addr_start;

    // erase unit must in [4096, 32768, 65536], according to hal_data.c line g_qspi0_erase_command_list
    while(addr_end > pos){
        if(addr_end - pos >= 65536){
            unit = 65536;
        }else if (addr_end - pos >= 32768){
            unit = 32768;
        }else if (addr_end - pos >= 4096) {
            unit = 4096;
        }else{
            break;
        }

        if(FSP_SUCCESS != R_XSPI_QSPI_Erase(&g_qspi0_ctrl, (uint8_t*)pos, unit)){
            LOG_E("ERROR: erase wrong");
            return RT_ERROR;
        }

        /* Wait for status register to update. */
        do {
            rt_thread_mdelay(1);
            (void) R_XSPI_QSPI_StatusGet(&g_qspi0_ctrl, &status_erase);
        } while (true == status_erase.write_in_progress);

        pos += unit;

        LOG_D("erased flash: 0x%08x, %d", pos, unit);
    }

    return size;
}

static int n2l_flash_write(rt_uint32_t addr, const uint8_t *buf, size_t size) {
    spi_flash_status_t status_write;

    if ((addr + size) > FLASH_END_ADDR) {
        LOG_E("ERROR: write outrange flash size! addr is (0x%p)\n", (void * ) (addr + size));
        return -RT_EINVAL;
    }

    LOG_D("wrt 0x%08x, %d", addr, size);

    /* write data 64 per time */
    for(int i = 0; i < size; i += 64){
        if (FSP_SUCCESS != R_XSPI_QSPI_Write(&g_qspi0_ctrl, &buf[i], (uint8_t *) (addr + i), 64)) {
            LOG_E("ERROR: Flash write error");
            return -RT_ERROR;
        }

        do {
            rt_thread_mdelay(1);
            (void) R_XSPI_QSPI_StatusGet(&g_qspi0_ctrl, &status_write);
        } while (true == status_write.write_in_progress);
    }

    return size;
}

static int fal_flash_init(void){
    return n2l_flash_init();
}

static int fal_flash_read(long offset, rt_uint8_t *buf, size_t size) {
    return n2l_flash_read(n2l_xspi_flash.addr + offset, buf, size);
}

static int fal_flash_write(long offset, const rt_uint8_t *buf, size_t size) {
    return n2l_flash_write(n2l_xspi_flash.addr + offset, buf, size);
}

static int fal_flash_erase(long offset, size_t size) {
    return n2l_flash_erase(n2l_xspi_flash.addr + offset, size);
}

const struct fal_flash_dev n2l_xspi_flash = {
    "qspi0cs0_flash",
    FLASH_START_ADDR,
    FLASH_SIZE,
    FLASH_PAGESIZE,
    {
        fal_flash_init,
        fal_flash_read,
        fal_flash_write,
        fal_flash_erase
    }
};
