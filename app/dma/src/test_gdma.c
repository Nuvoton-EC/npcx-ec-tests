/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/random/rand32.h>
#include "../../zephyr/drivers/dma/dma_npcx.h"

#define LOG_LEVEL LOG_LEVEL_INF
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gdma);

#define MAX_ARGUMNETS	3
#define MAX_ARGU_SIZE	10
#define TRANSFER_SIZE	1024

#define TASK_STACK_SIZE	1024
#define PRIORITY	7

#define DMA0_CTLER			DT_NODELABEL(dma0)
#define DMA1_CTLER			DT_NODELABEL(dma1)

static uint8_t arguments[MAX_ARGUMNETS][MAX_ARGU_SIZE];
struct k_event dma_event;
static struct k_thread temp_id;

K_THREAD_STACK_DEFINE(temp_stack, TASK_STACK_SIZE);


/* Get device from device tree */
static const struct device *const dma_devices[] = {
	DEVICE_DT_GET(DMA0_CTLER),
	DEVICE_DT_GET(DMA1_CTLER),
};

#define NUM_DMA_DEVICE ARRAY_SIZE(dma_devices)
#define cal_flash_type_num(ADDR)	ARRAY_SIZE(ADDR)
#define MRAM(inst)	((const struct dma_npcx_config *)dma_devices[inst]->config)->buttom_mram
#define INT_FLASH_BASE1_ADDR	0x60000000	/* private flash */
#define FLASH_BASE1_ADDR		0x68000000	/* shared flash */


/* data transfer config */
static const uint8_t align_val = 2;
static struct dma_config dma_cfg = { 0 };
static struct dma_block_config dma_block_cfg = { 0 };
static struct dma_config dma_cfg1 = { 0 };
static struct dma_block_config dma_block_cfg1 = { 0 };

/* mem to mem config */
static uint32_t GDMAMemPool[MOVE_SIZE + 4] __aligned(16);

/* flash to mem config */
static uint32_t gdma_test_flash_src[] = {
	INT_FLASH_BASE1_ADDR,	/* internal flash */
	FLASH_BASE1_ADDR,		/* external flash */
};

static uint32_t gdma_data_check(const struct device *dev, const uint8_t channel)
{
	struct dma_reg *const inst = HAL_INSTANCE(dev, channel);

	uint8_t tws = GET_FIELD(inst->CONTROL, NPCX_DMACTL_TWS);
	uint8_t bme = GET_BIT(inst->CONTROL, NPCX_DMACTL_BME);
	uint8_t dadir = GET_BIT(inst->CONTROL, NPCX_DMACTL_DADIR);
	uint8_t sadir = GET_BIT(inst->CONTROL, NPCX_DMACTL_SADIR);
	uint32_t tcnt = inst->TCNT;
	uint32_t src_ = inst->SRCB;
	uint32_t dst_ = inst->DSTB;
	uint32_t ctcnt = inst->CTCNT;

	volatile uint8_t *dst_B, *src_B;
	volatile uint16_t *dst_W, *src_W;
	volatile uint32_t *dst_DW, *src_DW;

	dst_B = (uint8_t *) dst_;
	src_B = (uint8_t *) src_;
	dst_W = (uint16_t *) dst_;
	src_W = (uint16_t *) src_;
	dst_DW = (uint32_t *) dst_;
	src_DW = (uint32_t *) src_;

	if (IS_BIT_SET(inst->CONTROL, NPCX_DMACTL_TC)) {
		LOG_INF("[FAIL][GDMA] ch%d GDMA TC couldn't clear\r\n", channel);
	}
	if (IS_BIT_SET(inst->CONTROL, NPCX_DMACTL_GDMAEN)) {
		LOG_INF("[FAIL][GDMA] ch%d GDMA EN is Enable\r\n", channel);
	}
	if (ctcnt) {
		LOG_INF("[FAIL][GDMA] ch%d CTCNT not completed \r\n", channel);
	}

	if (bme) {
		tcnt <<= 2;
	}

	switch (tws) {
	case 0:
		LOG_INF("Byte mode | ");
		LOG_INF("src | %p: %hhx dst | %p: %hhx \r\n",
			(void *)src_B, *src_B, (void *)dst_B, *dst_B);
		do {
			if (*dst_B != *src_B) {
				break;
			}
			if (dadir)
				dst_B -= 1;
			else
				dst_B += 1;
			if (sadir)
				src_B -= 1;
			else
				src_B += 1;
			tcnt--;
		} while (tcnt != 0);
		break;
	case 1:
		LOG_INF("Word mode | ");
		LOG_INF("src | %p: %hx dst | %p: %hx \r\n",
			(void *)src_W, *src_W, (void *)dst_W, *dst_W);
		do {
			if (*dst_W != *src_W) {
				break;
			}
			if (dadir)
				dst_W -= 1;
			else
				dst_W += 1;
			if (sadir)
				src_W -= 1;
			else
				src_W += 1;
			tcnt--;
		} while (tcnt != 0);
		break;
	case 2:
	case 3:
		if (bme) {
			LOG_INF("Double Word mode | Burst Mode");
			LOG_INF("src %p: %x | dst %p: %x \r\n",
				(void *)src_DW, *src_DW, (void *)dst_DW, *dst_DW);
			do {
				if (*dst_DW != *src_DW) {
					break;
				}
				if (dadir)
					dst_DW -= 1;
				else
					dst_DW += 1;
				if (sadir)
					src_DW -= 1;
				else
					src_DW += 1;
				tcnt--;
			} while (tcnt != 0);
		} else {
			LOG_INF("Double Word mode | Normal Mode");
			LOG_INF("src %p: %x | dst %p: %x \r\n",
				(void *)src_DW, *src_DW, (void *)dst_DW, *dst_DW);
			do {
				if (*dst_DW != *src_DW) {
					break;
				}
				if (dadir)
					dst_DW -= 1;
				else
					dst_DW += 1;
				if (sadir)
					src_DW -= 1;
				else
					src_DW += 1;
				tcnt--;
			} while (tcnt != 0);
		}
		break;
	}

	return tcnt;
}
static void check_char_data(const char *tx_data, const char *rx_data, bool enable)
{
	if (strcmp(tx_data, rx_data) != 0) {
		LOG_INF("[FAIL] Data transfer");
	} else {
		LOG_INF("[PASS] Data transfer");
	}
	if (enable) {
		LOG_INF("rx_data : %s", rx_data);
	}
}

