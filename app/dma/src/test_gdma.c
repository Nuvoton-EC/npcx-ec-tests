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
#include <zephyr/sys/printk.h>
#include <zephyr/console/console.h>

#define LOG_LEVEL LOG_LEVEL_INF
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gdma);

#define MAX_ARGUMNETS 3
#define MAX_ARGU_SIZE 10

#define TASK_STACK_SIZE 1024
#define PRIORITY 7
static uint8_t arguments[MAX_ARGUMNETS][MAX_ARGU_SIZE];
struct k_event dma_event;
static struct k_thread temp_id;
K_THREAD_STACK_DEFINE(temp_stack, TASK_STACK_SIZE);

static const struct device *const dma_dev = DEVICE_DT_GET(DT_NODELABEL(dma0));

/*
 * Finished Function
 */

static void npcx_power_down2(void)
{
	uint32_t val1 = 0x11223344, val2 = 0x9ABCDEF0;

	for (uint8_t ch = 0; ch < MAX_DMA_CHANNELS; ch++) {
		int *res = npcx_power_down_gpd(dma_dev, ch, val1, val2);

		if (res[0]) {
			LOG_INF("[FAIL][GDMA]: GPD%d power down", ch);
		} else {
			LOG_INF("[PASS][GDMA]: GPD%d power down", ch);
		}
		if (res[1]) {
			LOG_INF("[PASS][GDMA]: GPD%d still run when GDP%d power down", ch ^ 1, ch);
		} else {
			LOG_INF("[FAIL][GDMA]: GPD%d still run when GDP%d power down", ch ^ 1, ch);
		}
	}
}

/*
 * Working Function // todo area
 */

volatile uint8_t isr_flag;

static inline void dma0_isr_callback(uint8_t error)
{
	isr_flag |= 0x01;
	if (error) {
		isr_flag |= 0x02;
	}
}

static inline void dma1_isr_callback(uint8_t error)
{
	isr_flag |= 0x10;
	if (error) {
		isr_flag |= 0x20;
	}
}

static GDMA_CALLBACK dma_isr_callback[2] = {dma0_isr_callback, dma1_isr_callback};

static void dma_set_rand_para(uint8_t channel, uint32_t *src_addr, uint32_t *dst_addr)
{

	uint8_t dma_ctf;
	GDMA_CTLR_T dma_ctrl = {0};

	dma_ctf = sys_rand32_get() & 0x0f;
	dma_ctrl.tws_bme = (dma_ctf >> 2) & 0x03;
	dma_ctrl.sadir = (dma_ctf >> 1) & 0x01;

	dma_ctrl.dadir = dma_ctf & 0x01;

	dma_ctrl.safix = GDMA_ADDR_CHANGE;
	dma_ctrl.dafix = GDMA_ADDR_CHANGE;

	dma_ctrl.cnt = DMA_Calculate_Cnt(MOVE_SIZE, dma_ctrl.tws_bme);
	dma_ctrl.src = dma_ctrl.sadir ? (uint32_t)(src_addr + MOVE_SIZE) : (uint32_t)src_addr;
	dma_ctrl.dst = dma_ctrl.dadir ? (uint32_t)(dst_addr + MOVE_SIZE) : (uint32_t)dst_addr;

	dma_ctrl.callback = dma_isr_callback[channel];

	DMA_Set_Controller(dma_dev, channel, dma_ctrl, dma_isr_callback);

}

static uint32_t GDMAMemPool[MOVE_SIZE + 4] __aligned(16);

static void power_down_test(void)
{
	uint32_t *src, *dst;

	src = ((const struct dma_npcx_config *)dma_dev->config)->buttom_mram;
	dst = GDMAMemPool;

	memset(src, 0, sizeof(src));
	memset(dst, 0, sizeof(dst));

	dma_set_rand_para(0, src, dst);
	dma_start_softreq(dma_dev, 0);

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
	dma_start_softreq(dma_dev, 0);

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


/*
 * Under checking Function
 */


static void dma_validation_func(void *dummy1, void *dummy2, void *dummy3)
{
	uint32_t events;
	k_event_init(&dma_event);

	while (true) {
		events = k_event_wait(&dma_event, 0xFFF, true, K_FOREVER);
		switch (events)
		{
		case 0x001: /* no argu */
			break;
		case 0x002:
			if (!strcmp("power_down_gpd", arguments[0])) {
				LOG_INF("DMA power down test");
				npcx_power_down2();
			}
			if (!strcmp("power_down", arguments[0])) {
				LOG_INF("DMA power down test");
				power_down_test();
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
	if (!device_is_ready(dma_dev)) {
		LOG_INF("[FAIL]: DMA not ready");
		return;
	}
	LOG_INF("[PASS]: DMA ready");

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
SHELL_CMD_REGISTER(dma, &sub_dma, "nuvoton adc validation", NULL);