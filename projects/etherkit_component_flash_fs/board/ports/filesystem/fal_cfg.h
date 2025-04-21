/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-02-06     newfl       the first version
 */
#ifndef BOARD_FAL_CFG_H_
#define BOARD_FAL_CFG_H_

extern const struct fal_flash_dev n2l_xspi_flash;

/* flash device table */
#define FAL_FLASH_DEV_TABLE { \
    &n2l_xspi_flash,          \
}

#define FAL_PART_TABLE {                                                                                            \
    {FAL_PART_MAGIC_WROD,   "app", "qspi0cs0_flash",               0, 7 * 1024 * 1024, 0}, \
    {FAL_PART_MAGIC_WROD, "param", "qspi0cs0_flash", 7 * 1024 * 1024, 1 * 1024 * 1024, 0}, \
}

#endif /* BOARD_FAL_CFG_H_ */
