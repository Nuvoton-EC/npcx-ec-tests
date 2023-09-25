/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <soc.h>
#include <stdlib.h>
#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#if defined(CONFIG_ESPI_SAF)
#include <zephyr/drivers/espi_saf.h>
#include <zephyr/drivers/flash.h>
#include "taf_util.h"
#endif
#if defined(CONFIG_FLASH_NPCX_FIU_NAND_INIT)
#include <zephyr/drivers/flash/npcx_flash_api_ex.h>
#include <zephyr/../../drivers/flash/spi_nand.h>
#endif

LOG_MODULE_REGISTER(espi_saf);

#define ESPI_BASE_ADDR			0x4000A000
#define ESPI_FREQ_20MHZ			20u
#define ESPI_FREQ_25MHZ			25u

#define K_WAIT_DELAY			100u

/* eSPI flash parameters */
#define MAX_TEST_BUF_SIZE		1024u
#define MAX_FLASH_REQUEST		64u
#if defined(CONFIG_SOC_SERIES_NPCK3) || defined(CONFIG_SOC_SERIES_NPCX4)
/* Max Flash tx buffer is 64-byte in NPCK3/NPCX4 EC */
#define MAX_FLASH_WRITE_REQUEST		64u
#else
/* Max Flash tx buffer is 16-byte in NPCX EC */
#define MAX_FLASH_WRITE_REQUEST		16u
#endif
#define MAX_FLASH_ERASE_BLOCK		64u
#define TARGET_FLASH_REGION		0x72000ul

/* eSPI event */
#define EVENT_MASK			0x0000FFFFu
#define EVENT_DETAILS_MASK		0xFFFF0000u
#define EVENT_DETAILS_POS		16u
#define EVENT_TYPE(x)			(x & EVENT_MASK)
#define EVENT_DETAILS(x)		((x & EVENT_DETAILS_MASK) >> EVENT_DETAILS_POS)
#define SPI_NAND_PAGE_SIZE		0x0800U
#define NUM_FLASH_DEVICE		ARRAY_SIZE(flash_devices)

static const struct device *const espi_dev = DEVICE_DT_GET(DT_NODELABEL(espi0));
#if defined(CONFIG_ESPI_SAF)
static const struct device *const taf_dev = DEVICE_DT_GET(DT_NODELABEL(espi_taf));
static const struct device *const spi_dev = DEVICE_DT_GET(DT_ALIAS(taf_flash));
#endif

static struct espi_callback espi_bus_cb;
static struct espi_callback vw_rdy_cb;
static struct espi_callback vw_cb;
static struct espi_callback p80_cb;
#if defined(CONFIG_ESPI_SAF) && defined(CONFIG_ESPI_FLASH_CHANNEL)
static struct espi_callback espi_taf_cb;
#endif
static uint8_t espi_rst_sts;
static uint8_t nand_data_buf[SPI_NAND_PAGE_SIZE];

#ifdef CONFIG_ESPI_FLASH_CHANNEL
#if defined(CONFIG_ESPI_SAF)
static uint8_t tx_buf_data[MAX_TEST_BUF_SIZE];
#endif
#endif

static const struct espi_saf_pr flash_protect_regions[2] = {
	{
		.start = 0x10004U,
		.end =  0x10004U,
		.override_r = 0x5A5A,
		.override_w = 0xA5A5,
		.master_bm_we = 1,
		.master_bm_rd = 0,
		.pr_num = 0,
		.flags = 1,
	},
	{
		.start = 0x10006U,
		.end =  0x10006U,
		.override_r = 0x5A5A,
		.override_w = 0xA5A5,
		.master_bm_we = 0,
		.master_bm_rd = 1,
		.pr_num = 1,
		.flags = 1,
	},
};

static struct espi_saf_cfg taf_cfg_flash = {
	.nflash_devices = 1U,
	.hwcfg = {
		.mode = ESPI_TAF_AUTO_MODE,
	},
};

static const struct espi_saf_protection taf_pr_flash = {
	.nregions = 2U,
	.pregions = flash_protect_regions
};

