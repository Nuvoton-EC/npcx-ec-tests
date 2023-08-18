/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/kscan.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(main);

const struct device *const kscan_dev = DEVICE_DT_GET(DT_NODELABEL(kbd));

#define TASK_STACK_SIZE		 1024
#define PRIORITY                7
static struct k_thread temp_id;
K_THREAD_STACK_DEFINE(temp_stack, TASK_STACK_SIZE);

#define MAX_MATRIX_KEY_COLS 13
#define MAX_MATRIX_KEY_ROWS 8

static uint8_t Resp[21];
static void kb_callback(const struct device *dev, uint32_t row, uint32_t col,
			bool pressed)
{
	ARG_UNUSED(dev);

	if (Resp[0] > 20) {
		LOG_INF("[FAIL] Over CmdResp buffer\n");
		return;
	}
	Resp[Resp[0]] = 0x0;
	if (pressed) {
		Resp[Resp[0]] = 0x80;
	}
	Resp[Resp[0]] |= (row<<4)&0x70;
	Resp[Resp[0]] |= (col<<0)&0x0F;
	LOG_INF("Resp[%02x]:%02x\n", Resp[0], Resp[Resp[0]]);
	Resp[0]++;
}
/* Cmd[0] : commad type
 * Cmd[1~10] : command argunment
 */
static uint8_t Cmd[11];
static void check_resp(void)
{
	uint8_t ok, i, j, tmp;

	if (Cmd[0] == 0) { /* stree test one by one keys */
		ok = 1;
		if (Resp[0] != 3) {
			ok = 0;
			LOG_INF("[FAIL](%02x)Key lost response\n", Resp[0]);
		}
		if (((Resp[1] & 0x80) != 0x80) || ((Resp[2] & 0x80) != 0x00)) {
			ok = 0;
			LOG_INF("[FAIL](%02x-%02x) Key press and release abornal\n",
				Resp[1], Resp[2]);
		}
		Resp[1] &= 0x7F;
		Resp[2] &= 0x7F;
		if (Resp[1] != Resp[2]) {
			ok = 0;
			LOG_INF("[FAIL](%02x-%02x) press and release not same key\n",
				Resp[1], Resp[2]);
		}
		if (Resp[1] != Cmd[1]) {
			ok = 0;
			LOG_INF("[FAIL](%02x->%02x) response key incorrect\n",
				Cmd[1], Resp[1]);
		}
		if (ok) {
			LOG_INF("[PASS](%02x)\n", Cmd[1]);
		}
		return;
	}
	if (Cmd[0] == 1) { /* Ghost Keys validation*/
		if ((Cmd[1] != 0) && (Resp[0] != 7)) {
			LOG_INF("[FAIL] lost key\n");
			return;
		}
		if (Cmd[1] == 0x01) {
			if (((Resp[1] & 0x7f) == (Resp[4] & 0x7f)) &&
			((Resp[2] & 0x7f) == (Resp[5] & 0x7f)) &&
			((Resp[3] & 0x7f) == (Resp[6] & 0x7f))) {
				if (((Resp[4] & 0x0F) == (Resp[5] & 0x0F)) &&
				((Resp[4] & 0xF0) == (Resp[6] & 0xF0))) {
					LOG_INF("[PASS]\n");
					return;
				}
			}
			LOG_INF("[FAIL] incorrect\n");
		}
		if (Cmd[1] == 0x02) {
			if (((Resp[1] & 0x7f) == (Resp[4] & 0x7f)) &&
			((Resp[2] & 0x7f) == (Resp[5] & 0x7f)) &&
			((Resp[3] & 0x7f) == (Resp[6] & 0x7f))) {
				if (((Resp[4] & 0xF0) == (Resp[6] & 0xF0)) &&
				((Resp[5] & 0x0F) == (Resp[6] & 0x0F))) {
					LOG_INF("[PASS]\n");
					return;
				}
			}
			LOG_INF("[FAIL] incorrect\n");
		}
		if (Cmd[1] == 0x00) {
			LOG_INF("[%s]\n", (Resp[0] == 1) ? "PASS" : "FAIL");
		}
		return;
	}
	if (Cmd[0] == 2) { /* multi-key press*/
		ok = 1;
		if ((Cmd[1] * 2) != (Resp[0] - 1)) {
			LOG_INF("[FAIL] (%02x) lost some key\n", Resp[0]);
			ok = 0;
		}
		for (j = 2; j < Cmd[1]; j++) {
			tmp = 0;
			for (i = 1; i < Resp[0]; i++) {
				if (Cmd[j] == (Resp[i] & 0x7F)) {
					if (Resp[i] & 0x80) {
						tmp += 0x10; /* press */
					} else {
						tmp += 0x01; /* release */
					}
					continue;
				}
			}
			if (tmp != 0x11) {
				LOG_INF("[FAIL] (%02x) key not found or lost\n", Cmd[j]);
				ok = 0;
			}
		}
		if (ok) {
			LOG_INF("[PASS] mulit-keys: %02x\n", Cmd[1]);
		}
		{
			extern void make_keyplan(void);
			make_keyplan();
		}
	}
}
static uint32_t multikey_map;
void make_keyplan(void)
{
	uint32_t map;
	uint8_t i;

	for (map = 0, i = 0; i < 8; i++) {
		map <<= 4;
		map |= (multikey_map * i + multikey_map + i) % MAX_MATRIX_KEY_COLS;
		;
		multikey_map += map;
	}
	LOG_INF("map:%08x", map);
	Cmd[2] = 0x00 | (map & 0xF);
	map >>= 4;
	Cmd[3] = 0x10 | (map & 0xF);
	map >>= 4;
	Cmd[4] = 0x20 | (map & 0xF);
	map >>= 4;
	Cmd[5] = 0x30 | (map & 0xF);
	map >>= 4;
	Cmd[6] = 0x40 | (map & 0xF);
	map >>= 4;
	Cmd[7] = 0x50 | (map & 0xF);
	map >>= 4;
	Cmd[8] = 0x60 | (map & 0xF);
	map >>= 4;
	Cmd[9] = 0x70 | (map & 0xF);
	map >>= 4;
	Cmd[1] = 8;
}

