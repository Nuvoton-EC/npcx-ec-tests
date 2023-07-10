/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#if defined(CONFIG_ESPI_SAF)
#include <../soc/arm/nuvoton_npcx/common/soc_espi_saf.h>
#include <zephyr/drivers/espi_saf.h>
#include <zephyr/drivers/flash.h>
#include "../../nand_flash/src/flash_access.h"
#include "../../nand_flash/src/spi_nand.h"
#include "saf_util.h"
#endif

LOG_MODULE_REGISTER(espi_saf);

#define ESPI_FREQ_20MHZ	   			20u
#define ESPI_FREQ_25MHZ    			25u

#define K_WAIT_DELAY          		100u

/* eSPI flash parameters */
#define MAX_TEST_BUF_SIZE           1024u
#define MAX_FLASH_REQUEST           64u
#if defined(CONFIG_SOC_SERIES_NPCK3)
/* Max Flash tx buffer is 64-byte in NPCK EC */
#define MAX_FLASH_WRITE_REQUEST     64u
#else
/* Max Flash tx buffer is 16-byte in NPCX EC */
#define MAX_FLASH_WRITE_REQUEST     16u
#endif
#define MAX_FLASH_ERASE_BLOCK       64u
#define TARGET_FLASH_REGION         0x72000ul

/* eSPI event */
#define EVENT_MASK            0x0000FFFFu
#define EVENT_DETAILS_MASK    0xFFFF0000u
#define EVENT_DETAILS_POS     16u
#define EVENT_TYPE(x)         (x & EVENT_MASK)
#define EVENT_DETAILS(x)      ((x & EVENT_DETAILS_MASK) >> EVENT_DETAILS_POS)

static const struct device *const espi_dev
				 = DEVICE_DT_GET(DT_NODELABEL(espi0));
#if defined(CONFIG_ESPI_SAF)
static const struct device *const saf_dev
				 = DEVICE_DT_GET(DT_NODELABEL(espi_saf));
#if defined(CONFIG_SOC_SERIES_NPCK3)
static const struct device *const spi_dev
				 = DEVICE_DT_GET(DT_ALIAS(spi_flash0));
#endif
#endif

static struct espi_callback espi_bus_cb;
static struct espi_callback vw_rdy_cb;
static struct espi_callback vw_cb;
static struct espi_callback p80_cb;
#if defined(CONFIG_ESPI_SAF) && defined(CONFIG_ESPI_FLASH_CHANNEL)
static struct espi_callback espi_saf_cb;
#endif
static uint8_t espi_rst_sts;

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

static const struct espi_saf_cfg saf_cfg_nand = {
	.nflash_devices = 1U,
};

static const struct espi_saf_protection saf_pr_flash = {
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
			espi_send_vwire(espi_dev,
					ESPI_VWIRE_SIGNAL_HOST_RST_ACK,
					status);
		}
		break;
	case ESPI_VWIRE_SIGNAL_SUS_WARN:
		LOG_INF("Host suspend warning %d", status);
		if (!IS_ENABLED(CONFIG_ESPI_AUTOMATIC_WARNING_ACKNOWLEDGE)) {
			LOG_INF("SUS ACK %d", status);
			espi_send_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SUS_ACK,
					status);
		}
		break;
	default:
		break;
	}
}

/* eSPI bus event handler */
static void espi_reset_handler(const struct device *dev,
				   struct espi_callback *cb,
				   struct espi_event event)
{
	if (event.evt_type == ESPI_BUS_RESET) {
		espi_rst_sts = event.evt_data;
		LOG_INF("eSPI BUS reset %d", event.evt_data);
	}
}

/* eSPI logical channels enable/disable event handler */
static void espi_ch_handler(const struct device *dev,
				struct espi_callback *cb,
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

static void vwire_handler(const struct device *dev,
				struct espi_callback *cb,
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
			host_warn_handler(event.evt_details,
						  event.evt_data);
			break;
		}
	}
}

/* eSPI peripheral channel notifications handler */
static void periph_handler(const struct device *dev,
				struct espi_callback *cb,
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
		LOG_INF("%s periph 0x%x [%x]", __func__, periph_type,
			event.evt_data);
	}
}

#ifdef CONFIG_ESPI_FLASH_CHANNEL
#if defined(CONFIG_ESPI_SAF)
int flash_nand_read(const struct device *dev,
						uint32_t addr,
						uint8_t *dest,
						uint16_t size) {
	uint8_t* data_ptr = (uint8_t*)dest;
	int rc;

	LOG_INF("Read address: 0x%x, length: 0x%x", addr, size);

	rc = spi_nand_single_read(dev, addr, data_ptr, size);

	return rc;
}

int flash_nand_write(const struct device *dev,
						uint32_t addr,
			 			uint8_t *src,
						uint16_t size) {
	int rc;

	LOG_INF("Write address: 0x%x, length: 0x%x", addr, size);

	rc = spi_nand_write(dev, addr, src, size);

	return rc;
}

