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


static int dma_init(void)
{
	int ret;

	return 0;
}

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
			if (!strcmp("init", arguments[0])) {
				LOG_INF("DMA module init\n");
				dma_init();
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