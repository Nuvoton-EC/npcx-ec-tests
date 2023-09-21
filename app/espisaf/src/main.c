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

#define LOG_LEVEL LOG_LEVEL_INF
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

/* Test function declarations */
void test_espi_taf_init(void);

/* Main entry */
int main(void)
{
	LOG_INF("--- CI20 Zephyr Driver Validation ---");

	/* Zephyr driver validation init */
	test_espi_taf_init();

	/* Let main thread go to sleep state */
	k_sleep(K_FOREVER);

	return 0;
}

/* Custom PM policy handler to forbid ec enter deep sleep if CONFIG_PM is enabled */
const struct pm_state_info *pm_policy_next_state(uint8_t cpu, int32_t ticks)
{
	ARG_UNUSED(cpu);

	return NULL;
}
