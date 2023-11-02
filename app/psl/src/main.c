/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#define LOG_LEVEL LOG_LEVEL_INF
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#define TASK_STACK_SIZE	1024
#define PRIORITY	7
#define MAX_ARGUMNETS	3
#define MAX_ARGU_SIZE	10




/* Soc specific system local functions */
#define PSL_OUT_GPIO_DRIVEN	0
#define PSL_OUT_FW_CTRL_DRIVEN	1
#define PINCTRL_STATE_HIBERNATE 1

#define PSL_NODE DT_INST(0, nuvoton_npcx_power_psl)
PINCTRL_DT_DEFINE(PSL_NODE);

static struct k_thread temp_id;
K_THREAD_STACK_DEFINE(temp_stack, TASK_STACK_SIZE);
static uint8_t arguments[MAX_ARGUMNETS][MAX_ARGU_SIZE];
struct k_event psl_event;

static int cros_system_npcx_configure_psl_in(void)
{
	const struct pinctrl_dev_config *pcfg =
		PINCTRL_DT_DEV_CONFIG_GET(PSL_NODE);
	return pinctrl_apply_state(pcfg, PINCTRL_STATE_DEFAULT);
}

static void cros_system_npcx_psl_out_inactive(void)
{
#if (DT_ENUM_IDX(PSL_NODE, psl_driven_type) == PSL_OUT_GPIO_DRIVEN)
	struct gpio_dt_spec enable = GPIO_DT_SPEC_GET(PSL_NODE, enable_gpios);

	gpio_pin_set_dt(&enable, 1);
#elif (DT_ENUM_IDX(PSL_NODE, psl_driven_type) == PSL_OUT_FW_CTRL_DRIVEN)

	LOG_INF("PINCTRL_STATE_HIBERNATE\n");
	const struct pinctrl_dev_config *pcfg =
		PINCTRL_DT_DEV_CONFIG_GET(PSL_NODE);

	pinctrl_apply_state(pcfg, PINCTRL_STATE_HIBERNATE);
#endif
}

static void npcx_psl_init(void)
{
	int ret;

	/* Configure detection settings of PSL_IN pads first */
	ret = cros_system_npcx_configure_psl_in();

	if (ret < 0) {
		LOG_ERR("PSL_IN pinctrl setup failed (%d)", ret);
		return;
	}
}

static void npcx_psl_test(void)
{
	int ret;

	/* Configure detection settings of PSL_IN pads first */
	ret = cros_system_npcx_configure_psl_in();

	if (ret < 0) {
		LOG_ERR("PSL_IN pinctrl setup failed (%d)", ret);
		return;
	}

	/*
	 * A transition from 0 to 1 of specific IO (GPIO85) data-out bit
	 * set PSL_OUT to inactive state. Then, it will turn Core Domain
	 * power supply (VCC1) off for better power consumption.
	 */
	cros_system_npcx_psl_out_inactive();
}

static void psl_validation_func(void *dummy1, void *dummy2, void *dummy3)
{
	uint32_t events;

	k_event_init(&psl_event);

	LOG_INF("PSL hook success\n");
	while (true) {
		events = k_event_wait(&psl_event, 0xFFF, true, K_FOREVER);
		switch (events) {
		case 0x001: /* no argu */
			break;
		case 0x002:
			if (!strcmp("init", arguments[0])) {
				npcx_psl_init();
				LOG_INF("PSL module init\n");
			}
			if (!strcmp("test", arguments[0])) {
				npcx_psl_test();
				LOG_INF("PSL module init\n");
			}
			break;
		case 0x004:
			break;
		default:
			LOG_INF("Error command\n");
		}
	}
}

int main(void)
{
	/* Zephyr driver validation */
	LOG_INF("Start PSL Validation Task\n");
	k_thread_create(&temp_id, temp_stack, TASK_STACK_SIZE, psl_validation_func,
		NULL, NULL, NULL, PRIORITY, K_INHERIT_PERMS, K_FOREVER);
	k_thread_name_set(&temp_id, "PSL Validation");
	k_thread_start(&temp_id);

	return 0;
}

static int psl_command(const struct shell *shell, size_t argc, char **argv)
{
	int i, evt;

	evt = 1;
	for (evt = 1, i = 1; i < argc; i++) {
		strcpy(arguments[i - 1], argv[i]);
		evt <<= 1;
	}
	k_event_post(&psl_event, evt);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_psl, SHELL_CMD_ARG(c0, NULL, "psl c0", psl_command, 1, 0),
	SHELL_CMD_ARG(c1, NULL, "psl c1 arg0", psl_command, 2, 0),
	SHELL_CMD_ARG(c2, NULL, "psl c2 arg0 arg1", psl_command, 3, 0),
	SHELL_CMD_ARG(cfg, NULL, "psl cfg arg0 arg1 arg2", psl_command, 4, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(psl, &sub_psl, "nuvoton psl validation", NULL);
