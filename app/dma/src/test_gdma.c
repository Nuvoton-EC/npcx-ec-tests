/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 *	@file
 *	@brief	Verify NPCX4 DMA Function.
 *			Please refer `dma_npcx_api_example` for simplest usage.
 *			channel_direction, block_size, source_address, dest_address
 *			are the minimal requirments to use DMA api.
 *	@details
 *	- Test steps
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

/* mem to mem config */
static uint32_t GDMAMemPool[MOVE_SIZE + 4] __aligned(16);

static uint32_t src[TRANSFER_SIZE];
static uint32_t dst[TRANSFER_SIZE] __aligned(16);

static const char tx_data[] = "It is harder to be kind than to be wise........";
static char rx_data[48] = { 0 };

struct dma_config dma_cfg = { 0 };
struct dma_block_config dma_block_cfg = { 0 };

/* flash to mem config */
static uint32_t gdma_test_flash_src[] = {
	INT_FLASH_BASE1_ADDR,	/* internal flash */
	FLASH_BASE1_ADDR,		/* external flash */
};

static uint32_t gdma_data_check(const struct device *dev, const uint8_t channel)
{
	struct dma_reg *const inst = HAL_INSTANCE(dev, channel);

	uint8_t tws = GET_FIELD(inst->CONTROL, NPCX_DMACTL_TWS);
	uint8_t bme = inst->CONTROL & BIT(NPCX_DMACTL_BME);
	uint8_t dadir = inst->CONTROL & BIT(NPCX_DMACTL_DADIR);
	uint8_t sadir = inst->CONTROL & BIT(NPCX_DMACTL_SADIR);
	uint32_t tcnt = inst->TCNT;
	uint32_t src_ = inst->SRCB;
	uint32_t dst_ = inst->DSTB;
	uint32_t ctcnt = inst->CTCNT;
	uint32_t csrc = inst->CSRC;
	uint32_t cdst = inst->CDST;

	uint32_t moveSize = (tcnt << tws) << (bme ? 2 : 0);

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
	if ((sadir && (csrc != (src_ - moveSize))) ||
		(!sadir && (csrc != (src_ + moveSize)))) {
		LOG_INF("[FAIL][GDMA] ch%d CSRC incorrect \r\n", channel);
	}
	if ((dadir && (cdst != (dst_ - moveSize))) ||
		(!dadir && (cdst != (dst_ + moveSize)))) {
		LOG_INF("[FAIL][GDMA] ch%d CDST incorrect \r\n", channel);
	}
	if (bme)
		tcnt <<= 2;

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
		LOG_INF("Double Word mode | ");
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
		break;
	}

	return tcnt;
}
/* isr event */

static void dma_callback_test(const struct device *dma_dev, void *arg,
				uint32_t id, int status)
{
	struct dma_reg *const inst = HAL_INSTANCE(dma_dev, id);
	uint8_t isr = inst->CONTROL & BIT(NPCX_DMACTL_TC);

	if (status < 0) {
		LOG_INF("DMA could not proceed, an error occurred\n");
	}
	LOG_INF("Data transfer completed");
	LOG_INF("%s: status is %02X\n", __func__, isr);
	if (gdma_data_check(dma_dev, id)) {
		LOG_INF("[FAIL]");
	} else {
		LOG_INF("[PASS]");
	}
}

/*
 * Validation Variable Setting
 */

static uint8_t testSetting;
static uint32_t ch, ram;

static void rand_setting_init(uint32_t mask)
{
	testSetting = sys_rand32_get() & mask;
	ch = testSetting & 0x01;
	ram = (testSetting >> 1) & 0x01;
}
static uint8_t fiu_ctl, dma_FIUMode, dma_FIUBurst;
static void rand_setting_init_fiu(uint32_t mask)
{
	fiu_ctl = sys_rand32_get() & mask;
	dma_FIUMode = fiu_ctl & 0x03;
	dma_FIUBurst = (fiu_ctl >> 2) & 0x01;
}

