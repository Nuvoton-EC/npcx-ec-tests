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
#include <soc.h>


LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

enum {
	FALIING_FAIL,
	RISING_FAIL,
	ANY_FALLING_FAIL,
	ANY_RISING_FAIL,
	LOW_LEVEL_FAIL,
	HIGH_LEVEL_FAIL,
	MIWU_PASS,
	MIWU_RESULT_COUNT,
};

enum{
	ARG_TYPE = 0,
	ARG_GROUP,
	ARG_PIN,
	ARG_ACTION,
};

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
#elif defined(CONFIG_SOC_SERIES_NPCX4)
#define GPIO_PINIO_TABLE { \
	{ GPIO_GROUP_0, 0, IOTYPE_IO },			\
	{ GPIO_GROUP_0, 1, IOTYPE_IO },			\
	{ GPIO_GROUP_0, 2, IOTYPE_IO },			\
	{ GPIO_GROUP_0, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_0, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_0, 5, IOTYPE_IO },         \
	{ GPIO_GROUP_0, 6, IOTYPE_IO },         \
	{ GPIO_GROUP_0, 7, IOTYPE_IO },         \
	{ GPIO_GROUP_1, 0, IOTYPE_IO },			\
	{ GPIO_GROUP_1, 1, IOTYPE_IO },			\
	{ GPIO_GROUP_1, 2, IOTYPE_IO },			\
	{ GPIO_GROUP_1, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_1, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_1, 5, IOTYPE_IO },         \
	{ GPIO_GROUP_1, 6, IOTYPE_IO },         \
	{ GPIO_GROUP_1, 7, IOTYPE_IO },         \
	{ GPIO_GROUP_2, 0, IOTYPE_IO },			\
	{ GPIO_GROUP_2, 1, IOTYPE_IO },			\
	{ GPIO_GROUP_2, 2, IOTYPE_IO },			\
	{ GPIO_GROUP_2, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_2, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_2, 5, IOTYPE_IO },         \
	{ GPIO_GROUP_2, 6, IOTYPE_IO },         \
	{ GPIO_GROUP_2, 7, IOTYPE_IO },         \
	{ GPIO_GROUP_3, 0, IOTYPE_IO },			\
	{ GPIO_GROUP_3, 1, IOTYPE_IO },			\
	{ GPIO_GROUP_3, 2, IOTYPE_OUTPUT_ONLY },			\
	{ GPIO_GROUP_3, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_3, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_3, 5, IOTYPE_OUTPUT_ONLY },         \
	{ GPIO_GROUP_3, 6, IOTYPE_IO },         \
	{ GPIO_GROUP_3, 7, IOTYPE_IO },         \
	{ GPIO_GROUP_4, 0, IOTYPE_IO },			\
	{ GPIO_GROUP_4, 1, IOTYPE_IO },			\
	{ GPIO_GROUP_4, 2, IOTYPE_IO },			\
	{ GPIO_GROUP_4, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_4, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_4, 5, IOTYPE_IO },         \
	{ GPIO_GROUP_4, 6, IOTYPE_IO },         \
	{ GPIO_GROUP_4, 7, IOTYPE_IO },         \
	{ GPIO_GROUP_5, 0, IOTYPE_IO },			\
	{ GPIO_GROUP_5, 1, IOTYPE_IO },			\
	{ GPIO_GROUP_5, 2, IOTYPE_IO },			\
	{ GPIO_GROUP_5, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_5, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_5, 5, IOTYPE_IO },         \
	{ GPIO_GROUP_5, 6, IOTYPE_IO },         \
	{ GPIO_GROUP_5, 7, IOTYPE_IO },         \
	{ GPIO_GROUP_6, 0, IOTYPE_IO },			\
	{ GPIO_GROUP_6, 1, IOTYPE_IO },			\
	{ GPIO_GROUP_6, 2, IOTYPE_IO },			\
	{ GPIO_GROUP_6, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_6, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_6, 5, IOTYPE_OUTPUT_ONLY },         \
	{ GPIO_GROUP_6, 6, IOTYPE_IO },         \
	{ GPIO_GROUP_6, 7, IOTYPE_IO },         \
	{ GPIO_GROUP_7, 0, IOTYPE_IO },			\
	{ GPIO_GROUP_7, 2, IOTYPE_IO },			\
	{ GPIO_GROUP_7, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_7, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_7, 5, IOTYPE_IO },         \
	{ GPIO_GROUP_7, 6, IOTYPE_IO },         \
	{ GPIO_GROUP_7, 7, IOTYPE_OUTPUT_ONLY },         \
	{ GPIO_GROUP_8, 0, IOTYPE_IO },			\
	{ GPIO_GROUP_8, 1, IOTYPE_IO },			\
	{ GPIO_GROUP_8, 2, IOTYPE_IO },			\
	{ GPIO_GROUP_8, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_8, 5, IOTYPE_OUTPUT_ONLY },         \
	{ GPIO_GROUP_8, 6, IOTYPE_OUTPUT_ONLY },         \
	{ GPIO_GROUP_8, 7, IOTYPE_IO },         \
	{ GPIO_GROUP_9, 0, IOTYPE_IO },			\
	{ GPIO_GROUP_9, 1, IOTYPE_IO },			\
	{ GPIO_GROUP_9, 2, IOTYPE_IO },			\
	{ GPIO_GROUP_9, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_9, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_9, 5, IOTYPE_IO },         \
	{ GPIO_GROUP_9, 6, IOTYPE_IO },         \
	{ GPIO_GROUP_A, 7, IOTYPE_IO },         \
	{ GPIO_GROUP_A, 0, IOTYPE_IO },			\
	{ GPIO_GROUP_A, 1, IOTYPE_IO },			\
	{ GPIO_GROUP_A, 2, IOTYPE_IO },			\
	{ GPIO_GROUP_A, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_A, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_A, 5, IOTYPE_IO },         \
	{ GPIO_GROUP_A, 6, IOTYPE_IO },         \
	{ GPIO_GROUP_A, 7, IOTYPE_IO },         \
	{ GPIO_GROUP_B, 1, IOTYPE_IO },			\
	{ GPIO_GROUP_B, 2, IOTYPE_IO },			\
	{ GPIO_GROUP_B, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_B, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_B, 5, IOTYPE_IO },         \
	{ GPIO_GROUP_B, 6, IOTYPE_OUTPUT_ONLY },         \
	{ GPIO_GROUP_B, 7, IOTYPE_IO },         \
	{ GPIO_GROUP_C, 0, IOTYPE_IO },			\
	{ GPIO_GROUP_C, 1, IOTYPE_IO },			\
	{ GPIO_GROUP_C, 2, IOTYPE_IO },			\
	{ GPIO_GROUP_C, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_C, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_C, 5, IOTYPE_IO },         \
	{ GPIO_GROUP_C, 6, IOTYPE_IO },         \
	{ GPIO_GROUP_C, 7, IOTYPE_IO },         \
	{ GPIO_GROUP_D, 1, IOTYPE_IO },			\
	{ GPIO_GROUP_D, 2, IOTYPE_IO },			\
	{ GPIO_GROUP_D, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_D, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_D, 5, IOTYPE_IO },         \
	{ GPIO_GROUP_D, 6, IOTYPE_IO },         \
	{ GPIO_GROUP_D, 7, IOTYPE_OUTPUT_ONLY },         \
	{ GPIO_GROUP_D, 0, IOTYPE_IO },			\
	{ GPIO_GROUP_E, 1, IOTYPE_IO },			\
	{ GPIO_GROUP_E, 2, IOTYPE_IO },			\
	{ GPIO_GROUP_E, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_E, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_E, 5, IOTYPE_IO },         \
	{ GPIO_GROUP_E, 7, IOTYPE_IO },         \
	{ GPIO_GROUP_F, 0, IOTYPE_IO },			\
	{ GPIO_GROUP_F, 1, IOTYPE_IO },			\
	{ GPIO_GROUP_F, 2, IOTYPE_IO },			\
	{ GPIO_GROUP_F, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_F, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_F, 5, IOTYPE_IO },         \
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