/* isr event */

static void cb_data_check(const struct device *dma_dev, void *arg,
				uint32_t channel, int status)
{
	if (status < 0) {
		LOG_INF("DMA could not proceed, an error occurred\n");
	}
	LOG_INF("channel : %d", channel);

	if (gdma_data_check(dma_dev, channel)) {
		LOG_INF("[FAIL] Data check");
	} else {
		LOG_INF("[PASS] Data check");
		LOG_INF("Data transfer completed");
	}
}

/*
 * Finished Function
 */

int *npcx_power_down_gpd(const struct device *dev, const uint32_t channel,
					uint32_t val0, uint32_t val1)
{
	static int arr[2] = {0};
	const uint32_t dma_base = get_dev_base(dev);
	uint32_t ch0 = channel, ch1 = channel ^ 1;

	/* power down one channel */
	dma_set_power_down(dev, ch0, ENABLE); /* can't write ch0 */
	DMA_SRCB(dma_base, ch0) = val0;
	DMA_SRCB(dma_base, ch1) = val1;

	arr[0] = ((DMA_SRCB(dma_base, ch0)) == val0) ? 1 : 0;
	arr[1] = ((DMA_SRCB(dma_base, ch1)) == val1) ? 1 : 0;

	/* power up channel */
	dma_set_power_down(dev, ch0, DISABLE);

	return arr;
}

static void npcx_power_down_2(void)
{
	uint32_t val1 = 0x11223344, val2 = 0x9ABCDEF0;

	for (int i = 0; i < NUM_DMA_DEVICE; i++) {
		for (uint8_t ch = 0; ch < MAX_DMA_CHANNELS; ch++) {
			int *res = npcx_power_down_gpd(dma_devices[i], ch, val1, val2);

			if (res[0]) {
				LOG_INF("[FAIL][GDMA%d]: GPD%d power down", i, ch);
			} else {
				LOG_INF("[PASS][GDMA%d]: GPD%d power down", i, ch);
			}
			if (res[1]) {
				LOG_INF("[PASS][GDMA%d]: GPD%d still run when GDP%d power down",
					i, ch ^ 1, ch);
			} else {
				LOG_INF("[FAIL][GDMA%d]: GPD%d still run when GDP%d power down",
					i, ch ^ 1, ch);
			}
		}
	}
}


