/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/gpio/nuvoton-npcx-gpio.h>
#include <stdlib.h>


LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define ARG_TYPE 0
#define ARG_GROUP  1
#define ARG_PIN   2
#define ARG_ACTION   3

struct test_pin_t {
	uint8_t port;
	uint8_t bit;
	uint8_t iotype;
};

/* Test IO Type */

enum {
	IOTYPE_IO,
	IOTYPE_INPUT_ONLY,
	IOTYPE_OUTPUT_ONLY,
};

enum {
	GPIO_GROUP_0 = 0,
	GPIO_GROUP_1,
	GPIO_GROUP_2,
	GPIO_GROUP_3,
	GPIO_GROUP_4,
	GPIO_GROUP_5,
	GPIO_GROUP_6,
	GPIO_GROUP_7,
	GPIO_GROUP_8,
	GPIO_GROUP_9,
	GPIO_GROUP_A,
	GPIO_GROUP_B,
	GPIO_GROUP_C,
	GPIO_GROUP_D,
	GPIO_GROUP_E,
	GPIO_GROUP_F,
	GPIO_GROUP_G,
	GPIO_GROUP_H,
	GPIO_GROUP_STB0,
	GPIO_GROUP_STB1,
	GPIO_GROUP_COUNT,
} GPIO_GROUP_T;

