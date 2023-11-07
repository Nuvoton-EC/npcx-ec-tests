/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/bbram.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <soc.h>
#include <stdlib.h>

#define LOG_LEVEL LOG_LEVEL_INF
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

static const struct device *const bbram_dev = DEVICE_DT_GET(DT_NODELABEL(bbram));

static uint32_t offs, size;
static uint8_t buff[256];

static void npcx_check_bbram_sts(void)
{
	/* check bbram data valid */
	do {
		offs = bbram_check_invalid(bbram_dev);
		LOG_INF("[%d]IBBR", offs);
	} while (offs);
	if (offs == 0)
		LOG_INF("[PASS] BBRAM data is valid");
	else
		LOG_INF("[FAIL] VSBY/VBAT is invalid");

	/* check voltage standby / backup */
	if (bbram_check_standby_power(bbram_dev) == 0)
		LOG_INF("[PASS] VSBY/VBAT is on");
	else
		LOG_INF("[FAIL] VSBY/VBAT is on");

	/* check VCC1 failure */
	if (bbram_check_power(bbram_dev) == 0)
		LOG_INF("[PASS] VCC1 is on");
	else
		LOG_INF("[FAIL] VCC1 is on");
}

static void npcx_get_size(void)
{
	bbram_get_size(bbram_dev, &size);
	LOG_INF("[%d]", size);
}

static void npcx_write(void)
{
	/* buff -> bbram */
	if (!bbram_write(bbram_dev, (size_t)offs, (size_t)size, buff))
		LOG_INF("[PASS] Write");
	else
		LOG_INF("[FAIL] Write");
}

static void npcx_read(void)
{
	/* bbram -> buff */
	if (!bbram_read(bbram_dev, (size_t)offs, (size_t)size, buff))
		LOG_INF("[PASS] Read");
	else
		LOG_INF("[FAIL] Read");
}

static void npcx_bbram_init(const struct shell *shell, size_t argc, char **argv)
{
	offs = strtoul(argv[1], NULL, 0);
	size = strtoul(argv[2], NULL, 0);

	LOG_INF("offset %d, size %d", offs, size);
}

static void bbram_idxR(const struct shell *shell, size_t argc, char **argv)
{
	/* Read buffer at idx */
	int offset = strtoul(argv[1], NULL, 0);

	LOG_INF("%d:[%d]", offset, buff[offset]);
}
static void bbram_idxW(const struct shell *shell, size_t argc, char **argv)
{
	/* Write buffer at idx */
	int offset = strtoul(argv[1], NULL, 0);
	int data = strtoul(argv[2], NULL, 0);

	buff[offset] = data;
}


int main(void)
{
	if (!device_is_ready(bbram_dev)) {
		LOG_INF("[FAIL]: BBRAM not ready");
		return 0;
	}
	LOG_INF("[PASS]: BBRAM ready");
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_bbram,
	SHELL_CMD_ARG(init, NULL, "bbram init <offset> <size> : set bbram offset and size",
		npcx_bbram_init, 3, 0),
	SHELL_CMD_ARG(chk_sts, NULL, "bbram chk_sts : check bbram status",
		npcx_check_bbram_sts, 1, 0),
	SHELL_CMD_ARG(chk_sz, NULL, "bbram chk_sz : check size", npcx_get_size, 1, 0),
	SHELL_CMD_ARG(write, NULL, "bbram write : buff->bbram", npcx_write, 1, 0),
	SHELL_CMD_ARG(read, NULL, "bbram read : bbram->buff ", npcx_read, 1, 0),
	SHELL_CMD_ARG(idxR, NULL, "bbram idxR <idx>: Read buff at idx", bbram_idxR, 2, 0),
	SHELL_CMD_ARG(idxW, NULL, "bbram idxW <idx> <data>: Write data to buff at idx",
		bbram_idxW, 3, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(bbram, &sub_bbram, "BBRAM validation commands", NULL);
