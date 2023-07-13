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

struct test_pin_t {
	uint8_t port;
	uint8_t bit;
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

/* MIWU Verification Table */
#define DFT_MIWU_IO_TABLE { \
	{ GPIO_GROUP_0, 1 }, \
	{ GPIO_GROUP_0, 2 }, \
	{ GPIO_GROUP_0, 3 }, \
	{ GPIO_GROUP_0, 4 }, \
	{ GPIO_GROUP_0, 5 }, \
	{ GPIO_GROUP_0, 7 }, \
	{ GPIO_GROUP_1, 0 }, \
	{ GPIO_GROUP_1, 1 }, \
	{ GPIO_GROUP_1, 3 }, \
	{ GPIO_GROUP_1, 4 }, \
	{ GPIO_GROUP_1, 5 }, \
	{ GPIO_GROUP_1, 7 }, \
	{ GPIO_GROUP_2, 0 }, \
	{ GPIO_GROUP_2, 1 }, \
	{ GPIO_GROUP_2, 2 }, \
	{ GPIO_GROUP_2, 3 }, \
	{ GPIO_GROUP_2, 5 }, \
	{ GPIO_GROUP_2, 6 }, \
	{ GPIO_GROUP_2, 7 }, \
	{ GPIO_GROUP_3, 0 }, \
	{ GPIO_GROUP_3, 1 }, \
	{ GPIO_GROUP_3, 2 }, \
	{ GPIO_GROUP_3, 3 }, \
	{ GPIO_GROUP_3, 4 }, \
	{ GPIO_GROUP_3, 6 }, \
	{ GPIO_GROUP_4, 0 }, \
	{ GPIO_GROUP_4, 4 }, \
	{ GPIO_GROUP_4, 5 }, \
	{ GPIO_GROUP_4, 6 }, \
	{ GPIO_GROUP_4, 7 }, \
	{ GPIO_GROUP_5, 0 }, \
	{ GPIO_GROUP_5, 1 }, \
	{ GPIO_GROUP_5, 2 }, \
	{ GPIO_GROUP_5, 3 }, \
	{ GPIO_GROUP_5, 4 }, \
	{ GPIO_GROUP_5, 5 }, \
	{ GPIO_GROUP_5, 6 }, \
	{ GPIO_GROUP_5, 7 }, \
	{ GPIO_GROUP_6, 0 }, \
	{ GPIO_GROUP_6, 1 }, \
	{ GPIO_GROUP_6, 2 }, \
	{ GPIO_GROUP_6, 3 }, \
	{ GPIO_GROUP_6, 4 }, \
	{ GPIO_GROUP_6, 5 }, \
	{ GPIO_GROUP_6, 6 }, \
	{ GPIO_GROUP_6, 7 }, \
	{ GPIO_GROUP_7, 0 }, \
	{ GPIO_GROUP_7, 2 }, \
	{ GPIO_GROUP_7, 3 }, \
	{ GPIO_GROUP_7, 4 }, \
	{ GPIO_GROUP_7, 5 }, \
	{ GPIO_GROUP_7, 6 }, \
	{ GPIO_GROUP_7, 7 }, \
	{ GPIO_GROUP_8, 1 }, \
	{ GPIO_GROUP_8, 3 }, \
	{ GPIO_GROUP_8, 5 }, \
	{ GPIO_GROUP_8, 6 }, \
	{ GPIO_GROUP_8, 7 }, \
	{ GPIO_GROUP_9, 0 }, \
	{ GPIO_GROUP_9, 1 }, \
	{ GPIO_GROUP_9, 2 }, \
	{ GPIO_GROUP_9, 3 }, \
	{ GPIO_GROUP_9, 4 }, \
	{ GPIO_GROUP_A, 0 }, \
	{ GPIO_GROUP_A, 1 }, \
	{ GPIO_GROUP_A, 2 }, \
	{ GPIO_GROUP_A, 3 }, \
	{ GPIO_GROUP_A, 4 }, \
	{ GPIO_GROUP_A, 5 }, \
	{ GPIO_GROUP_A, 6 }, \
	{ GPIO_GROUP_A, 7 }, \
	{ GPIO_GROUP_B, 0 }, \
	{ GPIO_GROUP_D, 0 }, \
	{ GPIO_GROUP_D, 3 }, \
	{ GPIO_GROUP_D, 4 }, \
	{ GPIO_GROUP_D, 5 }, \
	{ GPIO_GROUP_D, 6 }, \
	{ GPIO_GROUP_D, 7 }, \
	{ GPIO_GROUP_E, 0 }, \
	{ GPIO_GROUP_E, 1 }, \
	{ GPIO_GROUP_E, 2 }, \
	{ GPIO_GROUP_E, 3 }, \
	{ GPIO_GROUP_E, 4 }, \
	{ GPIO_GROUP_E, 5 }, \
	{ GPIO_GROUP_E, 6 }, \
	{ GPIO_GROUP_G, 5 }, \
	{ GPIO_GROUP_G, 6 }, \
	{ GPIO_GROUP_G, 7 }, \
	{ GPIO_GROUP_H, 0 }, \
	{ GPIO_GROUP_H, 1 }, \
	{ GPIO_GROUP_H, 2 }, \
	{ GPIO_GROUP_H, 4 }, \
	{ GPIO_GROUP_STB0, 0 }, \
	{ GPIO_GROUP_STB0, 1 }, \
	{ GPIO_GROUP_STB0, 2 }, \
	{ GPIO_GROUP_STB0, 3 }, \
	{ GPIO_GROUP_STB0, 4 }, \
	{ GPIO_GROUP_STB1, 1 }, \
	{ GPIO_GROUP_STB1, 2 }, \
	{ GPIO_GROUP_STB1, 3 }, \
	{ GPIO_GROUP_STB1, 4 }, \
	{ GPIO_GROUP_STB1, 5 }, \
	{ GPIO_GROUP_STB1, 6 }, \
}


