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
#include <soc_dt.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#if defined(CONFIG_SOC_SERIES_NPCK3)
#define PIN_INPUT 1
#elif defined(CONFIG_SOC_SERIES_NPCX4)
#define PIN_INPUT 0
#endif
struct test_pin_t {
	uint8_t port;
	uint8_t bit;
	uint8_t iotype;
};

/* Test IO result */
enum {
	INPUT_HIGH_FAIL,
	INPUT_LOW_FAIL,
	OUTPUT_HIGH_FAIL,
	OUTPUT_LOW_FAIL,
	INT_PU_FAIL,
	INT_PD_FAIL,
	OD_OUTPUT_HIGH_FAIL,
	OD_OUTPUT_LOW_FAIL,
	IO_PASS,
	IO_RESULT_COUNT,
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
	{ GPIO_GROUP_1, 2, IOTYPE_IO },			\
	{ GPIO_GROUP_1, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_1, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_1, 5, IOTYPE_IO },         \
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
	{ GPIO_GROUP_8, 6, IOTYPE_OUTPUT_ONLY },         \
	{ GPIO_GROUP_8, 7, IOTYPE_IO },         \
	{ GPIO_GROUP_9, 0, IOTYPE_IO },			\
	{ GPIO_GROUP_9, 1, IOTYPE_IO },			\
	{ GPIO_GROUP_9, 2, IOTYPE_IO },			\
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
	{ GPIO_GROUP_D, 0, IOTYPE_IO },			\
	{ GPIO_GROUP_D, 1, IOTYPE_IO },			\
	{ GPIO_GROUP_D, 2, IOTYPE_IO },			\
	{ GPIO_GROUP_D, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_D, 6, IOTYPE_IO },         \
	{ GPIO_GROUP_D, 7, IOTYPE_OUTPUT_ONLY },         \
	{ GPIO_GROUP_E, 0, IOTYPE_IO },			\
	{ GPIO_GROUP_E, 1, IOTYPE_IO },			\
	{ GPIO_GROUP_E, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_E, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_E, 7, IOTYPE_IO },         \
	{ GPIO_GROUP_F, 0, IOTYPE_IO },			\
	{ GPIO_GROUP_F, 1, IOTYPE_IO },			\
	{ GPIO_GROUP_F, 2, IOTYPE_IO },			\
	{ GPIO_GROUP_F, 3, IOTYPE_IO },         \
	{ GPIO_GROUP_F, 4, IOTYPE_IO },         \
	{ GPIO_GROUP_F, 5, IOTYPE_IO },         \
}

#define UART1_IO_TABLE { \
	{ GPIO_GROUP_1, 0, IOTYPE_IO },			\
	{ GPIO_GROUP_1, 1, IOTYPE_IO },			\
}

#define JTAG0_IO_TABLE { \
	{ GPIO_GROUP_1, 6, IOTYPE_IO },			\
	{ GPIO_GROUP_1, 7, IOTYPE_IO },			\
	{ GPIO_GROUP_2, 0, IOTYPE_IO },			\
	{ GPIO_GROUP_2, 1, IOTYPE_IO },			\
}

#define JTAG1_IO_TABLE { \
	{ GPIO_GROUP_D, 4, IOTYPE_IO },			\
	{ GPIO_GROUP_D, 5, IOTYPE_IO },			\
	{ GPIO_GROUP_E, 2, IOTYPE_IO },			\
	{ GPIO_GROUP_E, 5, IOTYPE_IO },			\
}

#define FLM_IO_TABLE { \
	{ GPIO_GROUP_9, 3, IOTYPE_IO },			\
	{ GPIO_GROUP_9, 6, IOTYPE_IO },			\
	{ GPIO_GROUP_A, 0, IOTYPE_IO },			\
	{ GPIO_GROUP_A, 2, IOTYPE_IO },			\
	{ GPIO_GROUP_A, 4, IOTYPE_IO },			\
	{ GPIO_GROUP_A, 7, IOTYPE_IO },			\
}

#define PVU_IO_TABLE { \
	{ GPIO_GROUP_9, 4, IOTYPE_IO },			\
	{ GPIO_GROUP_9, 5, IOTYPE_IO },			\
	{ GPIO_GROUP_9, 7, IOTYPE_IO },			\
	{ GPIO_GROUP_A, 1, IOTYPE_IO },			\
	{ GPIO_GROUP_A, 3, IOTYPE_IO },			\
	{ GPIO_GROUP_B, 0, IOTYPE_IO },			\
}
#endif

const struct device *const dev_in = DEVICE_DT_GET(DT_NODELABEL(gpio0));
struct gpio_callback gpio_cb;
struct k_event gpio_test_event;

enum {
	NONE = 0,
	GPIO_SYNC = 1,
	GPIO_TEST_COUNT,
};