void flash_handler(struct k_work *item);

static void host_warn_handler(uint32_t signal, uint32_t status)
{
	switch (signal) {
	case ESPI_VWIRE_SIGNAL_HOST_RST_WARN:
		LOG_INF("Host reset warning %d", status);
		if (!IS_ENABLED(CONFIG_ESPI_AUTOMATIC_WARNING_ACKNOWLEDGE)) {
			LOG_INF("HOST RST ACK %d", status);
			espi_send_vwire(espi_dev, ESPI_VWIRE_SIGNAL_HOST_RST_ACK, status);
		}
		break;
	case ESPI_VWIRE_SIGNAL_SUS_WARN:
		LOG_INF("Host suspend warning %d", status);
		if (!IS_ENABLED(CONFIG_ESPI_AUTOMATIC_WARNING_ACKNOWLEDGE)) {
			LOG_INF("SUS ACK %d", status);
			espi_send_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SUS_ACK, status);
		}
		break;
	default:
		break;
	}
}

/* eSPI bus event handler */
static void espi_reset_handler(const struct device *dev, struct espi_callback *cb,
			       struct espi_event event)
{
	if (event.evt_type == ESPI_BUS_RESET) {
		espi_rst_sts = event.evt_data;
		LOG_INF("eSPI BUS reset %d", event.evt_data);
	}
}

/* eSPI logical channels enable/disable event handler */
static void espi_ch_handler(const struct device *dev, struct espi_callback *cb,
			    struct espi_event event)
{
	if (event.evt_type == ESPI_BUS_EVENT_CHANNEL_READY) {
		switch (event.evt_details) {
		case ESPI_CHANNEL_VWIRE:
			LOG_INF("VW channel event %x", event.evt_data);
			break;
		case ESPI_CHANNEL_FLASH:
			LOG_INF("Flash channel event %d", event.evt_data);
			break;
		case ESPI_CHANNEL_OOB:
			LOG_INF("OOB channel event %d", event.evt_data);
			break;
		default:
			LOG_ERR("Unknown channel event");
		}
	}
}

static void vwire_handler(const struct device *dev, struct espi_callback *cb,
			  struct espi_event event)
{
	if (event.evt_type == ESPI_BUS_EVENT_VWIRE_RECEIVED) {
		switch (event.evt_details) {
		case ESPI_VWIRE_SIGNAL_PLTRST:
			LOG_INF("PLT_RST changed %d", event.evt_data);
			break;
		case ESPI_VWIRE_SIGNAL_SLP_S3:
		case ESPI_VWIRE_SIGNAL_SLP_S4:
		case ESPI_VWIRE_SIGNAL_SLP_S5:
			LOG_INF("SLP signal changed %d", event.evt_data);
			break;
		case ESPI_VWIRE_SIGNAL_SUS_WARN:
		case ESPI_VWIRE_SIGNAL_HOST_RST_WARN:
			host_warn_handler(event.evt_details, event.evt_data);
			break;
		}
	}
}

/* eSPI peripheral channel notifications handler */
static void periph_handler(const struct device *dev, struct espi_callback *cb,
			   struct espi_event event)
{
	uint8_t periph_type;
	uint8_t periph_index;

	periph_type = EVENT_TYPE(event.evt_details);
	periph_index = EVENT_DETAILS(event.evt_details);

	switch (periph_type) {
	case ESPI_PERIPHERAL_DEBUG_PORT80:
		LOG_INF("Postcode %x", event.evt_data);
		break;
	case ESPI_PERIPHERAL_HOST_IO:
		LOG_INF("ACPI %x", event.evt_data);
		espi_remove_callback(espi_dev, &p80_cb);
		break;
	default:
		LOG_INF("%s periph 0x%x [%x]", __func__, periph_type, event.evt_data);
	}
}

#ifdef CONFIG_ESPI_FLASH_CHANNEL
#if defined(CONFIG_FLASH_NPCX_FIU_NAND_INIT)
/* Get look up table for NAND flash */
static int nand_flash_get_lut(const struct device *flash_dev,
			      struct nand_flash_lut *lut_ptr)
{
	return flash_ex_op(flash_dev, FLASH_NPCX_EX_OP_NAND_GET_BAD_BLOCK_LUT,
			   (uintptr_t)NULL, lut_ptr);
}

