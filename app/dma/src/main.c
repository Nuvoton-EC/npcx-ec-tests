/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/../../drivers/dma/dma_npcx.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#define LOG_LEVEL LOG_LEVEL_INF
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#define MAX_ARGUMNETS	3
#define MAX_ARGU_SIZE	10
#define TRANSFER_SIZE	1024

#define TASK_STACK_SIZE	1024
#define PRIORITY	7

#define DMA0_CTLER			DT_NODELABEL(dma0)
#ifdef CONFIG_SOC_SERIES_NPCX4
#define DMA1_CTLER			DT_NODELABEL(dma1)
#endif

static uint8_t arguments[MAX_ARGUMNETS][MAX_ARGU_SIZE];
struct k_event dma_event;
static struct k_thread temp_id;

K_THREAD_STACK_DEFINE(temp_stack, TASK_STACK_SIZE);

const struct shell *sh_ptr;

/* Get device from device tree */
static const struct device *const dma_devices[] = {
	DEVICE_DT_GET(DMA0_CTLER),
	#ifdef CONFIG_SOC_SERIES_NPCX4
	DEVICE_DT_GET(DMA1_CTLER),
	#endif
};

#define NUM_DMA_DEVICE ARRAY_SIZE(dma_devices)
#define MRAM				0x100B0000	/* buttom of code ram */
#ifdef CONFIG_SOC_SERIES_NPCK3
	#define	INT_FLASH_BASE1_ADDR	0x60000000	/* private flash */
	#define	FLASH_BASE1_ADDR	0x70000000	/* shared flash */
	#define BACKUP_FLASH_BASE1_ADDR 0x80000000	/* Back-up flash */
#elif CONFIG_SOC_SERIES_NPCX4
	#define INT_FLASH_BASE1_ADDR	0x60000000	/* private flash */
	#define FLASH_BASE1_ADDR	0x68000000	/* external flash */
#endif

/* data transfer config */
static const uint8_t align_val = 16;
static struct device *dev;
static uint8_t ch;
static struct dma_config dma_cfg = { 0 };
static struct dma_block_config dma_block_cfg = { 0 };
static struct dma_config dma_cfg1 = { 0 };
static struct dma_block_config dma_block_cfg1 = { 0 };

/* mem to mem config */
static uint8_t GDMAMemPool[MOVE_SIZE + 4] __aligned(16);
/* power save config */
static uint8_t GDMAMemPool2[MOVE_SIZE + 4] __aligned(16);
static uint8_t passFlag;

/* flash to mem config */
static uint32_t gdma_test_flash_src[] = {
	INT_FLASH_BASE1_ADDR,
	FLASH_BASE1_ADDR,
	#ifdef CONFIG_SOC_SERIES_NPCK3
	BACKUP_FLASH_BASE1_ADDR
	#endif
};
/* gdmaerr config */
#define INVALID_ADDR	0x10050000

