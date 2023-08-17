/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TAF_UTIL_H__
#define __TAF_UTIL_H__

#include <zephyr/sys/util.h>

#define CAL_DATA_FROM_ADDR(n) ((uint8_t)(((n) * 3) + 2) + (n >> 11))

struct taf_handle_data {
	uint8_t  taf_type;
	uint8_t  taf_tag;
	uint32_t address;
	uint16_t length;
	uint32_t src[16];
	uint8_t *buf;
	struct k_work work;
};

struct taf_handle_data taf_data;

#endif /*__TAF_UTIL_H__*/
