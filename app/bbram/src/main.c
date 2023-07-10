/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/bbram.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <soc.h>
#include <stdlib.h>

#define LOG_LEVEL LOG_LEVEL_INF
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#define MAX_ARGUMNETS 3
#define MAX_ARGU_SIZE 10
static uint8_t arguments[MAX_ARGUMNETS][MAX_ARGU_SIZE];
struct k_event bbram_event;

#define TASK_STACK_SIZE         1024
#define PRIORITY                7
static const struct device *const bbram_dev = DEVICE_DT_GET(DT_NODELABEL(bbram));

static struct k_thread temp_id;
K_THREAD_STACK_DEFINE(temp_stack, TASK_STACK_SIZE);

static void bbram_validation_func(void *dummy1, void *dummy2, void *dummy3)
{
	uint32_t events, offs, size;
	uint8_t buff[256];

	offs = 0;
	size = 0;
	k_event_init(&bbram_event);
	while (true) {
		events = k_event_wait(&bbram_event, 0xFFF, true, K_FOREVER);
		switch (events) {
		case 0x001: /* no argu */
			break;
		case 0x002:
			if (!strcmp("chk_valid", arguments[0])) {
				do {
					offs = bbram_check_invalid(bbram_dev);
					LOG_INF("[%d]IBBR", offs);
				} while (offs);
			}
			if (!strcmp("chk_sbypw", arguments[0])) {
				LOG_INF("[%d]SBY", bbram_check_standby_power(bbram_dev));
			}
			if (!strcmp("chk_pw", arguments[0])) {
				LOG_INF("[%d]PW", bbram_check_power(bbram_dev));
			}
			if (!strcmp("chk_sz", arguments[0])) {
				bbram_get_size(bbram_dev, &size);
				LOG_INF("[%d]", size);
			}
			if (!strcmp("write", arguments[0])) {
				if (!bbram_write(bbram_dev, (size_t)offs, (size_t)size, buff))
					LOG_INF("[PASS] Write");
				else
					LOG_INF("[FAIL] Write");
			}
			if (!strcmp("read", arguments[0])) {
				if (!bbram_read(bbram_dev, (size_t)offs, (size_t)size, buff))
					LOG_INF("[PASS] Read");
				else
					LOG_INF("[FAIL] Read");
			}
			break;
		case 0x004:
			if (!strcmp("offs", arguments[0]))
				offs = atoi(arguments[1]);
			if (!strcmp("size", arguments[0]))
				size = atoi(arguments[1]);
			if (!strcmp("idxR", arguments[0]))
				LOG_INF("%d:[%d]", atoi(arguments[1]), buff[atoi(arguments[1])]);
			break;
		case 0x008:
			if (!strcmp("idxW", arguments[0]))
				buff[atoi(arguments[1])] = (size_t)atoi(arguments[2]);
			break;
		}
	}
}

int main(void)
{
	if (!device_is_ready(bbram_dev)) {
		LOG_INF("[FAIL]: BBRAM not ready");
		return 0;
	}
	LOG_INF("[PASS]: BBRAM ready");
	k_thread_create(&temp_id, temp_stack, TASK_STACK_SIZE,
			bbram_validation_func, NULL, NULL, NULL, PRIORITY,
			K_INHERIT_PERMS, K_FOREVER);
	k_thread_name_set(&temp_id, "ps2 Validation");
	k_thread_start(&temp_id);
	return 0;
}

static int bbram_command(const struct shell *shell, size_t argc, char **argv)
{
	int i, evt;

	evt = 1;
	for (evt = 1, i = 1; i < argc; i++) {
		strcpy(arguments[i-1], argv[i]);
		evt <<= 1;
	}
	k_event_post(&bbram_event, evt);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_bbram,
	SHELL_CMD_ARG(c0, NULL, "bbram c0", bbram_command, 1, 0),
	SHELL_CMD_ARG(c1, NULL, "bbram c1 arg0", bbram_command, 2, 0),
	SHELL_CMD_ARG(c2, NULL, "bbram c2 arg0 arg1", bbram_command, 3, 0),
	SHELL_CMD_ARG(c3, NULL, "bbram c3 arg0 arg1 arg2", bbram_command, 4, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(bbram, &sub_bbram, "BBRAM validation commands", NULL);
