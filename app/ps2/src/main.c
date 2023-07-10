/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/ps2.h>
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
struct k_event ps2_event;

#define TASK_STACK_SIZE         1024
#define PRIORITY                7
#define SLEEP_DELAY_MS	500
#define SLEEP_DELAY	K_MSEC(SLEEP_DELAY_MS)

static const struct device *const ps2_dev_2 = DEVICE_DT_GET(DT_NODELABEL(ps2_channel2));
static const struct device *const ps2_dev_3 = DEVICE_DT_GET(DT_NODELABEL(ps2_channel3));

static struct k_thread temp_id;
K_THREAD_STACK_DEFINE(temp_stack, TASK_STACK_SIZE);

static void mb_callback2(const struct device *dev, uint8_t value)
{
	LOG_INF("[PASS]ch2:%x", value);
}
static void mb_callback3(const struct device *dev, uint8_t value)
{
	LOG_INF("[PASS]ch3:%x", value);
}
static void send_data(uint32_t ch)
{
	int i;

	for (i = 0; i < 256; i++) {
		if (ch == 2) {
			if (ps2_write(ps2_dev_2, i) == 0)
				LOG_INF("[PASS]:%x", i);
			else
				LOG_INF("[FAIL]:%x", i);
		}
		if (ch == 3) {
			if (ps2_write(ps2_dev_3, i) == 0)
				LOG_INF("[PASS]:%x", i);
			else
				LOG_INF("[FAIL]:%x", i);
		}
		k_sleep(SLEEP_DELAY);
	}
}

static void ps2_validation_func(void *dummy1, void *dummy2, void *dummy3)
{
	uint32_t events, data;

	k_event_init(&ps2_event);
	while (true) {
		events = k_event_wait(&ps2_event, 0xFFF, true, K_FOREVER);
		switch (events) {
		case 0x001: /* no argu */
			break;
		case 0x002:
			if (!strcmp("init2", arguments[0])) {
				if (ps2_config(ps2_dev_2, mb_callback2))
					LOG_INF("[FAIL]: ps2_ch2 set callback");
				else
					LOG_INF("[PASS]: ps2_ch2 set callback");
			}
			if (!strcmp("init3", arguments[0])) {
				if (ps2_config(ps2_dev_3, mb_callback3))
					LOG_INF("[FAIL]: ps2_ch3 set callback");
				else
					LOG_INF("[PASS]: ps2_ch3 set callback");
			}
			if (!strcmp("init", arguments[0])) {
				if (ps2_config(ps2_dev_3, mb_callback2))
					LOG_INF("[FAIL]: ps2_ch3 set callback");
				if (ps2_config(ps2_dev_2, mb_callback3))
					LOG_INF("[FAIL]: ps2_ch2 set callback");
			}
			if (!strcmp("w2", arguments[0]))
				send_data(2);
			if (!strcmp("w3", arguments[0]))
				send_data(3);
			if (!strcmp("s2", arguments[0]))
				ps2_write(ps2_dev_2, 0);
			if (!strcmp("s3", arguments[0]))
				ps2_write(ps2_dev_3, 0);
			break;
		case 0x004:
			if (!strcmp("ena_resp", arguments[0])) {
				data = 0xDEAD;
				if (!strcmp("2", arguments[1]))
					data = ps2_enable_callback(ps2_dev_2);
				else if (!strcmp("3", arguments[1]))
					data = ps2_enable_callback(ps2_dev_3);
				if (data == 0)
					LOG_INF("[PASS]: enable");
				else if (data == 0xDEAD)
					LOG_INF("[FAIL]: Channel incorrect");
				else
					LOG_INF("[FAIL]: enable");
			}
			if (!strcmp("dis_resp", arguments[0])) {
				data = 0xDEAD;
				if (!strcmp("2", arguments[1]))
					data = ps2_disable_callback(ps2_dev_2);
				else if (!strcmp("3", arguments[1]))
					data = ps2_disable_callback(ps2_dev_3);
				if (data == 0)
					LOG_INF("[PASS]: disable");
				else if (data == 0xDEAD)
					LOG_INF("[FAIL]: Channel incorrect");
				else
					LOG_INF("[FAIL]: disable");
			}
			break;
		}
	}
}

int main(void)
{
	if (!device_is_ready(ps2_dev_2)) {
		LOG_INF("[FAIL]: PS2 ch2 not ready");
		return 0;
	}
	if (!device_is_ready(ps2_dev_3)) {
		LOG_INF("[FAIL]: PS2 ch3 not ready");
		return 0;
	}
	LOG_INF("[PASS]: set ch2/3 callback");
	k_thread_create(&temp_id, temp_stack, TASK_STACK_SIZE,
			ps2_validation_func, NULL, NULL, NULL, PRIORITY,
			K_INHERIT_PERMS, K_FOREVER);
	k_thread_name_set(&temp_id, "ps2 Validation");
	k_thread_start(&temp_id);
	return 0;
}

static int ps2_command(const struct shell *shell, size_t argc, char **argv)
{
	int i, evt;

	evt = 1;
	for (evt = 1, i = 1; i < argc; i++) {
		strcpy(arguments[i-1], argv[i]);
		evt <<= 1;
	}
	k_event_post(&ps2_event, evt);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_ps2,
	SHELL_CMD_ARG(c0, NULL, "ps2 c0", ps2_command, 1, 0),
	SHELL_CMD_ARG(c1, NULL, "ps2 c1 arg0", ps2_command, 2, 0),
	SHELL_CMD_ARG(c2, NULL, "ps2 c2 arg0 arg1", ps2_command, 3, 0),
	SHELL_CMD_ARG(cfg, NULL, "ps2 cfg arg0 arg1 arg2", ps2_command, 4, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(ps2, &sub_ps2, "PS2 validation commands", NULL);
