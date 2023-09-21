/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#define LOG_LEVEL LOG_LEVEL_INF
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#define DT_SPEC_AND_COMMA(node_id, prop, idx)                                  \
	ADC_DT_SPEC_GET_BY_IDX(node_id, idx),
#define TASK_STACK_SIZE 1024
#define PRIORITY 7
#define MAX_ARGUMNETS 3
#define MAX_ARGU_SIZE 10
#define ADC_3300VFS_ACCURACY 25
#define ADC_2816VFS_ACCURACY_RANGE_LOW 20
#define ADC_2816VFS_ACCURACY_RANGE_HIGH 30
#define ADC_VOLTAGE_REFERENCE_3300 3300
#define ADC_VOLTAGE_REFERENCE_2816 2816

static struct k_thread temp_id;
K_THREAD_STACK_DEFINE(temp_stack, TASK_STACK_SIZE);

static uint8_t arguments[MAX_ARGUMNETS][MAX_ARGU_SIZE];

struct k_event adc_event;

/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)};

int16_t buf;
int ret;

static int adc_init(void)
{
	int err;
	/* Configure channels individually prior to sampling. */
	for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
		if (!device_is_ready(adc_channels[i].dev)) {
			LOG_INF("ADC controller device %s not ready\n",
				adc_channels[i].dev->name);
			return 0;
		}

		err = adc_channel_setup_dt(&adc_channels[i]);
		if (err < 0) {
			LOG_INF("Could not setup channel #%d (%d)\n", i, err);
			return 0;
		}
		LOG_INF("%s ADC channel %d\n", adc_channels[i].dev->name,
			adc_channels[i].channel_id);
	}
	return 1;
}

static void adc_read_func(char *channel)
{
	int i;

	i = atoi(channel);

	struct adc_sequence sequence = {
		.buffer = &buf,
		.buffer_size = sizeof(buf),
	};

	ret = adc_sequence_init_dt(&adc_channels[i], &sequence);
	if (ret) {
		LOG_INF("[FAIL] ADC sequence init\n");
	}

	ret = adc_read(adc_channels[i].dev, &sequence);
	if (ret) {
		LOG_INF("[FAIL] ADC module read\n");
	}

	LOG_INF("%s ADC channel %d\n", adc_channels[i].dev->name,
		adc_channels[i].channel_id);
	LOG_INF("Sample value: %x\n", buf);
}

#ifdef CONFIG_ADC_ASYNC
static void adc_read_async_all(void)
{
	struct adc_sequence sequence = {
		.buffer = &buf,
		.buffer_size = sizeof(buf),
	};

	for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
		ret = adc_sequence_init_dt(&adc_channels[i], &sequence);
		if (ret) {
			LOG_INF("[FAIL] ADC sequence init\n");
		}

		ret = adc_read_async(adc_channels[i].dev, &sequence, NULL);

		if (ret) {
			LOG_INF("[FAIL] ADC module read\n");
		}

		LOG_INF("%s ADC channel %d\n", adc_channels[i].dev->name,
			adc_channels[i].channel_id);
		LOG_INF("Sample value: %x\n", buf);
	}
}
#endif

static void adc_read_read_all(void)
{
	struct adc_sequence sequence = {
		.buffer = &buf,
		.buffer_size = sizeof(buf),
	};

	for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
		ret = adc_sequence_init_dt(&adc_channels[i], &sequence);
		if (ret) {
			LOG_INF("[FAIL] ADC sequence init\n");
		}

		ret = adc_read(adc_channels[i].dev, &sequence);
		if (ret) {
			LOG_INF("[FAIL] ADC module read\n");
		}

		LOG_INF("%s ADC channel %d\n", adc_channels[i].dev->name,
			adc_channels[i].channel_id);
		LOG_INF("Sample value %x\n", buf);
	}
}


