/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#ifdef CONFIG_FLASH_EX_OP_ENABLED
#include <zephyr/drivers/flash/npcx_flash_api_ex.h>
#endif /* CONFIG_FLASH_EX_OP_ENABLED */
#include <jesd216.h>
#include "spi_nand.h"
#include <zephyr/kernel.h>
#include <zephyr/pm/policy.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <stdlib.h>
#include <stdio.h>

/* Target drivers for testing */
#include <zephyr/drivers/flash.h>

#define LOG_LEVEL LOG_LEVEL_INF
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash);

#define FLASH_DEV0 DT_ALIAS(spi_flash0)
#define FLASH_DEV1 DT_ALIAS(spi_flash1)
#define FLASH_DEV2 DT_ALIAS(spi_flash2)
/* Get device from device tree */
static const struct device *const flash_devices[] = {
#if DT_NODE_HAS_STATUS(FLASH_DEV0, okay)
	DEVICE_DT_GET(FLASH_DEV0),
#endif
#if DT_NODE_HAS_STATUS(FLASH_DEV1, okay)
	DEVICE_DT_GET(FLASH_DEV1),
#endif
#if DT_NODE_HAS_STATUS(FLASH_DEV2, okay)
	DEVICE_DT_GET(FLASH_DEV2),
#endif
};