/* isr event */
static uint32_t gdma_data_check(const struct device *dev, const uint8_t channel)
{
	struct dma_reg *const inst = HAL_INSTANCE(dev, channel);
	uint8_t tws = GET_FIELD(inst->CONTROL, NPCX_DMACTL_TWS);
	uint8_t bme = GET_BIT(inst->CONTROL, NPCX_DMACTL_BME);
	uint8_t dadir = GET_BIT(inst->CONTROL, NPCX_DMACTL_DADIR);
	uint8_t sadir = GET_BIT(inst->CONTROL, NPCX_DMACTL_SADIR);
	uint32_t tcnt = inst->TCNT;
	uint32_t src_ = inst->SRCB;
	uint32_t dst_ = inst->DSTB;
	uint32_t ctcnt = inst->CTCNT;
	volatile uint8_t *dst_B, *src_B;
	volatile uint16_t *dst_W, *src_W;
	volatile uint32_t *dst_DW, *src_DW;

	dst_B = (uint8_t *) dst_;
	src_B = (uint8_t *) src_;
	dst_W = (uint16_t *) dst_;
	src_W = (uint16_t *) src_;
	dst_DW = (uint32_t *) dst_;
	src_DW = (uint32_t *) src_;

	if (IS_BIT_SET(inst->CONTROL, NPCX_DMACTL_TC)) {
		LOG_INF("[FAIL][GDMA] ch%d GDMA TC couldn't clear\r\n", channel);
	}
	if (IS_BIT_SET(inst->CONTROL, NPCX_DMACTL_GDMAEN)) {
		LOG_INF("[FAIL][GDMA] ch%d GDMA EN is Enable\r\n", channel);
	}
	if (ctcnt) {
		LOG_INF("[FAIL][GDMA] ch%d CTCNT not completed \r\n", channel);
	}

	if (bme) {
		tcnt <<= 2;
	}
	switch (tws) {
	case 0:
		LOG_INF("Byte mode | ");
		LOG_INF("src | %p: %hhx dst | %p: %hhx \r\n",
			(void *)src_B, *src_B, (void *)dst_B, *dst_B);
		do {
			if (*dst_B != *src_B) {
				break;
			}
			if (dadir)
				dst_B -= 1;
			else
				dst_B += 1;
			if (sadir)
				src_B -= 1;
			else
				src_B += 1;
			tcnt--;
		} while (tcnt != 0);
		break;
	case 1:
		LOG_INF("Word mode | ");
		LOG_INF("src | %p: %hx dst | %p: %hx \r\n",
			(void *)src_W, *src_W, (void *)dst_W, *dst_W);
		do {
			if (*dst_W != *src_W) {
				break;
			}
			if (dadir)
				dst_W -= 1;
			else
				dst_W += 1;
			if (sadir)
				src_W -= 1;
			else
				src_W += 1;
			tcnt--;
		} while (tcnt != 0);
		break;
	case 2:
		if (bme) {
			LOG_INF("Double Word mode | Burst Mode");
		} else {
			LOG_INF("Double Word mode | Normal Mode");
		}
		LOG_INF("src %p: %x | dst %p: %x \r\n",
		(void *)src_DW, *src_DW, (void *)dst_DW, *dst_DW);
		do {
			if (*dst_DW != *src_DW) {
				break;
			}
			if (dadir)
				dst_DW -= 1;
			else
				dst_DW += 1;
			if (sadir)
				src_DW -= 1;
			else
				src_DW += 1;
			tcnt--;
		} while (tcnt != 0);
		break;
	}
	return tcnt;
}

static void cb_data_check(const struct device *dma_dev, void *arg,
				uint32_t channel, int status)
{
	if (status < 0) {
		LOG_INF("DMA could not proceed, an error occurred\n");
	}
	LOG_INF("channel : %d", channel);

	if (gdma_data_check(dma_dev, channel)) {
		LOG_INF("[FAIL] Data check");
	} else {
		LOG_INF("[PASS] Data check");
	}
}

static void cb_power_save(const struct device *dma_dev, void *arg,
				uint32_t channel, int status)
{
	if (status < 0) {
		LOG_INF("DMA could not proceed, an error occurred\n");
	}
	LOG_INF("channel : %d", channel);
	struct dma_reg *const inst = HAL_INSTANCE(dma_dev, channel);

	passFlag |= (GET_BIT(inst->CONTROL, NPCX_DMACTL_TC) == 0) << 1;
	if (passFlag == 0b11) {
		LOG_INF("[PASS][GDMA]: ch-%d Power save successful.", channel);
	} else {
		LOG_INF("[FAIL][GDMA]: ch-%d power save failed.", channel);
	}
}
#ifdef CONFIG_SOC_SERIES_NPCK3
static void cb_gdmaerr(const struct device *dma_dev, void *arg,
				uint32_t channel, int status)
{
	if (status < 0) {
		LOG_INF("DMA could not proceed, an error occurred\n");
	}
	if (status == -EIO) {
		LOG_INF("[PASS][GDMA]: channel %d GDMAERR test", channel);
	} else {
		LOG_INF("[FAIL][GDMA]: channel %d GDMAERR test", channel);
	}
}

