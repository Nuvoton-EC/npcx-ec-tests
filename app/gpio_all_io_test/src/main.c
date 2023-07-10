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

#define PIN_INPUT 1

typedef struct {
	uint8_t port: 5;  /*!< GPIO Port Number */
	uint8_t bit: 3;  /*!< GPIO Pin Number */
} GPIO_PIN_T;

/* Test IO Type */
typedef enum {
	IO_INPUT_HIGH_FAIL,
	IO_INPUT_LOW_FAIL,
	IO_OUTPUT_HIGH_FAIL,
	IO_OUTPUT_LOW_FAIL,
	IO_INT_PU_FAIL,
	IO_INT_PD_FAIL,
	IO_OD_OUTPUT_HIGH_FAIL,
	IO_OD_OUTPUT_LOW_FAIL,
	IO_PASS,
	IO_RESULT_COUNT,
} IO_RESULT_T;

typedef enum {
	IOTYPE_IO,
	IOTYPE_INPUT_ONLY,
	IOTYPE_OUTPUT_ONLY,
} IOTYPE_T;

/* Test IO */
typedef struct {
	GPIO_PIN_T pin_no;
	uint8_t iotype;
} GPIO_TEST_PIN_T;

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
}GPIO_GROUP_T;

#define GPIO_PIN(grp, pin) { .port = GPIO_GROUP_##grp, .bit = pin }