static struct test_pin_t  miwu_io_pins[] = DFT_MIWU_IO_TABLE;
static const uint16_t sz_miwu_io_inputs   = ARRAY_SIZE(miwu_io_pins);

const char io_name[GPIO_GROUP_COUNT][5] = {
	"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
	"A", "B", "C", "D", "E", "F", "G", "H",
	"STB0", "STB1"
};

uint8_t miwu_results[MIWU_RESULT_COUNT-1][GPIO_GROUP_COUNT][8];

enum {
	MIWU_TEST_NONE = 0,
	MIWU_TEST_AUTO_FALLING = 1,
	MIWU_TEST_AUTO_RISING = 2,
	MIWU_TEST_AUTO_ANY_FALLING = 3,
	MIWU_TEST_AUTO_ANY_RISING = 4,
	MIWU_TEST_AUTO_LOW_LEVEL = 5,
	MIWU_TEST_AUTO_HIGH_LEVEL = 6,
	MIWU_TEST_ALL_CLEAR = 7,
	MIWU_TEST_REPORT = 8,
	MIWU_CMD_COUNT,
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
struct gpio_callback miwu_cb[ARRAY_SIZE(miwu_io_pins)];
struct k_event miwu_test_event;
uint32_t cmd_type = MIWU_TEST_NONE;
uint16_t io;
int default_raw_data[ARRAY_SIZE(miwu_io_pins)];


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

uint8_t t_group;
const struct device *t_dev;
uint8_t t_bit;

int miwu_type = FALIING_FAIL;

static void gpio_isr(const struct device *port, struct gpio_callback *cb,
			  gpio_port_pins_t pins)
{
	int pin_number = 0;
	uint32_t evt = cmd_type;

	while (pins != 0) {
		pins = pins >> 1;
		pin_number++;
	}
	t_group = parser_gpio_group(port->name);
	miwu_results[miwu_type][t_group][pin_number-1] = MIWU_PASS;
	if (t_group == GPIO_GROUP_STB1 && (pin_number-1) == 6) {
		k_event_post(&miwu_test_event, evt);
	}
}

static void auto_miwu_thread_entry(void)
{
	uint32_t events;

	k_event_init(&miwu_test_event);
	for (io = 0; io < sz_miwu_io_inputs; io++) {
		t_dev = gpio_objs[miwu_io_pins[io].port].dev;
		t_bit = miwu_io_pins[io].bit;
		t_group = parser_gpio_group(t_dev->name);

		default_raw_data[io] = gpio_pin_get_raw(t_dev, t_bit);
		miwu_results[FALIING_FAIL][t_group][t_bit] = FALIING_FAIL;
		gpio_pin_configure(t_dev, t_bit, GPIO_INPUT | GPIO_PULL_UP);
		gpio_pin_interrupt_configure(t_dev, t_bit, GPIO_INT_EDGE_FALLING);
		gpio_init_callback(&miwu_cb[io], gpio_isr, BIT(t_bit));
		gpio_add_callback(t_dev, &miwu_cb[io]);
	}
	while (true) {
		events = k_event_wait(&miwu_test_event, 0xFFF, true, K_FOREVER);
		switch (events) {
		case MIWU_TEST_AUTO_FALLING:
			miwu_type = RISING_FAIL;
			for (io = 0; io < sz_miwu_io_inputs; io++) {
				t_dev = gpio_objs[miwu_io_pins[io].port].dev;
				t_bit = miwu_io_pins[io].bit;
				t_group = parser_gpio_group(t_dev->name);
				miwu_results[RISING_FAIL][t_group][t_bit] = RISING_FAIL;
				gpio_pin_configure(t_dev, t_bit, GPIO_INPUT | GPIO_PULL_DOWN);
				gpio_pin_interrupt_configure(t_dev, t_bit, GPIO_INT_EDGE_RISING);
				gpio_init_callback(&miwu_cb[io], gpio_isr, BIT(t_bit));
				gpio_add_callback(t_dev, &miwu_cb[io]);
			}
			cmd_type = MIWU_TEST_AUTO_RISING;
		break;
		case MIWU_TEST_AUTO_RISING:
			miwu_type = ANY_FALLING_FAIL;
			for (io = 0; io < sz_miwu_io_inputs; io++) {
				t_dev = gpio_objs[miwu_io_pins[io].port].dev;
				t_bit = miwu_io_pins[io].bit;
				t_group = parser_gpio_group(t_dev->name);
				miwu_results[ANY_FALLING_FAIL][t_group][t_bit] = ANY_FALLING_FAIL;
				gpio_pin_configure(t_dev, t_bit, GPIO_INPUT | GPIO_PULL_UP);
				gpio_pin_interrupt_configure(t_dev, t_bit, GPIO_INT_EDGE_BOTH);
				gpio_init_callback(&miwu_cb[io], gpio_isr, BIT(t_bit));
				gpio_add_callback(t_dev, &miwu_cb[io]);
			}
			cmd_type = MIWU_TEST_AUTO_ANY_FALLING;
		break;
		case MIWU_TEST_AUTO_ANY_FALLING:
			miwu_type = ANY_RISING_FAIL;
			for (io = 0; io < sz_miwu_io_inputs; io++) {
				t_dev = gpio_objs[miwu_io_pins[io].port].dev;
				t_bit = miwu_io_pins[io].bit;
				t_group = parser_gpio_group(t_dev->name);
				miwu_results[ANY_RISING_FAIL][t_group][t_bit] = ANY_RISING_FAIL;
				gpio_pin_configure(t_dev, t_bit, GPIO_INPUT | GPIO_PULL_DOWN);
				gpio_pin_interrupt_configure(t_dev, t_bit, GPIO_INT_EDGE_BOTH);
				gpio_init_callback(&miwu_cb[io], gpio_isr, BIT(t_bit));
				gpio_add_callback(t_dev, &miwu_cb[io]);
			}
			cmd_type = MIWU_TEST_AUTO_ANY_RISING;
		break;
		case MIWU_TEST_AUTO_ANY_RISING:
			miwu_type = LOW_LEVEL_FAIL;
			for (io = 0; io < sz_miwu_io_inputs; io++) {
				t_dev = gpio_objs[miwu_io_pins[io].port].dev;
				t_bit = miwu_io_pins[io].bit;
				t_group = parser_gpio_group(t_dev->name);
				miwu_results[LOW_LEVEL_FAIL][t_group][t_bit] = LOW_LEVEL_FAIL;
				gpio_pin_configure(t_dev, t_bit, GPIO_INPUT | GPIO_PULL_UP);
				gpio_pin_interrupt_configure(t_dev, t_bit, GPIO_INT_LEVEL_LOW);
				gpio_init_callback(&miwu_cb[io], gpio_isr, BIT(t_bit));
				gpio_add_callback(t_dev, &miwu_cb[io]);
			}
			cmd_type = MIWU_TEST_AUTO_LOW_LEVEL;
		break;
		case MIWU_TEST_AUTO_LOW_LEVEL:
			miwu_type = HIGH_LEVEL_FAIL;
			for (io = 0; io < sz_miwu_io_inputs; io++) {
				t_dev = gpio_objs[miwu_io_pins[io].port].dev;
				t_bit = miwu_io_pins[io].bit;
				t_group = parser_gpio_group(t_dev->name);
				miwu_results[HIGH_LEVEL_FAIL][t_group][t_bit] = HIGH_LEVEL_FAIL;
				gpio_pin_configure(t_dev, t_bit, GPIO_INPUT | GPIO_PULL_DOWN);
				gpio_pin_interrupt_configure(t_dev, t_bit, GPIO_INT_LEVEL_HIGH);
				gpio_init_callback(&miwu_cb[io], gpio_isr, BIT(t_bit));
				gpio_add_callback(t_dev, &miwu_cb[io]);
			}
			cmd_type = MIWU_TEST_AUTO_HIGH_LEVEL;
		break;
		case MIWU_TEST_AUTO_HIGH_LEVEL:
			for (io = 0; io < sz_miwu_io_inputs; io++) {
				t_dev = gpio_objs[miwu_io_pins[io].port].dev;
				t_bit = miwu_io_pins[io].bit;
				t_group = parser_gpio_group(t_dev->name);

				if (default_raw_data[io])
					gpio_pin_set_raw(t_dev, t_bit, 1);
				else
					gpio_pin_set_raw(t_dev, t_bit, 0);
			}
			*(volatile uint8_t*)(0x400C3011) = 0x06;
			LOG_INF("MIWU Report");
			for (io = 0; io < sz_miwu_io_inputs; io++) {
				t_dev = gpio_objs[miwu_io_pins[io].port].dev;
				t_bit = miwu_io_pins[io].bit;
				t_group = parser_gpio_group(t_dev->name);
				if (miwu_results[FALIING_FAIL][t_group][t_bit] == MIWU_PASS) {
					LOG_INF("[PASS][MIWU] IO%s%01X Falling Edge Event",
						io_name[miwu_io_pins[io].port], t_bit);
				} else {
					LOG_ERR("[FAIL][MIWU] IO%s%01X Falling Edge Event",
						io_name[miwu_io_pins[io].port], t_bit);
				}
				k_sleep(K_MSEC(50));
			}

			for (io = 0; io < sz_miwu_io_inputs; io++) {
				t_dev = gpio_objs[miwu_io_pins[io].port].dev;
				t_bit = miwu_io_pins[io].bit;
				t_group = parser_gpio_group(t_dev->name);
				if (miwu_results[RISING_FAIL][t_group][t_bit] == MIWU_PASS) {
					LOG_INF("[PASS][MIWU] IO%s%01X Rising Edge Event",
						io_name[miwu_io_pins[io].port], t_bit);
				} else {
					LOG_ERR("[FAIL][MIWU] IO%s%01X Rising Edge Event",
						io_name[miwu_io_pins[io].port], t_bit);
				}
				k_sleep(K_MSEC(50));
			}

			for (io = 0; io < sz_miwu_io_inputs; io++) {
				t_dev = gpio_objs[miwu_io_pins[io].port].dev;
				t_bit = miwu_io_pins[io].bit;
				t_group = parser_gpio_group(t_dev->name);
				if (miwu_results[ANY_FALLING_FAIL][t_group][t_bit] == MIWU_PASS) {
					LOG_INF("[PASS][MIWU] IO%s%01X Any Falling Edge Event",
						io_name[miwu_io_pins[io].port], t_bit);
				} else {
					LOG_ERR("[FAIL][MIWU] IO%s%01X Any Falling Edge Event",
						io_name[miwu_io_pins[io].port], t_bit);
				}
				k_sleep(K_MSEC(50));
			}

			for (io = 0; io < sz_miwu_io_inputs; io++) {
				t_dev = gpio_objs[miwu_io_pins[io].port].dev;
				t_bit = miwu_io_pins[io].bit;
				t_group = parser_gpio_group(t_dev->name);
				if (miwu_results[ANY_RISING_FAIL][t_group][t_bit] == MIWU_PASS) {
					LOG_INF("[PASS][MIWU] IO%s%01X Any Rising Edge Event",
						io_name[miwu_io_pins[io].port], t_bit);
				} else {
					LOG_ERR("[FAIL][MIWU] IO%s%01X Any Rising Edge Event",
						io_name[miwu_io_pins[io].port], t_bit);
				}
				k_sleep(K_MSEC(50));
			}

			for (io = 0; io < sz_miwu_io_inputs; io++) {
				t_dev = gpio_objs[miwu_io_pins[io].port].dev;
				t_bit = miwu_io_pins[io].bit;
				t_group = parser_gpio_group(t_dev->name);
				if (miwu_results[LOW_LEVEL_FAIL][t_group][t_bit] == MIWU_PASS) {
					LOG_INF("[PASS][MIWU] IO%s%01X Low level Event",
						io_name[miwu_io_pins[io].port], t_bit);
				} else {
					LOG_ERR("[FAIL][MIWU] IO%s%01X Low level Event",
						io_name[miwu_io_pins[io].port], t_bit);
				}
				k_sleep(K_MSEC(50));
			}

			for (io = 0; io < sz_miwu_io_inputs; io++) {
				t_dev = gpio_objs[miwu_io_pins[io].port].dev;
				t_bit = miwu_io_pins[io].bit;
				t_group = parser_gpio_group(t_dev->name);
				if (miwu_results[HIGH_LEVEL_FAIL][t_group][t_bit] == MIWU_PASS) {
					LOG_INF("[PASS][MIWU] IO%s%01X High level Event",
						io_name[miwu_io_pins[io].port], t_bit);
				} else {
					LOG_ERR("[FAIL][MIWU] IO%s%01X High level Event",
						io_name[miwu_io_pins[io].port], t_bit);
				}
				k_sleep(K_MSEC(50));

			}
			LOG_INF("[GO]\r\n");
		break;
		default:
		break;
		}
	}
}

#define NPCX_EPURST_CTL_EPUR2_EN 3
/* Main entry */
void main(void)
{
	cmd_type = MIWU_TEST_AUTO_FALLING;
	LOG_INF("--- CI20 Zephyr MIWU Driver Validation ---");
	for (int io = 0; io < sz_miwu_io_inputs; io++) {
		t_dev = gpio_objs[miwu_io_pins[io].port].dev;
		t_bit = miwu_io_pins[io].bit;

		default_raw_data[io] = gpio_pin_get_raw(t_dev, t_bit);
	}
	k_sleep(K_MSEC(100));
#if defined(CONFIG_SOC_SERIES_NPCK3)
	/*Disable EPUR2 and UART*/
	struct glue_reg *inst_glue = (struct glue_reg *)(NPCX_GLUE_REG_ADDR);

	inst_glue->EPURST_CTL &= ~BIT(NPCX_EPURST_CTL_EPUR2_EN);
	*(volatile uint8_t*)(0x400C3011) = 0x00;
#elif defined(CONFIG_SOC_SERIES_NPCX4)/*ToDo pinctl*/
	*(volatile uint8_t*)(0x400C3000) = 0xD4;
	*(volatile uint8_t*)(0x400C3011) = 0x80;
#endif
	/* Zephyr driver validation init */
	auto_miwu_thread_entry();

	/* Let main thread go to sleep state */
	k_sleep(K_FOREVER);
}

