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

#define PIN_OUTPUT  5
#define PIN_INPUT   7
#define PIN_OBSERVE 6

const struct device *const dev_out = DEVICE_DT_GET(DT_NODELABEL(gpioa));
const struct device *const dev_in = DEVICE_DT_GET(DT_NODELABEL(gpioc));
struct gpio_callback gpio_cb;

#define CALL_ITEMS_SIZE 7
const struct device *const dev_dummy = DEVICE_DT_GET(DT_NODELABEL(gpioc));
struct gpio_callback gpio_dummy[8];

static void gpio_isr(const struct device *port, struct gpio_callback *cb,
			  gpio_port_pins_t pins)
{
	/* drive observe pin to low */
	gpio_pin_set_raw(dev_out, PIN_OBSERVE, 0);

	LOG_INF("%08x is pressed", pins);
	/* drive observe pin to high */
	gpio_pin_set_raw(dev_out, PIN_OBSERVE, 1);
}

static int gpio_command(const struct shell *shell, size_t argc, char **argv)
{
	int ret;

	ret = gpio_pin_configure(dev_out, PIN_OUTPUT, GPIO_OUTPUT_HIGH);
	if (ret) {
		LOG_ERR("Configure output fail.\n");
		return ret;
	}

	ret = gpio_pin_configure(dev_out, PIN_OBSERVE, GPIO_OUTPUT_HIGH);
	if (ret) {
		LOG_ERR("Configure observe fail.\n");
		return ret;
	}

	ret = gpio_pin_configure(dev_in, PIN_INPUT, GPIO_INPUT);
	if (ret) {
		LOG_ERR("Configure input fail.\n");
		return ret;
	}

	ret = gpio_pin_interrupt_configure(dev_in, PIN_INPUT, GPIO_INT_EDGE_RISING);
	if (ret) {
		LOG_ERR("Configure interrupt fail.\n");
		return ret;
	}

	gpio_init_callback(&gpio_cb, gpio_isr, BIT(PIN_INPUT));
	ret = gpio_add_callback(dev_in, &gpio_cb);
	if (ret) {
		LOG_ERR("Configure callback fail.\n");
		return ret;
	}

	/* Add more callbacks */
	for (int pin = 0; pin < CALL_ITEMS_SIZE; pin++) {
		gpio_pin_configure(dev_dummy, pin, GPIO_INPUT | GPIO_PULL_UP);
		gpio_pin_interrupt_configure(dev_dummy, pin, GPIO_INT_EDGE_RISING);
		gpio_init_callback(&gpio_dummy[pin], gpio_isr, BIT(pin));
		gpio_add_callback(dev_dummy, &gpio_dummy[pin]);
	}

	/* drive to low */
	gpio_pin_set_raw(dev_out, PIN_OUTPUT, 0);

	/* drive to high */
	gpio_pin_set_raw(dev_out, PIN_OUTPUT, 1);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_gpio,
	SHELL_CMD_ARG(go, NULL, "gpio go", gpio_command, 1, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(gpio, &sub_gpio, "gpio validation commands", NULL);
