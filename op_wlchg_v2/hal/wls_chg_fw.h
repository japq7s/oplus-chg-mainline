// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020-2022 Oplus. All rights reserved.
 */

#ifndef __WLS_CHG_FW_H__
#define __WLS_CHG_FW_H__
#include <linux/types.h>

#ifdef MODULE

#define OPLUS_CHG_FIRMWARE_MAGIC 0x68305120

struct oplus_wls_chg_firmware {
	const char *name;
	int fw_size;
	char *wls_chg_firmware;
	int magic;
};

#define oplus_wls_chg_firmware_register(__name)					\
__attribute__((section(".oplus_chg_wls_fw.data"),				\
			used)) struct oplus_wls_chg_firmware __name##_fw = {	\
	.name = #__name,							\
	.fw_size = sizeof(__name),						\
	.wls_chg_firmware = __name,						\
	.magic = OPLUS_CHG_FIRMWARE_MAGIC,					\
}

#else /* MODULE */

#define OPLUS_CHG_FIRMWARE_MAGIC0 0x68305120
#define OPLUS_CHG_FIRMWARE_MAGIC1 0x68305121
struct oplus_wls_chg_firmware {
	const char *name;
    int fw_size;
    char *wls_chg_firmware;
    int magic0;
    int magic1;
};

#define oplus_wls_chg_firmware_register(__name)					\
__attribute__((section(".oplus_wls_chg_fw.data"),				\
			used)) struct oplus_wls_chg_firmware __name##_fw = {	\
	.name = #__name,							\
	.fw_size = sizeof(__name),						\
	.wls_chg_firmware = __name,						\
	.magic0 = OPLUS_CHG_FIRMWARE_MAGIC0,					\
	.magic1 = OPLUS_CHG_FIRMWARE_MAGIC1,					\
}

#define oplus_wls_chg_firmware_register_null(__name)				\
__attribute__((section(".oplus_wls_chg_fw.data"),				\
			used)) struct oplus_wls_chg_firmware __name##_fw = {	\
	.name = #__name,							\
	.fw_size = 0,								\
	.wls_chg_firmware = NULL,						\
	.magic0 = OPLUS_CHG_FIRMWARE_MAGIC0,					\
	.magic1 = OPLUS_CHG_FIRMWARE_MAGIC1,					\
}

static oplus_wls_chg_firmware_register_null(oplus_wls_firmware_start);

#endif /* MODULE */

#endif
