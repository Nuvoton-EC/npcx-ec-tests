/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <zephyr/crypto/crypto.h>
#include <zephyr/logging/log.h>
#include "sha_data.h"

LOG_MODULE_REGISTER(main);

#define STACK_SIZE                         1024
#define THREAD_PRIORITY                    1
#define LOG_LEVEL LOG_LEVEL_INF
#define CRYPTO_DEV_NODE DT_NODELABEL(sha0)

K_THREAD_STACK_DEFINE(temp_stack, STACK_SIZE);

static struct k_thread sha_id;

const struct device *const sha_dev = DEVICE_DT_GET(CRYPTO_DEV_NODE);

enum hash_algo g_sha = CRYPTO_HASH_ALGO_SHA256;

const char *getSHA_Alg(enum hash_algo alg)
{
	switch (alg) {
	case CRYPTO_HASH_ALGO_SHA256:
		return "CRYPTO_HASH_ALGO_SHA256";
	case CRYPTO_HASH_ALGO_SHA384:
		return "CRYPTO_HASH_ALGO_SHA384";
	case CRYPTO_HASH_ALGO_SHA512:
		return "CRYPTO_HASH_ALGO_SHA512";
	default:
		break;
	}

	return "";
}

static void hash_test(uint8_t index, enum hash_algo sha, struct hash_ctx *ctx)
{
	const struct hash_tp *tp = &hash_test_tbl[(sha - 2)];

	uint8_t ret;
	uint8_t out_buf[64] = {0};
	uint32_t *msg_size = tp->msg_sz;
	uint8_t *dig_ptr = tp->digest;
	uint32_t dig_index = index * tp->digest_sz;
	uint32_t cnt;

	struct hash_pkt pkt = {
		.in_buf = tp->msg[index],
		.in_len = msg_size[index],
		.out_buf = out_buf,
	};

	LOG_INF("SHA alg: %s, index: %x", getSHA_Alg(sha), index);

	ret = hash_compute(ctx, &pkt);
	if (ret != 0) {
		LOG_ERR("Failed to compute hash for test");
		return;
	}
	ret = memcmp(pkt.out_buf, &dig_ptr[dig_index], tp->digest_sz);
	if (ret != 0) {
		LOG_ERR("Not match of result for HASH test");
		for (cnt = 0; cnt < tp->digest_sz; cnt++) {
			if (pkt.out_buf[cnt] != dig_ptr[dig_index + cnt]) {
				LOG_INF("out[%x]=%x, dig[%x]=%x", cnt, pkt.out_buf[cnt],
					   cnt, dig_ptr[dig_index + cnt]);
			}
		}
		return;
	}
}

static void sha_thread_entry(void *dummy1, void *dummy2, void *dummy3)
{
	if (!device_is_ready(sha_dev)) {
		printk("%s is not ready\n", sha_dev->name);
		return;
	}

	LOG_INF("sha device %s is ready", sha_dev->name);
}

static int sha_alg_mode(const struct shell *shell, size_t argc, char **argv)
{
	char *eptr;
	uint32_t val = strtoul(argv[1], &eptr, 0);

	if ((val < 2) || (val > 4)) {
		shell_error(shell, "Invalid argument 2 - 4 (%s)", argv[1]);
		return -EINVAL;
	}

	g_sha = val;

	return 0;
}

static int sha_test(const struct shell *shell, size_t argc, char **argv)
{
	int ret, cnt;
	char *eptr;
	struct hash_ctx ctx;
	uint32_t val = strtoul(argv[1], &eptr, 0);

	if ((val < 0) || (val > 7)) {
		shell_error(shell, "Invalid argument 0 - 7 (%s)", argv[1]);
		return -EINVAL;
	}

	LOG_INF("Start SHA test...");

	ctx.flags = CAP_SYNC_OPS | CAP_SEPARATE_IO_BUFS;

	ret = hash_begin_session(sha_dev, &ctx, g_sha);
	if (ret != 0) {
		LOG_ERR("Failed to init session");
		return -EINVAL;
	}

	for (cnt = 0; cnt <= val; cnt++) {
		hash_test(cnt, g_sha, &ctx);
	}

	hash_free_session(sha_dev, &ctx);

	LOG_INF("[PASS] SHA test completion");

	return 0;
}

/* Main entry */
void main(void)
{
	LOG_INF("Start SHA Task");
	k_thread_create(&sha_id, temp_stack, STACK_SIZE, sha_thread_entry, NULL, NULL,
			NULL, THREAD_PRIORITY, K_INHERIT_PERMS, K_FOREVER);
	k_thread_name_set(&sha_id, "sha_testing");
	k_thread_start(&sha_id);
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_sha,
	SHELL_CMD_ARG(set_alg, NULL, "sha set_alg", sha_alg_mode, 2, 0),
	SHELL_CMD_ARG(sha_test, NULL, "sha sha_test", sha_test, 2, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(sha, &sub_sha, "SHA validation commands", NULL);
