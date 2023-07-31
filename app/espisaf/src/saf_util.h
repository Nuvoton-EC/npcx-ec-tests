/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SAF_UTIL_H__
#define __SAF_UTIL_H__

#include <zephyr/sys/util.h>

#define CAL_DATA_FROM_ADDR(n) ((uint8_t)(((n) * 3) + 2) + (n >> 11))

enum {
	ESPI_FLASH_SAF_REQ_READ  = 0,
	ESPI_FLASH_SAF_REQ_WRITE = 1,
	ESPI_FLASH_SAF_REQ_ERASE = 2,
	ESPI_FLASH_SAF_REQ_RPMC_OP1  = 3,
	ESPI_FLASH_SAF_REQ_RPMC_OP2  = 4,
	ESPI_FLASH_SAF_REQ_UNKNOWN = 5,
};

struct saf_handle_data {
	uint8_t  saf_type;
	uint8_t  saf_tag;
	uint32_t address;
	uint16_t length;
	uint32_t src[16];
	uint8_t *buf;
	struct k_work work;
};

struct saf_handle_data saf_data;

#endif /*__SAF_UTIL_H__*/