static void dma_gdmaerr(const struct shell *shell, size_t argc, char **argv)
{
	/* Minimal setting for DMA api */
	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = dma_cfg.dest_data_size = 1;
	dma_block_cfg.block_size = MOVE_SIZE;
	dma_block_cfg.source_address = INVALID_ADDR;
	dma_block_cfg.dest_address = MRAM;

	/* isr event */
	dma_cfg.dma_callback = cb_gdmaerr;
	dma_cfg.head_block = &dma_block_cfg;

	if (dma_config(dev, ch, &dma_cfg)) {
		LOG_INF("ERROR: configure\n");
		return;
	}
	if (dma_start(dev, ch)) {
		LOG_INF("ERROR: transfer\n");
		return;
	}
}
#endif

static void check_char_data(const char *tx_d, const char *rx_d, bool enable)
{
	if (strcmp(tx_d, rx_d) != 0) {
		LOG_INF("[FAIL] Data transfer");
	} else {
		LOG_INF("[PASS] Data transfer");
	}
	if (enable) {
		shell_info(sh_ptr, "rx_data : %s", rx_d);
	}
}

static void check_def_value(const struct shell *shell, size_t argc, char **argv)
{
	const uint32_t dma_base = get_dev_base(dev);

	if (DMA_CTL(dma_base, ch) != 0x00
	#ifdef CONFIG_SOC_SERIES_NPCK3
		|| IS_BIT_SET(DMA_CTL(dma_base, ch), NPCX_DMACTL_GDMAERR)
		|| IS_BIT_SET(DMA_CTL(dma_base, ch), NPCX_DMACTL_BUSY_EN)
	#elif CONFIG_SOC_SERIES_NPCX4
		|| IS_BIT_SET(DMA_CTL(dma_base, ch), NPCX_DMACTL_BMDAFIX)
		|| IS_BIT_SET(DMA_CTL(dma_base, ch), NPCX_DMACTL_BMSAFIX)
	#endif
	){
		LOG_INF("[FAIL]Default value");
	} else {
		LOG_INF("[PASS]Default value");
	}
}
static void dma_init(char *dev_num, char *ch_num)
{
	uint8_t dev_n = atoi(dev_num);
	uint8_t ch_n = atoi(ch_num);
	#ifdef CONFIG_SOC_SERIES_NPCX4
	if (dev_n < 2) {
		dev = (void *)dma_devices[dev_n];
	} else {
		LOG_ERR("Only device 0/1 available");
	}
	#else
	if (dev_n == 0) {
		dev = (void *)dma_devices[dev_n];
	} else {
		LOG_ERR("Only device 0 available");
	}
	#endif
	if (ch_n < 2) {
		ch = ch_n;
	} else {
		LOG_ERR("Only channel 0/1 available");
	}
}



int *npcx_power_down_gpd(const struct device *dev, const uint32_t channel,
					uint32_t val0, uint32_t val1)
{
	static int arr[2] = {0};
	const uint32_t dma_base = get_dev_base(dev);
	uint32_t ch0 = channel, ch1 = channel ^ 1;

	/* power down one channel */
	dma_set_power_down(dev, ch0, ENABLE); /* can't write ch0 */
	DMA_SRCB(dma_base, ch0) = val0;
	DMA_SRCB(dma_base, ch1) = val1;

	arr[0] = ((DMA_SRCB(dma_base, ch0)) == val0) ? 1 : 0;
	arr[1] = ((DMA_SRCB(dma_base, ch1)) == val1) ? 1 : 0;

	/* power up channel */
	dma_set_power_down(dev, ch0, DISABLE);

	return arr;
}