#if defined(CONFIG_SOC_SERIES_NPCK3)
#define GPIO_PINIO_TABLE { \
	{ GPIO_GROUP_0, 1, IOTYPE_IO },			\
	{ GPIO_GROUP_0, 2, IOTYPE_IO },			\
	{ GPIO_GROUP_0, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_0, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_0, 5, IOTYPE_IO },         \
	{ GPIO_GROUP_0, 7, IOTYPE_IO },         \
	{ GPIO_GROUP_1, 0, IOTYPE_IO },         \
	{ GPIO_GROUP_1, 1, IOTYPE_IO },         \
	{ GPIO_GROUP_1, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_1, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_1, 5, IOTYPE_IO },         \
	{ GPIO_GROUP_1, 7, IOTYPE_IO },         \
	{ GPIO_GROUP_2, 0, IOTYPE_IO },         \
	{ GPIO_GROUP_2, 1, IOTYPE_IO },         \
	{ GPIO_GROUP_2, 2, IOTYPE_IO },         \
	{ GPIO_GROUP_2, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_2, 5, IOTYPE_IO },         \
	{ GPIO_GROUP_2, 6, IOTYPE_IO },         \
	{ GPIO_GROUP_2, 7, IOTYPE_IO },         \
	{ GPIO_GROUP_3, 0, IOTYPE_IO },         \
	{ GPIO_GROUP_3, 1, IOTYPE_IO },         \
	{ GPIO_GROUP_3, 2, IOTYPE_IO },         \
	{ GPIO_GROUP_3, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_3, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_3, 6, IOTYPE_IO },         \
	{ GPIO_GROUP_4, 0, IOTYPE_IO },         \
	{ GPIO_GROUP_4, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_4, 5, IOTYPE_IO },         \
	{ GPIO_GROUP_4, 6, IOTYPE_IO },         \
	{ GPIO_GROUP_4, 7, IOTYPE_IO },         \
	{ GPIO_GROUP_5, 0, IOTYPE_IO },         \
	{ GPIO_GROUP_5, 1, IOTYPE_IO },         \
	{ GPIO_GROUP_5, 2, IOTYPE_IO },         \
	{ GPIO_GROUP_5, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_5, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_5, 5, IOTYPE_IO },         \
	{ GPIO_GROUP_5, 6, IOTYPE_IO },         \
	{ GPIO_GROUP_5, 7, IOTYPE_IO },         \
	{ GPIO_GROUP_6, 0, IOTYPE_IO },         \
	{ GPIO_GROUP_6, 1, IOTYPE_IO },         \
	{ GPIO_GROUP_6, 2, IOTYPE_IO },         \
	{ GPIO_GROUP_6, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_6, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_6, 5, IOTYPE_IO },         \
	{ GPIO_GROUP_6, 6, IOTYPE_IO },         \
	{ GPIO_GROUP_6, 7, IOTYPE_IO },         \
	{ GPIO_GROUP_7, 0, IOTYPE_INPUT_ONLY }, \
	{ GPIO_GROUP_7, 2, IOTYPE_IO },         \
	{ GPIO_GROUP_7, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_7, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_7, 5, IOTYPE_IO },         \
	{ GPIO_GROUP_7, 6, IOTYPE_IO },         \
	{ GPIO_GROUP_7, 7, IOTYPE_IO },         \
	{ GPIO_GROUP_8, 1, IOTYPE_IO },         \
	{ GPIO_GROUP_8, 3, IOTYPE_IO },			\
	{ GPIO_GROUP_8, 5, IOTYPE_IO },         \
	{ GPIO_GROUP_8, 6, IOTYPE_IO },         \
	{ GPIO_GROUP_8, 7, IOTYPE_IO },			\
	{ GPIO_GROUP_9, 0, IOTYPE_IO },         \
	{ GPIO_GROUP_9, 1, IOTYPE_IO },         \
	{ GPIO_GROUP_9, 2, IOTYPE_IO },         \
	{ GPIO_GROUP_9, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_9, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_A, 0, IOTYPE_IO },         \
	{ GPIO_GROUP_A, 1, IOTYPE_IO },         \
	{ GPIO_GROUP_A, 2, IOTYPE_IO },         \
	{ GPIO_GROUP_A, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_A, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_A, 5, IOTYPE_IO },         \
	{ GPIO_GROUP_A, 6, IOTYPE_IO },         \
	{ GPIO_GROUP_A, 7, IOTYPE_IO },         \
	{ GPIO_GROUP_B, 0, IOTYPE_IO },         \
	{ GPIO_GROUP_B, 1, IOTYPE_IO },         \
	{ GPIO_GROUP_B, 2, IOTYPE_IO },         \
	{ GPIO_GROUP_B, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_B, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_B, 5, IOTYPE_IO },         \
	{ GPIO_GROUP_B, 6, IOTYPE_IO },         \
	{ GPIO_GROUP_B, 7, IOTYPE_IO },         \
	{ GPIO_GROUP_C, 0, IOTYPE_IO },         \
	{ GPIO_GROUP_C, 1, IOTYPE_IO },         \
	{ GPIO_GROUP_C, 2, IOTYPE_IO },         \
	{ GPIO_GROUP_C, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_C, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_C, 5, IOTYPE_IO },         \
	{ GPIO_GROUP_C, 6, IOTYPE_IO },         \
	{ GPIO_GROUP_C, 7, IOTYPE_IO },         \
	{ GPIO_GROUP_D, 0, IOTYPE_IO },         \
	{ GPIO_GROUP_D, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_D, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_D, 5, IOTYPE_IO },         \
	{ GPIO_GROUP_D, 6, IOTYPE_IO },         \
	{ GPIO_GROUP_D, 7, IOTYPE_IO },         \
	{ GPIO_GROUP_E, 0, IOTYPE_IO },         \
	{ GPIO_GROUP_E, 1, IOTYPE_IO },         \
	{ GPIO_GROUP_E, 2, IOTYPE_IO },         \
	{ GPIO_GROUP_E, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_E, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_E, 5, IOTYPE_IO },         \
	{ GPIO_GROUP_E, 6, IOTYPE_IO },         \
	{ GPIO_GROUP_F, 0, IOTYPE_IO },         \
	{ GPIO_GROUP_F, 6, IOTYPE_IO },         \
	{ GPIO_GROUP_F, 7, IOTYPE_IO },         \
	{ GPIO_GROUP_G, 5, IOTYPE_IO },         \
	{ GPIO_GROUP_G, 6, IOTYPE_IO },         \
	{ GPIO_GROUP_G, 7, IOTYPE_IO },         \
	{ GPIO_GROUP_H, 0, IOTYPE_IO },         \
	{ GPIO_GROUP_H, 1, IOTYPE_IO },         \
	{ GPIO_GROUP_H, 2, IOTYPE_IO },         \
	{ GPIO_GROUP_H, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_STB0, 0, IOTYPE_IO },      \
	{ GPIO_GROUP_STB0, 1, IOTYPE_IO },      \
	{ GPIO_GROUP_STB0, 2, IOTYPE_IO },      \
	{ GPIO_GROUP_STB0, 3, IOTYPE_IO },      \
	{ GPIO_GROUP_STB0, 4, IOTYPE_IO },      \
	{ GPIO_GROUP_STB1, 1, IOTYPE_IO },      \
	{ GPIO_GROUP_STB1, 2, IOTYPE_IO },      \
	{ GPIO_GROUP_STB1, 3, IOTYPE_IO },      \
	{ GPIO_GROUP_STB1, 4, IOTYPE_IO },      \
	{ GPIO_GROUP_STB1, 5, IOTYPE_IO },      \
	{ GPIO_GROUP_STB1, 6, IOTYPE_IO },      \
	{ GPIO_GROUP_STB1, 7, IOTYPE_OUTPUT_ONLY },        \
}
#endif