#define shell_printk(...) \
	shell_print(sh_ptr, ##__VA_ARGS__)

#define NUM_FLASH_DEVICE ARRAY_SIZE(flash_devices)
uint8_t flash_dev_idx_sel;

#define CAL_DATA_FROM_ADDR(n) ((uint8_t)(((n) * 3) + 2) + (n >> 11))

/* Data buffer */
#define TEMP_DATA_BUF_SIZE 2048
static uint8_t temp_data_buf[TEMP_DATA_BUF_SIZE];

/* Commands used for validation */
enum {
	NAND_FLASH_CMD_READ_ID,
	NAND_FLASH_CMD_ERASE,
	NAND_FLASH_CMD_READ,
	NAND_FLASH_CMD_WRITE,
	NAND_FLASH_CMD_READ_STS_REG,
	NAND_FLASH_CMD_WRITE_STS_REG,
	NAND_FLASH_CMD_WP_EN,
	NAND_FLASH_CMD_IS_WP_EN,
	NAND_FLASH_CMD_GET_BAD_BLOCK_LUT,
	NAND_FLASH_CMD_COUNT,
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

	struct {
		uint32_t read_addr;
		uint32_t read_size;
	} cmd_flash_read_args;

	struct {
		uint32_t write_addr;
		uint32_t write_size;
	} cmd_flash_write_args;

	struct {
		uint8_t status[2];
	} cmd_flash_wr_status_args;
} nand_flash_test_objs;


uint8_t sts_reg_addr[FLASH_STS_REG_MAX] = {NAND_STS_REG1_ADDR,
									NAND_STS_REG2_ADDR,
									NAND_STS_REG3_ADDR};

struct nand_flash_lut nand_lut;
const struct shell *sh_ptr;

/* Test Function Declarations */
static int nand_flash_read_id(const struct device *flash_dev)
{
	uint8_t id[3];
	int rc;

	rc = flash_read_jedec_id(flash_dev, id);
	if (rc == 0) {
		shell_info(sh_ptr, "JEDEC ID = %#02x %#02x %#02x",
		       id[0], id[1], id[2]);
	} else {
		shell_info(sh_ptr, "JEDEC ID read failed: %d\n", rc);
	}

	return 0;
}

static int nand_flash_erase(const struct device *flash_dev, off_t addr, size_t size)
{
	int rc;
	uint32_t i;
	size_t temp_size, read_size;
	off_t read_addr;

	if (size > SPI_NAND_BLOCK_NUM) {
		LOG_ERR("erase block count exceed %d", SPI_NAND_BLOCK_NUM);
		return -ENODEV;
	}

	size = size * SPI_NAND_BLOCK_SIZE;

	/* Block erase */
	rc = flash_erase(flash_dev, addr, size);
	if (rc != 0) {
		LOG_ERR("flash_erase() failed: %d", rc);
		return -ENODEV;
	}

	/* Read back check */
	temp_size = size;
	read_addr = addr;
	while (temp_size) {
		read_size = (temp_size >= TEMP_DATA_BUF_SIZE) ? TEMP_DATA_BUF_SIZE : temp_size;

		rc = flash_read(flash_dev, read_addr, temp_data_buf, read_size);
		if (rc != 0) {
			LOG_ERR("flash_read() failed: %d", rc);
			return -ENODEV;
		}

		for (i = 0; i < read_size; i++) {
			if (temp_data_buf[i] != 0xFF) {
				LOG_ERR("flash_erase() check failed. Addr: 0x%lx, Data: 0x%x",
					(read_addr + i), temp_data_buf[i]);
				return -ENODEV;
			}
		}

		read_addr += read_size;
		temp_size -= read_size;
	}


	shell_info(sh_ptr, "[PASS] Flash erase succeeded!");
	shell_info(sh_ptr, "[GO]");

	return 0;
}

static int nand_flash_read(const struct device *flash_dev, off_t addr, size_t size)
{
	int rc;
	uint8_t golden8;
	size_t temp_size, read_size;
	off_t read_addr;

	temp_size = size;
	read_addr = addr;
	shell_info(sh_ptr, "Flash Address: 0x%lx, size: 0x%x", addr, size);

	while (temp_size) {
		read_size = (temp_size >= TEMP_DATA_BUF_SIZE) ? TEMP_DATA_BUF_SIZE : temp_size;

		memset(temp_data_buf, 0, sizeof(temp_data_buf));
		rc = flash_read(flash_dev, read_addr, temp_data_buf, read_size);
		if (rc != 0) {
			LOG_ERR("flash_read() failed: %d", rc);
			return -ENODEV;
		}

		/* Verify golden */
		for (int i = 0; i < read_size; i++) {
			golden8 = CAL_DATA_FROM_ADDR((read_addr + i));

			if (temp_data_buf[i] != golden8) {
				shell_info(sh_ptr, "[FAIL] addr=%ld(%#lx), data=%#x, golden: %#x",
						(read_addr + i), (read_addr + i),
						temp_data_buf[i], golden8);
				return -ENODEV;
			}
		}

		read_addr += read_size;
		temp_size -= read_size;
	}

	shell_info(sh_ptr, "[PASS] Flash read succeeded!");
	shell_info(sh_ptr, "[GO]");
	return 0;
}

static int nand_flash_write(const struct device *flash_dev, off_t addr, size_t size)
{
	int ret;
	uint32_t tmp_size;

	/* check address and size sector (512 byte) alignment */
	if ((addr & 0x1ff) != 0 || (size & 0x1ff) != 0) {
		LOG_ERR("flash addr or size not 512 byte alignment");
		return -ENODEV;
	}

	while (size > 0) {
		if (size >= sizeof(temp_data_buf))
			tmp_size = sizeof(temp_data_buf);
		else
			tmp_size = size;

		/* Write golden */
		memset(temp_data_buf, 0, sizeof(temp_data_buf));
		for (int i = 0 ; i < tmp_size ; i++) {
			temp_data_buf[i]  = CAL_DATA_FROM_ADDR((addr + i));
		}

		/* Program data into flash */
		ret = flash_write(flash_dev, addr, temp_data_buf, tmp_size);
		if (ret != 0) {
			LOG_ERR("nand flash write failed: %d", ret);
			return -ENODEV;
		}

		size -= tmp_size;
		addr += tmp_size;
	}

	shell_info(sh_ptr, "Flash write succeeded!");
	shell_info(sh_ptr, "[GO]");
	return 0;
}

static int nand_flash_rdst(const struct device *flash_dev, uint8_t *dest)
{
	struct npcx_ex_ops_uma_in op_in = {
		.opcode = SPI_NAND_CMD_RDSR,
		.tx_buf = sts_reg_addr,
		.tx_count = 1,
		.rx_count = 1,
	};

	struct npcx_ex_ops_uma_out op_out = {
		.rx_buf = dest,
	};

	/* Read status 1 reg */
	flash_ex_op(flash_devices[flash_dev_idx_sel],
					FLASH_NPCX_EX_OP_EXEC_UMA,
					(uintptr_t)&op_in, &op_out);

	/* Read status 2 reg */
	op_in.tx_buf = sts_reg_addr + 1;
	op_out.rx_buf = dest + 1;

	flash_ex_op(flash_devices[flash_dev_idx_sel],
					FLASH_NPCX_EX_OP_EXEC_UMA,
					(uintptr_t)&op_in, &op_out);

	/* Read status 3 reg */
	op_in.tx_buf = sts_reg_addr + 2;
	op_out.rx_buf = dest + 2;
	flash_ex_op(flash_devices[flash_dev_idx_sel],
					FLASH_NPCX_EX_OP_EXEC_UMA,
					(uintptr_t)&op_in, &op_out);
	return 0;
}

static int nand_flash_wrst(const struct device *flash_dev,
				uint8_t reg1, uint8_t reg2)
{
	uint8_t tx_val[2];
	struct npcx_ex_ops_uma_in op_in;

	/* Write status_1 regs */
	tx_val[0] = sts_reg_addr[0]; /* pack addr */
	tx_val[1] = reg1;			 /* pack data */
	op_in.opcode = SPI_NAND_CMD_WRSR;
	op_in.tx_buf = tx_val;
	op_in.tx_count = 2;
	flash_ex_op(flash_dev,
				FLASH_NPCX_EX_OP_EXEC_UMA,
				(uintptr_t)&op_in, NULL);

	/* Write status_2 regs */
	tx_val[0] = sts_reg_addr[1];
	tx_val[1] = reg2;
	op_in.opcode = SPI_NAND_CMD_WRSR;
	op_in.tx_buf = tx_val;
	op_in.tx_count = 2;
	flash_ex_op(flash_dev,
				FLASH_NPCX_EX_OP_EXEC_UMA,
				(uintptr_t)&op_in, NULL);
	return 0;
}

static int nand_flash_get_lut(const struct device *flash_dev,
				struct nand_flash_lut *lut_ptr)
{
	return flash_ex_op(flash_dev, FLASH_NPCX_EX_OP_NAND_GET_BAD_BLOCK_LUT,
				(uintptr_t)NULL, lut_ptr);
}

static void nand_flash_thread_entry(void)
{
	for (int i = 0; i < NUM_FLASH_DEVICE; i++) {
		if (!device_is_ready(flash_devices[i])) {
			LOG_ERR("flash device %s is not ready", flash_devices[i]->name);
			return;
		}

		LOG_INF("flash device [%d]:%s is ready", i, flash_devices[i]->name);
	}

	/* Init semaphore first */
	k_sem_init(&nand_flash_test_objs.sem_sync, 0, 1);

	while (true) {
		/* Task wait event here */
		k_sem_take(&nand_flash_test_objs.sem_sync, K_FOREVER);

		switch (nand_flash_test_objs.cmd) {
		case NAND_FLASH_CMD_READ_ID:
			shell_info(sh_ptr, "Read NAND FLASH ID");
			nand_flash_read_id(flash_devices[flash_dev_idx_sel]);
			break;
		case NAND_FLASH_CMD_ERASE:
			shell_info(sh_ptr, "Erase NAND FLASH");
			nand_flash_erase(flash_devices[flash_dev_idx_sel],
					nand_flash_test_objs.cmd_flash_erase_args.erase_addr,
					nand_flash_test_objs.cmd_flash_erase_args.erase_size);
			break;
		case NAND_FLASH_CMD_READ:
			shell_info(sh_ptr, "Read NAND FLASH");
			nand_flash_read(flash_devices[flash_dev_idx_sel],
				       nand_flash_test_objs.cmd_flash_read_args.read_addr,
				       nand_flash_test_objs.cmd_flash_read_args.read_size);
			break;
		case NAND_FLASH_CMD_WRITE:
			shell_info(sh_ptr, "Write NAND FLASH");
			nand_flash_write(flash_devices[flash_dev_idx_sel],
					nand_flash_test_objs.cmd_flash_write_args.write_addr,
					nand_flash_test_objs.cmd_flash_write_args.write_size);
			break;
#ifdef CONFIG_FLASH_EX_OP_ENABLED
		case NAND_FLASH_CMD_READ_STS_REG:
		{
			uint8_t reg[3];

			nand_flash_rdst(flash_devices[flash_dev_idx_sel], reg);
			shell_info(sh_ptr, "READ NAND FLASH STATUS 0~3 (%#02x, %#02x, %#02x)",
					reg[0], reg[1], reg[2]);
			break;
		}
		case NAND_FLASH_CMD_WRITE_STS_REG:
		{
			uint8_t reg[2];

			reg[0] = nand_flash_test_objs.cmd_flash_wr_status_args.status[0];
			reg[1] = nand_flash_test_objs.cmd_flash_wr_status_args.status[1];
			shell_info(sh_ptr, "WRITE NAND FLASH STATUS 0 and 1 (%#02x, %#02x)", reg[0], reg[1]);
			nand_flash_wrst(flash_devices[flash_dev_idx_sel], reg[0], reg[1]);
			break;
		}
		case NAND_FLASH_CMD_GET_BAD_BLOCK_LUT:
		{
			struct nand_flash_lut nand_lut;

			shell_info(sh_ptr, "Get bad block lut");
			nand_flash_get_lut(flash_devices[flash_dev_idx_sel],
								&nand_lut);

			/* Show result */
			shell_info(sh_ptr, "Is lut init: %s",
					(nand_lut.is_inited == 1) ? "YES" : "NO");
			if (nand_lut.bbt_count <= 0x0) {
				shell_info(sh_ptr, "All blocks are good !!");
			} else {
				shell_info(sh_ptr, "Bad block count: %d", nand_lut.bbt_count);
				/* Show bad block index (0~1023) */
				for (int i = 0; i < nand_lut.bbt_count; i++)
					shell_info(sh_ptr, "Bad block index: %d",
							nand_lut.bbt_list[i]);
			}

			break;
		}
#endif /* CONFIG_FLASH_EX_OP_ENABLED */
		default:
			break;
		}
	};
}

/* Test thread declaration */
#define STACK_SIZE      1024
#define THREAD_PRIORITY 1
K_THREAD_DEFINE(nand_flash_thread_id, STACK_SIZE, nand_flash_thread_entry, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, -1);

void main(void)
{

	/* Zephyr driver validation main */
	LOG_INF("Start CI20 Validation Task");

	/* Init thread */
	k_thread_name_set(nand_flash_thread_id, "nand_flash_testing");
	k_thread_start(nand_flash_thread_id);
}

static int nand_flash_read_id_handler(const struct shell *shell, size_t argc, char **argv)
{
	sh_ptr = shell;

	nand_flash_test_objs.cmd = NAND_FLASH_CMD_READ_ID;

	/* Send event to task and wake it up */
	k_sem_give(&nand_flash_test_objs.sem_sync);

	return 0;
}

static int nand_flash_erase_handler(const struct shell *shell, size_t argc, char **argv)
{
	char *eptr;
	uint32_t addr;
	uint32_t size;

	sh_ptr = shell;

	nand_flash_test_objs.cmd = NAND_FLASH_CMD_ERASE;

	/* Convert integer from string */
	addr = strtoul(argv[1], &eptr, 0);
	if (*eptr != '\0') {
		shell_error(shell, "Invalid argument, '%s' is not an integer", argv[1]);
		return -EINVAL;
	}

	size = strtoul(argv[2], &eptr, 0);
	if (*eptr != '\0') {
		shell_error(shell, "Invalid argument, '%s' is not an integer", argv[2]);
		return -EINVAL;
	}

	nand_flash_test_objs.cmd_flash_erase_args.erase_addr = addr;
	nand_flash_test_objs.cmd_flash_erase_args.erase_size = size;

	/* Send event to task and wake it up */
	k_sem_give(&nand_flash_test_objs.sem_sync);

	return 0;
}

static int nand_flash_read_handler(const struct shell *shell, size_t argc, char **argv)
{
	char *eptr;
	uint32_t addr;
	uint32_t size;

	sh_ptr = shell;

	nand_flash_test_objs.cmd = NAND_FLASH_CMD_READ;

	/* Convert integer from string */
	addr = strtoul(argv[1], &eptr, 0);
	if (*eptr != '\0') {
		shell_error(shell, "Invalid argument, '%s' is not an integer", argv[1]);
		return -EINVAL;
	}

	size = strtoul(argv[2], &eptr, 0);
	if (*eptr != '\0') {
		shell_error(shell, "Invalid argument, '%s' is not an integer", argv[2]);
		return -EINVAL;
	}

	nand_flash_test_objs.cmd_flash_read_args.read_addr = addr;
	nand_flash_test_objs.cmd_flash_read_args.read_size = size;

	/* Send event to task and wake it up */
	k_sem_give(&nand_flash_test_objs.sem_sync);

	return 0;
}

static int nand_flash_write_handler(const struct shell *shell, size_t argc, char **argv)
{
	char *eptr;
	uint32_t addr;
	uint32_t size;

	sh_ptr = shell;

	nand_flash_test_objs.cmd = NAND_FLASH_CMD_WRITE;

	/* Convert integer from string */
	addr = strtoul(argv[1], &eptr, 0);
	if (*eptr != '\0') {
		shell_error(shell, "Invalid argument, '%s' is not an integer", argv[1]);
		return -EINVAL;
	}

	size = strtoul(argv[2], &eptr, 0);
	if (*eptr != '\0') {
		shell_error(shell, "Invalid argument, '%s' is not an integer", argv[2]);
		return -EINVAL;
	}

	nand_flash_test_objs.cmd_flash_write_args.write_addr = addr;
	nand_flash_test_objs.cmd_flash_write_args.write_size = size;

	/* Send event to task and wake it up */
	k_sem_give(&nand_flash_test_objs.sem_sync);

	return 0;
}

#ifdef CONFIG_FLASH_EX_OP_ENABLED
static int nand_flash_read_sts_reg_handler(const struct shell *shell, size_t argc, char **argv)
{
	nand_flash_test_objs.cmd = NAND_FLASH_CMD_READ_STS_REG;

	sh_ptr = shell;

	/* Send event to task and wake it up */
	k_sem_give(&nand_flash_test_objs.sem_sync);

	return 0;
}

static int nand_flash_write_sts_reg_handler(const struct shell *shell, size_t argc, char **argv)
{
	char *eptr;
	uint32_t sts1, sts2;

	sh_ptr = shell;

	nand_flash_test_objs.cmd = NAND_FLASH_CMD_WRITE_STS_REG;
	/* Convert integer from string */
	sts1 = strtoul(argv[1], &eptr, 0);
	if (*eptr != '\0') {
		shell_error(shell, "Invalid argument, '%s' is not an integer", argv[1]);
		return -EINVAL;
	}

	sts2 = strtoul(argv[2], &eptr, 0);
	if (*eptr != '\0') {
		shell_error(shell, "Invalid argument, '%s' is not an integer", argv[2]);
		return -EINVAL;
	}

	nand_flash_test_objs.cmd_flash_wr_status_args.status[0] = (sts1 & 0xff);
	nand_flash_test_objs.cmd_flash_wr_status_args.status[1] = (sts2 & 0xff);

	/* Send event to task and wake it up */
	k_sem_give(&nand_flash_test_objs.sem_sync);

	return 0;
}

static int nand_flash_rdlut_handler(const struct shell *shell, size_t argc, char **argv)
{
	nand_flash_test_objs.cmd = NAND_FLASH_CMD_GET_BAD_BLOCK_LUT;

	sh_ptr = shell;

	/* Send event to task and wake it up */
	k_sem_give(&nand_flash_test_objs.sem_sync);

	return 0;
}
#endif /* CONFIG_FLASH_EX_OP_ENABLED */

/* Multi SPI NAND flash devices */
static int nand_flash_active_handler(const struct shell *shell, size_t argc, char **argv)
{
	char *eptr;
	uint32_t active_dev;

	sh_ptr = shell;

	if (argc == 1) {
		shell_info(sh_ptr, "Active flash device Index = %d", flash_dev_idx_sel);
		return 0;
	}

	active_dev = strtoul(argv[1], &eptr, 0);
	if (*eptr != '\0') {
		shell_error(shell, "Invalid argument, '%s' is not an integer", argv[1]);
		return -EINVAL;
	}

	if (active_dev < NUM_FLASH_DEVICE) {
		flash_dev_idx_sel = active_dev;
	} else {
		flash_dev_idx_sel = 0;
	}

	shell_info(sh_ptr, "Select active flash device to [%d]%s", flash_dev_idx_sel,
					flash_devices[flash_dev_idx_sel]->name);
	return 0;
}

static int nand_flash_list_handler(const struct shell *shell, size_t argc, char **argv)
{
	sh_ptr = shell;

	for (int i = 0; i < NUM_FLASH_DEVICE; i++) {
		shell_info(sh_ptr, "flash device [%d]:%s", i, flash_devices[i]->name);
	}
	shell_info(sh_ptr, "Current active index = %d", flash_dev_idx_sel);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_nand_flash,
	SHELL_CMD_ARG(id, NULL, "nand_flash id: read flash id",
		nand_flash_read_id_handler, 1, 0),
	SHELL_CMD_ARG(erase, NULL, "nand_flash erase <addr> <block>: erase flash",
		nand_flash_erase_handler, 3, 0),
	SHELL_CMD_ARG(read, NULL, "nand_flash read <addr> <size>: read flash",
		nand_flash_read_handler, 3, 0),
	SHELL_CMD_ARG(write, NULL, "nand_flash write <addr> <size>: write flash",
		nand_flash_write_handler, 3, 0),
#ifdef CONFIG_FLASH_EX_OP_ENABLED
	SHELL_CMD_ARG(rdst, NULL, "nand_flash rdst: read flash status registers",
		nand_flash_read_sts_reg_handler, 1, 0),
	SHELL_CMD_ARG(wrst, NULL, "nand_flash wrst <sts1> <sts2>: write flash status registers",
		nand_flash_write_sts_reg_handler, 3, 0),
	SHELL_CMD_ARG(rdlut, NULL, "nand_flash rdlut: read lookup table",
		nand_flash_rdlut_handler, 1, 0),
#endif /* CONFIG_FLASH_EX_OP_ENABLED */
	SHELL_CMD_ARG(active, NULL, "nand_flash active <device>: select active device",
		nand_flash_active_handler, 1, 1),
	SHELL_CMD_ARG(list, NULL, "nand_flash list: list all flash devices",
		nand_flash_list_handler, 1, 0),
SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(nand_flash, &sub_nand_flash, "NAND Flash validation commands", NULL);
