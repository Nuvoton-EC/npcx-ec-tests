/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <stdlib.h>

/* Target drivers for testing */
#include <zephyr/drivers/pwm.h>

#define LOG_LEVEL LOG_LEVEL_INF
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test_pwm);

#define ARG_CHANNEL 1
#define ARG_PERIOD  2
#define ARG_PLUSE   3
#define ARG_FLAGS   4

/* Objects used for validation */
struct pwm_object {
	const struct device *dev;
	const char *label;
	/* test parameters */
	uint32_t period;
	uint32_t pulse;
	uint32_t flags;
};

/* define your DT nodes' compatible string here */
#define NPCX_PWM_OBJS_INIT(id) {		\
		.dev = DEVICE_DT_GET(id),	\
		.label = DT_PROP(id, label),	\
		.period = 0,			\
		.pulse = 0,			\
		.flags = 0,			\
	},

static struct pwm_object pwm_objs[] = {
	DT_FOREACH_STATUS_OKAY(nuvoton_npcx_pwm, NPCX_PWM_OBJS_INIT)
};

static int pwm_test_set_cycles(uint32_t channel, uint32_t period, uint32_t pulse, pwm_flags_t flags)
{
	/* Record setting */
	pwm_objs[channel].period = period;
	pwm_objs[channel].pulse = pulse;
	pwm_objs[channel].flags = flags;

	LOG_INF("Device_Set : %s, Channel : %d", pwm_objs[channel].label, channel);
	if (pwm_set_cycles(pwm_objs[channel].dev, channel, period, pulse, flags)) {
		LOG_ERR("Fail to set the period and pulse width\n");
		return -EIO;
	}

	return 0;
}

static int pwm_cmd_get_cycles_per_sec(const struct shell *shell, size_t argc, char **argv)
{
	uint64_t cycles_per_sec;
	uint32_t channel = strtoul(argv[ARG_CHANNEL], NULL, 0);

	if (pwm_get_cycles_per_sec(pwm_objs[channel].dev, channel, &cycles_per_sec)){
		LOG_ERR("Fail to get cycles per sec\n");
		return -EIO;
	}
	LOG_INF("Cycles per sec: %lld", cycles_per_sec);
	return 0;
}

static int pwm_cmd_set(const struct shell *shell, size_t argc, char **argv)
{
	pwm_flags_t flags = 0;
	uint32_t period;
	uint32_t pulse;

	/* Convert integer from string */
	period = strtoul(argv[ARG_PERIOD], NULL, 0);
	pulse = strtoul(argv[ARG_PLUSE], NULL, 0);
	if (argc == (ARG_FLAGS + 1)) {
		flags = strtoul(argv[ARG_FLAGS], NULL, 0);
	}

	if (!strcmp(argv[ARG_CHANNEL], "auto")) {
		/* Set all channels with the same cycles */
		for (int i = 0; i < ARRAY_SIZE(pwm_objs); i++) {
			pwm_test_set_cycles(i, period, pulse, flags);
		}
	} else {
		uint32_t channel = strtoul(argv[ARG_CHANNEL], NULL, 0);

		if (channel >= ARRAY_SIZE(pwm_objs)) {
			return -EINVAL;
		}
		pwm_test_set_cycles(channel, period, pulse, flags);

	}

	return 0;
}

static int pwm_cmd_list(const struct shell *shell, size_t argc, char **argv)
{
	printk("chan\tname\tperiod\tpulse\tflags\n");
	for (int i = 0; i < ARRAY_SIZE(pwm_objs); i++) {
		printk("%d\t%s\t%d\t%d\t%d\n", i,
					pwm_objs[i].label,
					pwm_objs[i].period,
					pwm_objs[i].pulse,
					pwm_objs[i].flags);
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_pwm,
	SHELL_CMD_ARG(list, NULL, "pwm list - show pwm status", pwm_cmd_list, 0, 0),
	SHELL_CMD_ARG(set, NULL, "pwm set <auto/chan> <period> <pulse> <flags>",
			pwm_cmd_set, 5, 0),
	SHELL_CMD_ARG(get, NULL, "pwm get <chan> - cycles per sec ", pwm_cmd_get_cycles_per_sec, 2, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(pwm, &sub_pwm, "pwm validation commands", NULL);

/* Custom PM policy handler to forbid ec enter deep sleep if CONFIG_PM is enabled */
const struct pm_state_info *pm_policy_next_state(uint8_t cpu, int32_t ticks)
{
	ARG_UNUSED(cpu);

	return NULL;
}