int cmd_type = NONE;
static struct test_pin_t io_pins[] = GPIO_PINIO_TABLE;
static const uint16_t sz_io_pins = ARRAY_SIZE(io_pins);

const char io_name[GPIO_GROUP_COUNT][5] = {
	"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
	"A", "B", "C", "D", "E", "F", "G", "H",
	"STB0", "STB1"
};
uint8_t io_results[IO_RESULT_COUNT-1][ARRAY_SIZE(io_pins)];

static void gpio_isr(const struct device *port, struct gpio_callback *cb,
			  gpio_port_pins_t pins)
{
	int evt = cmd_type;

	k_event_post(&gpio_test_event, evt);
}

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

static int gpio_config_init(void)
{
	int ret;

	ret = gpio_pin_configure(dev_in, PIN_INPUT, GPIO_INPUT);
	if (ret) {
		LOG_ERR("Configure input fail.\n");
		return ret;
	}

	ret = gpio_pin_interrupt_configure(dev_in, PIN_INPUT, GPIO_INT_EDGE_FALLING);
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
	return ret;
}

static void gpio_print_io_report(void)
{
	int io;

	LOG_INF("other GPIO results:\n");
	for (io = 0; io < sz_io_pins; io++) {

		if (io_pins[io].iotype != IOTYPE_INPUT_ONLY) {
			if (io_results[OUTPUT_HIGH_FAIL][io] == IO_PASS) {
				LOG_INF("[PASS][GPIO] IO%s%01X Write High",
						io_name[io_pins[io].port], io_pins[io].bit);
			} else {
				LOG_ERR("[FAIL][GPIO] IO%s%01X Write High",
						io_name[io_pins[io].port], io_pins[io].bit);
			}
			if (io_results[OUTPUT_LOW_FAIL][io] == IO_PASS) {
				LOG_INF("[PASS][GPIO] IO%s%01X Write LOW",
						io_name[io_pins[io].port], io_pins[io].bit);
			} else {
				LOG_ERR("[FAIL][GPIO] IO%s%01X Write LOW",
						io_name[io_pins[io].port], io_pins[io].bit);
			}
		}

		if (io_pins[io].iotype != IOTYPE_OUTPUT_ONLY) {
			if (io_results[INPUT_HIGH_FAIL][io] == IO_PASS) {
				LOG_INF("[PASS][GPIO] IO%s%01X Read High",
						io_name[io_pins[io].port], io_pins[io].bit);
			} else {
				LOG_ERR("[FAIL][GPIO] IO%s%01X Read High",
						io_name[io_pins[io].port], io_pins[io].bit);
			}

			if (io_results[INPUT_LOW_FAIL][io] == IO_PASS) {
				LOG_INF("[PASS][GPIO] IO%s%01X Read Low",
						io_name[io_pins[io].port], io_pins[io].bit);
			} else {
				LOG_ERR("[FAIL][GPIO] IO%s%01X Read Low",
						io_name[io_pins[io].port], io_pins[io].bit);
			}
			if (io_results[INT_PU_FAIL][io] == IO_PASS) {
				LOG_INF("[PASS][GPIO] IO%s%01X Pull-Up",
						io_name[io_pins[io].port], io_pins[io].bit);
			} else {
				LOG_ERR("[FAIL][GPIO] IO%s%01X Pull-Up",
						io_name[io_pins[io].port], io_pins[io].bit);
			}

			if (io_results[INT_PD_FAIL][io] == IO_PASS) {
				LOG_INF("[PASS][GPIO] IO%s%01X Pull-Down",
						io_name[io_pins[io].port], io_pins[io].bit);
			} else {
				LOG_ERR("[FAIL][GPIO] IO%s%01X Pull-Down",
						io_name[io_pins[io].port], io_pins[io].bit);
			}
		}

		if (io_pins[io].iotype == IOTYPE_IO) {
			if (io_results[OD_OUTPUT_LOW_FAIL][io] == IO_PASS) {
				LOG_INF("[PASS][GPIO] IO%s%01X Open-Drain/PU ",
						io_name[io_pins[io].port], io_pins[io].bit);
			} else {
				LOG_ERR("[FAIL][GPIO] IO%s%01X Open-Drain/PU ",
						io_name[io_pins[io].port], io_pins[io].bit);
			}

			if (io_results[OD_OUTPUT_HIGH_FAIL][io] == IO_PASS) {
				LOG_INF("[PASS][GPIO] IO%s%01X Open-Drain/PD ",
						io_name[io_pins[io].port], io_pins[io].bit);
			} else {
				LOG_ERR("[FAIL][GPIO] IO%s%01X Open-Drain/PD ",
						io_name[io_pins[io].port], io_pins[io].bit);
			}
		}

		k_sleep(K_MSEC(500));
	}
}