static void npcx_power_down_2(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t val1 = 0x11223344, val2 = 0x9ABCDEF0;

	for (int i = 0; i < NUM_DMA_DEVICE; i++) {
		for (uint8_t ch = 0; ch < MAX_DMA_CHANNELS; ch++) {
			int *res = npcx_power_down_gpd(dma_devices[i], ch, val1, val2);

			if (res[0]) {
				LOG_INF("[FAIL][GDMA%d]: GPD%d power down", i, ch);
			} else {
				LOG_INF("[PASS][GDMA%d]: GPD%d power down", i, ch);
			}
			if (res[1]) {
				LOG_INF("[PASS][GDMA%d]: GPD%d still run when GDP%d power down",
					i, ch ^ 1, ch);
			} else {
				LOG_INF("[FAIL][GDMA%d]: GPD%d still run when GDP%d power down",
					i, ch ^ 1, ch);
			}
		}
	}
}

static void npcx_power_save(const struct shell *shell, size_t argc, char **argv)
{
	const uint32_t dma_base = get_dev_base(dev);

	dma_set_power_save(dev, ch, ENABLE);

	memset((uint8_t *)GDMAMemPool, 0, MOVE_SIZE + 4);
	memset((uint8_t *)GDMAMemPool2, 0, MOVE_SIZE + 4);

	/* Minimal setting for DMA api */
	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = dma_cfg.dest_data_size = align_val;
	dma_block_cfg.block_size = MOVE_SIZE;
	dma_block_cfg.source_address = (uint32_t)GDMAMemPool;
	dma_block_cfg.dest_address = (uint32_t)GDMAMemPool2;

	for (uint32_t i = 0; i < MOVE_SIZE; i++) {
		*(uint8_t *)(dma_block_cfg.source_address + i) = 0x5a;
		*(uint8_t *)(dma_block_cfg.dest_address + i) = 0xa5;
	}

	/* isr event */
	dma_cfg.dma_callback = cb_power_save;
	dma_cfg.head_block = &dma_block_cfg;

	/* assign gps to dma_cfg reversed */
	dma_cfg.reserved |= GET_BIT(DMA_CTL(dma_base, ch), NPCX_DMACTL_GPS);

	if (dma_config(dev, ch, &dma_cfg)) {
		LOG_INF("ERROR: configure\n");
		return;
	}
	if (dma_start(dev, ch)) {
		LOG_INF("ERROR: transfer\n");
		return;
	}

	DMA_SRCB(dma_base, ch) = 0x9ABCDEF0;
	if (DMA_SRCB(dma_base, ch) != 0x9ABCDEF0) {
		LOG_INF("[FAIL][GDMA]: ch-%d write src address failed.", ch);
	}
	passFlag = 0;

	passFlag |= 1;
	for (uint32_t i = 0; i < MOVE_SIZE; i++) {
		if (GDMAMemPool2[i] != 0xa5) {
			passFlag = 0;
			break;
		}
	}
	dma_set_power_save(dev, ch, DISABLE);
}

/* base on chan_blen_transfer */
static void dma_api_example(const struct shell *shell, size_t argc, char **argv)
{
	sh_ptr = shell;
	/* simple memory(char) to memory(char) example*/
	const char tx_data[] __aligned(align_val) =
		"It is harder to be kind than to be wise........It is harder to ";
	char rx_data[64] __aligned(align_val) = { 0 };

	memset(rx_data, 0, sizeof(rx_data));

	/* Minimal setting for DMA api */
	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = dma_cfg.dest_data_size = align_val;
	dma_block_cfg.block_size = sizeof(tx_data);
	dma_block_cfg.source_address = (uint32_t)tx_data;
	dma_block_cfg.dest_address = (uint32_t)rx_data;

	/* isr event */
	dma_cfg.dma_callback = cb_data_check;
	dma_cfg.head_block = &dma_block_cfg;

	shell_info(sh_ptr, "Preparing DMA Controller: Name=%s, Chan_ID=%u", dev->name, ch);

	if (dma_config(dev, ch, &dma_cfg)) {
		LOG_INF("ERROR: configure\n");
		return;
	}
	if (dma_start(dev, ch)) {
		LOG_INF("ERROR: transfer\n");
		return;
	}
	check_char_data(tx_data, rx_data, 1);
}

