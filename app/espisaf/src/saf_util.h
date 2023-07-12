/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SAF_UTIL_H__
#define __SAF_UTIL_H__

#include <zephyr/sys/util.h>

#define MSN(u8)        ((uint8_t)((uint8_t)(u8) >> 4))
#define LSN(u8)        ((uint8_t)((uint8_t)u8 & 0x0F))

#define MSB(u16)        ((uint8_t)((uint16_t)(u16) >> 8))
#define LSB(u16)        ((uint8_t)(u16))

#define MSW(u32)        ((uint16_t)((uint32_t)(u32) >> 16))
#define LSW(u32)        ((uint16_t)(u32))
#define MSB0(u32)       ((uint8_t)(((uint32_t)(u32) & 0xFF000000) >> 24))
#define MSB1(u32)       ((uint8_t)(((uint32_t)(u32) & 0xFF0000) >> 16))
#define MSB2(u32)       ((uint8_t)(((uint16_t)(u32) & 0xFF00) >> 8))
#define MSB3(u32)       ((uint8_t)((u32) & 0xFF))
#define LSB0(u32)       MSB3(u32)
#define LSB1(u32)       MSB2(u32)
#define LSB2(u32)       MSB1(u32)
#define LSB3(u32)       MSB0(u32)

#define MAKE8(nlo, nhi)                                           \
    ((uint8_t)(((uint8_t)(nlo)) | (((uint8_t)(nhi)) << 4)))
#define DIV_CEILING(a, b)     (((a) + ((b)-1)) / (b))
#define MASK_BIT(nb)               (1L<<(nb))

#define CAL_DATA_FROM_ADDR(n) ((uint8_t)(((n) * 3) + 2) + (n >> 11))

#define _4KB_                               (4 * 1024)

/*!< Successful Completion Without Data     */
#define CYC_SCS_CMP_WITHOUT_DATA            0x06
/*!< Successful middle Completion With Data */
#define CYC_SCS_CMP_WITH_DATA_MIDDLE        0x09
/*!< Successful first Completion With Data  */
#define CYC_SCS_CMP_WITH_DATA_FIRST         0x0B
/*!< Successful last Completion With Data   */
#define CYC_SCS_CMP_WITH_DATA_LAST          0x0D
/*!< Successful only Completion With Data   */
#define CYC_SCS_CMP_WITH_DATA_ONLY          0x0F
/*!< Unsuccessful Completion Without Data   */
#define CYC_UNSCS_CMP_WITHOUT_DATA          0x08
/*!< Unsuccessful Last Completion Without Data */
#define CYC_UNSCS_CMP_WITHOUT_DATA_LAST     0x0C
/*!< Unsuccessful Only Completion Without Data */
#define CYC_UNSCS_CMP_WITHOUT_DATA_ONLY     0x0E

#define NPCX_ESPI_SAF_PR_MAX                20
#define FLASH_PRTR_BADDR(n)                 (0x4000A600 + 4 * n)
#define FLASH_PRTR_HADDR(n)                 (0x4000A650 + 4 * n)
#define FLASH_TAG_OVR(n)                    (0x4000A6A0 + 4 * n)

typedef enum
{
    ESPI_FLASH_SAF_REQ_READ  = 0,
    ESPI_FLASH_SAF_REQ_WRITE = 1,
    ESPI_FLASH_SAF_REQ_ERASE = 2,
    ESPI_FLASH_SAF_REQ_RPMC_OP1  = 3,
    ESPI_FLASH_SAF_REQ_RPMC_OP2  = 4,
	ESPI_FLASH_SAF_REQ_UNKNOWN = 5,
    ESPI_FLASH_SAF_REQ_NUM = ESPI_FLASH_SAF_REQ_UNKNOWN,
} ESPI_FLASH_SAF_REQ_T;

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