/* Check access region is invalid or not */
bool nand_invalid_check(uint32_t addr, uint32_t len)
{
	struct nand_flash_lut lut_table;
	uint8_t bb_cnt;
	uint16_t cnt;
	uint32_t st_addr = addr;
	uint32_t ed_addr = st_addr + len - 1;
	uint32_t bk_start, bk_end;

	nand_flash_get_lut(spi_dev, &lut_table);
	bb_cnt = lut_table.bbt_count;

	if (bb_cnt > 0 && bb_cnt < 20) {
		for (cnt = 0; cnt < bb_cnt; cnt++) {
			bk_start = _128KB_ * lut_table.bbt_list[cnt];
			bk_end = bk_start + _128KB_ - 1;
			if (bk_start <= ed_addr && st_addr <= bk_end)
				return true;
		}
		return false;
	} else {
		return false;
	}
}
#endif /* CONFIG_FLASH_NPCX_FIU_NAND_INIT */

#if defined(CONFIG_ESPI_SAF)
int taf_npcx_flash_read(const struct device *dev, struct taf_handle_data *info)
{
	struct espi_taf_npcx_pckt taf_data;
	struct espi_saf_packet pckt_taf;

#if defined(CONFIG_FLASH_NPCX_FIU_NAND_INIT)
	if (nand_invalid_check(info->address, info->length)) {
		LOG_ERR("Access NAND invalid region");
		return -EINVAL;
	}
#endif

	pckt_taf.flash_addr = info->address;
	pckt_taf.len = info->length;
	taf_data.tag = info->taf_tag;
	taf_data.data = (uint8_t *)info->buf;
	pckt_taf.buf = (uint8_t *)&taf_data;

	return espi_saf_flash_read(dev, &pckt_taf);
}

int taf_npcx_flash_write(const struct device *dev, struct taf_handle_data *info)
{
	struct espi_taf_npcx_pckt taf_data;
	struct espi_saf_packet pckt_taf;
#if defined(CONFIG_FLASH_NPCX_FIU_NAND_INIT)
	uint8_t data_buf[512];

	if (info->length < 512) {
		memset(data_buf, 0xFF, sizeof(data_buf));
		memcpy(&data_buf, info->src, info->length);
		info->length = 512;
	}

	if (nand_invalid_check(info->address, info->length)) {
		LOG_ERR("Access NAND flash invalid region");
		return -EINVAL;
	}
#endif

	pckt_taf.flash_addr = info->address;
	pckt_taf.len = info->length;
	taf_data.tag = info->taf_tag;
#if defined(CONFIG_FLASH_NPCX_FIU_NAND_INIT)
	taf_data.data = (uint8_t *)&data_buf;
#else
	taf_data.data = (uint8_t *)info->src;
#endif
	pckt_taf.buf = (uint8_t *)&taf_data;

	return espi_saf_flash_write(dev, &pckt_taf);
}

int taf_npcx_flash_erase(const struct device *dev, struct taf_handle_data *info)
{
	struct espi_taf_npcx_pckt taf_data;
	struct espi_saf_packet pckt_taf;
	uint32_t len;
	int erase_blk[4] = {_4KB_, _32KB_, _64KB_, _128KB_};

	len = erase_blk[info->length];

#if defined(CONFIG_FLASH_NPCX_FIU_NAND_INIT)
	if (len != _128KB_) {
		LOG_ERR("Erase size not meet 128KB alignment");
		return -EINVAL;
	}

	if (nand_invalid_check(info->address, len)) {
		LOG_ERR("Access invalid NAND flash region");
		return -EINVAL;
	}
#endif

	pckt_taf.flash_addr = info->address;
	pckt_taf.len = len;
	taf_data.tag = info->taf_tag;
	taf_data.data = (uint8_t *)info->buf;
	pckt_taf.buf = (uint8_t *)&taf_data;
	return espi_saf_flash_erase(dev, &pckt_taf);
}