/* base on chan_blen_transfer */
static void dma_npcx_api_example(void)
{
	/* simple memory(char) to memory(char) example*/
	char tx_data[] __aligned(align_val) = "It is harder to be kind than to be wise........";
	char rx_data[48] __aligned(align_val) = { 0 };
	uint8_t dev_num = 0;
	uint8_t ch = 0;
	const struct device *dev = dma_devices[dev_num];
	memset(rx_data, 0, sizeof(rx_data));

	/* Minimal setting for DMA api */
	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = dma_cfg.dest_data_size = align_val;
	dma_block_cfg.block_size = sizeof(tx_data);
	dma_block_cfg.source_address = (uint32_t)tx_data;
	dma_block_cfg.dest_address = (uint32_t)rx_data;

	/* isr event */
	dma_cfg.dma_callback = cb_data_check;
	dma_cfg.head_block = &dma_block_cfg;

	LOG_INF("Preparing DMA Controller: Name=%s, Chan_ID=%u", dev->name, ch);

	if (dma_config(dev, ch, &dma_cfg)) {
		LOG_INF("ERROR: configure\n");
		return;
	}
	if (dma_start(dev, ch)) {
		LOG_INF("ERROR: transfer\n");
		return;
	}
	check_char_data(tx_data, rx_data, 0);
}

static void read_flash(char *src, char *dst)
{
	uint8_t dev_num = 0;
	uint8_t opt = atoi(src), ram = atoi(dst);
	uint8_t ch = 0;
	const struct device *dev = dma_devices[dev_num];

	memset((uint8_t *)MRAM(dev_num), 0, MOVE_SIZE + 4);
	memset((uint8_t *) GDMAMemPool, 0, MOVE_SIZE + 4);
	uint32_t src1, dst1;

	src1 = (uint32_t)(gdma_test_flash_src[opt]);
	dst1 = ram ?  (uint32_t)MRAM(dev_num) : (uint32_t)GDMAMemPool;

	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_block_cfg.source_address = src1;
	dma_block_cfg.dest_address = dst1;
	dma_block_cfg.block_size = MOVE_SIZE + 4;

	dma_cfg.dma_callback = cb_data_check;

	dma_cfg.head_block = &dma_block_cfg;

	dma_cfg.source_data_size = dma_cfg.dest_data_size = align_val;
	if (dma_config(dev, ch, &dma_cfg)) {
		LOG_INF("ERROR: configure\n");
		return;
	}
	if (dma_start(dev, ch)) {
		LOG_INF("ERROR: transfer\n");
		return;
	}

}

static void move_data_ram_to_ram(char *ram, char *ch_set)
{
	uint8_t dev_num = 0;
	uint8_t ram_set, ch;

	ram_set = atoi(ram);
	ch = atoi(ch_set);
	const struct device *dev = dma_devices[dev_num];

	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	memset((uint8_t *) GDMAMemPool, 0, MOVE_SIZE + 4);
	memset((uint8_t *) MRAM(dev_num), 0, MOVE_SIZE + 4);

	dma_block_cfg.block_size = MOVE_SIZE + 4;
	if (ram_set) {
		dma_block_cfg.source_address = (uint32_t)MRAM(dev_num);
		dma_block_cfg.dest_address = (uint32_t)GDMAMemPool;
		for (uint32_t i = 0; i < MOVE_SIZE; i++) {
			*(uint8_t *)(MRAM(dev_num) + i) = *(uint8_t *)(INT_FLASH_BASE1_ADDR + i);
		}
	} else {
		dma_block_cfg.source_address = (uint32_t)GDMAMemPool;
		dma_block_cfg.dest_address = (uint32_t)MRAM(dev_num);
		for (uint32_t i = 0; i < MOVE_SIZE; i++) {
			*(uint8_t *)(dma_block_cfg.source_address + i) =
			*(uint8_t *)(INT_FLASH_BASE1_ADDR + i);
		}
	}

	dma_cfg.dma_callback = cb_data_check;

	dma_cfg.head_block = &dma_block_cfg;

	dma_cfg.source_data_size = dma_cfg.dest_data_size = align_val;
	if (dma_config(dev, ch, &dma_cfg)) {
		LOG_INF("ERROR: configure\n");
		return;
	}
	if (dma_start(dev, ch)) {
		LOG_INF("ERROR: transfer\n");
		return;
	}
}
static void sync_data_transfer(char *option)
{
	uint8_t opt = atoi(option);
	const char tx_data[] __aligned(align_val) =
		"It is harder to be kind than to be wise........";
	char rx_data[48] __aligned(align_val) = { 0 };

	const char tx_data1[] __aligned(align_val) =
		"........wise be to than kind be to harder is It";
	char rx_data1[48] __aligned(align_val) = { 0 };

	const struct device *dev, *dev1;
	uint8_t dev1_channel, dev2_channel;

	switch (opt) {
	case 1:
		dev = dma_devices[0];
		dev1 = dma_devices[1];
		dev1_channel = 0;
		dev2_channel = 1;
		break;
	case 2:
		dev = dma_devices[0];
		dev1 = dma_devices[1];
		dev1_channel = 1;
		dev2_channel = 0;
		break;
	case 3:
		dev = dma_devices[0];
		dev1 = dma_devices[1];
		dev1_channel = dev2_channel = 0;
		break;
	case 4:
		dev = dma_devices[0];
		dev1 = dma_devices[1];
		dev1_channel = dev2_channel = 1;
		break;
	case 5:
		dev = dev1 = dma_devices[0];
		dev1_channel = 0;
		dev2_channel = 1;
		break;
	case 6:
		dev = dev1 = dma_devices[1];
		dev1_channel = 0;
		dev2_channel = 1;
		break;
	}

	memset(rx_data, 0, sizeof(rx_data));
	memset(rx_data1, 0, sizeof(rx_data1));

	dma_cfg.channel_direction = dma_cfg1.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = dma_cfg.dest_data_size = align_val;
	dma_cfg1.source_data_size = dma_cfg1.dest_data_size = align_val;

	dma_block_cfg.block_size = sizeof(tx_data);
	dma_block_cfg1.block_size = sizeof(tx_data1);

	dma_block_cfg.source_address = (uint32_t)tx_data;
	dma_block_cfg.dest_address = (uint32_t)rx_data;
	dma_cfg.head_block = &dma_block_cfg;
	dma_cfg.dma_callback = cb_data_check;

	dma_block_cfg1.source_address = (uint32_t)tx_data1;
	dma_block_cfg1.dest_address = (uint32_t)rx_data1;
	dma_cfg1.head_block = &dma_block_cfg1;
	dma_cfg1.dma_callback = cb_data_check;

	if (dma_config(dev, dev1_channel, &dma_cfg)) {
		LOG_INF("ERROR: configure\n");
		return;
	}
	if (dma_config(dev1, dev2_channel, &dma_cfg1)) {
		LOG_INF("ERROR: configure\n");
		return;
	}
	if (dma_start(dev, dev1_channel)) {
		LOG_INF("ERROR: transfer\n");
		return;
	}
	if (dma_start(dev1, dev2_channel)) {
		LOG_INF("ERROR: transfer\n");
		return;
	}
	check_char_data(tx_data, rx_data, 1);
	check_char_data(tx_data1, rx_data1, 1);

}