/* UTILITY */
uint32_t atoh(uint8_t *arg)
{
	uint8_t i;
	uint32_t mask;

	mask = 0;
	for (i = 0; i < 8; i++) {
		if (arg[i] == 0) {
			break;
		}
		mask <<= 4;
		if ((arg[i] >= '0') && (arg[i] <= '9')) {
			arg[i] = arg[i] - '0';
		} else if ((arg[i] >= 'a') && (arg[i] <= 'f')) {
			arg[i] = arg[i] - 'a' + 0xa;
		} else if ((arg[i] >= 'A') && (arg[i] <= 'F')) {
			arg[i] = arg[i] - 'A' + 0xa;
		} else {
			arg[i] = 0;
		}
		mask |= arg[i];
	}
	return (mask);
}

#define MAX_ARGUMNETS 3
#define MAX_ARGU_SIZE 10
static uint8_t arguments[MAX_ARGUMNETS][MAX_ARGU_SIZE];
struct k_event kscan_event;
static void kscan_validation_func(void *dummy1, void *dummy2, void *dummy3)
{
	uint32_t events;
	uint8_t tmp;

	k_event_init(&kscan_event);
	if (!device_is_ready(kscan_dev)) {
		LOG_ERR("kscan device %s not ready", kscan_dev->name);
		return;
	}

	LOG_INF("KSCAN module hook success\n");
	kscan_config(kscan_dev, kb_callback);
	kscan_enable_callback(kscan_dev);

	Cmd[0] = 0xFF;
	while (true) {
		events = k_event_wait(&kscan_event, 0xFFF, true, K_FOREVER);
		switch (events) {
		case 0x001: /* no argu */
			check_resp();
			Resp[0] = 1;
			break;
		case 0x002:
			break;
		case 0x004: /* select validation command */
			Cmd[0] = ((uint8_t)atoi(arguments[0]));
			Resp[0] = 1;
			if (Cmd[0] == 0) { /* stree test one by one keys */
				tmp = (Cmd[1] >> 4) + atoi(arguments[1]);
				Cmd[1] = ((tmp % MAX_MATRIX_KEY_ROWS) << 4) | (Cmd[1] & 0x0F);
				tmp = ((Cmd[1] & 0x0F) + (tmp / MAX_MATRIX_KEY_ROWS)) %
				  MAX_MATRIX_KEY_COLS;
				Cmd[1] = (Cmd[1] & 0xF0) | tmp;
				LOG_INF("Stress Test Cmd[1]:%02x\n", Cmd[1]);
			}
			if (Cmd[0] == 1) { /* Ghost Keys validation*/
				/* if non-zreo ghost key must happen */
				Cmd[1] = (uint8_t)atoi(arguments[1]);
			}
			if (Cmd[0] == 2) { /* multi-key press */
				/* how many keys in the Cmd plan */
				multikey_map = atoh(arguments[1]);
				LOG_INF("multikey_map = %08x\n", multikey_map);
				make_keyplan();
			}
			break;
		case 0x008: /* fill key press plan */
			tmp = (uint8_t)atoi(arguments[0]);
			Cmd[tmp] = (((uint8_t)atoi(arguments[1])) & 0x07) << 4;
			Cmd[tmp] |= (((uint8_t)atoi(arguments[2])) & 0x0F) << 0;
			LOG_INF("Cmd[%02x]:%02x\n", tmp, Cmd[tmp]);
			break;
		}
	}
}
void main(void)
{
	/* Zephyr driver validation */
	LOG_INF("Start KSCAN Validation Task\n");
	k_thread_create(&temp_id, temp_stack, TASK_STACK_SIZE, kscan_validation_func, NULL, NULL,
			NULL, PRIORITY, K_INHERIT_PERMS, K_FOREVER);
	k_thread_name_set(&temp_id, "KSCAN Validation");
	k_thread_start(&temp_id);
}
static int kscan_command(const struct shell *shell, size_t argc, char **argv)
{
	int i, evt;

	evt = 1;
	for (evt = 1, i = 1; i < argc; i++) {
		strcpy(arguments[i - 1], argv[i]);
		evt <<= 1;
	}
	k_event_post(&kscan_event, evt);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_kscan,
	SHELL_CMD_ARG(c0, NULL, "kscan c0", kscan_command, 1, 0),
	SHELL_CMD_ARG(c1, NULL, "kscan c1 arg0", kscan_command, 2, 0),
	SHELL_CMD_ARG(c2, NULL, "kscan c2 arg0 arg1", kscan_command, 3, 0),
	SHELL_CMD_ARG(c3, NULL, "kscan c3 arg0 arg1 arg2", kscan_command, 4, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(kscan, &sub_kscan, "kscan validation commands", NULL);
