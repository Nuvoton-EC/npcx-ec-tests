/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/adc_cmp_npcx.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#define LOG_LEVEL LOG_LEVEL_INF
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#define DEVICE_INIT(node_id, prop, idx) \
	DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx)),
#define TASK_STACK_SIZE			1024
#define PRIORITY			7
#define MAX_ARGUMNETS			3
#define MAX_ARGU_SIZE			10

#define ADC_CMP_UPPER_THRESHOLD_MV	1000
#define ADC_CMP_LOWER_THRESHOLD_MV	250
#define IS_RUNNING			!(atomic_test_bit(&stop, 0))
#define STOP()				atomic_set_bit(&stop, 0)

#define ADC0_CH0_CMP_NODE		DT_NODELABEL(npcx_adc0_ch0)
#define ADC0_CH1_CMP_NODE		DT_NODELABEL(npcx_adc0_ch1)
#define ADC0_CH2_CMP_NODE		DT_NODELABEL(npcx_adc0_ch2)
#define ADC0_CH3_CMP_NODE		DT_NODELABEL(npcx_adc0_ch3)
#define ADC0_CH4_CMP_NODE		DT_NODELABEL(npcx_adc0_ch4)
#define ADC0_CH5_CMP_NODE		DT_NODELABEL(npcx_adc0_ch5)
#define ADC1_CH0_CMP_NODE		DT_NODELABEL(npcx_adc1_ch0)
#define ADC1_CH1_CMP_NODE		DT_NODELABEL(npcx_adc1_ch1)
#define ADC1_CH2_CMP_NODE		DT_NODELABEL(npcx_adc1_ch2)
#define ADC1_CH3_CMP_NODE		DT_NODELABEL(npcx_adc1_ch3)
#define ADC1_CH4_CMP_NODE		DT_NODELABEL(npcx_adc1_ch4)
#define ADC1_CH5_CMP_NODE		DT_NODELABEL(npcx_adc1_ch5)


enum threshold_state {
	THRESHOLD_UPPER,
	THRESHOLD_LOWER
} state;

atomic_val_t stop;

static struct k_thread temp_id;
K_THREAD_STACK_DEFINE(temp_stack, TASK_STACK_SIZE);
static uint8_t arguments[MAX_ARGUMNETS][MAX_ARGU_SIZE];
struct k_event adc_event;
const struct sensor_trigger trigger = {
	.type = SENSOR_TRIG_THRESHOLD,
	.chan = SENSOR_CHAN_VOLTAGE
};


static const struct device *regs[] = {
	DEVICE_DT_GET(ADC0_CH0_CMP_NODE),
	DEVICE_DT_GET(ADC0_CH1_CMP_NODE),
	DEVICE_DT_GET(ADC0_CH2_CMP_NODE),
	DEVICE_DT_GET(ADC0_CH3_CMP_NODE),
	DEVICE_DT_GET(ADC0_CH4_CMP_NODE),
	DEVICE_DT_GET(ADC0_CH5_CMP_NODE),
	DEVICE_DT_GET(ADC1_CH0_CMP_NODE),
	DEVICE_DT_GET(ADC1_CH1_CMP_NODE),
	DEVICE_DT_GET(ADC1_CH2_CMP_NODE),
	DEVICE_DT_GET(ADC1_CH3_CMP_NODE),
	DEVICE_DT_GET(ADC1_CH4_CMP_NODE),
	DEVICE_DT_GET(ADC1_CH5_CMP_NODE),
};

void enable_threshold(const struct device *dev, bool enable)
{
	struct sensor_value val;
	int err = 0;

	val.val1 = enable;
	err = sensor_attr_set(dev, SENSOR_CHAN_VOLTAGE, SENSOR_ATTR_ALERT, &val);
	if (err) {
		LOG_INF("ADC CMP: Error %sabling threshold\n",
			enable ? "en":"dis");
		STOP();
	}
}

void set_upper_threshold(const struct device *dev)
{
	struct sensor_value val;
	int err = 0;

	LOG_INF("ADC CMP: Set Upper threshold\n");

	val.val1 = ADC_CMP_UPPER_THRESHOLD_MV;

	err = sensor_attr_set(dev, SENSOR_CHAN_VOLTAGE, SENSOR_ATTR_UPPER_VOLTAGE_THRESH, &val);
	if (err) {
		LOG_INF("Error setting upper threshold\n");
		STOP();
	}
}

void set_lower_threshold(const struct device *dev)
{
	struct sensor_value val;
	int err = 0;

	LOG_INF("ADC CMP: Set Lower threshold\n");

	val.val1 = ADC_CMP_LOWER_THRESHOLD_MV;

	err = sensor_attr_set(dev, SENSOR_CHAN_VOLTAGE,
			      SENSOR_ATTR_LOWER_VOLTAGE_THRESH, &val);
	if (err) {
		LOG_INF("ADC CMP: Error setting lower threshold\n");
		STOP();
	}
}