int flash_nand_erase(const struct device *dev,
								uint32_t addr,
								uint16_t size) {
	int rc;

	LOG_INF("Erase address: 0x%x, length: 0x%x", addr, size);

	rc = spi_nand_erase(dev, addr, _128KB_);

	return rc;
}

int saf_npcx_flash_read(const struct device *dev,
							struct saf_handle_data *info) {

	struct espi_saf_npcx_pckt saf_data;
	struct espi_saf_packet pckt_saf;
	pckt_saf.flash_addr = info->address;
	pckt_saf.len = info->length;
	saf_data.tag = info->saf_tag;
	saf_data.data = (uint8_t*)info->buf;
	pckt_saf.buf = (uint8_t*)&saf_data;

	return espi_saf_flash_read(dev, &pckt_saf);
}

int saf_npcx_flash_write(const struct device *dev,
							struct saf_handle_data *info) {
	struct espi_saf_npcx_pckt saf_data;
	struct espi_saf_packet pckt_saf;
	pckt_saf.flash_addr = info->address;
	pckt_saf.len = info->length;
	saf_data.tag = info->saf_tag;
	saf_data.data = (uint8_t*)info->src;
	pckt_saf.buf = (uint8_t*)&saf_data;
	return espi_saf_flash_write(dev, &pckt_saf);
}

int saf_npcx_flash_erase(const struct device *dev,
							struct saf_handle_data *info) {
	struct espi_saf_npcx_pckt saf_data;
	struct espi_saf_packet pckt_saf;
	pckt_saf.flash_addr = info->address;
	pckt_saf.len = info->length;
	saf_data.tag = info->saf_tag;
	saf_data.data = (uint8_t*)info->buf;
	pckt_saf.buf = (uint8_t*)&saf_data;
	return espi_saf_flash_erase(dev, &pckt_saf);
}

int saf_npcx_flash_unsupport(const struct device *dev,
									struct saf_handle_data *info)
{
	struct espi_saf_npcx_pckt saf_data;
	struct espi_saf_packet pckt_saf;
	pckt_saf.flash_addr = info->address;
	pckt_saf.len = info->length;
	saf_data.tag = info->saf_tag;
	saf_data.data = (uint8_t*)info->buf;
	pckt_saf.buf = (uint8_t*)&saf_data;

	return espi_saf_flash_unsuccess(dev, &pckt_saf);
}

/* Parse the information for eSPI SAF */
void espi_saf_handler(const struct device *dev,
						struct saf_handle_data *pckt,
						struct espi_event event)
{
	uint32_t saf_hdr;
	uint32_t *data_ptr;
	uint8_t  i, roundsize;

	data_ptr = (uint32_t *)event.evt_data;

	saf_hdr = LE32(*data_ptr);
	pckt->saf_type = MSB1(saf_hdr);
	pckt->length = LSW(saf_hdr) & 0xFFF;
	pckt->saf_tag = MSN(MSB2(saf_hdr));

	if (pckt->length == 0 &&
		(pckt->saf_type & 0xF) != ESPI_FLASH_SAF_REQ_ERASE) {
		pckt->length = _4KB_;
	}

	/* Get address from FLASHRXBUF1 */
	pckt->address = LE32(*(data_ptr + 1));
	pckt->buf = tx_buf_data;

	/* Get written data if eSPI SAF write */
	if ((pckt->saf_type & 0xF) == ESPI_FLASH_SAF_REQ_WRITE) {
		roundsize = DIV_CEILING(pckt->length, sizeof(uint32_t));
		for (i = 0; i < roundsize; i++) {
			pckt->src[i] = *(data_ptr + (i + 2));
		}
	}
}