static void auto_gpio_thread_entry(void)
{
	k_event_init(&gpio_test_event);
	int ret;
	int gpio_val;
	uint32_t io = 0;

	LOG_INF("auto gpio thread entry");
	
	while (true) {
		switch (k_event_wait(&gpio_test_event, 0xFFF, true, K_FOREVER)) {
		case GPIO_SYNC:
			gpio_remove_callback(dev_in, &gpio_cb);
			if (io_pins[io].iotype != IOTYPE_OUTPUT_ONLY) {

				ret = gpio_pin_configure(gpio_objs[io_pins[io].port].dev,
					io_pins[io].bit, GPIO_INPUT);
				if (ret)
					LOG_ERR("[FAIL]configure GPIO_INPUT error: %d", ret);
				k_sleep(K_MSEC(500));
				gpio_val = gpio_pin_get_raw(gpio_objs[io_pins[io].port].dev,
					io_pins[io].bit);
				if (gpio_val == 1) {
					io_results[INPUT_HIGH_FAIL][io] = IO_PASS;
				} else {
					io_results[INPUT_HIGH_FAIL][io] = INPUT_HIGH_FAIL;
				}

				k_sleep(K_MSEC(1000));
				gpio_val = gpio_pin_get_raw(gpio_objs[io_pins[io].port].dev,
					io_pins[io].bit);
				if (gpio_val == 0) {
					io_results[INPUT_LOW_FAIL][io] = IO_PASS;
				} else {
					io_results[INPUT_LOW_FAIL][io] = INPUT_LOW_FAIL;
				}
			}

			if (io_pins[io].iotype != IOTYPE_INPUT_ONLY) {

				/* Case 3: Test write high */
				ret = gpio_pin_configure(gpio_objs[io_pins[io].port].dev,
					io_pins[io].bit,
					GPIO_OUTPUT_HIGH);
				if (ret)
					LOG_ERR("[FAIL]configure GPIO_OUTPUT_HIGH error: %d", ret);

				k_sleep(K_MSEC(1000));
				gpio_val = gpio_pin_get_raw(gpio_objs[io_pins[io].port].dev,
					io_pins[io].bit);
				if (gpio_val == 1) {
					io_results[OUTPUT_HIGH_FAIL][io] = IO_PASS;
				} else {
					io_results[OUTPUT_HIGH_FAIL][io] = INPUT_LOW_FAIL;
				}

				/* Case 4: Test write low */
				ret = gpio_pin_configure(gpio_objs[io_pins[io].port].dev,
					io_pins[io].bit,
					GPIO_OUTPUT_LOW);
				if (ret)
					LOG_ERR("[FAIL]configure GPIO_OUTPUT_LOW error: %d", ret);

				k_sleep(K_MSEC(1000));
				gpio_val = gpio_pin_get_raw(gpio_objs[io_pins[io].port].dev,
					io_pins[io].bit);
				if (gpio_val == 0) {
					io_results[OUTPUT_LOW_FAIL][io] = IO_PASS;
				} else {
					io_results[OUTPUT_LOW_FAIL][io] = INPUT_LOW_FAIL;
				}
			}

			if (io_pins[io].iotype != IOTYPE_OUTPUT_ONLY) {

				/* Case 5: Test PU */
				ret = gpio_pin_configure(gpio_objs[io_pins[io].port].dev,
					io_pins[io].bit,
					GPIO_INPUT | GPIO_PULL_UP);
				if (ret)
					LOG_ERR("[FAIL]configure GPIO_PULL_UP error: %d\n", ret);

				k_sleep(K_MSEC(1000));
				gpio_val = gpio_pin_get_raw(gpio_objs[io_pins[io].port].dev,
					io_pins[io].bit);
				if (gpio_val == 1) {
					io_results[INT_PU_FAIL][io] = IO_PASS;
				} else {
					io_results[INT_PU_FAIL][io] = INT_PU_FAIL;
				}
#if defined(CONFIG_SOC_SERIES_NPCK3)
				/* GPIO70 */
				if (io == 46)
					k_sleep(K_MSEC(1500));
#endif
				/* Case 6: Test PD */
				ret = gpio_pin_configure(gpio_objs[io_pins[io].port].dev,
					io_pins[io].bit,
					GPIO_INPUT | GPIO_PULL_DOWN);
				if (ret)
					LOG_ERR("[FAIL]configure GPIO_PULL_DOWN error: %d\n", ret);

				k_sleep(K_MSEC(1000));
				gpio_val = gpio_pin_get_raw(gpio_objs[io_pins[io].port].dev,
					io_pins[io].bit);
				if (gpio_val == 0) {
					io_results[INT_PD_FAIL][io] = IO_PASS;
				} else {
					io_results[INT_PD_FAIL][io] = INT_PD_FAIL;
				}
			}

			if (io_pins[io].iotype == IOTYPE_IO) {

				/* Case 7: Test OD/UP */
				ret = gpio_pin_configure(gpio_objs[io_pins[io].port].dev,
					io_pins[io].bit,
					GPIO_OUTPUT_INIT_HIGH | GPIO_OPEN_DRAIN | GPIO_PULL_UP);
				if (ret)
					LOG_ERR("[FAIL]configure Open-Drain/PU error: %d", ret);

				k_sleep(K_MSEC(1000));
				gpio_val = gpio_pin_get_raw(gpio_objs[io_pins[io].port].dev,
					io_pins[io].bit);
				if (gpio_val == 1) {
					io_results[OD_OUTPUT_HIGH_FAIL][io] = IO_PASS;
				} else {
					io_results[OD_OUTPUT_HIGH_FAIL][io] = OD_OUTPUT_HIGH_FAIL;
				}

				/* Case 8: Test OD/DOWN*/
				ret = gpio_pin_configure(gpio_objs[io_pins[io].port].dev,
					io_pins[io].bit,
					GPIO_OUTPUT_INIT_HIGH | GPIO_OPEN_DRAIN | GPIO_PULL_DOWN);
				if (ret)
					LOG_ERR("[FAIL]configure Open-Drain/PD error: %d", ret);

				k_sleep(K_MSEC(1000));
				gpio_val = gpio_pin_get_raw(gpio_objs[io_pins[io].port].dev,
					io_pins[io].bit);
				if (gpio_val == 0) {
					io_results[OD_OUTPUT_LOW_FAIL][io] = IO_PASS;
				} else {
					io_results[OD_OUTPUT_LOW_FAIL][io] = OD_OUTPUT_LOW_FAIL;
				}
			}
			io++;

			if (io < sz_io_pins) {
				LOG_INF("[%d]GPIO test keep going!!", io);
				ret = gpio_pin_configure(dev_in, PIN_INPUT, GPIO_INPUT);
				if (ret)
					LOG_ERR("Configure input fail.\n");

				ret = gpio_pin_interrupt_configure(dev_in, PIN_INPUT,
					GPIO_INT_EDGE_FALLING);
				if (ret)
					LOG_ERR("Configure interrupt fail.\n");

				gpio_init_callback(&gpio_cb, gpio_isr, BIT(PIN_INPUT));
				ret = gpio_add_callback(dev_in, &gpio_cb);
				if (ret)
					LOG_ERR("Configure callback fail.\n");
			} else {
			#if defined(CONFIG_SOC_SERIES_NPCK3)
				*(volatile uint8_t*)(0x400C3011) = 0x06;
			#elif defined(CONFIG_SOC_SERIES_NPCX4)
				/*Enable UART1_2*/
				*(volatile uint8_t*)(0x400C3022) = 0x0C;
			#endif
				gpio_print_io_report();
				LOG_INF("GPIO test finish!!");
				LOG_INF("[GO]\r\n");
			}
		break;
		default:
		break;
		}
	}
}

