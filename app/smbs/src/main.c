/*
 * Copyright (c) 2017 BayLibre, SAS
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <stdio.h>
#include <zephyr/sys/util.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/target/eeprom.h>

#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <stdlib.h>

#define LOG_LEVEL LOG_LEVEL_INF
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#if DT_NODE_HAS_STATUS(DT_ALIAS(i2c_0), okay)
#define NODE_EP0 DT_NODELABEL(eeprom0)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(i2c_1), okay)
#define NODE_EP0 DT_NODELABEL(eeprom1)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(i2c_2), okay)
#define NODE_EP0 DT_NODELABEL(eeprom2)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(i2c_3), okay)
#define NODE_EP0 DT_NODELABEL(eeprom3)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(i2c_4), okay)
#define NODE_EP0 DT_NODELABEL(eeprom4)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(i2c_5), okay)
#define NODE_EP0 DT_NODELABEL(eeprom5)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(i2c_6), okay)
#define NODE_EP0 DT_NODELABEL(eeprom6)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(i2c_7), okay)
#define NODE_EP0 DT_NODELABEL(eeprom7)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(i2c_8), okay)
#define NODE_EP0 DT_NODELABEL(eeprom8)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(i2c_9), okay)
#define NODE_EP0 DT_NODELABEL(eeprom9)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(i2c_10), okay)
#define NODE_EP0 DT_NODELABEL(eeprom10)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(i2c_11), okay)
#define NODE_EP0 DT_NODELABEL(eeprom11)
#else
#error "Please set the correct SMB device"
#endif

#define TEST_DATA_SIZE	20
static const uint8_t eeprom_0_data[TEST_DATA_SIZE] = "0123456789abcdefghij";

/* Commands used for validation */
enum {
	SMB_CMD0,
	SMB_CMD1,
	SMB_CMD_COUNT,
};

/* Objects used for validation */
static struct {
	int cmd;
	struct k_sem sem_sync;

	/* command args */
	struct {
		uint32_t val;
	} cmd1_args;

	struct {
		uint32_t val;
		uint32_t status;
	} cmd2_args;

} test_objs;


static void smb_thread_entry(void)
{

	const struct device *const eeprom_0 = DEVICE_DT_GET(NODE_EP0);
	const struct device *const i2c_0 = DEVICE_DT_GET(DT_BUS(NODE_EP0));
	int addr_0 = DT_REG_ADDR(NODE_EP0);
	int ret;


	if (!i2c_0) {
		LOG_ERR("EEPROM 0 - I2C bus not found");
		return;
	}

	if (!eeprom_0) {
		LOG_ERR("EEPROM 0 - device not found");
		return;
	}

	if (!device_is_ready(i2c_0)) {
		LOG_ERR("device %s not ready", i2c_0->name);
		return;
}
	LOG_INF("device %s is ready", i2c_0->name);

	LOG_INF("Found EEPROM 0 on I2C bus device %s at addr %02x\n",
		 i2c_0->name, addr_0);


		/* Program differentiable data into the two devices through a back door
		* that doesn't use I2C.
		*/
		ret = eeprom_target_program(eeprom_0, eeprom_0_data, TEST_DATA_SIZE);
	if(ret) {
		LOG_ERR("Failed to program EEPROM 0");
		}

	/* Init semaphore first */
	k_sem_init(&test_objs.sem_sync, 0, 1);

	while (true) {
		/* Task wait event here */
		k_sem_take(&test_objs.sem_sync, K_FOREVER);

		switch (test_objs.cmd) {
		case SMB_CMD0:
			LOG_INF("Handle CMD0");
			break;

		case SMB_CMD1:
			LOG_INF("Handle CMD1 with %d", test_objs.cmd1_args.val);
			if (test_objs.cmd1_args.val == 1) {

		/* Attach each EEPROM to its owning bus as a target device. */
		ret = i2c_target_driver_register(eeprom_0);
				if(ret) {
					LOG_ERR("Failed to register EEPROM 0");
		}
				LOG_INF("[GO]\r\n");
		}

			if (test_objs.cmd1_args.val == 2) {

				/* Detach EEPROM */
				ret = i2c_target_driver_unregister(eeprom_0);
				if(ret) {
					LOG_ERR("Failed to unregister EEPROM 0");
			}
				LOG_INF("[GO]\r\n");
		}
			break;

		default:
			break;
		}
			}
		}


/* Test thread declaration */
#define STACK_SIZE	1024
#define THREAD_PRIORITY 1
K_THREAD_DEFINE(smb_id, STACK_SIZE, smb_thread_entry, NULL, NULL, NULL, THREAD_PRIORITY, 0, -1);

void main(void)
{
	k_thread_name_set(smb_id, "smb_testing");
	k_thread_start(smb_id);
		}


static int smb_command0(const struct shell *shell, size_t argc, char **argv)
{
	test_objs.cmd = SMB_CMD0;

	/* Send event to task and wake it up */
	k_sem_give(&test_objs.sem_sync);

	return 0;
		}

static int smb_command1(const struct shell *shell, size_t argc, char **argv)
{
	test_objs.cmd = SMB_CMD1;

	char *eptr;
	unsigned long val;

	/* Convert integer from string */
	val = strtoul(argv[1], &eptr, 0);
	if (*eptr != '\0') {
		shell_error(shell, "Invalid argument, '%s' is not an integer", argv[1]);
		return -EINVAL;
		}

	if (val >= 100) {
		shell_error(shell, "val %s out of range", argv[1]);
		return -EINVAL;
	}

	test_objs.cmd1_args.val = val;
	/* Send event to task and wake it up */
	k_sem_give(&test_objs.sem_sync);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_smb,
			       SHELL_CMD_ARG(cmd0, NULL, "smb cmd0", smb_command0, 1, 0),
			       SHELL_CMD_ARG(cmd1, NULL, "smb cmd1 <val_1>", smb_command1, 2, 0),
			       SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(smb, &sub_smb, "smb validation commands", NULL);