#if defined(CONFIG_SOC_SERIES_NPCK3)
#define GPIO_PINIO_TABLE { \
	{ GPIO_PIN(0, 1), IOTYPE_IO }, /* Group 0 */       \
	{ GPIO_PIN(0, 2), IOTYPE_IO },                     \
	{ GPIO_PIN(0, 3), IOTYPE_IO },                     \
	{ GPIO_PIN(0, 4), IOTYPE_IO },                     \
	{ GPIO_PIN(0, 5), IOTYPE_IO },                     \
	{ GPIO_PIN(0, 7), IOTYPE_IO },                     \
	{ GPIO_PIN(1, 0), IOTYPE_IO }, /* Group 1 */       \
	{ GPIO_PIN(1, 1), IOTYPE_IO },                     \
	{ GPIO_PIN(1, 3), IOTYPE_IO },                     \
	{ GPIO_PIN(1, 4), IOTYPE_IO },                     \
	{ GPIO_PIN(1, 5), IOTYPE_IO },                     \
	{ GPIO_PIN(1, 7), IOTYPE_IO },                     \
	{ GPIO_PIN(2, 0), IOTYPE_IO }, /* Group 2 */       \
	{ GPIO_PIN(2, 1), IOTYPE_IO },                     \
	{ GPIO_PIN(2, 2), IOTYPE_IO },                     \
	{ GPIO_PIN(2, 3), IOTYPE_IO },                     \
	{ GPIO_PIN(2, 5), IOTYPE_IO },                     \
	{ GPIO_PIN(2, 6), IOTYPE_IO },                     \
	{ GPIO_PIN(2, 7), IOTYPE_IO },                     \
	{ GPIO_PIN(3, 0), IOTYPE_IO }, /* Group 3 */       \
	{ GPIO_PIN(3, 1), IOTYPE_IO },                     \
	{ GPIO_PIN(3, 2), IOTYPE_IO },                     \
	{ GPIO_PIN(3, 3), IOTYPE_IO },                     \
	{ GPIO_PIN(3, 4), IOTYPE_IO },                     \
	{ GPIO_PIN(3, 6), IOTYPE_IO },                     \
	{ GPIO_PIN(4, 0), IOTYPE_IO }, /* Group 4 */       \
	{ GPIO_PIN(4, 4), IOTYPE_IO },                     \
	{ GPIO_PIN(4, 5), IOTYPE_IO },                     \
	{ GPIO_PIN(4, 6), IOTYPE_IO },                     \
	{ GPIO_PIN(4, 7), IOTYPE_IO },                     \
	{ GPIO_PIN(5, 0), IOTYPE_IO }, /* Group 5 */       \
	{ GPIO_PIN(5, 1), IOTYPE_IO },                     \
	{ GPIO_PIN(5, 2), IOTYPE_IO },                     \
	{ GPIO_PIN(5, 3), IOTYPE_IO },                     \
	{ GPIO_PIN(5, 4), IOTYPE_IO },                     \
	{ GPIO_PIN(5, 5), IOTYPE_IO },                     \
	{ GPIO_PIN(5, 6), IOTYPE_IO },                     \
	{ GPIO_PIN(5, 7), IOTYPE_IO },                     \
	{ GPIO_PIN(6, 0), IOTYPE_IO }, /* Group 6 */       \
	{ GPIO_PIN(6, 1), IOTYPE_IO },                     \
	{ GPIO_PIN(6, 2), IOTYPE_IO },                     \
	{ GPIO_PIN(6, 3), IOTYPE_IO },                     \
	{ GPIO_PIN(6, 4), IOTYPE_IO },                     \
	{ GPIO_PIN(6, 5), IOTYPE_IO },                     \
	{ GPIO_PIN(6, 6), IOTYPE_IO },                     \
	{ GPIO_PIN(6, 7), IOTYPE_IO },                     \
	{ GPIO_PIN(7, 0), IOTYPE_INPUT_ONLY },/* Group 7 */\
	{ GPIO_PIN(7, 2), IOTYPE_IO },                     \
	{ GPIO_PIN(7, 3), IOTYPE_IO },                     \
	{ GPIO_PIN(7, 4), IOTYPE_IO },                     \
	{ GPIO_PIN(7, 5), IOTYPE_IO },                     \
	{ GPIO_PIN(7, 6), IOTYPE_IO },                     \
	{ GPIO_PIN(7, 7), IOTYPE_IO },                     \
	{ GPIO_PIN(8, 1), IOTYPE_IO }, /* Group 8 */       \
	{ GPIO_PIN(8, 3), IOTYPE_IO },					   \
	{ GPIO_PIN(8, 5), IOTYPE_IO },                     \
	{ GPIO_PIN(8, 6), IOTYPE_IO },                     \
	{ GPIO_PIN(8, 7), IOTYPE_IO },					   \
	{ GPIO_PIN(9, 0), IOTYPE_IO }, /* Group 9 */       \
	{ GPIO_PIN(9, 1), IOTYPE_IO },                     \
	{ GPIO_PIN(9, 2), IOTYPE_IO },                     \
	{ GPIO_PIN(9, 3), IOTYPE_IO },                     \
	{ GPIO_PIN(9, 4), IOTYPE_IO },                     \
	{ GPIO_PIN(A, 0), IOTYPE_IO }, /* Group A */       \
	{ GPIO_PIN(A, 1), IOTYPE_IO },                     \
	{ GPIO_PIN(A, 2), IOTYPE_IO },                     \
	{ GPIO_PIN(A, 3), IOTYPE_IO },                     \
	{ GPIO_PIN(A, 4), IOTYPE_IO },                     \
	{ GPIO_PIN(A, 5), IOTYPE_IO },                     \
	{ GPIO_PIN(A, 6), IOTYPE_IO },                     \
	{ GPIO_PIN(A, 7), IOTYPE_IO },                     \
	{ GPIO_PIN(B, 0), IOTYPE_IO }, /* Group B */       \
	{ GPIO_PIN(B, 1), IOTYPE_IO },                     \
	{ GPIO_PIN(B, 2), IOTYPE_IO },                     \
	{ GPIO_PIN(B, 3), IOTYPE_IO },                     \
	{ GPIO_PIN(B, 4), IOTYPE_IO },                     \
	{ GPIO_PIN(B, 5), IOTYPE_IO },                     \
	{ GPIO_PIN(B, 6), IOTYPE_IO },                     \
	{ GPIO_PIN(B, 7), IOTYPE_IO },                     \
	{ GPIO_PIN(C, 0), IOTYPE_IO }, /* Group C */       \
	{ GPIO_PIN(C, 1), IOTYPE_IO },                     \
	{ GPIO_PIN(C, 2), IOTYPE_IO },                     \
	{ GPIO_PIN(C, 3), IOTYPE_IO },                     \
	{ GPIO_PIN(C, 4), IOTYPE_IO },                     \
	{ GPIO_PIN(C, 5), IOTYPE_IO },                     \
	{ GPIO_PIN(C, 6), IOTYPE_IO },                     \
	{ GPIO_PIN(C, 7), IOTYPE_IO },                     \
	{ GPIO_PIN(D, 0), IOTYPE_IO }, /* Group D */       \
	{ GPIO_PIN(D, 3), IOTYPE_IO },                     \
	{ GPIO_PIN(D, 4), IOTYPE_IO },                     \
	{ GPIO_PIN(D, 5), IOTYPE_IO },                     \
	{ GPIO_PIN(D, 6), IOTYPE_IO },                     \
	{ GPIO_PIN(D, 7), IOTYPE_IO },                     \
	{ GPIO_PIN(E, 0), IOTYPE_IO }, /* Group E */       \
	{ GPIO_PIN(E, 1), IOTYPE_IO },                     \
	{ GPIO_PIN(E, 2), IOTYPE_IO },                     \
	{ GPIO_PIN(E, 3), IOTYPE_IO },                     \
	{ GPIO_PIN(E, 4), IOTYPE_IO },                     \
	{ GPIO_PIN(E, 5), IOTYPE_IO },                     \
	{ GPIO_PIN(E, 6), IOTYPE_IO },                     \
	{ GPIO_PIN(F, 0), IOTYPE_IO }, /* Group F */       \
	{ GPIO_PIN(F, 6), IOTYPE_IO },                     \
	{ GPIO_PIN(F, 7), IOTYPE_IO },                     \
	{ GPIO_PIN(G, 5), IOTYPE_IO }, /* Group G */       \
	{ GPIO_PIN(G, 6), IOTYPE_IO },                     \
	{ GPIO_PIN(G, 7), IOTYPE_IO },                     \
	{ GPIO_PIN(H, 0), IOTYPE_IO }, /* Group H */       \
	{ GPIO_PIN(H, 1), IOTYPE_IO },                     \
	{ GPIO_PIN(H, 2), IOTYPE_IO },                     \
	{ GPIO_PIN(H, 4), IOTYPE_IO },                     \
	{ GPIO_PIN(STB0, 0), IOTYPE_IO },/* Group STBY0 */\
	{ GPIO_PIN(STB0, 1), IOTYPE_IO },                 \
	{ GPIO_PIN(STB0, 2), IOTYPE_IO },                 \
	{ GPIO_PIN(STB0, 3), IOTYPE_IO },                 \
	{ GPIO_PIN(STB0, 4), IOTYPE_IO },                 \
	{ GPIO_PIN(STB1, 1), IOTYPE_IO },/* Group STBY1 */\
	{ GPIO_PIN(STB1, 2), IOTYPE_IO },                 \
	{ GPIO_PIN(STB1, 3), IOTYPE_IO },                 \
	{ GPIO_PIN(STB1, 4), IOTYPE_IO },                 \
	{ GPIO_PIN(STB1, 5), IOTYPE_IO },                 \
	{ GPIO_PIN(STB1, 6), IOTYPE_IO },                 \
	{ GPIO_PIN(STB1, 7), IOTYPE_OUTPUT_ONLY },        \
}
#endif

