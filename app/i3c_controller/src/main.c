/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>

#include <zephyr/kernel.h>
#include <zephyr/pm/policy.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <stdlib.h>
#include <stdio.h>

/* Target drivers for testing */
#include <zephyr/drivers/i3c.h>

#define LOG_LEVEL LOG_LEVEL_INF
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i3c_m);

#define I3C_M_DEV0 DT_ALIAS(i3c_m_0)
#define I3C_M_DEV1 DT_ALIAS(i3c_m_1)
#define I3C_M_DEV2 DT_ALIAS(i3c_m_2)
/* Get device from device tree */
static const struct device *const i3c_m_devices[] = {
#if DT_NODE_HAS_STATUS(I3C_M_DEV0, okay)
	DEVICE_DT_GET(I3C_M_DEV0),
#endif
#if DT_NODE_HAS_STATUS(I3C_M_DEV1, okay)
	DEVICE_DT_GET(I3C_M_DEV1),
#endif
#if DT_NODE_HAS_STATUS(I3C_M_DEV2, okay)
	DEVICE_DT_GET(I3C_M_DEV2),
#endif
};

#define shell_printk(...) \
	shell_print(sh_ptr, ##__VA_ARGS__)

#define NUM_I3C_DEVICE ARRAY_SIZE(i3c_m_devices)
uint8_t i3c_m_dev_sel;

#define CAL_DATA_FROM_ADDR(n) ((uint8_t)(((n) * 3) + 2) + (n >> 11))

/* Commands used for validation */
enum {
	I3C_M_CMD_CCC,
	I3C_M_CMD_COUNT,
};

/* Objects used for validation */
static struct {
	int cmd;
	struct k_sem sem_sync;

	/* command args */
	struct {
		uint32_t erase_addr;
		uint32_t erase_size;
	} cmd_flash_erase_args;
} i3c_m_test_objs;


const struct shell *sh_ptr;

static void i3c_m_entry(void)
{
	for (int i = 0; i < NUM_I3C_DEVICE; i++) {
		if (!device_is_ready(i3c_m_devices[i])) {
			LOG_ERR("i3c device %s is not ready", i3c_m_devices[i]->name);
			return;
		}

		LOG_INF("i3c device [%d]:%s is ready", i, i3c_m_devices[i]->name);
	}

	/* Init semaphore first */
	k_sem_init(&i3c_m_test_objs.sem_sync, 0, 1);

	while (true) {
		/* Task wait event here */
		k_sem_take(&i3c_m_test_objs.sem_sync, K_FOREVER);

		switch (i3c_m_test_objs.cmd) {
		case I3C_M_CMD_CCC:
			shell_info(sh_ptr, "CCC handle");

			break;
		default:
			break;
		}
	};
}

/* Test thread declaration */
#define STACK_SIZE      1024
#define THREAD_PRIORITY 1
K_THREAD_DEFINE(i3c_m_thread_id, STACK_SIZE, i3c_m_entry, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, -1);

int main(void)
{
	/* Zephyr driver validation main */
	LOG_INF("Start CI20 Validation Task");

	/* Init thread */
	k_thread_name_set(i3c_m_thread_id, "i3c_cntlr_testing");
	k_thread_start(i3c_m_thread_id);

	return 0;
}

static int i3c_ccc_handler(const struct shell *shell, size_t argc, char **argv)
{
	sh_ptr = shell;

	i3c_m_test_objs.cmd = I3C_M_CMD_CCC;
	printk("CCC handler\n");

	/* Send event to task and wake it up */
	k_sem_give(&i3c_m_test_objs.sem_sync);

	return 0;
}


/* Multi I3C controller devices */
static int i3c_m_active_handler(const struct shell *shell, size_t argc, char **argv)
{
	char *eptr;
	uint32_t active_dev;

	sh_ptr = shell;

	if (argc == 1) {
		shell_info(sh_ptr, "Active i3c device Index = %d", i3c_m_dev_sel);
		return 0;
	}

	active_dev = strtoul(argv[1], &eptr, 0);
	if (*eptr != '\0') {
		shell_error(shell, "Invalid argument, '%s' is not an integer", argv[1]);
		return -EINVAL;
	}

	if (active_dev < NUM_I3C_DEVICE) {
		i3c_m_dev_sel = active_dev;
	} else {
		i3c_m_dev_sel = 0;
	}

	shell_info(sh_ptr, "Select active flash device to [%d]%s", i3c_m_dev_sel,
					i3c_m_devices[i3c_m_dev_sel]->name);
	return 0;
}

static int i3c_m_list_handler(const struct shell *shell, size_t argc, char **argv)
{
	sh_ptr = shell;

	for (int i = 0; i < NUM_I3C_DEVICE; i++) {
		shell_info(sh_ptr, "i3c device [%d]:%s", i, i3c_m_devices[i]->name);
	}
	shell_info(sh_ptr, "Current active index = %d", i3c_m_dev_sel);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_i3c,
	SHELL_CMD_ARG(ccc, NULL, "i3c_cntlr ccc",
		i3c_ccc_handler, 1, 0),
	SHELL_CMD_ARG(active, NULL, "i3c_cntlr active <device>: select active device",
		i3c_m_active_handler, 1, 1),
	SHELL_CMD_ARG(list, NULL, "i3c_cntlr list: list all flash devices",
		i3c_m_list_handler, 1, 0),
SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(i3c_cntlr, &sub_i3c, "I3C cntlr validation commands", NULL);