#define NPCX_EPURST_CTL_EPUR2_EN	3
#define NPCX_HIF_TYPE_LPC	1
/* Main entry */
void main(void)
{
	LOG_INF("--- CI20 Zephyr GPIO Driver Validation ---");
	int ret;
	cmd_type = GPIO_SYNC;
#if defined(CONFIG_SOC_SERIES_NPCK3)
	/*Disable EPUR2 and UART*/
	struct glue_reg *inst_glue = (struct glue_reg *)(NPCX_GLUE_REG_ADDR);
	inst_glue->EPURST_CTL &= ~BIT(NPCX_EPURST_CTL_EPUR2_EN);
	*(volatile uint8_t*)(0x400C3011) = 0x00;
#elif defined(CONFIG_SOC_SERIES_NPCX4)/*ToDo pinctl*/
	/*Select VHIF 3.3V*/
	/*Register addr: 400c3000, bit set 0xD4*/
	struct scfg_reg *inst_scfg = (struct scfg_reg *)(NPCX_SCFG_REG_ADDR);

	SET_FIELD(inst_scfg->DEVCNT, NPCX_DEVCNT_HIF_TYP_SEL_FIELD, NPCX_HIF_TYPE_LPC);

	/*No LPC or eSPI interface select*/
	*(volatile uint8_t*)(0x400C3011) = 0x90;
	/*No Vcc1 Rst*/
	*(volatile uint8_t*)(0x400C301A) = 0x53;
	/*Disable UART1_2*/
	*(volatile uint8_t*)(0x400C3022) = 0x00;
#endif

	ret = gpio_config_init();
	if (ret) {
		LOG_ERR("GPIO Configurefail.\n");
	}
	auto_gpio_thread_entry();
}