const struct device *t_dev;
uint8_t t_pin;

uint8_t parser_gpio_group(const char *name)
{
	if (!strcmp(name, "gpio0"))
		return GPIO_GROUP_0;
	else if (!strcmp(name, "gpio1"))
		return GPIO_GROUP_1;
	else if (!strcmp(name, "gpio2"))
		return GPIO_GROUP_2;
	else if (!strcmp(name, "gpio3"))
		return GPIO_GROUP_3;
	else if (!strcmp(name, "gpio4"))
		return GPIO_GROUP_4;
	else if (!strcmp(name, "gpio5"))
		return GPIO_GROUP_5;
	else if (!strcmp(name, "gpio6"))
		return GPIO_GROUP_6;
	else if (!strcmp(name, "gpio7"))
		return GPIO_GROUP_7;
	else if (!strcmp(name, "gpio8"))
		return GPIO_GROUP_8;
	else if (!strcmp(name, "gpio9"))
		return GPIO_GROUP_9;
	else if (!strcmp(name, "gpioa"))
		return GPIO_GROUP_A;
	else if (!strcmp(name, "gpiob"))
		return GPIO_GROUP_B;
	else if (!strcmp(name, "gpioc"))
		return GPIO_GROUP_C;
	else if (!strcmp(name, "gpiod"))
		return GPIO_GROUP_D;
	else if (!strcmp(name, "gpioe"))
		return GPIO_GROUP_E;
	else if (!strcmp(name, "gpiof"))
		return GPIO_GROUP_F;
	else if (!strcmp(name, "gpiog"))
		return GPIO_GROUP_G;
	else if (!strcmp(name, "gpioh"))
		return GPIO_GROUP_H;
	else if (!strcmp(name, "gpiostb0"))
		return GPIO_GROUP_STB0;
	else if (!strcmp(name, "gpiostb1"))
		return GPIO_GROUP_STB1;
	else
		return 0xFF;
}