static struct test_pin_t io_pins[] = GPIO_PINIO_TABLE;
static const uint16_t sz_io_pins = ARRAY_SIZE(io_pins);
const char io_name[GPIO_GROUP_COUNT][5] = {
	"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
	"A", "B", "C", "D", "E", "F", "G", "H",
	"STB0", "STB1"
};
struct gpio_object {
	const struct device *dev;
	const char *label;
	/* test parameters */
	gpio_pin_t pin;
	gpio_flags_t flags;
};

/* define your DT nodes' compatible string here */
#define NPCX_GPIO_OBJS_INIT(id) {		\
		.dev = DEVICE_DT_GET(id),	\
		.label = DT_PROP(id, label),		\
		.pin = 0,			\
		.flags = 0,			\
	},

static struct gpio_object gpio_objs[] = {
	DT_FOREACH_STATUS_OKAY(nuvoton_npcx_gpio, NPCX_GPIO_OBJS_INIT)
};

static int gpio_input_test(const struct shell *shell, size_t argc, char **argv)
{
	int pin_in_val;
	int ret;
	uint32_t gpio_group;

	gpio_group = strtoul(argv[ARG_GROUP], NULL, 0);
	gpio_objs[gpio_group].pin = strtoul(argv[ARG_PIN], NULL, 0);
	gpio_objs[gpio_group].flags = GPIO_INPUT;
	ret = gpio_pin_configure(gpio_objs[gpio_group].dev,
							gpio_objs[gpio_group].pin,
							gpio_objs[gpio_group].flags);
	if (ret != 0) {
		LOG_ERR("failed to configure to %d\n", gpio_objs[gpio_group].flags);
		return -EIO;
	}

	pin_in_val = gpio_pin_get_raw(gpio_objs[gpio_group].dev, gpio_objs[gpio_group].pin);
	LOG_INF("Get value: %d", pin_in_val);

	return 0;
}

static int gpio_output_test(const struct shell *shell, size_t argc, char **argv)
{
	int pin_val;
	int ret;
	uint32_t gpio_group;

	gpio_group = strtoul(argv[ARG_GROUP], NULL, 0);
	gpio_objs[gpio_group].pin = strtoul(argv[ARG_PIN], NULL, 0);


	if (!strcmp(argv[ARG_TYPE], "output") && !strcmp(argv[ARG_ACTION], "high"))
		gpio_objs[gpio_group].flags = GPIO_OUTPUT_HIGH;
	else if (!strcmp(argv[ARG_TYPE], "output") && !strcmp(argv[ARG_ACTION], "low"))
		gpio_objs[gpio_group].flags = GPIO_OUTPUT_LOW;
	else {
		LOG_ERR("Please enter the correct parameters");
		return -EINVAL;
	}

	ret = gpio_pin_configure(gpio_objs[gpio_group].dev,
							gpio_objs[gpio_group].pin,
							gpio_objs[gpio_group].flags);
	if (ret != 0) {
		LOG_ERR("failed to configure to %d\n", gpio_objs[gpio_group].flags);
		return -EIO;
	}
	k_sleep(K_MSEC(1000));
	pin_val = gpio_pin_get_raw(gpio_objs[gpio_group].dev, gpio_objs[gpio_group].pin);
	LOG_INF("Get value: %d", pin_val);

	return 0;
}

