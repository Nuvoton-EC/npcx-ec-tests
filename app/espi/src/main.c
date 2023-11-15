/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/policy.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <stdlib.h>
#include <zephyr/drivers/espi.h>

#define LOG_LEVEL LOG_LEVEL_INF
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#define ESPI_FREQ_20MHZ             20u
#define ESPI_FREQ_25MHZ             25u
#define ESPI_FREQ_33MHZ             33u
#define ESPI_FREQ_50MHZ             50u
#define ESPI_FREQ_66MHZ             66u

#define TASK_STACK_SIZE             1024
#define PRIORITY                    7
static struct k_thread temp_id;
K_THREAD_STACK_DEFINE(temp_stack, TASK_STACK_SIZE);

#define MAX_ARGUMNETS               3
#define MAX_ARGU_SIZE               10
#define MAX_TEST_BUF_SIZE           1024u
#define MAX_FLASH_REQUEST           64u
#define MAX_FLASH_WRITE_REQUEST     64u
#define MAX_FLASH_ERASE_BLOCK       64u
static uint8_t arguments[MAX_ARGUMNETS][MAX_ARGU_SIZE];
static uint8_t flash_write_buf[MAX_TEST_BUF_SIZE];
static uint8_t flash_read_buf[MAX_TEST_BUF_SIZE];
static const struct device *const espi_dev = DEVICE_DT_GET(DT_NODELABEL(espi0));
struct k_event espi_event;

enum maf_erase_size {
	MAF_ERASE_4K = 1,
	MAF_ERASE_64K = 2,
	MAF_ERASE_128K = 4,
	MAF_ERASE_256K = 5,
};

const struct shell *sh_ptr;
static int p80_sum;
void espi_init(void);
void espi_set_cfg(char *channel, char *speed, char *iomode);
void espi_p80_validation(void);
void espi_oob_txrx(uint8_t txrx);
void espi_check_channel_ready(void);
void espi_send_vw(char vw, char lv);
void espi_hcmd(uint32_t offs);
int espi_maf_read(uint32_t start_addr);
int espi_maf_write(uint32_t start_addr);
int espi_maf_erase(uint32_t start_addr);