const struct device *const dev_in = DEVICE_DT_GET(DT_NODELABEL(gpio0));
struct gpio_callback gpio_cb;
struct k_event gpio_test_event;
int cmd_type = 0;

enum {
	GPIO_SYNC = 1,
	GPIO_TEST_COUNT,
};

static const GPIO_TEST_PIN_T io_pins[] = GPIO_PINIO_TABLE;
static const uint16_t sz_io_pins = ARRAY_SIZE(io_pins);
const char io_name[GPIO_GROUP_COUNT][5] = {
	"0","1","2","3","4","5","6","7","8","9","A","B","C","D","E","F","G","H","STB0","STB1"
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

static int gpio_config_init()
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

static void gpio_print_io_report()
{
	int io;
	LOG_INF("other GPIO results:\n");
	for (io = 0; io < sz_io_pins; io++) {
		GPIO_PIN_T pin = io_pins[io].pin_no;
		if (io_pins[io].iotype != IOTYPE_INPUT_ONLY) {
			if (io_results[IO_OUTPUT_HIGH_FAIL][io] == IO_PASS) {
				LOG_INF("[PASS][GPIO] IO%s%01X Write High", io_name[pin.port], pin.bit);
			} else {
				LOG_ERR("[FAIL][GPIO] IO%s%01X Write High", io_name[pin.port], pin.bit);
			}
			if (io_results[IO_OUTPUT_LOW_FAIL][io] == IO_PASS) {
				LOG_INF("[PASS][GPIO] IO%s%01X Write LOW", io_name[pin.port], pin.bit);
			} else {
				LOG_ERR("[FAIL][GPIO] IO%s%01X Write LOW", io_name[pin.port], pin.bit);
			}
		}

		if (io_pins[io].iotype != IOTYPE_OUTPUT_ONLY) {
			if (io_results[IO_INPUT_HIGH_FAIL][io] == IO_PASS) {
				LOG_INF("[PASS][GPIO] IO%s%01X Read High", io_name[pin.port], pin.bit);
			} else {
				LOG_ERR("[FAIL][GPIO] IO%s%01X Read High", io_name[pin.port], pin.bit);
			}

			if (io_results[IO_INPUT_LOW_FAIL][io] == IO_PASS) {
				LOG_INF("[PASS][GPIO] IO%s%01X Read Low", io_name[pin.port], pin.bit);
			} else {
				LOG_ERR("[FAIL][GPIO] IO%s%01X Read Low", io_name[pin.port], pin.bit);
			}
			if (io_results[IO_INT_PU_FAIL][io] == IO_PASS) {
				LOG_INF("[PASS][GPIO] IO%s%01X Pull-Up", io_name[pin.port], pin.bit);
			} else {
				LOG_ERR("[FAIL][GPIO] IO%s%01X Pull-Up", io_name[pin.port], pin.bit);
			}

			if (io_results[IO_INT_PD_FAIL][io] == IO_PASS) {
				LOG_INF("[PASS][GPIO] IO%s%01X Pull-Down", io_name[pin.port], pin.bit);
			} else {
				LOG_ERR("[FAIL][GPIO] IO%s%01X Pull-Down", io_name[pin.port], pin.bit);
			}
		}

		if (io_pins[io].iotype == IOTYPE_IO){
			if (io_results[IO_OD_OUTPUT_LOW_FAIL][io] == IO_PASS) {
				LOG_INF("[PASS][GPIO] IO%s%01X Open-Drain/PU ", io_name[pin.port], pin.bit);
			} else {
				LOG_ERR("[FAIL][GPIO] IO%s%01X Open-Drain/PU ", io_name[pin.port], pin.bit);
			}

			if (io_results[IO_OD_OUTPUT_HIGH_FAIL][io] == IO_PASS) {
				LOG_INF("[PASS][GPIO] IO%s%01X Open-Drain/PD ", io_name[pin.port], pin.bit);
			} else {
				LOG_ERR("[FAIL][GPIO] IO%s%01X Open-Drain/PD ", io_name[pin.port], pin.bit);
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
	LOG_INF("auto_gpio_thread_entry");
	
	while (true) {
		switch (k_event_wait(&gpio_test_event, 0xFFF, true, K_FOREVER)) {
			case GPIO_SYNC:
				gpio_remove_callback(dev_in, &gpio_cb);

				if (io_pins[io].iotype != IOTYPE_OUTPUT_ONLY)
				{
					ret = gpio_pin_configure(gpio_objs[io_pins[io].pin_no.port].dev, io_pins[io].pin_no.bit, GPIO_INPUT);
					if(ret)
						LOG_ERR("[FAIL]gpio_pin_configure GPIO_INPUT error: %d",ret);
					k_sleep(K_MSEC(500));
					gpio_val = gpio_pin_get_raw(gpio_objs[io_pins[io].pin_no.port].dev, io_pins[io].pin_no.bit);
					if (gpio_val == 1){
						io_results[IO_INPUT_HIGH_FAIL][io] = IO_PASS;
					}else{
						io_results[IO_INPUT_HIGH_FAIL][io] = IO_INPUT_HIGH_FAIL;
					}

					k_sleep(K_MSEC(1000));
					gpio_val = gpio_pin_get_raw(gpio_objs[io_pins[io].pin_no.port].dev, io_pins[io].pin_no.bit);
					if (gpio_val == 0)
					{
						io_results[IO_INPUT_LOW_FAIL][io] = IO_PASS;
					}else{
						io_results[IO_INPUT_LOW_FAIL][io] = IO_INPUT_LOW_FAIL;
					}
				}
				
				if (io_pins[io].iotype != IOTYPE_INPUT_ONLY)
				{
					/* Case 3: Test write high */
					ret = gpio_pin_configure(gpio_objs[io_pins[io].pin_no.port].dev, io_pins[io].pin_no.bit, GPIO_OUTPUT_HIGH);
					if(ret)
						LOG_ERR("[FAIL]gpio_pin_configure GPIO_OUTPUT_HIGH error: %d",ret);

					k_sleep(K_MSEC(1000));
					gpio_val = gpio_pin_get_raw(gpio_objs[io_pins[io].pin_no.port].dev, io_pins[io].pin_no.bit);
					if (gpio_val == 1){
						io_results[IO_OUTPUT_HIGH_FAIL][io] = IO_PASS;
					}else{
						io_results[IO_OUTPUT_HIGH_FAIL][io] = IO_INPUT_LOW_FAIL;
					}

					/* Case 4: Test write low */
					ret = gpio_pin_configure(gpio_objs[io_pins[io].pin_no.port].dev, io_pins[io].pin_no.bit, GPIO_OUTPUT_LOW);
					if(ret)
						LOG_ERR("[FAIL]gpio_pin_configure GPIO_OUTPUT_LOW error: %d",ret);

					k_sleep(K_MSEC(1000));
					gpio_val = gpio_pin_get_raw(gpio_objs[io_pins[io].pin_no.port].dev, io_pins[io].pin_no.bit);
					if (gpio_val == 0){
						io_results[IO_OUTPUT_LOW_FAIL][io] = IO_PASS;
					}else{
						io_results[IO_OUTPUT_LOW_FAIL][io] = IO_INPUT_LOW_FAIL;
					}
						
				}

				if (io_pins[io].iotype != IOTYPE_OUTPUT_ONLY)
				{
					/* Case 5: Test PU */
					ret = gpio_pin_configure(gpio_objs[io_pins[io].pin_no.port].dev, io_pins[io].pin_no.bit, GPIO_INPUT | GPIO_PULL_UP);
					if(ret)
						LOG_ERR("[FAIL]GPIO_PULL_UP error: %d\n",ret);

					k_sleep(K_MSEC(1000));
					gpio_val = gpio_pin_get_raw(gpio_objs[io_pins[io].pin_no.port].dev, io_pins[io].pin_no.bit);
					if (gpio_val == 1){
						io_results[IO_INT_PU_FAIL][io] = IO_PASS;
					}else{
						io_results[IO_INT_PU_FAIL][io] = IO_INT_PU_FAIL;
					}
#if defined(CONFIG_SOC_SERIES_NPCK3)
					if(io == 46)/*GPI70*/
					{
						k_sleep(K_MSEC(1500));
					}
#endif
					/* Case 6: Test PD */
					ret = gpio_pin_configure(gpio_objs[io_pins[io].pin_no.port].dev, io_pins[io].pin_no.bit, GPIO_INPUT | GPIO_PULL_DOWN);
					if(ret)
						LOG_ERR("[FAIL]GPIO_PULL_DOWN error: %d\n",ret);

					k_sleep(K_MSEC(1000));
					gpio_val = gpio_pin_get_raw(gpio_objs[io_pins[io].pin_no.port].dev, io_pins[io].pin_no.bit);
					if (gpio_val == 0){
						io_results[IO_INT_PD_FAIL][io] = IO_PASS;
					}else{
						io_results[IO_INT_PD_FAIL][io] = IO_INT_PD_FAIL;
					}
						
				}

				if (io_pins[io].iotype == IOTYPE_IO)
				{
					/* Case 7: Test OD/UP */
					ret = gpio_pin_configure(gpio_objs[io_pins[io].pin_no.port].dev, io_pins[io].pin_no.bit, GPIO_OUTPUT_INIT_HIGH |
									GPIO_OPEN_DRAIN | GPIO_PULL_UP);
					if(ret)
						LOG_ERR("[FAIL]gpio_pin_configure Open-Drain/PU error: %d",ret);

					k_sleep(K_MSEC(1000));
					gpio_val = gpio_pin_get_raw(gpio_objs[io_pins[io].pin_no.port].dev, io_pins[io].pin_no.bit);
					if (gpio_val == 1){
						io_results[IO_OD_OUTPUT_HIGH_FAIL][io] = IO_PASS;
					}else{
						io_results[IO_OD_OUTPUT_HIGH_FAIL][io] = IO_OD_OUTPUT_HIGH_FAIL;
					}
						

					/* Case 8: Test OD/DOWN*/
					ret = gpio_pin_configure(gpio_objs[io_pins[io].pin_no.port].dev, io_pins[io].pin_no.bit, GPIO_OUTPUT_INIT_HIGH |
									GPIO_OPEN_DRAIN | GPIO_PULL_DOWN);
					if(ret)
						LOG_ERR("[FAIL]gpio_pin_configure Open-Drain/PD error: %d",ret);

					k_sleep(K_MSEC(1000));
					gpio_val = gpio_pin_get_raw(gpio_objs[io_pins[io].pin_no.port].dev, io_pins[io].pin_no.bit);
					if (gpio_val == 0){
						io_results[IO_OD_OUTPUT_LOW_FAIL][io] = IO_PASS;
					}else{
						io_results[IO_OD_OUTPUT_LOW_FAIL][io] = IO_OD_OUTPUT_LOW_FAIL;
					}
				}

				io++;

				if (io != sz_io_pins)
				{
					LOG_INF("GPIO test keep going!!");
					ret = gpio_pin_configure(dev_in, PIN_INPUT, GPIO_INPUT);
					if (ret)
						LOG_ERR("Configure input fail.\n");

					ret = gpio_pin_interrupt_configure(dev_in, PIN_INPUT, GPIO_INT_EDGE_FALLING);
					if (ret)
						LOG_ERR("Configure interrupt fail.\n");

					gpio_init_callback(&gpio_cb, gpio_isr, BIT(PIN_INPUT));
					ret = gpio_add_callback(dev_in, &gpio_cb);
					if (ret)
						LOG_ERR("Configure callback fail.\n");
				}
				else
				{
#if defined(CONFIG_SOC_SERIES_NPCK3)
					*(volatile uint8_t*)(0x400C3011) = 0x06; /*Enable UART*/
#endif
					gpio_print_io_report();
					LOG_INF("GPIO test finish!!");
				}
			break;
			default:
			break;
		}
	}
}

#define NPCX_EPURST_CTL_EPUR2_EN			3
/* Main entry */
void main(void)
{
	LOG_INF("--- CI20 Zephyr Driver Validation ---");
	int ret;
	cmd_type = GPIO_SYNC;
#if defined(CONFIG_SOC_SERIES_NPCK3)
	/*Disable EPUR2 and UART*/
	struct glue_reg *inst_glue = (struct glue_reg *)(NPCX_GLUE_REG_ADDR);
	inst_glue->EPURST_CTL &= ~BIT(NPCX_EPURST_CTL_EPUR2_EN);
	*(volatile uint8_t*)(0x400C3011) = 0x00;
#endif

	ret = gpio_config_init();
	if (ret) {
		LOG_ERR("GPIO Configurefail.\n");
	}
	auto_gpio_thread_entry();
}