uintptr_t align_up(uintptr_t addr, tw_set tws)
{
	uintptr_t mask = tws - 1;
	/* check power of two */
	if ((tws & mask) == 0) {
		return ((addr + mask) & ~mask);
	} else {
		return (((addr + mask) / tws) * tws);
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

/*
 * Working Function // todo area
 */

volatile uint8_t isr_flag;

static void dma_set_rand_para(uint8_t channel, uint32_t *src_addr, uint32_t *dst_addr)
{

	uint8_t dma_ctf;
	struct dma_npcx_ch_config dma_ctrl = {0};

	dma_ctf = sys_rand32_get() & 0x0f;
	dma_ctrl.tws_bme = (dma_ctf >> 2) & 0x03;
	dma_ctrl.sadir = (dma_ctf >> 1) & 0x01;

	dma_ctrl.dadir = dma_ctf & 0x01;

	dma_ctrl.safix = GDMA_ADDR_CHANGE;
	dma_ctrl.dafix = GDMA_ADDR_CHANGE;

	dma_ctrl.cnt = dma_calculate_cnt(MOVE_SIZE, dma_ctrl.tws_bme);
	dma_ctrl.src = dma_ctrl.sadir ? (uint32_t)(src_addr + MOVE_SIZE) : (uint32_t)src_addr;
	dma_ctrl.dst = dma_ctrl.dadir ? (uint32_t)(dst_addr + MOVE_SIZE) : (uint32_t)dst_addr;

	dma_set_controller(dma_devices[0], channel, &dma_ctrl);

}


/*
 * Under checking Function
 */

static void power_down_test(void)
{
	memset(src, 0, sizeof(src));
	memset(dst, 0, sizeof(dst));

	dma_set_rand_para(0, src, dst);
	dma_start_softreq(dma_devices[0], 0);

	isr_flag = 0;
	/* todo */
	while ((BIT(NPCX_DMACTL_TC)) == 0) {
		if (isr_flag != 0) {
			break;
		}
	}

	LOG_INF("[PASS][GDMA]:got interrupt");

	*(uint8_t *)0x4000D00A = 0x80;
	dma_set_rand_para(0, src, dst);
	dma_start_softreq(dma_devices[0], 0);

	isr_flag = 0;
	while ((BIT(NPCX_DMACTL_TC)) == 0) {
		if (isr_flag != 0) {
			break;
		}
	}
	if (isr_flag == 0) {
		LOG_INF("[PASS][GDMA]:no interrupt after power down");
	} else {
		LOG_INF("[FAIL][GDMA]:Got interrupt after power down");
	}
	*(uint8_t *)0x4000D00A = 0x00;
}



/* base on chan_blen_transfer */
static void dma_npcx_api_example(void)
{
	/* simple memory(char) to memory(char) example*/
	uint8_t dev_num = 0;
	uint8_t ch = 0;
	const struct device *dev = dma_devices[dev_num];
	memset(rx_data, 0, sizeof(rx_data));

	/* Minimal setting for DMA api */
	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_block_cfg.block_size = sizeof(tx_data);
	dma_block_cfg.source_address = (uint32_t)tx_data;
	dma_block_cfg.dest_address = (uint32_t)rx_data;

	/* isr event */
	dma_cfg.dma_callback = dma_callback_test;
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
	LOG_INF("%s\n", rx_data);
	if (strcmp(tx_data, rx_data) != 0) {
		LOG_INF("[FAIL] Data transfer");
		return;
	} else {
		LOG_INF("[PASS] Data transfer");
	}
}

static void read_flash(char *src, char *dst)
{
	uint8_t dev_num = 0;
	uint8_t opt = atoi(src);
	uint8_t ram = atoi(dst);
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

	dma_cfg.dma_callback = dma_callback_test;

	dma_cfg.head_block = &dma_block_cfg;

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
	LOG_INF("test");
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

	dma_cfg.dma_callback = dma_callback_test;

	dma_cfg.head_block = &dma_block_cfg;

	if (dma_config(dev, ch, &dma_cfg)) {
		LOG_INF("ERROR: configure\n");
		return;
	}
	if (dma_start(dev, ch)) {
		LOG_INF("ERROR: transfer\n");
		return;
	}
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
			if (!strcmp("gps", arguments[0])) {
				LOG_INF("DMA power down interrupt test");
				power_down_test();
			}
			break;
		case 0x004:
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