struct gpio_callback gpio_cb;
int miwu_type = FALIING_FAIL;
uint8_t miwu_results[MIWU_RESULT_COUNT-1][GPIO_GROUP_COUNT][8];
uint8_t t_group;

static void gpio_isr(const struct device *port, struct gpio_callback *cb,
			  gpio_port_pins_t pins)
{
	int pin_number = 0;

	while (pins != 0) {
		pins = pins >> 1;
		pin_number++;
	}
	t_group = parser_gpio_group(port->name);
	miwu_results[miwu_type][t_group][pin_number-1] = MIWU_PASS;
}

/* Main entry */
int main(void)
{
	LOG_INF("--- CI20 Zephyr GPIO Driver Validation ---");
	LOG_INF("Usage: gpio [operation type] [group] [pin] [action]");
	LOG_INF("operation type:\r\n\t-i: input\r\n\t\tno need to use action\r\n"
		"\t-o: output\r\n\t\taction: -ol(output low), -oh(output high)\r\n"
		"\t-pp: push-pull \r\n\t\taction: -u(pull up), -d(pull down)\r\n"
		"\t-od: open-drain \r\n\t\taction: -u(pull up), -d(pull down)\r\n"
		"\t-m: miwu and -ret: miwu result\r\n\t\taction:\r\n\t\t"
		"\t-f(falling)\r\n\t\t\t-r(rising)\r\n"
		"\t\t\t-af(any falling)\r\n\t\t\t-ar(any rising)\r\n"
		"\t\t\t-ll(low level)\r\n\t\t\t-hl(high level)\r\n");

	/* Let main thread go to sleep state */
	k_sleep(K_FOREVER);

	return 0;
}