static void adc_check_conversion_result(char *input, char *channel)
{
	int ch;
	int err;
	int input_signal;
	int32_t vref_mv;
	int32_t ch_dat;

	ch = atoi(channel);

	/* input voltage */
	input_signal = atoi(input);

	/* channel data */
	ch_dat = (int32_t)buf;

	/* voltage reference */
	vref_mv = adc_channels[ch].vref_mv;
	if (vref_mv < input_signal) {
		LOG_INF("[FAIL] Test voltage is not available\n");
	}

	err = adc_raw_to_millivolts_dt(&adc_channels[ch], &ch_dat);

	/* conversion to mV may not be supported, skip if not */
	if (err < 0) {
		LOG_INF("[FAIL] value in mV not available\n");
	}

	LOG_INF(" %s\n", adc_channels[ch].dev->name);

	if (vref_mv == ADC_VOLTAGE_REFERENCE_3300) {
		if ((ch_dat <= (input_signal + ADC_3300VFS_ACCURACY)) &&
			(input_signal >= (input_signal - ADC_3300VFS_ACCURACY))) {
			LOG_INF("[PASS]Vref 3300, ADC%d Vi: %d\n", ch,
			adc_channels[ch].channel_id);
		} else {
			LOG_INF("[FAIL]Vref 3300, ADC%d Vi: %d\n", ch,
			adc_channels[ch].channel_id);
		}
	} else if (vref_mv == ADC_VOLTAGE_REFERENCE_2816) {
		if ((ch_dat <= (input_signal + ADC_2816VFS_ACCURACY_RANGE_HIGH)) &&
			(ch_dat >= (input_signal - ADC_2816VFS_ACCURACY_RANGE_HIGH))) {
			LOG_INF("[PASS]Vref 2816, ADC%d Vi: %d\n", ch,
			adc_channels[ch].channel_id);
		} else {
			LOG_INF("[FAIL]Vref 2816, ADC%d Vi: %d\n", ch,
			adc_channels[ch].channel_id);
		}
	}

}

static void adc_validation_func(void *dummy1, void *dummy2, void *dummy3)
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
			if (!strcmp("init", arguments[0])) {
				LOG_INF("ADC module init\n");
				adc_init();
			}
#ifdef CONFIG_ADC_ASYNC
			if (!strcmp("async", arguments[0])) {
				LOG_INF("Test async\n");
				adc_read_async_all();
			}
	#endif
			if (!strcmp("all", arguments[0])) {
				LOG_INF("Test read all channel\n");
				adc_read_read_all();
			}
			break;
		case 0x004:
			if (!strcmp("read", arguments[0])) {
				adc_read_func(arguments[1]);
			}

			if (!strcmp("3300", arguments[0])) {
				adc_read_func(arguments[1]);
				adc_check_conversion_result(arguments[0], arguments[1]);
			}
			if (!strcmp("1800", arguments[0])) {
				adc_read_func(arguments[1]);
				adc_check_conversion_result(arguments[0], arguments[1]);
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
	LOG_INF("Start ADC Validation Task\n");
	k_thread_create(&temp_id, temp_stack, TASK_STACK_SIZE, adc_validation_func,
		NULL, NULL, NULL, PRIORITY, K_INHERIT_PERMS, K_FOREVER);
	k_thread_name_set(&temp_id, "ADC Validation");
	k_thread_start(&temp_id);

	return 0;
}

static int adc_command(const struct shell *shell, size_t argc, char **argv)
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
	sub_adc, SHELL_CMD_ARG(c0, NULL, "adc c0", adc_command, 1, 0),
	SHELL_CMD_ARG(c1, NULL, "adc c1 arg0", adc_command, 2, 0),
	SHELL_CMD_ARG(c2, NULL, "adc c2 arg0 arg1", adc_command, 3, 0),
	SHELL_CMD_ARG(cfg, NULL, "adc cfg arg0 arg1 arg2", adc_command, 4, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(adc, &sub_adc, "nuvoton adc validation", NULL);
