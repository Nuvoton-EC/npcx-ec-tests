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