int taf_npcx_flash_unsupport(const struct device *dev, struct taf_handle_data *info)
{
	struct espi_taf_npcx_pckt taf_data;
	struct espi_saf_packet pckt_taf;

	pckt_taf.flash_addr = info->address;
	pckt_taf.len = info->length;
	taf_data.tag = info->taf_tag;
	taf_data.data = (uint8_t *)info->buf;
	pckt_taf.buf = (uint8_t *)&taf_data;

	espi_send_vwire(espi_dev, ESPI_VWIRE_SIGNAL_ERR_NON_FATAL, 1);

	return espi_saf_flash_unsuccess(dev, &pckt_taf);
}

/* Parse the information for eSPI TAF */
void espi_taf_handler(const struct device *dev, struct taf_handle_data *pckt,
		      struct espi_event event)
{
	struct espi_taf_pckt *data_ptr;

	data_ptr = (struct espi_taf_pckt *)event.evt_data;

	pckt->taf_type = data_ptr->type;
	pckt->length = data_ptr->len;
	pckt->taf_tag = data_ptr->tag;
	pckt->address = data_ptr->addr;
	pckt->buf = tx_buf_data;
	memcpy(pckt->src, data_ptr->src, pckt->length);
}

/* eSPI TAF event handler */
static void espi_taf_ev_handler(const struct device *dev, struct espi_callback *cb,
				struct espi_event event)
{
	if (event.evt_type == ESPI_BUS_SAF_NOTIFICATION) {
		if (event.evt_details == ESPI_CHANNEL_FLASH) {
			espi_taf_handler(dev, &taf_data, event);
			k_work_submit(&taf_data.work);
		}
	}
}
#endif /* CONFIG_ESPI_SAF */
#endif /* CONFIG_ESPI_FLASH_CHANNEL */

int espi_init(void)
{
	int ret;
	struct espi_cfg cfg = {
		.io_caps = ESPI_IO_MODE_SINGLE_LINE,
		.channel_caps = ESPI_CHANNEL_VWIRE | ESPI_CHANNEL_PERIPHERAL,
		.max_freq = ESPI_FREQ_20MHZ,
	};

	cfg.channel_caps |= ESPI_CHANNEL_OOB;
	cfg.channel_caps |= ESPI_CHANNEL_FLASH;
	cfg.io_caps |= (ESPI_IO_MODE_QUAD_LINES | ESPI_IO_MODE_DUAL_LINES);
	cfg.max_freq = ESPI_FREQ_25MHZ;

	ret = espi_config(espi_dev, &cfg);
	if (ret) {
		LOG_ERR("Failed to configure eSPI target channels:%x err: %d",
			cfg.channel_caps, ret);
		return ret;
	}

	LOG_INF("eSPI target configured successfully!");
	LOG_INF("eSPI test - callbacks initialization... ");

	espi_init_callback(&espi_bus_cb, espi_reset_handler, ESPI_BUS_RESET);
	espi_init_callback(&vw_rdy_cb, espi_ch_handler, ESPI_BUS_EVENT_CHANNEL_READY);
	espi_init_callback(&vw_cb, vwire_handler, ESPI_BUS_EVENT_VWIRE_RECEIVED);
	espi_init_callback(&p80_cb, periph_handler, ESPI_BUS_PERIPHERAL_NOTIFICATION);
#if defined(CONFIG_ESPI_SAF) && defined(CONFIG_ESPI_FLASH_CHANNEL)
	espi_init_callback(&espi_taf_cb, espi_taf_ev_handler, ESPI_BUS_SAF_NOTIFICATION);
#endif
	LOG_INF("complete");
	LOG_INF("eSPI test - callbacks registration... ");
	espi_add_callback(espi_dev, &espi_bus_cb);
	espi_add_callback(espi_dev, &vw_rdy_cb);
	espi_add_callback(espi_dev, &vw_cb);
	espi_add_callback(espi_dev, &p80_cb);
#if defined(CONFIG_ESPI_SAF) && defined(CONFIG_ESPI_FLASH_CHANNEL)
	espi_add_callback(espi_dev, &espi_taf_cb);
#endif
	LOG_INF("complete");
	return ret;
}