static void dam_flash_to_ram(const struct shell *shell, size_t argc, char **argv)
{
	uint8_t opt = strtoul(argv[1], NULL, 0);
	uint8_t ram = strtoul(argv[2], NULL, 0);
	uint8_t t_sz = strtoul(argv[3], NULL, 0);

	memset((uint8_t *)MRAM, 0, MOVE_SIZE + 4);
	memset((uint8_t *)GDMAMemPool, 0, MOVE_SIZE + 4);
	uint32_t src, dst;

	src = (uint32_t)(gdma_test_flash_src[opt]);
	dst = ram ?  (uint32_t)MRAM : (uint32_t)GDMAMemPool;

	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_block_cfg.source_address = src;
	dma_block_cfg.dest_address = dst;
	dma_block_cfg.block_size = MOVE_SIZE + 4;

	dma_cfg.dma_callback = cb_data_check;

	dma_cfg.head_block = &dma_block_cfg;

	dma_cfg.source_data_size = dma_cfg.dest_data_size = t_sz;
	if (dma_config(dev, ch, &dma_cfg)) {
		LOG_INF("ERROR: configure\n");
		return;
	}
	if (dma_start(dev, ch)) {
		LOG_INF("ERROR: transfer\n");
		return;
	}

}

static void dam_ram_to_ram(const struct shell *shell, size_t argc, char **argv)
{
	sh_ptr = shell;
	uint8_t ram, t_sz;

	ram = strtoul(argv[1], NULL, 0);
	t_sz = strtoul(argv[2], NULL, 0);

	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	memset((uint8_t *) GDMAMemPool, 0, MOVE_SIZE + 4);
	memset((uint8_t *) MRAM, 0, MOVE_SIZE + 4);

	dma_block_cfg.block_size = MOVE_SIZE + 4;
	if (ram) {
		dma_block_cfg.source_address = (uint32_t)MRAM;
		dma_block_cfg.dest_address = (uint32_t)GDMAMemPool;
	} else {
		dma_block_cfg.source_address = (uint32_t)GDMAMemPool;
		dma_block_cfg.dest_address = (uint32_t)MRAM;
	}
	for (uint32_t i = 0; i < MOVE_SIZE; i++) {
		*(uint8_t *)(dma_block_cfg.source_address + i) =
		*(uint8_t *)(INT_FLASH_BASE1_ADDR + i);
	}

	dma_cfg.dma_callback = cb_data_check;

	dma_cfg.head_block = &dma_block_cfg;

	dma_cfg.source_data_size = dma_cfg.dest_data_size = t_sz;
	if (dma_config(dev, ch, &dma_cfg)) {
		LOG_INF("ERROR: configure\n");
		return;
	}
	if (dma_start(dev, ch)) {
		LOG_INF("ERROR: transfer\n");
		return;
	}
}
/* try use thread to implement parallel */
static void sync_data_transfer(const struct shell *shell, size_t argc, char **argv)
{
	sh_ptr = shell;
	uint8_t opt = strtoul(argv[1], NULL, 0);
	const struct device *dev = dma_devices[0], *dev1 = dma_devices[0];
	uint8_t dev1_channel = 0, dev2_channel = 0;

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
	const char tx_da[] __aligned(16) =
		"It is harder to be kind than to be wise........It is "
		"harder to be kind than to be wise........It is harder to b";
	char rx_da[128] __aligned(16) = { 0 };

	const char tx_da1[] __aligned(16) =
		"........wise be to than kind be to harder is It";
	char rx_da1[48] __aligned(16) = { 0 };

	memset(rx_da, 0, sizeof(rx_da));
	memset(rx_da1, 0, sizeof(rx_da1));

	dma_cfg.channel_direction = dma_cfg1.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = dma_cfg.dest_data_size = 1;
	dma_cfg1.source_data_size = dma_cfg1.dest_data_size = 1;

	dma_block_cfg.block_size = sizeof(tx_da);
	dma_block_cfg1.block_size = sizeof(tx_da1);

	dma_block_cfg.source_address = (uint32_t)tx_da;
	dma_block_cfg.dest_address = (uint32_t)rx_da;
	dma_cfg.head_block = &dma_block_cfg;
	dma_cfg.dma_callback = cb_data_check;

	dma_block_cfg1.source_address = (uint32_t)tx_da1;
	dma_block_cfg1.dest_address = (uint32_t)rx_da1;
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
	check_char_data(tx_da, rx_da, 1);
	check_char_data(tx_da1, rx_da1, 1);
}