static int gpio_pupd_test(const struct shell *shell, size_t argc, char **argv)
{
	int pin_val;
	int ret;
	uint32_t gpio_group;

	gpio_group = strtoul(argv[ARG_GROUP], NULL, 0);
	gpio_objs[gpio_group].pin = strtoul(argv[ARG_PIN], NULL, 0);


	if (!strcmp(argv[ARG_TYPE], "pupd") && !strcmp(argv[ARG_ACTION], "pu"))
		gpio_objs[gpio_group].flags = (GPIO_INPUT | GPIO_PULL_UP);
	else if (!strcmp(argv[ARG_TYPE], "pupd") && !strcmp(argv[ARG_ACTION], "pd"))
		gpio_objs[gpio_group].flags = (GPIO_INPUT | GPIO_PULL_DOWN);
	else {
		LOG_ERR("Please enter the correct parameters");
		return -EINVAL;
	}

	ret = gpio_pin_configure(gpio_objs[gpio_group].dev,
							gpio_objs[gpio_group].pin,
							gpio_objs[gpio_group].flags);
	if (ret != 0) {
		LOG_ERR("failed to configure to %d\n", gpio_objs[gpio_group].flags);
		return -EIO;
	}
	k_sleep(K_MSEC(1000));
	pin_val = gpio_pin_get_raw(gpio_objs[gpio_group].dev, gpio_objs[gpio_group].pin);
	LOG_INF("Get value: %d", pin_val);

	return 0;
}

static int gpio_open_drain_test(const struct shell *shell, size_t argc, char **argv)
{
	int pin_val;
	int ret;
	uint32_t gpio_group;

	gpio_group = strtoul(argv[ARG_GROUP], NULL, 0);
	gpio_objs[gpio_group].pin = strtoul(argv[ARG_PIN], NULL, 0);

	if (!strcmp(argv[ARG_TYPE], "od") && !strcmp(argv[ARG_ACTION], "pu"))
		gpio_objs[gpio_group].flags = (GPIO_OUTPUT_INIT_HIGH | GPIO_OPEN_DRAIN | GPIO_PULL_UP);
	else if (!strcmp(argv[ARG_TYPE], "od") && !strcmp(argv[ARG_ACTION], "pd"))
		gpio_objs[gpio_group].flags = (GPIO_OUTPUT_INIT_HIGH | GPIO_OPEN_DRAIN | GPIO_PULL_DOWN);
	else {
		LOG_ERR("Please enter the correct parameters");
		return -EINVAL;
	}

	ret = gpio_pin_configure(gpio_objs[gpio_group].dev,
							gpio_objs[gpio_group].pin,
							gpio_objs[gpio_group].flags);
	if (ret != 0) {
		LOG_ERR("failed to configure to %d\n", gpio_objs[gpio_group].flags);
		return -EIO;
	}
	k_sleep(K_MSEC(1000));
	pin_val = gpio_pin_get_raw(gpio_objs[gpio_group].dev, gpio_objs[gpio_group].pin);
	LOG_INF("Get value: %d", pin_val);

	return 0;
}

static int gpio_switch_1V8_level(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	uint32_t io;

	for (io = 0; io < sz_io_pins; io++) {
		ret = gpio_pin_configure(gpio_objs[io_pins[io].port].dev,
								io_pins[io].bit,
								NPCX_GPIO_VOLTAGE_MASK);
		if (ret != 0) {
			LOG_ERR("IO%s%01X Switch 1.8V Fail",
				io_name[io_pins[io].port], io_pins[io].bit);
		}
	}

	return 0;
}

static int gpio_cmd_list(const struct shell *shell, size_t argc, char **argv)
{
	for (int i = 0; i < ARRAY_SIZE(gpio_objs); i++) {
		LOG_INF("group: %d,name: %s,pin:%d,flag:%d\n", i,
					gpio_objs[i].label,
					gpio_objs[i].pin,
					gpio_objs[i].flags);
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_gpio,
	SHELL_CMD_ARG(input, NULL, "gpio input <group> <pin>", gpio_input_test, 3, 0),
	SHELL_CMD_ARG(output, NULL, "gpio output <group> <pin> <high/low>", gpio_output_test, 4, 0),
	SHELL_CMD_ARG(pupd, NULL, "gpio pupd <group> <pin> <pu/pd>", gpio_pupd_test, 4, 0),
	SHELL_CMD_ARG(od, NULL, "gpio od <group> <pin> <pu/pd>", gpio_open_drain_test, 4, 0),
	SHELL_CMD_ARG(level, NULL, "gpio level", gpio_switch_1V8_level, 1, 0),
	SHELL_CMD_ARG(list, NULL, "gpio list", gpio_cmd_list, 1, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(gpio, &sub_gpio, "gpio validation commands", NULL);