static void espi_validation_func(void *dummy1, void *dummy2, void *dummy3)
{
	uint32_t  events;
	int data;

	k_event_init(&espi_event);
	while (true) {
		events = k_event_wait(&espi_event, 0xFFF, true, K_FOREVER);
		switch (events) {
		case 0x001: /* no argu */
			break;
		case 0x002:
			if (!strcmp("init", arguments[0]))
				espi_init();
			if (!strcmp("p80_start", arguments[0]))
				p80_sum = 0;
			if (!strcmp("chkrdy", arguments[0]))
				espi_check_channel_ready();
			if (!strcmp("oob_rx", arguments[0]))
				espi_oob_txrx(0);
			if (!strcmp("oob_tx", arguments[0]))
				espi_oob_txrx(1);
			if (!strcmp("dis_kbc_irq", arguments[0])) {
				if (espi_write_lpc_request(espi_dev, E8042_PAUSE_IRQ, 0x0) == 0)
					LOG_INF("KBC IRQ disable pass");
			}
			if (!strcmp("ena_kbc_irq", arguments[0])) {
				if (espi_write_lpc_request(espi_dev, E8042_RESUME_IRQ, 0x0) == 0)
					LOG_INF("KBC IRQ enable pass");
			}
			break;
		case 0x004: /* select validation command */
			if (!strcmp("sendvw0", arguments[0])) {
				data = atoi(arguments[1]);
				espi_send_vw(data, 0);
			}
			if (!strcmp("set_st", arguments[0])) {
				data = atoi(arguments[1]);
				if (espi_write_lpc_request(espi_dev, E8042_SET_FLAG, &data) == 0)
					LOG_INF("Set KBC FLAG pass");
			}
			if (!strcmp("clr_st", arguments[0])) {
				data = atoi(arguments[1]);
				if (espi_write_lpc_request(espi_dev, E8042_CLEAR_FLAG, &data) == 0)
					LOG_INF("Clear KBC FLAG pass");
			}
			if (!strcmp("kbw", arguments[0])) { /* keyboard */
				data = atoi(arguments[1]);
				if (espi_write_lpc_request(espi_dev, E8042_WRITE_KB_CHAR, &data)
						== 0)
					LOG_INF("Write keyboard pass");
			}
			if (!strcmp("msw", arguments[0])) { /* mouse */
				data = atoi(arguments[1]);
				if (espi_write_lpc_request(espi_dev, E8042_WRITE_MB_CHAR, &data)
						== 0)
					LOG_INF("Write mouse pass");
			}
			if (!strcmp("acpi", arguments[0])) {
				data = atoi(arguments[1]);
				if (espi_write_lpc_request(espi_dev, EACPI_WRITE_CHAR, &data) == 0)
					LOG_INF("Write ACPI pass");
			}
			if (!strcmp("acpi_sts", arguments[0])) {
				data = atoi(arguments[1]);
				if (espi_write_lpc_request(espi_dev, EACPI_WRITE_STS, &data) == 0)
					LOG_INF("Write ACPI STS pass");
			}
			if (!strcmp("p80_check", arguments[0])) {
				data = atoi(arguments[1]);
				if (p80_sum ==  data)
					LOG_INF("[PASS] P80");
				else
					LOG_INF("[FAIL] P80(%x)(%x)", data, p80_sum);
			}
			if (!strcmp("sendvw0", arguments[0])) {
				data = atoi(arguments[1]);
				espi_send_vw(data, 0);
			}
			if (!strcmp("sendvw1", arguments[0])) {
				data = atoi(arguments[1]);
				espi_send_vw(data, 1);
			}
			if (!strcmp("shm", arguments[0])) {
				data = atoi(arguments[1]);
				espi_hcmd(data);
			}
			if (!strcmp("hcmd", arguments[0])) {
				data = atoi(arguments[1]);
				if (espi_write_lpc_request(espi_dev, ECUSTOM_HOST_CMD_SEND_RESULT,
							   &data) == 0)
					LOG_INF("Write hcmd pass");
			}
			if (!strcmp("fread", arguments[0])) {
				data = atoi(arguments[1]);
				espi_maf_read(data);
			}
			if (!strcmp("fwrite", arguments[0])) {
				data = atoi(arguments[1]);
				espi_maf_write(data);
			}
			if (!strcmp("ferase", arguments[0])) {
				data = atoi(arguments[1]);
				espi_maf_erase(data);
			}
			break;
		case 0x008: /* fill key press plan */
			espi_set_cfg(arguments[0], arguments[1], arguments[2]);
			break;
		}
	}
}
int main(void)
{
	k_thread_create(&temp_id, temp_stack, TASK_STACK_SIZE,
			espi_validation_func, NULL, NULL, NULL, PRIORITY,
			K_INHERIT_PERMS, K_FOREVER);
	k_thread_name_set(&temp_id, "eSPI Validation");
	k_thread_start(&temp_id);

	return 0;
}
static int espi_command(const struct shell *shell, size_t argc, char **argv)
{
	int i, evt;

	sh_ptr = shell;
	evt = 1;
	for (evt = 1, i = 1; i < argc; i++) {
		strcpy(arguments[i-1], argv[i]);
		evt <<= 1;
	}
	k_event_post(&espi_event, evt);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_espi,
	SHELL_CMD_ARG(c0, NULL, "espi c0", espi_command, 1, 0),
	SHELL_CMD_ARG(c1, NULL, "espi c1 arg0", espi_command, 2, 0),
	SHELL_CMD_ARG(c2, NULL, "espi c2 arg0 arg1", espi_command, 3, 0),
	SHELL_CMD_ARG(cfg, NULL, "espi cfg arg0 arg1 arg2", espi_command, 4, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(espi, &sub_espi, "eSPI validation commands", NULL);

/*** utility below here ****/
/* BUG : Virtual wire & Peripheral always true
 *		: Not support 66Mhz
 * bit0: Peripheral
 * bit1: Virtual wire
 * bit2: OOB
 * bit3: Flash
 * bit0: 20Mhz
 * bit1: 25Mhz
 * bit2: 33Mhz
 * bit3: 50Mhz
 * bit4: 66Mhz
 * bit0: Single
 * bit1: Dual
 * bit2: Quad
 */
void espi_set_cfg(char *channel, char *speed, char *iomode)
{
	int i;
	struct espi_cfg cfg;

	i = atoi(channel);
	if ((i & 0x3) != 3)
		LOG_INF("[FAIL]Peripheral & VW need always");
	cfg.channel_caps = ESPI_CHANNEL_PERIPHERAL | ESPI_CHANNEL_VWIRE;
	if (i&0x4)
		cfg.channel_caps |= ESPI_CHANNEL_OOB;
	if (i&0x8)
		cfg.channel_caps |= ESPI_CHANNEL_FLASH;

	cfg.io_caps = 0;
	i = atoi(iomode);
	if ((i&0x1) == 0)
		LOG_INF("[FAIL]Single must have");
	cfg.io_caps |= ESPI_IO_MODE_SINGLE_LINE;
	if (i&0x2)
		cfg.io_caps |= ESPI_IO_MODE_DUAL_LINES;
	if (i&0x4)
		cfg.io_caps |= ESPI_IO_MODE_QUAD_LINES;

	cfg.io_caps = 0;
	i = atoi(iomode);
	if ((i&0x1) == 0)
		LOG_INF("[FAIL]Single must have");
	cfg.io_caps |= ESPI_IO_MODE_SINGLE_LINE;
	if (i&0x2)
		cfg.io_caps |= ESPI_IO_MODE_DUAL_LINES;
	if (i&0x4)
		cfg.io_caps |= ESPI_IO_MODE_QUAD_LINES;

	cfg.max_freq = 0;
	i = atoi(speed);
	if ((i&0x1f) == 0) {
		LOG_INF("[FAIL] at last 20Mhz");
		cfg.max_freq = ESPI_FREQ_20MHZ;
	}
	if (i&0x1)
		cfg.max_freq = ESPI_FREQ_20MHZ;
	if (i&0x2)
		cfg.max_freq = ESPI_FREQ_25MHZ;
	if (i&0x4)
		cfg.max_freq = ESPI_FREQ_33MHZ;
	if (i&0x8)
		cfg.max_freq = ESPI_FREQ_50MHZ;
	if (i&0x10) {
		cfg.max_freq = ESPI_FREQ_50MHZ;
		LOG_INF("[FAIL] not support 66Mhz");
	}
	i = espi_config(espi_dev, &cfg);
	/* Zephyr driver validation */
	LOG_INF("Start eSPI Validation Task");
	if (i) {
		LOG_INF("[FAIL]eSPI config failure");
	} else {
		LOG_INF("[PASS]eSPI config success");
	}
}

/* eSPI event */
#define EVENT_MASK            0x0000FFFFu
#define EVENT_DETAILS_MASK    0xFFFF0000u
#define EVENT_DETAILS_POS     16u
#define EVENT_TYPE(x)         (x & EVENT_MASK)
#define EVENT_DETAILS(x)      ((x & EVENT_DETAILS_MASK) >> EVENT_DETAILS_POS)

static struct espi_callback espi_bus_cb;
static struct espi_callback vw_rdy_cb;
static struct espi_callback vw_cb;
static struct espi_callback p80_cb;
static struct espi_callback oob_cb;
static uint8_t espi_rst_sts;

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

static void vwire_handler(const struct device *dev, struct espi_callback *cb,
			  struct espi_event event)
{
	if (event.evt_type == ESPI_BUS_EVENT_VWIRE_RECEIVED) {
		switch (event.evt_details) {
		case ESPI_VWIRE_SIGNAL_PLTRST:
			LOG_INF("PLT_RST:%d", event.evt_data);
			break;
		case ESPI_VWIRE_SIGNAL_SLP_S3:
			LOG_INF("S3:%d", event.evt_data);
			break;
		case ESPI_VWIRE_SIGNAL_SLP_S4:
			LOG_INF("S4:%d", event.evt_data);
			break;
		case ESPI_VWIRE_SIGNAL_SLP_S5:
			LOG_INF("S5:%d", event.evt_data);
			break;
		case ESPI_VWIRE_SIGNAL_SLP_A:
			LOG_INF("SLP_A:%d", event.evt_data);
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
		p80_sum += (unsigned int)event.evt_data;
		break;
	case ESPI_PERIPHERAL_HOST_IO:
		LOG_INF("ACPI:%02x from %s", (unsigned char)(event.evt_data>>8),
					(event.evt_data&0x1) ? "COMMAND" : "DBBIN");
		break;
	case ESPI_PERIPHERAL_8042_KBC:
		if (event.evt_data & 0x20000)
			LOG_INF("Got HOST_KBC_EVT_OBE evt");
		if (event.evt_data & 0x10000) {
			LOG_INF("Got HOST_KBC_EVT_IBF evt");
			LOG_INF("KBC:%02x from %s", (unsigned char)(event.evt_data>>8),
					(event.evt_data&0x1) ? "COMMAND" : "DBBIN");
		}
		break;
	case ESPI_PERIPHERAL_EC_HOST_CMD:
		LOG_INF("HC %x", event.evt_data);
		break;
	default:
		LOG_INF("%s periph 0x%x [%x]", __func__, periph_type,
			event.evt_data);
	}
}
static void oob_handler(const struct device *dev, struct espi_callback *cb,
			   struct espi_event event)
{
	LOG_INF("Got OOB evt");
}
void espi_init(void)
{
	int enable = 1;

	espi_init_callback(&espi_bus_cb, espi_reset_handler, ESPI_BUS_RESET);
	espi_init_callback(&vw_cb, vwire_handler, ESPI_BUS_EVENT_VWIRE_RECEIVED);
	espi_init_callback(&vw_rdy_cb, espi_ch_handler,
						ESPI_BUS_EVENT_CHANNEL_READY);
	espi_init_callback(&p80_cb, periph_handler,
						ESPI_BUS_PERIPHERAL_NOTIFICATION);
	espi_init_callback(&oob_cb, oob_handler,
						ESPI_BUS_EVENT_OOB_RECEIVED);
	espi_add_callback(espi_dev, &espi_bus_cb);
	espi_add_callback(espi_dev, &vw_rdy_cb);
	espi_add_callback(espi_dev, &vw_cb);
	espi_add_callback(espi_dev, &p80_cb);
	espi_add_callback(espi_dev, &oob_cb);
	LOG_INF("initial complete");
	espi_write_lpc_request(espi_dev, ECUSTOM_HOST_SUBS_INTERRUPT_EN, &enable);
}

void espi_oob_txrx(uint8_t txrx)
{
	struct espi_oob_packet pckt;
	uint8_t buff[20], i;

	if (txrx == 0) {
		pckt.buf = buff;
		if (espi_receive_oob(espi_dev, &pckt) == 0)
			LOG_INF("[PASS] len:%d", pckt.len);
		else {
			LOG_INF("[FAIL]");
			return;
		}
		for (i = 0; i < pckt.len; i++)
			LOG_INF("[%02x]", pckt.buf[i]);
	} else {
		pckt.len = 5;
		pckt.buf = buff;
		buff[0] = 1;
		buff[1] = 2;
		buff[2] = 3;
		buff[3] = 4;
		buff[4] = 10;
		if (espi_send_oob(espi_dev, &pckt) == 0)
			LOG_INF("[PASS] len:%d", pckt.len);
		else
			LOG_INF("[FAIL]");
	}
}

void espi_check_channel_ready(void)
{
	LOG_INF("Peripheral support:%s", (espi_get_channel_status(espi_dev,
						ESPI_CHANNEL_PERIPHERAL)) ? "[PASS]" : "[FAIL]");
	LOG_INF("Virtual wire support:%s", (espi_get_channel_status(espi_dev,
						ESPI_CHANNEL_VWIRE)) ? "[PASS]" : "[FAIL]");
	LOG_INF("OOB support:%s", (espi_get_channel_status(espi_dev,
						ESPI_CHANNEL_OOB)) ? "[PASS]" : "[FAIL]");
	LOG_INF("Flash support:%s", (espi_get_channel_status(espi_dev,
						ESPI_CHANNEL_FLASH)) ? "[PASS]" : "[FAIL]");
}
/* CAUTION : ESPI_VWIRE_SIGNAL_SCI and ESPI_VWIRE_SIGNAL_SMI by hardware */
const enum espi_vwire_signal vw_out[] = {
ESPI_VWIRE_SIGNAL_OOB_RST_ACK, ESPI_VWIRE_SIGNAL_WAKE,
ESPI_VWIRE_SIGNAL_PME, ESPI_VWIRE_SIGNAL_SLV_BOOT_DONE,
ESPI_VWIRE_SIGNAL_ERR_FATAL, ESPI_VWIRE_SIGNAL_ERR_NON_FATAL,
ESPI_VWIRE_SIGNAL_SLV_BOOT_STS, ESPI_VWIRE_SIGNAL_HOST_RST_ACK,
ESPI_VWIRE_SIGNAL_SUS_ACK,ESPI_VWIRE_SIGNAL_SCI,
ESPI_VWIRE_SIGNAL_SMI
};
void espi_send_vw(char vw, char level)
{
	if (vw >= sizeof(vw_out)/sizeof(unsigned char)) {
		LOG_INF("Out range [FAIL]");
	}
	level = (char)espi_send_vwire(espi_dev, (enum espi_vwire_signal)vw_out[(int)vw], level);
	LOG_INF("send [%s]", (level) ? "FAIL" : "PASS");
}

void espi_hcmd(uint32_t offs)
{
	uint32_t *ptr1,*ptr2,i;

	if (espi_read_lpc_request(espi_dev, ECUSTOM_HOST_CMD_GET_PARAM_MEMORY,
		&i) == 0) {
		/* got share memory buffer ptr */
		ptr1 = (uint32_t *)i;
		LOG_INF("HCMD[PASS](%x)", i);
	} else {
		LOG_INF("HCMD[FAIL]");
		return;
	}
	if (espi_read_lpc_request(espi_dev, EACPI_GET_SHARED_MEMORY,
		&i) == 0) {
		/* got share memory buffer ptr */
		ptr2 = (uint32_t *)i;
		LOG_INF("ACPI[PASS](%x)", i);
	} else {
		LOG_INF("ACPI[FAIL]");
		return;
	}
	LOG_INF("HCMD:%x", ptr1[offs]);
	i = ptr1[offs];
	//ptr2[offs] = 0x11223344;
	ptr2[offs] = ((i & 0xFF000000) >> 24)|((i & 0x00FF0000) >> 8)|
					((i & 0x0000FF00) << 8)|((i & 0x000000FF) << 24);
	ptr1[offs] = ~ptr1[offs];
}

int read_test_block(uint8_t *buf, uint32_t start_flash_adr, uint16_t len)
{
	int ret = 0;
	struct espi_flash_packet pckt;

	pckt.buf = buf;
	pckt.flash_addr = start_flash_adr;
	pckt.len = len;

	ret = espi_read_flash(espi_dev, &pckt);
	if (ret) {
		LOG_ERR("espi_read_flash failed: %d", ret);
		return ret;
	}

	LOG_INF("read flash transactions (total=%d Bytes) completed", len);
	LOG_INF("[PASS]");
	return 0;
}

int write_test_block(uint8_t *buf, uint32_t start_flash_adr, uint16_t len)
{
	int ret = 0;
	struct espi_flash_packet pckt;

	/* Split operation in multiple MAX_FLASH_REQ transactions */
	pckt.buf = buf;
	pckt.flash_addr = start_flash_adr;
	pckt.len = len;

	ret = espi_write_flash(espi_dev, &pckt);
	if (ret) {
		LOG_ERR("espi_write_flash failed: %d", ret);
		return ret;
	}

	LOG_INF("write flash transactions (total=%d Bytes) completed", len);
	LOG_INF("[PASS]");
	return 0;
}

int erase_test_block(uint32_t start_flash_adr, uint16_t block_len)
{
	uint32_t flash_addr = start_flash_adr;
	uint16_t transactions = block_len/MAX_FLASH_ERASE_BLOCK;
	struct espi_flash_packet pckt;
	uint8_t i = 0;
	int ret = 0;

	/* Split operation in multiple MAX_FLASH_ERASE_BLOCK transactions */
	for (i = 0; i < transactions; i++) {
		pckt.flash_addr = flash_addr;
		pckt.len = MAF_ERASE_64K;

		ret = espi_flash_erase(espi_dev, &pckt);
		if (ret) {
			LOG_ERR("espi_erase_flash failed: %d", ret);
			return ret;
		}

		flash_addr += MAX_FLASH_ERASE_BLOCK;
	}

	LOG_INF("%d erase flash transactions completed", transactions);
	LOG_INF("[PASS]");
	return 0;
}


int espi_maf_read(uint32_t start_addr)
{
	uint16_t test_len = 0xA;
	int ret, ind;

	ret = read_test_block(flash_read_buf, start_addr, test_len);
	if (ret) {
		LOG_ERR("Failed to read from eSPI MAF");
		return ret;
	}

	shell_info(sh_ptr, "DATA: ");
	for (ind = 0; ind < test_len; ind++) {
		shell_info(sh_ptr, "%x", flash_read_buf[ind]);
	}

	return 0;
}

int espi_maf_write(uint32_t start_addr)
{
	uint16_t test_len = 0xA;
	int ret, ind;

	for (ind = 0; ind < test_len; ind++) {
		flash_write_buf[ind] = ind;
	}

	ret = write_test_block(flash_write_buf, start_addr, test_len);
	if (ret) {
		LOG_ERR("Failed to write to eSPI MAF");
		return ret;
	}

	return 0;
}

int espi_maf_erase(uint32_t start_addr)
{
	int ret;

	ret = erase_test_block(start_addr, sizeof(flash_write_buf));
	if (ret) {
		LOG_ERR("Failed to erase to eSPI MAF");
		return ret;
	}

	return 0;
}