void threshold_trigger_handler(const struct device *dev,
			       const struct sensor_trigger *trigger)
{
	enable_threshold(dev, false);

	if (state == THRESHOLD_UPPER) {
		state = THRESHOLD_LOWER;
		LOG_INF("ADC CMP: Upper threshold detected\n");
		set_lower_threshold(dev);
	} else if (state == THRESHOLD_LOWER) {
		state = THRESHOLD_UPPER;
		LOG_INF("ADC CMP: Lower threshold detected\n");
		set_upper_threshold(dev);
	} else {
		LOG_INF("ADC CMP: Error, unexpected state\n");
		STOP();
	}

	enable_threshold(dev, true);
}

void threshold_upper_trigger_handler(const struct device *dev,
				     const struct sensor_trigger *trigger)
{
	enable_threshold(dev, false);

	LOG_INF("ADC CMP: Upper threshold detected\n");

	set_upper_threshold(dev);

	enable_threshold(dev, true);
}

void threshold_lower_trigger_handler(const struct device *dev,
				     const struct sensor_trigger *trigger)
{
	enable_threshold(dev, false);

	LOG_INF("ADC CMP: Lower threshold detected\n");

	enable_threshold(dev, true);
}


static void adc_cmp_trigger_select(char *channel)
{
	int ch;
	int err = 0;

	ch = atoi(channel);

	LOG_INF("ADC%d, %s\n", ch, regs[ch]->name);

	if (!device_is_ready(regs[ch])) {
		LOG_INF("ADC CMP: Error, device is not ready\n");
		return;
	}

	err = sensor_trigger_set(regs[ch], &trigger,
		threshold_trigger_handler);
	if (err) {
		LOG_INF("ADC CMP: Error setting handler\n");
	}

	enable_threshold(regs[ch], true);

	while (IS_RUNNING) {
		k_sleep(K_MSEC(1));
	}

	LOG_INF("ADC CMP: Exiting application\n");
}

static void adc_cmp_trigger_disable(char *channel)
{
	int ch;

	ch = atoi(channel);
	enable_threshold(regs[ch], false);
}

static void adc_cmp_threshold_trigger_select(char *channel, char *th_sel)
{
	int ch;
	int err = 0;
	int l_h_sel;

	ch = atoi(channel);
	l_h_sel = atoi(th_sel);

	LOG_INF("ADC%d, %s\n", ch, regs[ch]->name);

	if (!device_is_ready(regs[ch])) {
		LOG_INF("ADC CMP: Error, device is not ready\n");
		return;
	}

	switch (l_h_sel) {
	case THRESHOLD_UPPER:
		err = sensor_trigger_set(regs[ch], &trigger, threshold_upper_trigger_handler);
		set_upper_threshold(regs[ch]);
		break;
	case THRESHOLD_LOWER:
		err = sensor_trigger_set(regs[ch], &trigger, threshold_lower_trigger_handler);
		set_lower_threshold(regs[ch]);
		break;
	default:
		LOG_INF("Error command\n");
	}

	if (err) {
		LOG_INF("ADC CMP: Error setting handler\n");
	}

	enable_threshold(regs[ch], true);

}


static void adc_cmp_validation_func(void *dummy1, void *dummy2, void *dummy3)
{
	uint32_t events;

	k_event_init(&adc_event);

	LOG_INF("ADC module hook success\n");
	while (true) {
		events = k_event_wait(&adc_event, 0xFFF, true, K_FOREVER);
		switch (events) {
		case 0x001: /* no argu */
			break;
		case 0x002:
			break;
		case 0x004:
			if (!strcmp("test", arguments[0])) {
				adc_cmp_trigger_select(arguments[1]);
			}

			if (!strcmp("dis", arguments[0])) {
				LOG_INF("Disable channel\n");
				adc_cmp_trigger_disable(arguments[1]);
			}
			break;
		case 0x008:
			if (!strcmp("trig", arguments[0])) {
				LOG_INF("Trigger test\n");
				adc_cmp_threshold_trigger_select(arguments[1],
								 arguments[2]);
			}
			break;

		default:
			LOG_INF("Error command\n");
		}

	}
}

int main(void)
{
	/* Zephyr driver validation */
	LOG_INF("Start ADC CMP Validation Task\n");
	k_thread_create(&temp_id, temp_stack, TASK_STACK_SIZE, adc_cmp_validation_func,
			NULL, NULL, NULL, PRIORITY, K_INHERIT_PERMS, K_FOREVER);
	k_thread_name_set(&temp_id, "ADC CMP Validation");
	k_thread_start(&temp_id);

	return 0;
}

static int adc_cmp_command(const struct shell *shell, size_t argc, char **argv)
{
	int i, evt;

	evt = 1;
	for (evt = 1, i = 1; i < argc; i++) {
		strcpy(arguments[i - 1], argv[i]);
		evt <<= 1;
	}
	k_event_post(&adc_event, evt);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_adc_cmp, SHELL_CMD_ARG(c0, NULL, "adc_cmp c0", adc_cmp_command, 1, 0),
	SHELL_CMD_ARG(c1, NULL, "adc_cmp c1 arg0", adc_cmp_command, 2, 0),
	SHELL_CMD_ARG(c2, NULL, "adc_cmp c2 arg0 arg1", adc_cmp_command, 3, 0),
	SHELL_CMD_ARG(cfg, NULL, "adc_cmp cfg arg0 arg1 arg2", adc_cmp_command, 4, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(adc_cmp, &sub_adc_cmp, "nuvoton adc_cmp validation", NULL);