static int wait_for_vwire(const struct device *espi_dev, enum espi_vwire_signal signal,
			  uint16_t timeout, uint8_t exp_level)
{
	int ret;
	uint8_t level;
	uint16_t loop_cnt = timeout;

	do {
		ret = espi_receive_vwire(espi_dev, signal, &level);
		if (ret) {
			LOG_ERR("Failed to read %x %d", signal, ret);
			return -EIO;
		}

		if (exp_level == level) {
			break;
		}

		k_usleep(K_WAIT_DELAY);
		loop_cnt--;
	} while (loop_cnt > 0);

	if (loop_cnt == 0) {
		LOG_ERR("VWIRE %d is %x", signal, level);
		return -ETIMEDOUT;
	}

	return 0;
}

static int wait_for_espi_reset(uint8_t exp_sts)
{
	uint16_t loop_cnt = CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT;

	do {
		if (exp_sts == espi_rst_sts) {
			break;
		}
		k_usleep(K_WAIT_DELAY);
		loop_cnt--;
	} while (loop_cnt > 0);

	if (loop_cnt == 0)
		return -ETIMEDOUT;

	return 0;
}

int espi_handshake(void)
{
	int ret;

	LOG_INF("eSPI test - Handshake with eSPI master...");
	ret = wait_for_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SUS_WARN,
				 CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT, 1);
	if (ret) {
		LOG_ERR("SUS_WARN Timeout");
		return ret;
	}

	LOG_INF("1st phase completed");
	ret = wait_for_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SLP_S5,
				 CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT, 1);
	if (ret) {
		LOG_ERR("SLP_S5 Timeout");
		return ret;
	}

	ret = wait_for_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SLP_S4,
				 CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT, 1);
	if (ret) {
		LOG_ERR("SLP_S4 Timeout");
		return ret;
	}

	ret = wait_for_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SLP_S3,
				 CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT, 1);
	if (ret) {
		LOG_ERR("SLP_S3 Timeout");
		return ret;
	}

	LOG_INF("2nd phase completed");

	ret = wait_for_vwire(espi_dev, ESPI_VWIRE_SIGNAL_PLTRST,
				 CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT, 1);
	if (ret) {
		LOG_ERR("PLT_RST Timeout");
		return ret;
	}

	LOG_INF("3rd phase completed");

	return 0;
}

static void espi_taf_thread_entry(void)
{
	int ret;
	int enable = 1;

	if (!device_is_ready(espi_dev)) {
		LOG_ERR("espi device %s not ready", espi_dev->name);
		return;
	}

	LOG_INF("espi device %s is ready", espi_dev->name);

	espi_init();

	espi_write_lpc_request(espi_dev, ECUSTOM_HOST_SUBS_INTERRUPT_EN, &enable);

	ret = wait_for_espi_reset(1);
	if (ret) {
		LOG_INF("ESPI_RESET timeout");
		return;
	}

	ret = espi_handshake();
	if (ret) {
		LOG_ERR("eSPI VW handshake failed %d", ret);
		return;
	}
	LOG_INF("eSPI sample completed err: %d", ret);

	espi_saf_config(taf_dev, &taf_cfg_flash);
	espi_saf_activate(taf_dev);

	k_work_init(&taf_data.work, flash_handler);
}

void flash_handler(struct k_work *item)
{
	struct taf_handle_data *info = CONTAINER_OF(item, struct taf_handle_data, work);
	int ret = 0;

	switch (info->taf_type & 0x0F) {
	case ESPI_FLASH_TAF_REQ_READ:
		ret = taf_npcx_flash_read(taf_dev, &taf_data);
		break;
	case ESPI_FLASH_TAF_REQ_ERASE:
		ret = taf_npcx_flash_erase(taf_dev, &taf_data);
		break;
	case ESPI_FLASH_TAF_REQ_WRITE:
		ret = taf_npcx_flash_write(taf_dev, &taf_data);
		break;
	}

	if (ret != 0) {
		ret = taf_npcx_flash_unsupport(taf_dev, &taf_data);
	}
}

