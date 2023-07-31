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

/* Target drivers for testing */
#include <zephyr/drivers/i2c.h>

#define LOG_LEVEL LOG_LEVEL_INF
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

/* Get device from the node of device tree */
#if DT_NODE_HAS_STATUS(DT_ALIAS(i2c_0), okay)
#define SMB_DEV_NODE DT_ALIAS(i2c_0)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(i2c_1), okay)
#define SMB_DEV_NODE DT_ALIAS(i2c_1)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(i2c_2), okay)
#define SMB_DEV_NODE	DT_ALIAS(i2c_2)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(i2c_3), okay)
#define SMB_DEV_NODE	DT_ALIAS(i2c_3)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(i2c_4), okay)
#define SMB_DEV_NODE	DT_ALIAS(i2c_4)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(i2c_5), okay)
#define SMB_DEV_NODE	DT_ALIAS(i2c_5)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(i2c_6), okay)
#define SMB_DEV_NODE	DT_ALIAS(i2c_6)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(i2c_7), okay)
#define SMB_DEV_NODE	DT_ALIAS(i2c_7)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(i2c_8), okay)
#define SMB_DEV_NODE	DT_ALIAS(i2c_8)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(i2c_9), okay)
#define SMB_DEV_NODE	DT_ALIAS(i2c_9)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(i2c_10), okay)
#define SMB_DEV_NODE	DT_ALIAS(i2c_10)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(i2c_11), okay)
#define SMB_DEV_NODE	DT_ALIAS(i2c_11)
#else
#error "Please set the correct SMB device"
#endif

/* Use SIM TMP100 for testing */
uint16_t addr = 0x48;

const struct device *const smb_dev = DEVICE_DT_GET(SMB_DEV_NODE);

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


uint32_t smb_cfg = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER;

