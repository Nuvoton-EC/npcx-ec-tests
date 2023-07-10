/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @addtogroup t_i2c_basic
 * @{
 * @defgroup t_i2c_read_write test_i2c_read_write
 * @brief TestPurpose: verify I2C master can read and write
 * @}
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#if DT_NODE_HAS_STATUS(DT_ALIAS(i2c_0), okay)
#define I2C_DEV_NODE	DT_ALIAS(i2c_0)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(i2c_1), okay)
#define I2C_DEV_NODE	DT_ALIAS(i2c_1)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(i2c_2), okay)
#define I2C_DEV_NODE	DT_ALIAS(i2c_2)
#else
#error "Please set the correct I2C device"
#endif

uint32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER;

/* Use TMP100 for testing */
uint16_t addr = 0x48;
uint16_t test_cnt;

const uint8_t reg_data[] = {
	0x16, 0x04, 0x01, 0xc3, 0x01, 0x00, 0x03, 0x01,
	0x12, 0x30, 0x11, 0x20, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x02, 0xff, 0x7f, 0xff, 0xff, 0x01, 0x07,
	0x60, 0x00, 0x4a, 0x00, 0x60, 0x20, 0x08, 0x80,
	0x01, 0x00, 0x00, 0x00, 0xdf, 0x6e, 0xc3, 0x07,
	0x13, 0xd5, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
	0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static int test_gy271(void)
{
	unsigned char datas[10];
	const struct device *const i2c_dev = DEVICE_DT_GET(I2C_DEV_NODE);
	int offset = 0, temp;
	uint16_t raw;
	struct i2c_msg msg[2];

	if (test_cnt % 3 == 0)
		i2c_cfg = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER;
	else if (test_cnt % 3 == 1)
		i2c_cfg = I2C_SPEED_SET(I2C_SPEED_FAST) | I2C_MODE_CONTROLLER;
	else if (test_cnt % 3 == 2)
		i2c_cfg = I2C_SPEED_SET(I2C_SPEED_FAST_PLUS) | I2C_MODE_CONTROLLER;

	TC_PRINT("=====================================\n");
	TC_PRINT("Test %d, configure is 0x%x\n", test_cnt, i2c_cfg);
	test_cnt++;

	if (!device_is_ready(i2c_dev)) {
		TC_PRINT("I2C device is not ready\n");
		return TC_FAIL;
	}

	/* 1. Verify i2c_configure() */
	if (i2c_configure(i2c_dev, i2c_cfg)) {
		TC_PRINT("I2C config failed\n");
		return TC_FAIL;
	} else {
		TC_PRINT(" - 1: verify i2c_configure passed\n");
	}

	/* 2. Verify i2c_write_read() */
	offset = 0x00;
	datas[0] = offset;
	if (i2c_write_read(i2c_dev, addr,
			      &datas[0], 1,
			      &datas[1], 2)) {
		TC_PRINT("Fail to test 2\n");
		return TC_FAIL;
	}

	raw = (datas[1] << 4) + (datas[2] >> 4);
	temp = raw / 16;
	TC_PRINT(" -    TMP100: temp reg %02x: %02x%02x, temp is %d deg\n",
							datas[0], datas[2], datas[1], temp);
	offset = 0x01;
	datas[0] = offset;
	if (i2c_write_read(i2c_dev, addr,
			      &datas[0], 1,
			      &datas[1], 1)) {
		TC_PRINT("Fail to test 2\n");
		return TC_FAIL;
	}

	TC_PRINT(" -    TMP100: conf reg %02x: %02x\n", datas[0], datas[1]);
	if (datas[1] != 0x80) {
		TC_PRINT("Wrong data in '2. Verify i2c_write_read()'\n");
		return TC_FAIL;
	} else {
		TC_PRINT(" - 2: verify i2c_write_read passed\n");
	}

	/* 3. verify i2c_write() */
	offset = 0x01;
	datas[0] = offset;
	datas[1] = 0xe0;
	if (i2c_write(i2c_dev, datas, 2, addr)) {
		TC_PRINT("Fail to write CONF reg\n");
		return TC_FAIL;
	}

	offset = 0x01;
	datas[0] = offset;
	if (i2c_write_read(i2c_dev, addr,
			      &datas[0], 1,
			      &datas[1], 1)) {
		TC_PRINT("Fail to test 3\n");
		return TC_FAIL;
	}

	TC_PRINT(" -    TMP100: conf reg %02x: %02x\n", datas[0], datas[1]);
	if (datas[1] != 0xe0) {
		TC_PRINT("Wrong data in '3. Verify i2c_write()'\n");
		return TC_FAIL;
	}

	/* Write data back */
	offset = 0x01;
	datas[0] = offset;
	datas[1] = 0x80;
	if (i2c_write(i2c_dev, datas, 2, addr)) {
		TC_PRINT("Fail to write CONF reg\n");
		return TC_FAIL;
	} else {
		TC_PRINT(" - 3: verify i2c_write passed\n");
	}

	/* 4. verify i2c_read() */
	/* Back to offset 0x02 */
	offset = 0x01;
	datas[0] = offset;
	if (i2c_write(i2c_dev, datas, 1, addr)) {
		TC_PRINT("Fail to write offset reg\n");
		return TC_FAIL;
	}

	if (i2c_read(i2c_dev, datas + 1, 1, addr)) {
		TC_PRINT("Fail to fetch conf data from TMP100\n");
		return TC_FAIL;
	}
	TC_PRINT(" -    TMP100: conf reg %02x: %02x\n", datas[0], datas[1]);
	if (datas[1] != 0x80) {
		TC_PRINT("Wrong data in '4. Verify i2c_read()'\n");
		return TC_FAIL;
	} else {
		TC_PRINT(" - 4: verify i2c_read passed\n");
	}

	/* 5. verify i2c_read suspend */
	offset = 0x03;
	datas[0] = offset;
	if (i2c_write_read(i2c_dev, addr,
			      &datas[0], 1,
			      &datas[1], 2)) {
		TC_PRINT("Fail to test 5\n");
		return TC_FAIL;
	}
	raw = (datas[1] << 4) + (datas[2] >> 4);
	temp = raw / 16;
	TC_PRINT(" -    TMP100: T-hi reg %02x: %02x%02x, t-hi is %d deg\n",
						datas[0], datas[1], datas[2], temp);

	offset = 0x02;
	datas[0] = offset;
	if (i2c_write_read(i2c_dev, addr,
			      &datas[0], 1,
			      &datas[1], 2)) {
		TC_PRINT("Fail to test 5\n");
		return TC_FAIL;
	}
	raw = (datas[1] << 4) + (datas[2] >> 4);
	temp = raw / 16;
	TC_PRINT(" -    TMP100: T-lo reg %02x: %02x%02x, t-hi is %d deg\n",
						datas[0], datas[1], datas[2], temp);

	/* Write T-hi reg for testing */
	offset = 0x03;
	datas[0] = offset;
	datas[1] = 0x51;
	datas[2] = 0x80;
	if (i2c_write(i2c_dev, datas, 3, addr)) {
		TC_PRINT("Fail to test 5\n");
		return TC_FAIL;
	}

	/* test i2c_read suspend */
	msg[0].buf = &datas[0];
	msg[0].len = 1U;
	msg[0].flags = I2C_MSG_READ;
	msg[1].buf = &datas[1];
	msg[1].len = 1;
	msg[1].flags = I2C_MSG_READ | I2C_MSG_STOP;
	i2c_transfer(i2c_dev, msg, 2, addr);
	TC_PRINT(" -    T-hi reg : %02x %02x\n", datas[0], datas[1]);
	if (datas[0] != 0x51 && datas[1] != 0x80) {
		TC_PRINT("Wrong data in i2c_read suspend\n");
		return TC_FAIL;
	} else {
		TC_PRINT(" - 5: verify i2c_read suspend passed\n");
	}

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
	i2c_transfer(i2c_dev, msg, 2, addr);

	/* Read it back for verification */
	if (i2c_read(i2c_dev, datas, 2, addr)) {
		TC_PRINT("Fail to fetch T-hi data from TMP100\n");
		return TC_FAIL;
	}
	raw = (datas[0] << 4) + (datas[1] >> 4);
	temp = raw / 16;
	TC_PRINT(" -    TMP100: T-hi reg: %02x%02x, t-hi is %d deg\n", datas[0], datas[1], temp);


	if (datas[0] != 0x50 && datas[1] != 0x00) {
		TC_PRINT("Wrong data in i2c_read suspend\n");
		return TC_FAIL;
	}

	TC_PRINT(" - 6: verify i2c_write suspend passed\n");

	return TC_PASS;
}

ZTEST(i2c_gy271, test_i2c_gy271)
{
	while (test_cnt < 100) {
		zassert_true(test_gy271() == TC_PASS);
		k_msleep(300);
	};
}

ZTEST_SUITE(i2c_gy271, NULL, NULL, NULL, NULL, NULL);