#define STACK_SIZE	1024
#define THREAD_PRIORITY 1

K_THREAD_DEFINE(espi_taf_id, STACK_SIZE, espi_taf_thread_entry, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, -1);

void test_espi_taf_init(void)
{
	k_thread_name_set(espi_taf_id, "espi_taf_testing");
	k_thread_start(espi_taf_id);
}

static int flash_erase_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	char *eptr;
	/* Convert integer from string */
	uint32_t addr = strtoul(argv[1], &eptr, 0);

	if (*eptr != '\0') {
		shell_error(shell, "Invalid argument, '%s' is not an integer", argv[1]);
		return -EINVAL;
	}

	ret = flash_erase(spi_dev, addr, _128KB_);
	if (ret != 0) {
		LOG_ERR("flash erase failed: %d", ret);
		return -ENODEV;
	}

	LOG_INF("Flash erase succeeded!");
	LOG_INF("[GO]");
	return 0;
}

static int flash_write_cmd(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t data_sz_in = 0x10000U;
	uint32_t wr_size;
	char *eptr;
	int i, ret;
	uint32_t addr_in = strtoul(argv[1], &eptr, 0);

	if (*eptr != '\0') {
		shell_error(shell, "Invalid argument, '%s' is not an integer", argv[1]);
		return -EINVAL;
	}

	/* check address and size sector (512 byte) alignment */
	if ((addr_in & 0x1ff) != 0 || (data_sz_in & 0x1ff) != 0) {
		LOG_ERR("flash addr or size not 512 byte alignment");
		return -ENODEV;
	}

	while (data_sz_in > 0) {
		if (data_sz_in >= sizeof(nand_data_buf)) {
			wr_size = sizeof(nand_data_buf);
		} else {
			wr_size = data_sz_in;
		}

		(void)memset(nand_data_buf, 0, sizeof(nand_data_buf));

		for (i = 0 ; i < wr_size ; i++) {
			nand_data_buf[i]  = CAL_DATA_FROM_ADDR((addr_in + i));
		}

		ret = flash_write(spi_dev, addr_in, nand_data_buf, wr_size);
		if (ret != 0) {
			LOG_ERR("flash erase failed: %d", ret);
			return -ENODEV;
		}

		data_sz_in -= wr_size;
		addr_in += wr_size;
	}

	LOG_INF("Flash write succeeded!");
	LOG_INF("[GO]");
	return 0;
}

int cmd_flash_protection(void)
{
	espi_saf_set_protection_regions(taf_dev, &taf_pr_flash);
	return 0;
}

static int cmd_set_taf_flcapa(const struct shell *shell, size_t argc, char **argv)
{
	char *eptr;
	uint32_t val = strtoul(argv[1], &eptr, 0);
	struct espi_reg *const inst = (struct espi_reg *)ESPI_BASE_ADDR;

	if (*eptr != '\0') {
		shell_error(shell, "Invalid argument, '%s' is not an integer", argv[1]);
		return -EINVAL;
	}

	if ((val < 0) || (val > 3)) {
		shell_error(shell, "Invalid argument 0 - 3 (%s)", argv[1]);
		return -EINVAL;
	}

	SET_FIELD(inst->FLASHCFG, NPCX_FLASHCFG_FLCAPA, val);

	return 0;
}

static int cmd_check_taf_readsize(const struct shell *shell, size_t argc, char **argv)
{
	char *eptr;
	uint32_t reqsize;
	uint32_t val = strtoul(argv[1], &eptr, 0);
	struct espi_reg *const inst = (struct espi_reg *)ESPI_BASE_ADDR;

	if (*eptr != '\0') {
		shell_error(shell, "Invalid argument, '%s' is not an integer", argv[1]);
		return -EINVAL;
	}

	if ((val < 0) || (val > 7)) {
		shell_error(shell, "Invalid argument 0 - 7 (%s)", argv[1]);
		return -EINVAL;
	}

	reqsize = GET_FIELD(inst->FLASHCFG, NPCX_FLASHCFG_FLASHREQSIZE);
	if (reqsize == val) {
		LOG_INF("[PASS] FLASHREQSIZE test (%s)", argv[1]);
		LOG_INF("[GO]");
	} else {
		LOG_INF("[FAIL] FLASHREQSIZE test (%x)", reqsize);
	}

	return 0;
}