static void dma_validation_func(void *dummy1, void *dummy2, void *dummy3)
{
	uint32_t events;

	k_event_init(&dma_event);

	while (true) {
		events = k_event_wait(&dma_event, 0xFFF, true, K_FOREVER);
		switch (events) {
		case 0x008:
			if (!strcmp("init", arguments[0])) {
				LOG_INF("DMA init device and channel");
				dma_init(arguments[1], arguments[2]);
			}
			break;
		default:
			LOG_INF("Error command\n");
			break;
		}
	}
}

int dma_command(const struct shell *shell, size_t argc, char **argv)
{
	int i, evt;

	evt = 1;
	for (evt = 1, i = 1; i < argc; i++) {
		strcpy(arguments[i - 1], argv[i]);
		evt <<= 1;
	}
	k_event_post(&dma_event, evt);
	return 0;
}


void test_dma_init(void)
{
	LOG_INF("--- CI20 Zephyr DMA Driver Validation abc ---");
	for (int i = 0 ; i < NUM_DMA_DEVICE; i++) {
		if (!device_is_ready(dma_devices[i])) {
			LOG_ERR("dma device %s is not ready", dma_devices[i]->name);
			return;
		}
		LOG_INF("dma device [%d]:%s is ready", i, dma_devices[i]->name);
	}

	k_thread_create(&temp_id, temp_stack, TASK_STACK_SIZE, dma_validation_func,
		NULL, NULL, NULL, PRIORITY, K_INHERIT_PERMS, K_FOREVER);
	k_thread_name_set(&temp_id, "DMA Validation");
	k_thread_start(&temp_id);
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_dma, SHELL_CMD_ARG(ex, NULL, "dma ex: dma api test",
		dma_api_example, 1, 0),
	SHELL_CMD_ARG(gpd, NULL, "dma gpd: dma power down test",
		npcx_power_down_2, 1, 0),
	SHELL_CMD_ARG(gps, NULL, "dma gps: dma power save test",
		npcx_power_save, 1, 0),
	SHELL_CMD_ARG(sync, NULL, "dma sync <1~6>",
		sync_data_transfer, 2, 0),
	SHELL_CMD_ARG(c3, NULL, "dma c3 init <dev> <channel>: choose dev and channel",
		dma_command, 4, 0),
	SHELL_CMD_ARG(def, NULL, "dma def: check default value of register",
		check_def_value, 1, 0),
	#ifdef CONFIG_SOC_SERIES_NPCK3
	SHELL_CMD_ARG(err, NULL, "dma err: gdmaerr test",
		dma_gdmaerr, 1, 0),
	#endif
	SHELL_CMD_ARG(ram, NULL, "dma ram <0/1> <1B/1W/1DW/Burst>",
		dam_ram_to_ram, 3, 0),
	SHELL_CMD_ARG(flash, NULL, "dma flash <int/ext/bkp flash> <GDMAMemPool/code ram>"
		"<1B/1W/1DW/Burst>", dam_flash_to_ram, 4, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(dma, &sub_dma, "nuvoton dma validation", NULL);

/* Main entry */
int main(void)
{
	test_dma_init();

	return 0;
}