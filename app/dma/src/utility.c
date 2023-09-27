#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/random/rand32.h>
#include "../../zephyr/drivers/dma/dma_npcx.h"

#define LOG_LEVEL LOG_LEVEL_INF
#include <zephyr/logging/log.h>

static uint32_t src[TRANSFER_SIZE];
static uint32_t dst[TRANSFER_SIZE] __aligned(16);

static uint8_t testSetting;
static uint32_t ch, ram;
static void rand_setting_init(uint32_t mask)
{
	testSetting = sys_rand32_get() & mask;
	ch = testSetting & 0x01;
	ram = (testSetting >> 1) & 0x01;
}

uintptr_t align_up(uintptr_t addr, tw_set tws)
{
	uintptr_t mask = tws - 1;
	/* check power of two */
	if ((tws & mask) == 0) {
		return ((addr + mask) & ~mask);
	} else {
		return (((addr + mask) / tws) * tws);
	}
}

static void sync_data_transfer(char *option)
{
	uint8_t opt = atoi(option);
	const char tx_data[] __aligned(align_val) =
		"It is harder to be kind than to be wise........";
	char rx_data[48] __aligned(align_val) = { 0 };

	const char tx_data1[] __aligned(align_val) =
		"........wise be to than kind be to harder is It";
	char rx_data1[48] __aligned(align_val) = { 0 };

	const struct device *dev, *dev1;
	uint8_t dev1_channel, dev2_channel;

	switch (opt) {
	case 1:
		dev = dma_devices[0];
		dev1 = dma_devices[1];
		dev1_channel = 0;
		dev2_channel = 1;
		break;
	case 2:
		dev = dma_devices[0];
		dev1 = dma_devices[1];
		dev1_channel = 1;
		dev2_channel = 0;
		break;
	case 3:
		dev = dma_devices[0];
		dev1 = dma_devices[1];
		dev1_channel = dev2_channel = 0;
		break;
	case 4:
		dev = dma_devices[0];
		dev1 = dma_devices[1];
		dev1_channel = dev2_channel = 1;
		break;
	case 5:
		dev = dev1 = dma_devices[0];
		dev1_channel = 0;
		dev2_channel = 1;
		break;
	case 6:
		dev = dev1 = dma_devices[1];
		dev1_channel = 0;
		dev2_channel = 1;
		break;
	}

	memset(rx_data, 0, sizeof(rx_data));
	memset(rx_data1, 0, sizeof(rx_data1));

	dma_cfg.channel_direction = dma_cfg1.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = dma_cfg.dest_data_size = align_val;
	dma_cfg1.source_data_size = dma_cfg1.dest_data_size = align_val;

	dma_block_cfg.block_size = sizeof(tx_data);
	dma_block_cfg1.block_size = sizeof(tx_data1);

	dma_block_cfg.source_address = (uint32_t)tx_data;
	dma_block_cfg.dest_address = (uint32_t)rx_data;
	dma_cfg.head_block = &dma_block_cfg;
	dma_cfg.dma_callback = cb_data_check;

	dma_block_cfg1.source_address = (uint32_t)tx_data1;
	dma_block_cfg1.dest_address = (uint32_t)rx_data1;
	dma_cfg1.head_block = &dma_block_cfg1;
	dma_cfg1.dma_callback = cb_data_check;

	if (dma_config(dev, dev1_channel, &dma_cfg)) {
		LOG_INF("ERROR: configure\n");
		return;
	}
	if (dma_config(dev1, dev2_channel, &dma_cfg1)) {
		LOG_INF("ERROR: configure\n");
		return;
	}
	if (dma_start(dev, dev1_channel)) {
		LOG_INF("ERROR: transfer\n");
		return;
	}
	if (dma_start(dev1, dev2_channel)) {
		LOG_INF("ERROR: transfer\n");
		return;
	}
	check_char_data(tx_data, rx_data, 1);
	check_char_data(tx_data1, rx_data1, 1);

}