static int cmd_check_taf_plsize(const struct shell *shell, size_t argc, char **argv)
{
	char *eptr;
	uint32_t plsize;
	uint32_t val = strtoul(argv[1], &eptr, 0);
	struct espi_reg *const inst = (struct espi_reg *)ESPI_BASE_ADDR;

	if (*eptr != '\0') {
		shell_error(shell, "Invalid argument, '%s' is not an integer", argv[1]);
		return -EINVAL;
	}

	if ((val < 1) || (val > 3)) {
		shell_error(shell, "Invalid argument 0 - 3 (%s)", argv[1]);
		return -EINVAL;
	}

	plsize = GET_FIELD(inst->FLASHCFG, NPCX_FLASHCFG_FLASHPLSIZE);
	if (plsize == val) {
		LOG_INF("[PASS] FLASHPLSIZE test (%s)", argv[1]);
		LOG_INF("[GO]");
	} else {
		LOG_INF("[FAIL] FLASHPLSIZE test (%x)", plsize);
	}

	return 0;
}

static int cmd_set_taf_erasesize(const struct shell *shell, size_t argc, char **argv)
{
	char *eptr;
	uint32_t val = strtoul(argv[1], &eptr, 0);
	struct espi_reg *const inst = (struct espi_reg *)ESPI_BASE_ADDR;

	if (*eptr != '\0') {
		shell_error(shell, "Invalid argument, '%s' is not an integer", argv[1]);
		return -EINVAL;
	}

	if ((val < 0) || (val > 7)) {
		shell_error(shell, "Invalid argument 0 - 7 (%s)", argv[1]);
		return -EINVAL;
	}

	SET_FIELD(inst->FLASHCFG, NPCX_FLASHCFG_TRGFLEBLKSIZE, BIT(val));

	return 0;
}

static int cmd_set_taf_mode(const struct shell *shell, size_t argc, char **argv)
{
	char *eptr;
	uint32_t val = strtoul(argv[1], &eptr, 0);

	if ((val < 0) || (val > 7)) {
		shell_error(shell, "Invalid argument 0 - 1 (%s)", argv[1]);
		return -EINVAL;
	}

	taf_cfg_flash.hwcfg.mode = val;

	espi_saf_config(taf_dev, &taf_cfg_flash);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_espi,
	SHELL_CMD_ARG(flash_erase, NULL, "espi_taf flash_erase <addr>",
		flash_erase_cmd, 2, 0),
	SHELL_CMD_ARG(flash_write, NULL, "espi_taf flash_write <addr>",
		flash_write_cmd, 2, 0),
	SHELL_CMD_ARG(set_flcapa, NULL, "espi_taf set_flcapa <val>, val = 0 - 3",
		cmd_set_taf_flcapa, 2, 0),
	SHELL_CMD_ARG(check_rdsize, NULL, "espi_taf check_rdsize <val>, val = 0 - 7",
		cmd_check_taf_readsize, 2, 0),
	SHELL_CMD_ARG(check_plsize, NULL, "espi_taf check_plsize <val>, val = 1 - 3",
		cmd_check_taf_plsize, 2, 0),
	SHELL_CMD_ARG(set_ersize, NULL, "espi_taf set_ersize <val>, val = 0 - 7",
		cmd_set_taf_erasesize, 2, 0),
	SHELL_CMD_ARG(set_taf_mode, NULL, "espi_taf set_taf_mode <val>, val = 0 - 1",
		cmd_set_taf_mode, 2, 0),
	SHELL_CMD(set_pr, NULL, "espi_taf set_pr", cmd_flash_protection),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(espi_taf, &sub_espi, "Test Commands", NULL);
