/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#ifdef CONFIG_FLASH_EX_OP_ENABLED
#include <zephyr/drivers/flash/npcx_flash_api_ex.h>
#endif /* CONFIG_FLASH_EX_OP_ENABLED */
#include <jesd216.h>
#include "spi_nor.h"
#include <zephyr/kernel.h>
#include <zephyr/pm/policy.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <stdlib.h>
#include <stdio.h>

/* Target drivers for testing */
#include <zephyr/drivers/flash.h>

#define LOG_LEVEL LOG_LEVEL_INF
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash);

#define FLASH_DEV0 DT_ALIAS(spi_flash0)
#define FLASH_DEV1 DT_ALIAS(spi_flash1)
#define FLASH_DEV2 DT_ALIAS(spi_flash2)

/* Get device from device tree */
static const struct device *const flash_devices[] = {
#if DT_NODE_HAS_STATUS(FLASH_DEV0, okay)
	DEVICE_DT_GET(FLASH_DEV0),
#endif
#if DT_NODE_HAS_STATUS(FLASH_DEV1, okay)
	DEVICE_DT_GET(FLASH_DEV1),
#endif
#if DT_NODE_HAS_STATUS(FLASH_DEV2, okay)
	DEVICE_DT_GET(FLASH_DEV2),
#endif
};

#define NUM_FLASH_DEVICE ARRAY_SIZE(flash_devices)

/* Data buffer */
#define TEMP_DATA_BUF_SIZE 128
static uint8_t temp_data_buf[TEMP_DATA_BUF_SIZE];

static uint8_t temp_write_buf[TEMP_DATA_BUF_SIZE] = {
	0x92, 0xf4, 0xff, 0xc1, 0x96, 0xf4, 0xdf, 0x73,
	0x15, 0xf0, 0x1e, 0x03, 0x24, 0x59, 0xc4, 0x09,
	0x40, 0x5d, 0x00, 0x5f, 0x40, 0x4c, 0x20, 0x00,
	0xff, 0x00, 0x02, 0xf2, 0x30, 0xb0, 0xf2, 0x22,
	0x10, 0x5a, 0x20, 0x45, 0x03, 0x25, 0x40, 0x5d,
	0x00, 0x5f, 0x40, 0x4c, 0x20, 0x00, 0xff, 0x00,
	0x02, 0xf2, 0x30, 0xf0, 0xee, 0x0a, 0x25, 0x59,
	0xc5, 0x09, 0x50, 0x5d, 0x00, 0x5f, 0x40, 0x4c,
	0x20, 0x00, 0xff, 0x00, 0x00, 0xf2, 0x40, 0xb0,
	0xf2, 0x22, 0x13, 0x50, 0x15, 0x10, 0x10, 0x5a,
	0x20, 0x45, 0x04, 0x25, 0xe5, 0x10, 0x10, 0x5a,
	0x20, 0x45, 0x90, 0x28, 0x04, 0x21, 0x50, 0x5d,
	0x00, 0x5f, 0x40, 0x4c, 0x20, 0x00, 0xff, 0x00,
	0x00, 0xf2, 0x40, 0xf0, 0xee, 0x0a, 0x0f, 0x81,
	0x4c, 0xf3, 0x0f, 0x81, 0x52, 0xf3, 0xb0, 0x58,
	0x20, 0x00, 0x0f, 0xc8, 0x4a, 0xf3, 0xb0, 0x5a,
};

/* Objects used for validation */
static struct {
	const struct shell *shell;
	const struct device *cur_dev;
} test_objs;

/* SFDP Test Functions */
static const char * const mode_tags[] = {
	[JESD216_MODE_044] = "QSPI XIP",
	[JESD216_MODE_088] = "OSPI XIP",
	[JESD216_MODE_111] = "1-1-1",
	[JESD216_MODE_112] = "1-1-2",
	[JESD216_MODE_114] = "1-1-4",
	[JESD216_MODE_118] = "1-1-8",
	[JESD216_MODE_122] = "1-2-2",
	[JESD216_MODE_144] = "1-4-4",
	[JESD216_MODE_188] = "1-8-8",
	[JESD216_MODE_222] = "2-2-2",
	[JESD216_MODE_444] = "4-4-4",
};


