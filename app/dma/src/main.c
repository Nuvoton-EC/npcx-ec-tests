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
#include <zephyr/drivers/dma.h>

#define LOG_LEVEL LOG_LEVEL_INF
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

static const struct device *const dma_dev = DEVICE_DT_GET(DT_NODELABEL(dma0));

/* Main entry */
void main(void)
{
	LOG_INF("--- CI20 Zephyr DMA Driver Validation abc ---");

	if (!device_is_ready(dma_dev)) {
		LOG_ERR("dma device %s not ready", dma_dev->name);
		return;
	}

	LOG_INF("dma device %s is ready", dma_dev->name);

}