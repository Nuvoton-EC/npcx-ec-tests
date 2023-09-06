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
static uint32_t GDMAMemPool[MOVE_SIZE + 4] __aligned(16);

static uint8_t src[TRANSFER_SIZE];
static uint8_t dst[TRANSFER_SIZE] __aligned(16);

#define RX_BUFF_SIZE (48)
static const char tx_data[] = "It is harder to be kind than to be wise........";
static char rx_data[RX_BUFF_SIZE] = { 0 };


K_THREAD_STACK_DEFINE(temp_stack, TASK_STACK_SIZE);


/* Get device from device tree */
static const struct device *const dma_devices[] = {
	DEVICE_DT_GET(DMA0_CTLER),
	DEVICE_DT_GET(DMA1_CTLER),
};

#define NUM_DMA_DEVICE ARRAY_SIZE(dma_devices)
#define cal_flash_type_num(ADDR)	ARRAY_SIZE(ADDR)
#define MRAM	((const struct dma_npcx_config *)dma_devices[0]->config)->buttom_mram

/* isr event */
volatile uint32_t usr_flag;

static void dma_callback_test(const struct device *dev, void *user_data,
			       uint32_t channel, int status)
{
	if (status >= 0) {
		LOG_INF("DMA transfer done\n");
	} else {
		LOG_INF("DMA transfer met an error\n");
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

/*
 * Finished Function
 */

int *npcx_power_down_gpd(const struct device *dev, const uint32_t channel,
					uint32_t val0, uint32_t val1)
{
	static int arr[2] = {0};
	const uint32_t dma_base = ((const struct dma_npcx_config *)dev->config)->reg_base;
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

static void dma_set_rand_para(uint8_t channel, uint8_t *src_addr, uint8_t *dst_addr)
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

static void dma_read_flash_val(void)
{
	memset((uint8_t *)MRAM, 0, MOVE_SIZE + 4);
	memset(GDMAMemPool, 0, MOVE_SIZE + 4);

	rand_setting_init(0x3);
	rand_setting_init_fiu(0xf);
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
static void dma_npcx_api_test(void)
{
	/* memory to memory */
	struct dma_config dma_cfg = { 0 };
	struct dma_block_config dma_block_cfg = { 0 };
	const struct device *dev = dma_devices[0];
	uint8_t blen = 8;

	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = 1U;
	dma_cfg.dest_data_size = 1U;
	dma_cfg.source_burst_length = blen;
	dma_cfg.dest_burst_length = blen;
	dma_cfg.dma_callback = dma_callback_test;
	dma_cfg.complete_callback_en = 0U;
	dma_cfg.error_callback_en = 1U;
	dma_cfg.block_count = 1U;
	dma_cfg.head_block = &dma_block_cfg;

	LOG_INF("Preparing DMA Controller: Name=%s, Chan_ID=%u, BURST_LEN=%u\n",
		 dev->name, 0, blen >> 3);

	LOG_INF("Starting the transfer\n");

	memset(rx_data, 0, sizeof(rx_data));
	dma_block_cfg.block_size = sizeof(tx_data);

	dma_block_cfg.source_address = (uint32_t)tx_data;
	dma_block_cfg.dest_address = (uint32_t)rx_data;

	if (dma_config(dev, 0, &dma_cfg)) {
		LOG_INF("ERROR: transfer\n");
		return;
	}
	if (dma_start(dev, 0)) {
		LOG_INF("ERROR: transfer\n");
		return;
	}

	k_sleep(K_MSEC(2000));

	LOG_INF("%s\n", rx_data);
	if (strcmp(tx_data, rx_data) != 0) {
		LOG_INF("[FAIL] Data transfer");
		return;
	}
	LOG_INF("[PASS] Data transfer");
}
const struct shell *sh_ptr;
static void dma_validation_func(void *dummy1, void *dummy2, void *dummy3)
{
	uint32_t events;
	k_event_init(&dma_event);

	while (true) {
		events = k_event_wait(&dma_event, 0xFFF, true, K_FOREVER);
		switch (events) {
		case 0x001: /* no argu */
			LOG_INF("api test");
			dma_npcx_api_test();
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
			if (!strcmp("read", arguments[0])) {
				LOG_INF("DMA read flash");
				dma_read_flash_val();
			}
			if (!strcmp("ram", arguments[0])) {
				LOG_INF("RAM to RAM data transfer");
				dma_npcx_api_test();
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
	SHELL_CMD_ARG(cfg, NULL, "dma cfg arg0 arg1 arg2", dma_command, 4, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(dma, &sub_dma, "nuvoton dma validation", NULL);