/* eSPI SAF event handler */
static void espi_saf_ev_handler(const struct device *dev,
				struct espi_callback *cb,
				struct espi_event event)
{
	if (event.evt_type == ESPI_BUS_SAF_NOTIFICATION) {
		if (event.evt_details == ESPI_CHANNEL_FLASH) {
			espi_saf_handler(dev, &saf_data, event);
			k_work_submit(&saf_data.work);
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
		LOG_ERR("Failed to configure eSPI slave channels:%x err: %d",
			cfg.channel_caps, ret);
		return ret;
	}
	LOG_INF("eSPI slave configured successfully!");

	LOG_INF("eSPI test - callbacks initialization... ");
	espi_init_callback(&espi_bus_cb, espi_reset_handler, ESPI_BUS_RESET);
	espi_init_callback(&vw_rdy_cb, espi_ch_handler,
			   ESPI_BUS_EVENT_CHANNEL_READY);
	espi_init_callback(&vw_cb, vwire_handler,
			   ESPI_BUS_EVENT_VWIRE_RECEIVED);
	espi_init_callback(&p80_cb, periph_handler,
			   ESPI_BUS_PERIPHERAL_NOTIFICATION);
#if defined(CONFIG_ESPI_SAF) && defined(CONFIG_ESPI_FLASH_CHANNEL)
	espi_init_callback(&espi_saf_cb, espi_saf_ev_handler,
			   ESPI_BUS_SAF_NOTIFICATION);
#endif
	LOG_INF("complete");

	LOG_INF("eSPI test - callbacks registration... ");
	espi_add_callback(espi_dev, &espi_bus_cb);
	espi_add_callback(espi_dev, &vw_rdy_cb);
	espi_add_callback(espi_dev, &vw_cb);
	espi_add_callback(espi_dev, &p80_cb);
#if defined(CONFIG_ESPI_SAF) && defined(CONFIG_ESPI_FLASH_CHANNEL)
	espi_add_callback(espi_dev, &espi_saf_cb);
#endif

	LOG_INF("complete");
	return ret;
}

static int wait_for_vwire(const struct device *espi_dev,
			  enum espi_vwire_signal signal,
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

	if (loop_cnt == 0) {
		return -ETIMEDOUT;
	}

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

static void espi_saf_thread_entry(void)
{
	int ret;

	if (!device_is_ready(espi_dev)) {
		LOG_ERR("espi device %s not ready", espi_dev->name);
		return;
	}

	LOG_INF("espi device %s is ready", espi_dev->name);

	espi_init();

	int enable = 1;

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

	espi_saf_config(saf_dev, &saf_cfg_nand);
	espi_saf_activate(saf_dev);

	k_work_init(&saf_data.work, flash_handler);
}

void flash_handler(struct k_work *item)
{
	struct saf_handle_data *info
				= CONTAINER_OF(item, struct saf_handle_data, work);
	int ret = 0;

	switch (info->saf_type & 0x0F) {
		case ESPI_FLASH_SAF_REQ_READ:
			ret = saf_npcx_flash_read(saf_dev, &saf_data);
			break;
		case ESPI_FLASH_SAF_REQ_ERASE:
			ret = saf_npcx_flash_erase(saf_dev, &saf_data);
			break;
		case ESPI_FLASH_SAF_REQ_WRITE:
			ret = saf_npcx_flash_write(saf_dev, &saf_data);
			break;
	}

	if (ret != 0) {
		ret = saf_npcx_flash_unsupport(saf_dev, &saf_data);
	}
}

/* Test thread declaration */
#define STACK_SIZE	1024
#define THREAD_PRIORITY 1
K_THREAD_DEFINE(espi_saf_id,
				STACK_SIZE,
				espi_saf_thread_entry,
				NULL,
				NULL,
				NULL,
				THREAD_PRIORITY,
				0,
				-1);

void test_espi_saf_init(void)
{
	k_thread_name_set(espi_saf_id, "espi_saf_testing");
	k_thread_start(espi_saf_id);
}

#ifdef CONFIG_ESPI_FLASH_CHANNEL
#ifdef CONFIG_SOC_SERIES_NPCK3
int cmd_flash_erase(void)
{
	spi_nand_erase(spi_dev, 0x0, _128KB_);
	return 0;
}

int cmd_flash_write(void)
{
	uint8_t data_buf[SPI_NAND_PAGE_SIZE];
	uint32_t data_sz_in = 0x1000U;
	uint32_t addr_in = 0x0;
	uint32_t wr_size;

	while (data_sz_in > 0) {
		if (data_sz_in >= sizeof(data_buf))
			wr_size = sizeof(data_buf);
		else
			wr_size = data_sz_in;

		memset(data_buf, 0, sizeof(data_buf));
		for (int i = 0 ; i < wr_size ; i++) {
			data_buf[i]  = CAL_DATA_FROM_ADDR((addr_in + i));
		}

		spi_nand_write(spi_dev, addr_in, data_buf, wr_size);
		data_sz_in -= wr_size;
		addr_in += wr_size;
	}
	return 0;
}
#endif
int cmd_flash_protection(void)
{
	espi_saf_set_protection_regions(saf_dev, &saf_pr_flash);
	return 0;
}

#if defined(CONFIG_SOC_SERIES_NPCK3)
SHELL_STATIC_SUBCMD_SET_CREATE(sub_espi,
	SHELL_CMD(flash_erase, NULL, "erase NAND flash", cmd_flash_erase),
	SHELL_CMD(write_gold, NULL, "write NAND flash", cmd_flash_write),
	SHELL_CMD(set_pr, NULL, "set protection region", cmd_flash_protection),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
#else
SHELL_STATIC_SUBCMD_SET_CREATE(sub_espi,
	SHELL_CMD(set_pr, NULL, "set protection region", cmd_flash_protection),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
#endif

SHELL_CMD_REGISTER(espi_saf, &sub_espi, "eSPI Commands", NULL);
#endif /* CONFIG_ESPI_FLASH_CHANNEL */