static void dma_validation_func(void *dummy1, void *dummy2, void *dummy3)
{
	uint32_t events;
	k_event_init(&dma_event);

	while (true) {
		events = k_event_wait(&dma_event, 0xFFF, true, K_FOREVER);
		switch (events) {
		case 0x001: /* no argu */
			dma_npcx_api_example();
			break;
		case 0x002:
			if (!strcmp("gpd", arguments[0])) {
				LOG_INF("DMA power down gpd test");
				npcx_power_down_2();
			}
			break;
		case 0x004:
			if (!strcmp("para", arguments[0])) {
				LOG_INF("Parallel data transfer");
				sync_data_transfer(arguments[1]);
			}
			break;
		case 0x008:
			if (!strcmp("read", arguments[0])) {
				LOG_INF("DMA read flash");
				read_flash(arguments[1], arguments[2]);
			}
			if (!strcmp("ram", arguments[0])) {
				LOG_INF("RAM to RAM");
				move_data_ram_to_ram(arguments[1], arguments[2]);
			}

			break;
		default:
			LOG_INF("Error command\n");
			break;
		}
	}
}

int dma_command(const struct shell *shell, size_t argc, char **argv)
{
	int i, evt;

	evt = 1;
	for (evt = 1, i = 1; i < argc; i++) {
		strcpy(arguments[i - 1], argv[i]);
		evt <<= 1;
	}
	k_event_post(&dma_event, evt);
	return 0;
}


void test_dma_init(void)
{
	LOG_INF("--- CI20 Zephyr DMA Driver Validation abc ---");
	for (int i = 0 ; i < NUM_DMA_DEVICE; i++) {
		if (!device_is_ready(dma_devices[i])) {
			LOG_ERR("dma device %s is not ready", dma_devices[i]->name);
			return;
		}
		LOG_INF("dma device [%d]:%s is ready", i, dma_devices[i]->name);
	}

	k_thread_create(&temp_id, temp_stack, TASK_STACK_SIZE, dma_validation_func,
		NULL, NULL, NULL, PRIORITY, K_INHERIT_PERMS, K_FOREVER);
	k_thread_name_set(&temp_id, "DMA Validation");
	k_thread_start(&temp_id);
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_dma, SHELL_CMD_ARG(c0, NULL, "dma c0", dma_command, 1, 0),
	SHELL_CMD_ARG(c1, NULL, "dma c1 arg0", dma_command, 2, 0),
	SHELL_CMD_ARG(c2, NULL, "dma c2 arg0 arg1", dma_command, 3, 0),
	SHELL_CMD_ARG(c3, NULL, "dma c3 arg0 arg1 arg2", dma_command, 4, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(dma, &sub_dma, "nuvoton dma validation", NULL);