static int gpio_input_test(const struct shell *shell, size_t argc, char **argv)
{
	int pin_in_val;
	int ret;
	uint32_t gpio_group;

	gpio_group = strtoul(argv[ARG_GROUP], NULL, 0);
	gpio_objs[gpio_group].pin = strtoul(argv[ARG_PIN], NULL, 0);
	gpio_objs[gpio_group].flags = GPIO_INPUT;

	t_dev = gpio_objs[gpio_group].dev;
	t_pin = gpio_objs[gpio_group].pin;

	ret = gpio_pin_configure(t_dev, t_pin, gpio_objs[gpio_group].flags);
	if (ret != 0) {
		LOG_ERR("failed to configure to %d\n", gpio_objs[gpio_group].flags);
		return -EIO;
	}

	pin_in_val = gpio_pin_get_raw(t_dev, t_pin);
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

	t_dev = gpio_objs[gpio_group].dev;
	t_pin = gpio_objs[gpio_group].pin;

	if (!strcmp(argv[ARG_TYPE], "-o") && !strcmp(argv[ARG_ACTION], "-oh"))
		gpio_objs[gpio_group].flags = GPIO_OUTPUT_HIGH;
	else if (!strcmp(argv[ARG_TYPE], "-o") && !strcmp(argv[ARG_ACTION], "-ol"))
		gpio_objs[gpio_group].flags = GPIO_OUTPUT_LOW;
	else {
		LOG_ERR("Please enter the correct parameters");
		return -EINVAL;
	}

	ret = gpio_pin_configure(t_dev, t_pin, gpio_objs[gpio_group].flags);
	if (ret != 0) {
		LOG_ERR("failed to configure to %d\n", gpio_objs[gpio_group].flags);
		return -EIO;
	}
	k_sleep(K_MSEC(500));
	pin_val = gpio_pin_get_raw(t_dev, t_pin);
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

	t_dev = gpio_objs[gpio_group].dev;
	t_pin = gpio_objs[gpio_group].pin;

	if (!strcmp(argv[ARG_TYPE], "-pp") && !strcmp(argv[ARG_ACTION], "-u"))
		gpio_objs[gpio_group].flags = (GPIO_INPUT | GPIO_PULL_UP);
	else if (!strcmp(argv[ARG_TYPE], "-pp") && !strcmp(argv[ARG_ACTION], "-d"))
		gpio_objs[gpio_group].flags = (GPIO_INPUT | GPIO_PULL_DOWN);
	else {
		LOG_ERR("Please enter the correct parameters");
		return -EINVAL;
	}

	ret = gpio_pin_configure(t_dev, t_pin, gpio_objs[gpio_group].flags);
	if (ret != 0) {
		LOG_ERR("failed to configure to %d\n", gpio_objs[gpio_group].flags);
		return -EIO;
	}
	k_sleep(K_MSEC(500));
	pin_val = gpio_pin_get_raw(t_dev, t_pin);
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

	t_dev = gpio_objs[gpio_group].dev;
	t_pin = gpio_objs[gpio_group].pin;

	if (!strcmp(argv[ARG_TYPE], "-od") && !strcmp(argv[ARG_ACTION], "-u"))
		gpio_objs[gpio_group].flags = (GPIO_OUTPUT_INIT_HIGH | GPIO_OPEN_DRAIN | GPIO_PULL_UP);
	else if (!strcmp(argv[ARG_TYPE], "-od") && !strcmp(argv[ARG_ACTION], "-d"))
		gpio_objs[gpio_group].flags = (GPIO_OUTPUT_INIT_HIGH | GPIO_OPEN_DRAIN | GPIO_PULL_DOWN);
	else {
		LOG_ERR("Please enter the correct parameters");
		return -EINVAL;
	}

	ret = gpio_pin_configure(t_dev, t_pin, gpio_objs[gpio_group].flags);
	if (ret != 0) {
		LOG_ERR("failed to configure to %d\n", gpio_objs[gpio_group].flags);
		return -EIO;
	}
	k_sleep(K_MSEC(500));
	pin_val = gpio_pin_get_raw(t_dev, t_pin);
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

static int gpio_miwu_test(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t gpio_group;
	uint32_t flag;

	gpio_group = strtoul(argv[ARG_GROUP], NULL, 0);
	gpio_objs[gpio_group].pin = strtoul(argv[ARG_PIN], NULL, 0);
	flag = strtoul(argv[ARG_ACTION], NULL, 0);
	t_dev = gpio_objs[gpio_group].dev;
	t_pin = gpio_objs[gpio_group].pin;

	if (!strcmp(argv[ARG_ACTION], "-f")) {
		miwu_type = FALIING_FAIL;
		miwu_results[FALIING_FAIL][gpio_group][t_pin] = FALIING_FAIL;
		gpio_pin_configure(t_dev, t_pin, GPIO_INPUT | GPIO_PULL_UP);
		gpio_init_callback(&gpio_cb, gpio_isr, BIT(t_pin));
		gpio_pin_interrupt_configure(t_dev, t_pin, GPIO_INT_EDGE_FALLING);
		gpio_add_callback(t_dev, &gpio_cb);
	} else if (!strcmp(argv[ARG_ACTION], "-r")) {
		miwu_type = RISING_FAIL;
		miwu_results[RISING_FAIL][gpio_group][t_pin] = RISING_FAIL;
		gpio_pin_configure(t_dev, t_pin, GPIO_INPUT | GPIO_PULL_DOWN);
		gpio_init_callback(&gpio_cb, gpio_isr, BIT(t_pin));
		gpio_pin_interrupt_configure(t_dev, t_pin, GPIO_INT_EDGE_RISING);
		gpio_add_callback(t_dev, &gpio_cb);
	} else if (!strcmp(argv[ARG_ACTION], "-af")) {
		miwu_type = ANY_FALLING_FAIL;
		miwu_results[ANY_FALLING_FAIL][gpio_group][t_pin] = ANY_FALLING_FAIL;
		gpio_pin_configure(t_dev, t_pin, GPIO_INPUT | GPIO_PULL_UP);
		gpio_init_callback(&gpio_cb, gpio_isr, BIT(t_pin));
		gpio_pin_interrupt_configure(t_dev, t_pin, GPIO_INT_EDGE_BOTH);
		gpio_add_callback(t_dev, &gpio_cb);
	} else if (!strcmp(argv[ARG_ACTION], "-ar")) {
		miwu_type = ANY_RISING_FAIL;
		miwu_results[ANY_RISING_FAIL][gpio_group][t_pin] = ANY_RISING_FAIL;
		gpio_pin_configure(t_dev, t_pin, GPIO_INPUT | GPIO_PULL_DOWN);
		gpio_init_callback(&gpio_cb, gpio_isr, BIT(t_pin));
		gpio_pin_interrupt_configure(t_dev, t_pin, GPIO_INT_EDGE_BOTH);
		gpio_add_callback(t_dev, &gpio_cb);
	} else if (!strcmp(argv[ARG_ACTION], "-ll")) {
		miwu_type = LOW_LEVEL_FAIL;
		miwu_results[LOW_LEVEL_FAIL][gpio_group][t_pin] = LOW_LEVEL_FAIL;
		gpio_pin_configure(t_dev, t_pin, GPIO_INPUT | GPIO_PULL_UP);
		gpio_init_callback(&gpio_cb, gpio_isr, BIT(t_pin));
		gpio_pin_interrupt_configure(t_dev, t_pin, GPIO_INT_LEVEL_LOW);
		gpio_add_callback(t_dev, &gpio_cb);
	} else if (!strcmp(argv[ARG_ACTION], "-hl")) {
		miwu_type = HIGH_LEVEL_FAIL;
		miwu_results[HIGH_LEVEL_FAIL][gpio_group][t_pin] = HIGH_LEVEL_FAIL;
		gpio_pin_configure(t_dev, t_pin, GPIO_INPUT | GPIO_PULL_DOWN);
		gpio_init_callback(&gpio_cb, gpio_isr, BIT(t_pin));
		gpio_pin_interrupt_configure(t_dev, t_pin, GPIO_INT_LEVEL_HIGH);
		gpio_add_callback(t_dev, &gpio_cb);
	} else {
		LOG_ERR("Please enter the correct parameters");
		return -EINVAL;
	}

	LOG_INF("Test %s-%d MIWU", t_dev->name, t_pin);
	return 0;
}

static int miwu_IOresult(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t gpio_group;
	uint32_t flag;

	gpio_group = strtoul(argv[ARG_GROUP], NULL, 0);
	gpio_objs[gpio_group].pin = strtoul(argv[ARG_PIN], NULL, 0);
	flag = strtoul(argv[ARG_ACTION], NULL, 0);
	t_dev = gpio_objs[gpio_group].dev;
	t_pin = gpio_objs[gpio_group].pin;

	if (!strcmp(argv[ARG_ACTION], "-f")) {
		if (miwu_results[FALIING_FAIL][t_group][t_pin] == MIWU_PASS) {
			LOG_INF("[PASS][MIWU] %d-%d Falling Edge Event",
				gpio_group, t_pin);
		} else {
			LOG_ERR("[FAIL][MIWU] %d-%d Falling Edge Event",
				gpio_group, t_pin);
		}
	} else if (!strcmp(argv[ARG_ACTION], "-r")) {
		if (miwu_results[RISING_FAIL][t_group][t_pin] == MIWU_PASS) {
			LOG_INF("[PASS][MIWU] %d-%d Rising Edge Event",
				gpio_group, t_pin);
		} else {
			LOG_ERR("[FAIL][MIWU] %d-%d Rising Edge Event",
				gpio_group, t_pin);
		}
	} else if (!strcmp(argv[ARG_ACTION], "-af")) {
		if (miwu_results[ANY_FALLING_FAIL][t_group][t_pin] == MIWU_PASS) {
			LOG_INF("[PASS][MIWU] %d-%d Any Falling Edge Event",
				gpio_group, t_pin);
		} else {
			LOG_ERR("[FAIL][MIWU] %d-%d Any Falling Edge Event",
				gpio_group, t_pin);
		}
	} else if (!strcmp(argv[ARG_ACTION], "-ar")) {
		if (miwu_results[ANY_RISING_FAIL][t_group][t_pin] == MIWU_PASS) {
			LOG_INF("[PASS][MIWU] %d-%d Any Rising Edge Event",
				gpio_group, t_pin);
		} else {
			LOG_ERR("[FAIL][MIWU] %d-%d Any Rising Edge Event",
				gpio_group, t_pin);
		}
	} else if (!strcmp(argv[ARG_ACTION], "-ll")) {
		if (miwu_results[LOW_LEVEL_FAIL][t_group][t_pin] == MIWU_PASS) {
			LOG_INF("[PASS][MIWU] %d-%d Low level Event",
				gpio_group, t_pin);
		} else {
			LOG_ERR("[FAIL][MIWU] %d-%d Low level Event",
				gpio_group, t_pin);
		}
	} else if (!strcmp(argv[ARG_ACTION], "-hl")) {
		if (miwu_results[HIGH_LEVEL_FAIL][t_group][t_pin] == MIWU_PASS) {
			LOG_INF("[PASS][MIWU] %d-%d High level Event",
				gpio_group, t_pin);
		} else {
			LOG_ERR("[FAIL][MIWU] %d-%d High level Event",
				gpio_group, t_pin);
		}
	} else {
		LOG_ERR("Please enter the correct parameters");
		return -EINVAL;
	}

	return 0;
}

#define NPCX_HIF_TYPE_LPC 1
static int gpio_test_on_cofing(const struct shell *shell, size_t argc, char **argv)
{
#if defined(CONFIG_SOC_SERIES_NPCX4)
	/*Select VHIF 3.3V*/
	/*Register addr: 400c3000, bit set 0xD4*/
	struct scfg_reg *inst_scfg = (struct scfg_reg *)(NPCX_SCFG_REG_ADDR);

	SET_FIELD(inst_scfg->DEVCNT, NPCX_DEVCNT_HIF_TYP_SEL_FIELD, NPCX_HIF_TYPE_LPC);
	/*No LPC or eSPI interface select and set GPIO72*/
	*(volatile uint8_t*)(0x400C3011) = 0x90;
	/*Set GPIO77*/
	*(volatile uint8_t*)(0x400C301A) = 0x53;
	/*Disable JTAG*/
	*(volatile uint8_t*)(0x400C3120) = 0x66;
#elif defined(CONFIG_SOC_SERIES_NPCK3)
	struct glue_reg *inst_glue = (struct glue_reg *)(NPCX_GLUE_REG_ADDR);

	inst_glue->EPURST_CTL &= ~BIT(NPCX_EPURST_CTL_EPUR2_EN);
#endif
	return 0;
}

static int gpio_test_off_cofing(const struct shell *shell, size_t argc, char **argv)
{
#if defined(CONFIG_SOC_SERIES_NPCX4)
	/*Set Vcc1 Rst*/
	*(volatile uint8_t*)(0x400C301A) = 0x43;
	/*Enable JTAG*/
	*(volatile uint8_t*)(0x400C3120) = 0x69;
#endif
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_gpio,
	SHELL_CMD_ARG(-i, NULL, "gpio input", gpio_input_test, 3, 0),
	SHELL_CMD_ARG(-o, NULL, "gpio output", gpio_output_test, 4, 0),
	SHELL_CMD_ARG(-pp, NULL, "gpio push-pull", gpio_pupd_test, 4, 0),
	SHELL_CMD_ARG(-od, NULL, "gpio open-drain", gpio_open_drain_test, 4, 0),
	SHELL_CMD_ARG(level, NULL, "gpio level", gpio_switch_1V8_level, 1, 0),
	SHELL_CMD_ARG(on, NULL, "gpio on", gpio_test_on_cofing, 1, 0),
	SHELL_CMD_ARG(off, NULL, "gpio off", gpio_test_off_cofing, 1, 0),
	SHELL_CMD_ARG(-m, NULL, "gpio miwu", gpio_miwu_test, 4, 0),
	SHELL_CMD_ARG(-ret, NULL, "gpio miwu result", miwu_IOresult, 4, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(gpio, &sub_gpio, "gpio validation commands", NULL);