static void smb_thread_entry(void)
{
	uint32_t smb_cfg_tmp;
	unsigned char datas[10];
	int offset = 0, temp;
	uint16_t raw;
	struct i2c_msg msg[2];

	if (!device_is_ready(smb_dev)) {
		LOG_ERR("smb device %s not ready", smb_dev->name);
		return;
	}

	LOG_INF("smb device %s is ready", smb_dev->name);

	/* 1. Verify i2c_configure() */
	if (i2c_configure(smb_dev, smb_cfg)) {
		LOG_INF("I2C config failed\n");
		return;
	}

	/* 2. Verify i2c_get_config() */
	if (i2c_get_config(smb_dev, &smb_cfg_tmp)) {
		LOG_INF("I2C get_config failed\n");
		return;
	}

	if (smb_cfg != smb_cfg_tmp) {
		LOG_INF("I2C get_config returned invalid config\n");
		return;
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

				/* 2. Verify i2c_write_read() */
				offset = 0x00;
				datas[0] = offset;
				if (i2c_write_read(smb_dev, addr,
							&datas[0], 1,
							&datas[1], 2)) {
					LOG_INF("Fail to test 2\n");
					return;
				}

				raw = (datas[1] << 4) + (datas[2] >> 4);
				temp = raw / 16;
				LOG_INF(" -    SIM TMP100: temp reg %02x: %02x%02x,"
					" temp is %d deg\n",
					datas[0], datas[2], datas[1], temp);
				LOG_INF("[GO]\r\n");
			}

			if (test_objs.cmd1_args.val == 2) {

				offset = 0x01;
				datas[0] = offset;
				if (i2c_write_read(smb_dev, addr,
							&datas[0], 1,
							&datas[1], 1)) {
					LOG_INF("Fail to test 2\n");
					return;
				}

				LOG_INF(" -    SIM TMP100: conf reg %02x: %02x\n",
					datas[0], datas[1]);
				if (datas[1] != 0x80) {
					LOG_INF("Wrong data in '2. Verify i2c_write_read()'\n");
					return;
				}
				LOG_INF(" - 2: verify i2c_write_read passed\n");
				LOG_INF("[GO]\r\n");
			}

			if (test_objs.cmd1_args.val == 3) {

				/* 3. verify i2c_write() */
				offset = 0x01;
				datas[0] = offset;
				datas[1] = 0xe0;
				if (i2c_write(smb_dev, datas, 2, addr)) {
					LOG_INF("Fail to write CONF reg\n");
					return;
				}
				LOG_INF("[GO]\r\n");
			}

			if (test_objs.cmd1_args.val == 4) {

				offset = 0x01;
				datas[0] = offset;
				if (i2c_write_read(smb_dev, addr,
							&datas[0], 1,
							&datas[1], 1)) {
					LOG_INF("Fail to test 3\n");
					return;
				}

				LOG_INF(" -    SIM TMP100: conf reg %02x: %02x\n",
					datas[0], datas[1]);
				if (datas[1] != 0xe0) {
					LOG_INF("Wrong data in '3. Verify i2c_write()'\n");
					return;
				}
				LOG_INF("[GO]\r\n");
			}

			if (test_objs.cmd1_args.val == 5) {

				/* Write data back */
				offset = 0x01;
				datas[0] = offset;
				datas[1] = 0x80;
				if (i2c_write(smb_dev, datas, 2, addr)) {
					LOG_INF("Fail to write CONF reg\n");
					return;
				}
				LOG_INF(" - 3: verify i2c_write passed\n");
				LOG_INF("[GO]\r\n");
			}

			if (test_objs.cmd1_args.val == 6) {

				/* 4. verify i2c_read() */
				/* Back to offset 0x02 */
				offset = 0x01;
				datas[0] = offset;
				if (i2c_write(smb_dev, datas, 1, addr)) {
					LOG_INF("Fail to write offset reg\n");
					return;
				}
				LOG_INF("[GO]\r\n");
			}

			if (test_objs.cmd1_args.val == 7) {

				if (i2c_read(smb_dev, datas + 1, 1, addr)) {
					LOG_INF("Fail to fetch conf data from TMP100\n");
					return;
				}
				LOG_INF(" -    SIM TMP100: conf reg %02x: %02x\n",
					datas[0], datas[1]);
				if (datas[1] != 0x80) {
					LOG_INF("Wrong data in '4. Verify i2c_read()'\n");
					return;
				}
				LOG_INF(" - 4: verify i2c_read passed\n");
				LOG_INF("[GO]\r\n");
			}

			if (test_objs.cmd1_args.val == 8) {

				/* 5. verify i2c_read suspend */
				offset = 0x03;
				datas[0] = offset;
				if (i2c_write_read(smb_dev, addr,
							&datas[0], 1,
							&datas[1], 2)) {
					LOG_INF("Fail to test 5\n");
					return;
				}
				raw = (datas[1] << 4) + (datas[2] >> 4);
				temp = raw / 16;
				LOG_INF(" -    SIM TMP100: T-hi reg %02x: %02x%02x,"
					" t-hi is %d deg\n",
					datas[0], datas[1], datas[2], temp);
				LOG_INF("[GO]\r\n");
			}

			if (test_objs.cmd1_args.val == 9) {

				offset = 0x02;
				datas[0] = offset;
				if (i2c_write_read(smb_dev, addr,
							&datas[0], 1,
							&datas[1], 2)) {
					LOG_INF("Fail to test 5\n");
					return;
				}
				raw = (datas[1] << 4) + (datas[2] >> 4);
				temp = raw / 16;
				LOG_INF(" -    SIM TMP100: T-lo reg %02x: %02x%02x,"
					" t-hi is %d deg\n",
					datas[0], datas[1], datas[2], temp);
				LOG_INF("[GO]\r\n");
			}

			if (test_objs.cmd1_args.val == 10) {

				/* Write T-hi reg for testing */
				offset = 0x03;
				datas[0] = offset;
				datas[1] = 0x51;
				datas[2] = 0x80;
				if (i2c_write(smb_dev, datas, 3, addr)) {
					LOG_INF("Fail to test 5\n");
					return;
				}
				LOG_INF("[GO]\r\n");
			}

			if (test_objs.cmd1_args.val == 11) {

				/* test i2c_read suspend */
				msg[0].buf = &datas[0];
				msg[0].len = 1U;
				msg[0].flags = I2C_MSG_READ;
				msg[1].buf = &datas[1];
				msg[1].len = 1;
				msg[1].flags = I2C_MSG_READ | I2C_MSG_STOP;
				i2c_transfer(smb_dev, msg, 2, addr);
				LOG_INF(" -    T-hi reg : %02x %02x\n", datas[0], datas[1]);
				if (datas[0] != 0x51 && datas[1] != 0x80) {
					LOG_INF("Wrong data in i2c_read suspend\n");
					return;
				}
				LOG_INF(" - 5: verify i2c_read suspend passed\n");
				LOG_INF("[GO]\r\n");
			}

			if (test_objs.cmd1_args.val == 12) {

				/* 6. verify i2c_write suspend */
				/* Write T-hi reg for testing */
				offset = 0x03;
				datas[0] = offset;
				datas[1] = 0x50;
				datas[2] = 0x00;

				/* test i2c_write suspend */
				msg[0].buf = &datas[0];
				msg[0].len = 2U;
				msg[0].flags = I2C_MSG_WRITE;
				msg[1].buf = &datas[2];
				msg[1].len = 1;
				msg[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;
				i2c_transfer(smb_dev, msg, 2, addr);
				LOG_INF("[GO]\r\n");
			}

			if (test_objs.cmd1_args.val == 13) {

				/* Read it back for verification */
				if (i2c_read(smb_dev, datas, 2, addr)) {
					LOG_INF("Fail to fetch T-hi data from TMP100\n");
					return;
				}
				raw = (datas[0] << 4) + (datas[1] >> 4);
				temp = raw / 16;
				LOG_INF(" -    SIM TMP100: T-hi reg: %02x%02x, t-hi is %d deg\n",
					datas[0], datas[1], temp);


				if (datas[0] != 0x50 && datas[1] != 0x00) {
					LOG_INF("Wrong data in i2c_read suspend\n");
					return;
				}

				LOG_INF(" - 6: verify i2c_write suspend passed\n");
				LOG_INF("[GO]\r\n");
			}
			break;
		default:
			break;
		}
	};
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


