/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/peci.h>
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
struct k_event peci_event;

#define TASK_STACK_SIZE         1024
#define PRIORITY                7

/* PECI Host address */
#define PECI_HOST_ADDR          0x30u
/* PECI Host bitrate 1Mbps */
#define PECI_HOST_BITRATE       1000u

#define PECI_CONFIGINDEX_TJMAX  16u
#define PECI_CONFIGHOSTID       0u
#define PECI_CONFIGPARAM        0u

#define PECI_SAFE_TEMP          72

static const struct device *const peci_dev = DEVICE_DT_GET(DT_ALIAS(peci_0));
static uint8_t tjmax;
static uint8_t rx_fcs;

static struct k_thread temp_id;
K_THREAD_STACK_DEFINE(temp_stack, TASK_STACK_SIZE);

int peci_ping(void)
{
	int ret;
	struct peci_msg packet;

	packet.addr = PECI_HOST_ADDR;
	packet.cmd_code = PECI_CMD_PING;
	packet.tx_buffer.buf = NULL;
	packet.tx_buffer.len = PECI_PING_WR_LEN;
	packet.rx_buffer.buf = NULL;
	packet.rx_buffer.len = PECI_PING_RD_LEN;

	ret = peci_transfer(peci_dev, &packet);
	if (ret) {
		LOG_INF("[FAIL]:ping %d", ret);
		return ret;
	}
	LOG_INF("[PASS]:ping");
	return 0;
}

int peci_getdib(void)
{
	int ret;
	struct peci_msg packet;
	uint8_t peci_resp_buf[PECI_GET_DIB_RD_LEN];

	packet.addr = PECI_HOST_ADDR;
	packet.cmd_code = PECI_CMD_GET_DIB;
	packet.tx_buffer.buf = NULL;
	packet.tx_buffer.len = PECI_GET_DIB_WR_LEN;
	packet.rx_buffer.buf = peci_resp_buf;
	packet.rx_buffer.len = PECI_GET_DIB_RD_LEN;

	ret = peci_transfer(peci_dev, &packet);
	if (ret) {
		LOG_INF("[FAIL]:ping %d", ret);
		return ret;
	}
	for (int i = 0; i < PECI_GET_DIB_RD_LEN; i++) {
		LOG_INF("%02x", packet.rx_buffer.buf[i]);
	}
	LOG_INF("[PASS]");
	return 0;
}

int peci_get_tjmax(uint8_t *tjmax)
{
	int ret;
	uint8_t peci_resp;
	struct peci_msg packet;

	uint8_t peci_resp_buf[PECI_RD_PKG_LEN_DWORD+1];
	uint8_t peci_req_buf[] = {0x60, 0x02, 0x02, 0};

	packet.tx_buffer.buf = peci_req_buf;
	packet.tx_buffer.len = PECI_RD_PKG_WR_LEN;
	packet.rx_buffer.buf = peci_resp_buf;
	packet.rx_buffer.len = PECI_RD_PKG_LEN_DWORD;

	packet.addr = PECI_HOST_ADDR;
	packet.cmd_code = PECI_CMD_RD_PKG_CFG0;

	ret = peci_transfer(peci_dev, &packet);

	for (int i = 0; i < PECI_RD_PKG_LEN_DWORD; i++) {
		LOG_INF("%02x", packet.rx_buffer.buf[i]);
	}

	peci_resp = packet.rx_buffer.buf[0];
	rx_fcs = packet.rx_buffer.buf[PECI_RD_PKG_LEN_DWORD];

	*tjmax = packet.rx_buffer.buf[3];

	return 0;
}

void get_max_temp(void)
{
	int ret;

	ret = peci_get_tjmax(&tjmax);
	if (ret) {
		LOG_INF("[FAIL]Fail to obtain maximum temperature: %d\n", ret);
	} else {
		LOG_INF("{PASS]Maximum temperature: %u\n", tjmax);
	}
}

void peci_cmd_config(uint32_t speed)
{
	int ret;

	ret = peci_config(peci_dev, speed);
	if (ret) {
		LOG_INF("[FAIL]: configure bitrate\n");
	}
	LOG_INF("[PASS]: PECI config done");
	peci_enable(peci_dev);
}

static void peci_validation_func(void *dummy1, void *dummy2, void *dummy3)
{
	uint32_t  events, data;

	k_event_init(&peci_event);
	while (true) {
		events = k_event_wait(&peci_event, 0xFFF, true, K_FOREVER);
		switch (events) {
		case 0x001: /* no argu */
			break;
		case 0x002:
			if (!strcmp("ping", arguments[0]))
				peci_ping();
			if (!strcmp("rdcfg", arguments[0]))
				get_max_temp();
			if (!strcmp("getdib", arguments[0]))
				peci_getdib();
			break;
		case 0x004:
			if (!strcmp("config", arguments[0])) {
				data = atoi(arguments[1]);
				peci_cmd_config(data);
			}
		}
	}
}

int main(void)
{
	if (!device_is_ready(peci_dev)) {
		LOG_INF("[FAIL]: PECI not ready");
		return -ENODEV;
	}
	k_thread_create(&temp_id, temp_stack, TASK_STACK_SIZE,
			peci_validation_func, NULL, NULL, NULL, PRIORITY,
			K_INHERIT_PERMS, K_FOREVER);
	k_thread_name_set(&temp_id, "peci Validation");
	k_thread_start(&temp_id);

	return 0;
}

static int peci_command(const struct shell *shell, size_t argc, char **argv)
{
	int i, evt;

	evt = 1;
	for (evt = 1, i = 1; i < argc; i++) {
		strcpy(arguments[i-1], argv[i]);
		evt <<= 1;
	}
	k_event_post(&peci_event, evt);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_peci,
	SHELL_CMD_ARG(c0, NULL, "peci c0", peci_command, 1, 0),
	SHELL_CMD_ARG(c1, NULL, "peci c1 arg0", peci_command, 2, 0),
	SHELL_CMD_ARG(c2, NULL, "peci c2 arg0 arg1", peci_command, 3, 0),
	SHELL_CMD_ARG(cfg, NULL, "peci cfg arg0 arg1 arg2", peci_command, 4, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(peci, &sub_peci, "PECI validation commands", NULL);