#define shell_printk(...) \
	shell_fprintf(test_objs.shell, SHELL_NORMAL, ##__VA_ARGS__)

static void summarize_dw1(const struct jesd216_param_header *php,
			  const struct jesd216_bfp *bfp)
{
	uint32_t dw1 = sys_le32_to_cpu(bfp->dw1);

	shell_printk("DTR Clocking %ssupported\n",
	       (dw1 & JESD216_SFDP_BFP_DW1_DTRCLK_FLG) ? "" : "not ");

	static const char *const addr_bytes[] = {
		[JESD216_SFDP_BFP_DW1_ADDRBYTES_VAL_3B] = "3-Byte only",
		[JESD216_SFDP_BFP_DW1_ADDRBYTES_VAL_3B4B] = "3- or 4-Byte",
		[JESD216_SFDP_BFP_DW1_ADDRBYTES_VAL_4B] = "4-Byte only",
		[3] = "Reserved",
	};

	shell_printk("Addressing: %s\n", addr_bytes[(dw1 & JESD216_SFDP_BFP_DW1_ADDRBYTES_MASK)
					 >> JESD216_SFDP_BFP_DW1_ADDRBYTES_SHFT]);

	static const char *const bsersz[] = {
		[0] = "Reserved 00b",
		[JESD216_SFDP_BFP_DW1_BSERSZ_VAL_4KSUP] = "uniform",
		[2] = "Reserved 01b",
		[JESD216_SFDP_BFP_DW1_BSERSZ_VAL_4KNOTSUP] = "not uniform",
	};

	shell_printk("4-KiBy erase: %s\n", bsersz[(dw1 & JESD216_SFDP_BFP_DW1_BSERSZ_MASK)
					   >> JESD216_SFDP_BFP_DW1_BSERSZ_SHFT]);

	for (size_t mode = 0; mode < ARRAY_SIZE(mode_tags); ++mode) {
		const char *tag = mode_tags[mode];

		if (tag) {
			struct jesd216_instr cmd;
			int rc = jesd216_bfp_read_support(php, bfp,
							  (enum jesd216_mode_type)mode,
							  &cmd);

			if (rc == 0) {
				shell_printk("Support %s\n", tag);
			} else if (rc > 0) {
				shell_printk("Support %s: instr %02Xh, %u mode clocks, %u waits\n",
					     tag, cmd.instr, cmd.mode_clocks, cmd.wait_states);
			}
		}
	}
}

static void summarize_dw2(const struct jesd216_param_header *php,
			  const struct jesd216_bfp *bfp)
{
	shell_printk("Flash density: %u bytes\n", (uint32_t)(jesd216_bfp_density(bfp) / 8));
}

static void summarize_dw89(const struct jesd216_param_header *php,
			   const struct jesd216_bfp *bfp)
{
	struct jesd216_erase_type etype;
	uint32_t typ_ms;
	int typ_max_mul;

	for (uint8_t idx = 1; idx < JESD216_NUM_ERASE_TYPES; ++idx) {
		if (jesd216_bfp_erase(bfp, idx, &etype) == 0) {
			typ_max_mul = jesd216_bfp_erase_type_times(php, bfp,
								   idx, &typ_ms);

			shell_printk("ET%u: instr %02Xh for %u By", idx, etype.cmd,
			       (uint32_t)BIT(etype.exp));
			if (typ_max_mul > 0) {
				shell_printk("; typ %u ms, max %u ms",
				       typ_ms, typ_max_mul * typ_ms);
			}
			shell_printk("\n");
		}
	}
}

static void summarize_dw11(const struct jesd216_param_header *php,
			   const struct jesd216_bfp *bfp)
{
	struct jesd216_bfp_dw11 dw11;

	if (jesd216_bfp_decode_dw11(php, bfp, &dw11) != 0) {
		return;
	}

	shell_printk("Chip erase: typ %u ms, max %u ms\n",
		dw11.chip_erase_ms, dw11.typ_max_factor * dw11.chip_erase_ms);

	shell_printk("Byte program: type %u + %u * B us, max %u + %u * B us\n",
		dw11.byte_prog_first_us, dw11.byte_prog_addl_us,
		dw11.typ_max_factor * dw11.byte_prog_first_us,
		dw11.typ_max_factor * dw11.byte_prog_addl_us);

	shell_printk("Page program: typ %u us, max %u us\n",
		dw11.page_prog_us,
		dw11.typ_max_factor * dw11.page_prog_us);

	shell_printk("Page size: %u By\n", dw11.page_size);
}

static void summarize_dw12(const struct jesd216_param_header *php,
			   const struct jesd216_bfp *bfp)
{
	uint32_t dw12 = sys_le32_to_cpu(bfp->dw10[2]);
	uint32_t dw13 = sys_le32_to_cpu(bfp->dw10[3]);

	/* Inverted logic flag: 1 means not supported */
	if ((dw12 & JESD216_SFDP_BFP_DW12_SUSPRESSUP_FLG) != 0) {
		return;
	}

	uint8_t susp_instr = dw13 >> 24;
	uint8_t resm_instr = dw13 >> 16;
	uint8_t psusp_instr = dw13 >> 8;
	uint8_t presm_instr = dw13 >> 0;

	shell_printk("Suspend: %02Xh ; Resume: %02Xh\n",
	       susp_instr, resm_instr);
	if ((susp_instr != psusp_instr)
	    || (resm_instr != presm_instr)) {
		shell_printk("Program suspend: %02Xh ; Resume: %02Xh\n",
				psusp_instr, presm_instr);
	}
}

static void summarize_dw14(const struct jesd216_param_header *php,
			   const struct jesd216_bfp *bfp)
{
	struct jesd216_bfp_dw14 dw14;

	if (jesd216_bfp_decode_dw14(php, bfp, &dw14) != 0) {
		return;
	}
	shell_printk("DPD: Enter %02Xh, exit %02Xh ; delay %u ns ; poll 0x%02x\n",
		dw14.enter_dpd_instr, dw14.exit_dpd_instr,
		dw14.exit_delay_ns, dw14.poll_options);
}

static void summarize_dw15(const struct jesd216_param_header *php,
			   const struct jesd216_bfp *bfp)
{
	struct jesd216_bfp_dw15 dw15;

	if (jesd216_bfp_decode_dw15(php, bfp, &dw15) != 0) {
		return;
	}
	shell_printk("HOLD or RESET Disable: %ssupported\n",
			dw15.hold_reset_disable ? "" : "un");
	shell_printk("QER: %u\n", dw15.qer);
	if (dw15.support_044) {
		shell_printk("0-4-4 Mode methods: entry 0x%01x ; exit 0x%02x\n",
				dw15.entry_044, dw15.exit_044);
	} else {
		shell_printk("0-4-4 Mode: not supported");
	}
	shell_printk("4-4-4 Mode sequences: enable 0x%02x ; disable 0x%01x\n",
			dw15.enable_444, dw15.disable_444);
}

static void summarize_dw16(const struct jesd216_param_header *php,
			   const struct jesd216_bfp *bfp)
{
	struct jesd216_bfp_dw16 dw16;

	if (jesd216_bfp_decode_dw16(php, bfp, &dw16) != 0) {
		return;
	}

	uint8_t addr_support = jesd216_bfp_addrbytes(bfp);

	/* Don't display bits when 4-byte addressing is not supported. */
	if (addr_support != JESD216_SFDP_BFP_DW1_ADDRBYTES_VAL_3B) {
		shell_printk("4-byte addressing support: enter 0x%02x, exit 0x%03x\n",
		       dw16.enter_4ba, dw16.exit_4ba);
	}
	shell_printk("Soft Reset and Rescue Sequence support: 0x%02x\n",
			dw16.srrs_support);
	shell_printk("Status Register 1 support: 0x%02x\n",
			dw16.sr1_interface);
}

typedef void (*dw_extractor)(const struct jesd216_param_header *php,
			     const struct jesd216_bfp *bfp);

/* Indexed from 1 to match JESD216 data word numbering */
static const dw_extractor extractor[] = {
	[1] = summarize_dw1,
	[2] = summarize_dw2,
	[8] = summarize_dw89,
	[11] = summarize_dw11,
	[12] = summarize_dw12,
	[14] = summarize_dw14,
	[15] = summarize_dw15,
	[16] = summarize_dw16,
};

static void dump_bfp(const struct jesd216_param_header *php,
		     const struct jesd216_bfp *bfp)
{
	uint8_t dw = 1;
	uint8_t limit = MIN(1U + php->len_dw, ARRAY_SIZE(extractor));

	shell_printk("Summary of BFP content:\n");
	while (dw < limit) {
		dw_extractor ext = extractor[dw];

		if (ext != 0) {
			ext(php, bfp);
		}
		++dw;
	}
}

static void dump_bytes(const struct jesd216_param_header *php,
		       const uint32_t *dw)
{
	char buffer[4 * 3 + 1]; /* B1 B2 B3 B4 */
	uint8_t nw = 0;

	shell_printk(" [\n\t");
	while (nw < php->len_dw) {
		const uint8_t *u8p = (const uint8_t *)&dw[nw];
		++nw;

		bool emit_nl = (nw == php->len_dw) || ((nw % 4) == 0);

		sprintf(buffer, "%02x %02x %02x %02x",
			u8p[0], u8p[1], u8p[2], u8p[3]);
		if (emit_nl) {
			shell_printk("%s\n\t", buffer);
		} else {
			shell_printk("%s  ", buffer);
		}
	}
	shell_printk("];\n");
}

/* Test Function Declarations */
static int nor_flash_read_id(const struct device *flash_dev)
{
	uint8_t id[3];
	int rc;

	rc = flash_read_jedec_id(flash_dev, id);
	if (rc == 0) {
		shell_printk("jedec-id = [%02x %02x %02x];\n",
		       id[0], id[1], id[2]);
	} else {
		shell_printk("JEDEC ID read failed: %d\n", rc);
	}

	const uint8_t decl_nph = 5;
	union {
		uint8_t raw[JESD216_SFDP_SIZE(decl_nph)];
		struct jesd216_sfdp_header sfdp;
	} u;
	const struct jesd216_sfdp_header *hp = &u.sfdp;

	rc = flash_sfdp_read(flash_dev, 0, u.raw, sizeof(u.raw));

	if (rc != 0) {
		LOG_ERR("Read SFDP not supported: device not JESD216-compliant "
		       "(err %d)", rc);
		return 0;
	}

	uint32_t magic = jesd216_sfdp_magic(hp);

	if (magic != JESD216_SFDP_MAGIC) {
		LOG_ERR("SFDP magic %08x invalid", magic);
		return 0;
	}

	shell_printk("%s: SFDP v %u.%u AP %x with %u PH\n", flash_dev->name,
		hp->rev_major, hp->rev_minor, hp->access, 1 + hp->nph);

	const struct jesd216_param_header *php = hp->phdr;
	const struct jesd216_param_header *phpe = php + MIN(decl_nph, 1 + hp->nph);

	while (php != phpe) {
		uint16_t id = jesd216_param_id(php);
		uint32_t addr = jesd216_param_addr(php);

		shell_printk("PH%u: %04x rev %u.%u: %u DW @ %x\n",
		       (uint32_t)(php - hp->phdr), id, php->rev_major, php->rev_minor,
		       php->len_dw, addr);

		uint32_t dw[php->len_dw];

		rc = flash_sfdp_read(flash_dev, addr, dw, sizeof(dw));
		if (rc != 0) {
			shell_printk("Read failed: %d\n", rc);
			return 0;
		}

		if (id == JESD216_SFDP_PARAM_ID_BFP) {
			const struct jesd216_bfp *bfp = (struct jesd216_bfp *)dw;

			dump_bfp(php, bfp);
			shell_printk("size = <%u>;\n", (uint32_t)jesd216_bfp_density(bfp));
			shell_printk("sfdp-bfp =");
		} else {
			shell_printk("sfdp-%04x =", id);
		}

		dump_bytes(php, dw);

		++php;
	}

	return 0;
}

static int nor_flash_erase(const struct device *flash_dev, off_t addr, size_t size)
{
	int rc;
	uint32_t i;
	size_t temp_size, read_size;
	off_t read_addr;

	rc = flash_erase(flash_dev, addr, size);
	if (rc != 0) {
		LOG_ERR("flash_erase() failed: %d", rc);
		return -ENODEV;
	}

	temp_size = size;
	read_addr = addr;
	while (temp_size) {
		read_size = (temp_size >= TEMP_DATA_BUF_SIZE) ? TEMP_DATA_BUF_SIZE : temp_size;

		rc = flash_read(flash_dev, read_addr, temp_data_buf, read_size);
		if (rc != 0) {
			LOG_ERR("flash_read() failed: %d", rc);
			return -ENODEV;
		}

		for (i = 0; i < read_size; i++) {
			if (temp_data_buf[i] != 0xFF) {
				LOG_ERR("flash_erase() check failed. Addr: 0x%lx, Data: 0x%x",
					(read_addr + i), temp_data_buf[i]);
				return -ENODEV;
			}
		}

		read_addr += read_size;
		temp_size -= read_size;
	}

	shell_info(test_objs.shell, "[PASS] Flash erase succeeded!");
	shell_info(test_objs.shell, "[GO]");
	return 0;
}

static int nor_flash_read(const struct device *flash_dev, off_t addr, size_t size)
{
	int rc;
	uint32_t i;
	size_t temp_size, read_size;
	off_t read_addr;

	temp_size = size;
	read_addr = addr;
	shell_info(test_objs.shell, "Flash Address: 0x%lx, size: 0x%x", addr, size);

	while (temp_size) {
		read_size = (temp_size >= TEMP_DATA_BUF_SIZE) ? TEMP_DATA_BUF_SIZE : temp_size;

		rc = flash_read(flash_dev, read_addr, temp_data_buf, read_size);
		if (rc != 0) {
			LOG_ERR("flash_read() failed: %d", rc);
			return -ENODEV;
		}

		for (i = 0; i < read_size; i++) {
			if ((i % 16) == 0) {
				shell_printk("\n");
			}

			shell_printk("0x%.2x ", temp_data_buf[i]);
		}

		read_addr += read_size;
		temp_size -= read_size;
	}

	shell_printk("\n\n");
	shell_info(test_objs.shell, "Flash read succeeded!");
	return 0;
}

static int nor_flash_write(const struct device *flash_dev, off_t addr, size_t size)
{
	int rc;
	uint32_t i;
	size_t temp_size, read_size, write_size;
	off_t read_addr, write_addr;

	/* Erase flash first */
	rc = flash_erase(flash_dev, addr, size);
	if (rc != 0) {
		LOG_ERR("flash_erase() failed: %d", rc);
		return -ENODEV;
	}

	/* Check if erase flash operation is successful or not? */
	temp_size = size;
	read_addr = addr;
	while (temp_size) {
		read_size = (temp_size >= TEMP_DATA_BUF_SIZE) ? TEMP_DATA_BUF_SIZE : temp_size;

		rc = flash_read(flash_dev, read_addr, temp_data_buf, read_size);
		if (rc != 0) {
			LOG_ERR("flash_read() failed: %d", rc);
			return -ENODEV;
		}

		for (i = 0; i < read_size; i++) {
			if (temp_data_buf[i] != 0xFF) {
				LOG_ERR("flash_erase() check failed. Addr: 0x%lx, Data: 0x%x",
					(read_addr + i), temp_data_buf[i]);
				return -ENODEV;
			}
		}

		read_addr += read_size;
		temp_size -= read_size;
	}

	/* Write flash */
	temp_size = size;
	write_addr = addr;
	while (temp_size) {
		write_size = (temp_size >= TEMP_DATA_BUF_SIZE) ? TEMP_DATA_BUF_SIZE : temp_size;

		rc = flash_write(flash_dev, write_addr, temp_write_buf, write_size);
		if (rc != 0) {
			LOG_ERR("flash_write() failed: %d", rc);
			return -ENODEV;
		}

		rc = flash_read(flash_dev, write_addr, temp_data_buf, write_size);
		if (rc != 0) {
			LOG_ERR("flash_read() failed: %d", rc);
			return -ENODEV;
		}

		for (i = 0; i < write_size; i++) {
			if (temp_data_buf[i] != temp_write_buf[i]) {
				LOG_ERR("flash_write() check failed. A: 0x%lx, D: 0x%x, G: 0x%x",
					(write_addr + i), temp_data_buf[i], temp_write_buf[i]);
				return -ENODEV;
			}
		}

		write_addr += write_size;
		temp_size -= write_size;
	}

	shell_info(test_objs.shell, "[PASS] Flash write succeeded!");
	shell_info(test_objs.shell, "[GO]");
	return 0;
}

static int nor_flash_write_only(const struct device *flash_dev, off_t addr, size_t size)
{
	int rc;
	uint32_t i;
	size_t temp_size, read_size, write_size;
	off_t read_addr, write_addr;

	/* Check if erase flash operation is successful or not? */
	temp_size = size;
	read_addr = addr;
	while (temp_size) {
		read_size = (temp_size >= TEMP_DATA_BUF_SIZE) ? TEMP_DATA_BUF_SIZE : temp_size;

		rc = flash_read(flash_dev, read_addr, temp_data_buf, read_size);
		if (rc != 0) {
			LOG_ERR("flash_read() failed: %d", rc);
			return -ENODEV;
		}

		for (i = 0; i < read_size; i++) {
			if (temp_data_buf[i] != 0xFF) {
				LOG_ERR("flash_erase() check failed. Addr: 0x%lx, Data: 0x%x",
					(read_addr + i), temp_data_buf[i]);
				return -ENODEV;
			}
		}

		read_addr += read_size;
		temp_size -= read_size;
	}

	/* Write flash */
	temp_size = size;
	write_addr = addr;
	while (temp_size) {
		write_size = (temp_size >= TEMP_DATA_BUF_SIZE) ? TEMP_DATA_BUF_SIZE : temp_size;

		rc = flash_write(flash_dev, write_addr, temp_write_buf, write_size);
		if (rc != 0) {
			LOG_ERR("flash_write() failed: %d", rc);
			return -ENODEV;
		}

		rc = flash_read(flash_dev, write_addr, temp_data_buf, write_size);
		if (rc != 0) {
			LOG_ERR("flash_read() failed: %d", rc);
			return -ENODEV;
		}

		for (i = 0; i < write_size; i++) {
			if (temp_data_buf[i] != temp_write_buf[i]) {
				LOG_ERR("flash_write() check failed. A: 0x%lx, D: 0x%x, G: 0x%x",
					(write_addr + i), temp_data_buf[i], temp_write_buf[i]);
				return -ENODEV;
			}
		}

		write_addr += write_size;
		temp_size -= write_size;
	}

	shell_info(test_objs.shell, "Flash write only succeeded!");
	return 0;
}


static int nor_flash_read_id_handler(const struct shell *shell, size_t argc, char **argv)
{
	shell_info(shell, "Read NOR FLASH ID");
	/* Start to test */
	test_objs.shell = shell;
	nor_flash_read_id(test_objs.cur_dev);
	return 0;
}

static int nor_flash_erase_handler(const struct shell *shell, size_t argc, char **argv)
{
	char *eptr;
	uint32_t addr;
	uint32_t size;

	/* Convert integer from string */
	addr = strtoul(argv[1], &eptr, 0);
	if (*eptr != '\0') {
		shell_error(shell, "Invalid argument, '%s' is not an integer", argv[1]);
		return -EINVAL;
	}

	size = strtoul(argv[2], &eptr, 0);
	if (*eptr != '\0') {
		shell_error(shell, "Invalid argument, '%s' is not an integer", argv[2]);
		return -EINVAL;
	}

	shell_info(shell, "Erase NOR FLASH");
	/* Start to test */
	test_objs.shell = shell;
	nor_flash_erase(test_objs.cur_dev, addr, size);

	return 0;
}

static int nor_flash_read_handler(const struct shell *shell, size_t argc, char **argv)
{
	char *eptr;
	uint32_t addr;
	uint32_t size;

	/* Convert integer from string */
	addr = strtoul(argv[1], &eptr, 0);
	if (*eptr != '\0') {
		shell_error(shell, "Invalid argument, '%s' is not an integer", argv[1]);
		return -EINVAL;
	}

	size = strtoul(argv[2], &eptr, 0);
	if (*eptr != '\0') {
		shell_error(shell, "Invalid argument, '%s' is not an integer", argv[2]);
		return -EINVAL;
	}

	shell_info(shell, "Read NOR FLASH");
	/* Start to test */
	test_objs.shell = shell;
	nor_flash_read(test_objs.cur_dev, addr, size);

	return 0;
}

static int nor_flash_write_handler(const struct shell *shell, size_t argc, char **argv)
{
	char *eptr;
	uint32_t addr;
	uint32_t size;

	/* Convert integer from string */
	addr = strtoul(argv[1], &eptr, 0);
	if (*eptr != '\0') {
		shell_error(shell, "Invalid argument, '%s' is not an integer", argv[1]);
		return -EINVAL;
	}

	size = strtoul(argv[2], &eptr, 0);
	if (*eptr != '\0') {
		shell_error(shell, "Invalid argument, '%s' is not an integer", argv[2]);
		return -EINVAL;
	}

	shell_info(shell, "Write NOR FLASH");
	/* Start to test */
	test_objs.shell = shell;
	nor_flash_write(test_objs.cur_dev, addr, size);

	return 0;
}

static int nor_flash_write_only_handler(const struct shell *shell, size_t argc, char **argv)
{
	char *eptr;
	uint32_t addr;
	uint32_t size;

	/* Convert integer from string */
	addr = strtoul(argv[1], &eptr, 0);
	if (*eptr != '\0') {
		shell_error(shell, "Invalid argument, '%s' is not an integer", argv[1]);
		return -EINVAL;
	}

	size = strtoul(argv[2], &eptr, 0);
	if (*eptr != '\0') {
		shell_error(shell, "Invalid argument, '%s' is not an integer", argv[2]);
		return -EINVAL;
	}

	shell_info(shell, "Write NOR FLASH");
	/* Start to test */
	test_objs.shell = shell;
	nor_flash_write_only(test_objs.cur_dev, addr, size);

	return 0;
}

#ifdef CONFIG_FLASH_EX_OP_ENABLED
static int nor_flash_read_sts_reg_handler(const struct shell *shell, size_t argc, char **argv)
{
	uint8_t reg1, reg2;
	struct npcx_ex_ops_uma_in op_in = {
		.opcode = SPI_NOR_CMD_RDSR,
		.tx_count = 0,
		.addr_count = 0,
		.rx_count = 1,
	};

	struct npcx_ex_ops_uma_out op_out = {
		.rx_buf = &reg1,
	};

	/* Start to test */
	test_objs.shell = shell;

	/* Read status 0 reg */
	flash_ex_op(test_objs.cur_dev, FLASH_NPCX_EX_OP_EXEC_UMA, (uintptr_t)&op_in, &op_out);

	/* Read status 1 reg */
	op_in.opcode = SPI_NOR_CMD_RDSR2, op_out.rx_buf = &reg2;
	flash_ex_op(test_objs.cur_dev, FLASH_NPCX_EX_OP_EXEC_UMA, (uintptr_t)&op_in, &op_out);

	shell_info(shell, "READ NOR FLASH STATUS (%02x, %02x)", reg1, reg2);
	return 0;
}

static int nor_flash_write_sts_reg_handler(const struct shell *shell, size_t argc, char **argv)
{
	char *eptr;
	uint32_t sts1, sts2;
	uint8_t reg[2];
	struct npcx_ex_ops_uma_in op_in = {
		.opcode = SPI_NOR_CMD_WREN,
		.tx_count = 0,
		.addr_count = 0,
	};

	/* Convert integer from string */
	sts1 = strtoul(argv[1], &eptr, 0);
	if (*eptr != '\0') {
		shell_error(shell, "Invalid argument, '%s' is not an integer", argv[1]);
		return -EINVAL;
	}

	sts2 = strtoul(argv[2], &eptr, 0);
	if (*eptr != '\0') {
		shell_error(shell, "Invalid argument, '%s' is not an integer", argv[2]);
		return -EINVAL;
	}

	/* Start to test */
	test_objs.shell = shell;

	reg[0] = (sts1 & 0xff);
	reg[1] = (sts2 & 0xff);
	shell_info(shell, "WRITE NOR FLASH STATUS (%02x, %02x)", reg[0], reg[1]);

	/* Write enable */
	flash_ex_op(test_objs.cur_dev,
				  FLASH_NPCX_EX_OP_EXEC_UMA,
				  (uintptr_t)&op_in, NULL);
	/* Write status regs */
	op_in.opcode = SPI_NOR_CMD_WRSR;
	op_in.tx_buf = reg;
	op_in.tx_count = 2;
	flash_ex_op(test_objs.cur_dev,
				  FLASH_NPCX_EX_OP_EXEC_UMA,
				  (uintptr_t)&op_in, NULL);

	return 0;
}

static int nor_flash_en_wp_handler(const struct shell *shell, size_t argc, char **argv)
{

	struct npcx_ex_ops_qspi_oper_in oper_in = {
		.enable = true,
		.mask = NPCX_EX_OP_INT_FLASH_WP,
	};

	/* Start to test */
	test_objs.shell = shell;
	flash_ex_op(test_objs.cur_dev, FLASH_NPCX_EX_OP_SET_QSPI_OPER,
				(uintptr_t)&oper_in, NULL);
	shell_info(shell, "SPI Spec is %d, %08x", oper_in.enable, oper_in.mask);
	return 0;
}

static int nor_flash_lock_handler(const struct shell *shell, size_t argc, char **argv)
{
	char *eptr;
	int lock;
	struct npcx_ex_ops_qspi_oper_in oper_in = {
		.mask = NPCX_EX_OP_LOCK_UMA,
	};

	/* Convert integer from string */
	lock = strtoul(argv[1], &eptr, 0);
	if (*eptr != '\0') {
		shell_error(shell, "Invalid argument, '%s' is not an integer", argv[1]);
		return -EINVAL;
	}

	/* Start to test */
	test_objs.shell = shell;
	oper_in.enable = (lock == 1) ? true : false;
	flash_ex_op(test_objs.cur_dev, FLASH_NPCX_EX_OP_SET_QSPI_OPER,
				(uintptr_t)&oper_in, NULL);
	shell_info(shell, "SPI Spec is %d, %08x", oper_in.enable, oper_in.mask);

	return 0;
}

static int nor_flash_get_oper_handler(const struct shell *shell, size_t argc, char **argv)
{
	struct npcx_ex_ops_qspi_oper_out oper_out;

	/* Start to test */
	test_objs.shell = shell;

	flash_ex_op(test_objs.cur_dev,
				FLASH_NPCX_EX_OP_GET_QSPI_OPER,
				(uintptr_t)NULL, &oper_out);
	shell_info(shell, "SPI operation is %08x", oper_out.oper);
	shell_info(shell, "UMA lock is %ld", (oper_out.oper & NPCX_EX_OP_LOCK_UMA));
	shell_info(shell, "WP is %ld", (oper_out.oper & NPCX_EX_OP_INT_FLASH_WP));

	return 0;
}
#endif /* CONFIG_FLASH_EX_OP_ENABLED */

/* Multi SPI nor flash devices */
static int nor_flash_active_handler(const struct shell *shell, size_t argc, char **argv)
{
	char *eptr;
	uint32_t flash_dev_idx;

	if (argc == 1) {
		return 0;
	}

	flash_dev_idx = strtoul(argv[1], &eptr, 0);
	if (*eptr != '\0') {
		shell_error(shell, "Invalid argument, '%s' is not an integer", argv[1]);
		return -EINVAL;
	}

	if (flash_dev_idx < NUM_FLASH_DEVICE) {
		test_objs.cur_dev =  flash_devices[flash_dev_idx];
	}

	shell_info(shell, "Select active flash device to [%d]%s", flash_dev_idx,
		   test_objs.cur_dev->name);
	return 0;
}

static int nor_flash_list_handler(const struct shell *shell, size_t argc, char **argv)
{

	for (int i = 0; i < NUM_FLASH_DEVICE; i++) {
		shell_info(shell, "flash device [%d]:%s", i, flash_devices[i]->name);
	}
	shell_info(shell, "Active flash device to %s", test_objs.cur_dev->name);

	return 0;
}

int main(void)
{
	/* Zephyr driver validation main */
	LOG_INF("Start Nor Flash Validation");

	/* Check nor spi flash devices */
	for (int i = 0; i < NUM_FLASH_DEVICE; i++) {
		if (!device_is_ready(flash_devices[i])) {
			LOG_ERR("flash device %s is not ready", flash_devices[i]->name);
			return -ENODEV;
		}
		LOG_INF("flash device [%d]:%s is ready", i, flash_devices[i]->name);
	}

	/* Save current device */
	test_objs.cur_dev =  flash_devices[0];

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_nor_flash,
	SHELL_CMD_ARG(id, NULL, "nor_flash id: read flash id",
		nor_flash_read_id_handler, 1, 0),
	SHELL_CMD_ARG(erase, NULL, "nor_flash erase <addr> <size>: erase flash",
		nor_flash_erase_handler, 3, 0),
	SHELL_CMD_ARG(read, NULL, "nor_flash read <addr> <size>: read flash",
		nor_flash_read_handler, 3, 0),
	SHELL_CMD_ARG(write, NULL, "nor_flash write <addr> <size>: write flash",
		nor_flash_write_handler, 3, 0),
	SHELL_CMD_ARG(wr_only, NULL, "nor_flash wr_only <addr> <size>: write flash",
		nor_flash_write_only_handler, 3, 0),
#ifdef CONFIG_FLASH_EX_OP_ENABLED
	SHELL_CMD_ARG(rdst, NULL, "nor_flash rdst: read flash status registers",
		nor_flash_read_sts_reg_handler, 1, 0),
	SHELL_CMD_ARG(wrst, NULL, "nor_flash wrst <sts1> <sts2>: write flash status registers",
		nor_flash_write_sts_reg_handler, 3, 0),
	SHELL_CMD_ARG(lock, NULL, "nor_flash lock 0/1: Enable lock on",
		nor_flash_lock_handler, 2, 0),
	SHELL_CMD_ARG(wp, NULL, "nor_flash wp: Enable write protection",
		nor_flash_en_wp_handler, 1, 0),
	SHELL_CMD_ARG(oper, NULL, "nor_flash oper: Get current operation",
		nor_flash_get_oper_handler, 1, 0),
#endif /* CONFIG_FLASH_EX_OP_ENABLED */
	SHELL_CMD_ARG(active, NULL, "nor_flash active <device>: select active device",
		nor_flash_active_handler, 1, 1),
	SHELL_CMD_ARG(list, NULL, "nor_flash list: list all flash devices",
		nor_flash_list_handler, 1, 0),
SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(nor_flash, &sub_nor_flash, "NOR Flash validation commands", NULL);
