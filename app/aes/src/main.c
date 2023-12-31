/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/crypto/crypto.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(main);

#define TASK_STACK_SIZE      1024
#define PRIORITY             7

#define AES_TEST_EVENT       4
#define AES_TEST_DATA_SIZE   48

#define AES_TEST_PASS        0
#define AES_TEST_FAIL        1

#define AES_BLOCK_SIZE       16

K_THREAD_STACK_DEFINE(temp_stack, TASK_STACK_SIZE);

#define MAX_ARGUMNETS 3
#define MAX_ARGU_SIZE 10

static uint8_t arguments[MAX_ARGUMNETS][MAX_ARGU_SIZE];
struct k_event aes_event;
static struct k_thread temp_id;

static const struct device *const aes_dev = DEVICE_DT_GET(DT_NODELABEL(aes));

static const uint8_t aes_key[32] = {
0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08,
0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08,
};

static const uint8_t aes_iv[16] = {
0x8f, 0x94, 0x67, 0x86, 0x65, 0x73, 0x1c, 0x83,
0x6d, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x83, 0x08,
};

uint8_t aes_plaintext_golden[] __aligned(4) = {
0xd9, 0x31, 0x32, 0x25, 0xf8, 0x84, 0x06, 0xe5,
0xa5, 0x59, 0x09, 0xc5, 0xaf, 0xf5, 0x26, 0x9a,
0x86, 0xa7, 0xa9, 0x53, 0x15, 0x34, 0xf7, 0xda,
0x2e, 0x4c, 0x30, 0x3d, 0x8a, 0x31, 0x8a, 0x72,
0x1c, 0x3c, 0x0c, 0x95, 0x95, 0x68, 0x09, 0x53,
0x2f, 0xcf, 0x0e, 0x24, 0x49, 0xa6, 0xb5, 0x25,
};

uint8_t aes_ecb_128_ciphertext_golden[] __aligned(4) = {
0x96, 0x20, 0xE3, 0xF5, 0x4B, 0x18, 0x2E, 0x75,
0xFA, 0x21, 0x0A, 0x92, 0xD8, 0xEE, 0xD5, 0x8D,
0xA0, 0x90, 0x16, 0xAB, 0x72, 0xE2, 0x3C, 0x11,
0x82, 0x68, 0x0C, 0xA7, 0xB8, 0xBF, 0xF3, 0x34,
0x59, 0x09, 0x69, 0x34, 0x2B, 0xEE, 0xE2, 0xB4,
0x49, 0x4D, 0xF7, 0xC9, 0x94, 0x3C, 0x3A, 0x2F,
};

uint8_t aes_ecb_192_ciphertext_golden[] __aligned(4) = {
0x15, 0x5B, 0x77, 0xD0, 0xD7, 0x92, 0x65, 0x5E,
0xB0, 0xDA, 0x07, 0x73, 0xFE, 0xA5, 0x96, 0xFB,
0x51, 0x37, 0x04, 0xC7, 0xB5, 0x6D, 0xD2, 0x75,
0x78, 0xFE, 0xE8, 0xC2, 0x83, 0x4B, 0x87, 0xD4,
0x7C, 0xB8, 0x6C, 0x4A, 0xC9, 0x95, 0x48, 0xEC,
0x97, 0xB9, 0x9E, 0x97, 0xA6, 0xB1, 0xCA, 0x0E,
};

uint8_t aes_ecb_256_ciphertext_golden[] __aligned(4) = {
0x5F, 0xC4, 0xD4, 0xC2, 0x64, 0x34, 0xA6, 0xE5,
0xB5, 0x72, 0xCB, 0x42, 0x1A, 0x1F, 0xE3, 0x0A,
0xC9, 0x2D, 0xF0, 0xB7, 0x24, 0x93, 0x14, 0xC2,
0x67, 0x7C, 0x57, 0x97, 0xBC, 0x3D, 0xCF, 0x6F,
0xAC, 0x82, 0x3D, 0x10, 0x94, 0x8F, 0x24, 0xFA,
0xC8, 0x2A, 0x4A, 0x1A, 0x36, 0x9F, 0xE2, 0x76,
};

uint8_t aes_cbc_128_ciphertext_golden[] __aligned(4) = {
0xEE, 0x74, 0x05, 0x35, 0x8C, 0x53, 0x8E, 0xE5,
0x7E, 0x4F, 0x9F, 0xFB, 0xEA, 0xB2, 0x45, 0xD0,
0xEB, 0x61, 0x2F, 0x68, 0x5F, 0x42, 0x30, 0x82,
0xEF, 0xE3, 0xC7, 0x4C, 0x5C, 0x61, 0x0C, 0x3C,
0x7F, 0x64, 0x3D, 0x51, 0x3A, 0x10, 0xEA, 0xC2,
0x8F, 0x38, 0x38, 0xBF, 0x41, 0xC8, 0x49, 0x69,
};

uint8_t aes_cbc_192_ciphertext_golden[] __aligned(4) = {
0xC3, 0xC4, 0xA9, 0xE9, 0xE6, 0x2F, 0x77, 0x7B,
0xBE, 0x28, 0x7F, 0xF6, 0x8E, 0x50, 0x0B, 0xD2,
0x63, 0x50, 0x82, 0x34, 0x27, 0xFD, 0xF1, 0x9E,
0xCE, 0x49, 0x4D, 0x02, 0x32, 0x03, 0xCD, 0x42,
0x73, 0x1A, 0x93, 0xDB, 0x63, 0x3A, 0x04, 0x29,
0xFE, 0x32, 0x2C, 0x2C, 0xEF, 0x92, 0x8B, 0x1E,
};

uint8_t aes_cbc_256_ciphertext_golden[] __aligned(4) = {
0x77, 0x9D, 0x70, 0xDD, 0xBF, 0x98, 0xBE, 0x1B,
0x79, 0x6A, 0x8C, 0x31, 0x5B, 0x1A, 0x2B, 0xDB,
0xCA, 0x48, 0xED, 0xD5, 0x9E, 0x1F, 0xFF, 0x31,
0x51, 0x14, 0x36, 0x80, 0xAA, 0x83, 0x01, 0x8C,
0xE8, 0x7A, 0x72, 0x8A, 0xAC, 0xF6, 0x60, 0x6D,
0x8E, 0x5B, 0x8B, 0xD8, 0x6F, 0x82, 0x0F, 0x72,
};

uint8_t aes_ctr_128_ciphertext_golden[] __aligned(4) = {
0x28, 0x6B, 0x4D, 0xCB, 0xF0, 0x89, 0xCA, 0x28,
0xE6, 0xA0, 0xFD, 0x8D, 0xFE, 0x96, 0xA7, 0x97,
0xAD, 0xB3, 0xF3, 0xD6, 0xCA, 0xE2, 0x79, 0x68,
0x35, 0xE1, 0xA6, 0x1E, 0x05, 0xC5, 0xF1, 0x00,
0x0F, 0xE1, 0x3E, 0x9F, 0x15, 0x6A, 0x13, 0x4A,
0x9E, 0x9A, 0xF9, 0x31, 0x13, 0xC5, 0x55, 0xE9,
};

uint8_t aes_ctr_192_ciphertext_golden[] __aligned(4) = {
0x21, 0xEC, 0x57, 0xA7, 0x26, 0x2F, 0xED, 0x8A,
0xAB, 0x9A, 0x9C, 0x99, 0xEE, 0xBA, 0x4C, 0x02,
0x34, 0x04, 0xC5, 0x59, 0x87, 0xF8, 0xB4, 0x34,
0x49, 0x3D, 0x9F, 0xCA, 0x72, 0xDC, 0x60, 0xAF,
0xA8, 0x67, 0x30, 0x59, 0x90, 0xDA, 0x5A, 0x62,
0x8F, 0xB4, 0x05, 0x6F, 0x82, 0xD5, 0xAF, 0xB1,
};

uint8_t aes_ctr_256_ciphertext_golden[] __aligned(4) = {
0xD8, 0xF1, 0x0B, 0x25, 0xD1, 0xF5, 0xEA, 0x3F,
0x08, 0xE0, 0x87, 0xB3, 0xBB, 0x13, 0x22, 0x01,
0xEF, 0x61, 0xC2, 0x80, 0xF2, 0xD2, 0xFF, 0xC2,
0xD5, 0x8B, 0xF1, 0x0C, 0x55, 0x39, 0xAA, 0xB2,
0x67, 0x8F, 0x6B, 0xBB, 0x90, 0x26, 0x26, 0x41,
0x76, 0x9D, 0x61, 0x14, 0x27, 0x05, 0x2B, 0x45,
};

uint8_t aes_gcm_128_ciphertext_golden[] __aligned(4) = {
0x61, 0x35, 0x3B, 0x4C, 0x28, 0x06, 0x93, 0x4A,
0x77, 0x7F, 0xF5, 0x1F, 0xA2, 0x2A, 0x47, 0x55,
0x69, 0x9B, 0x2A, 0x71, 0x4F, 0xCD, 0xC6, 0xF8,
0x37, 0x66, 0xE5, 0xF9, 0x7B, 0x6C, 0x74, 0x23,
0x73, 0x80, 0x69, 0x00, 0xE4, 0x9F, 0x24, 0xB2,
0x2B, 0x09, 0x75, 0x44, 0xD4, 0x89, 0x6B, 0x42,
};

uint8_t aes_gcm_192_ciphertext_golden[] __aligned(4) = {
0x0F, 0x10, 0xF5, 0x99, 0xAE, 0x14, 0xA1, 0x54,
0xED, 0x24, 0xB3, 0x6E, 0x25, 0x32, 0x4D, 0xB8,
0xC5, 0x66, 0x63, 0x2E, 0xF2, 0xBB, 0xB3, 0x4F,
0x83, 0x47, 0x28, 0x0F, 0xC4, 0x50, 0x70, 0x57,
0xFD, 0xDC, 0x29, 0xDF, 0x9A, 0x47, 0x1F, 0x75,
0xC6, 0x65, 0x41, 0xD4, 0xD4, 0xDA, 0xD1, 0xC9,
};

uint8_t aes_gcm_256_ciphertext_golden[] __aligned(4) = {
0xC3, 0x76, 0x2D, 0xF1, 0xCA, 0x78, 0x7D, 0x32,
0xAE, 0x47, 0xC1, 0x3B, 0xF1, 0x98, 0x44, 0xCB,
0xAF, 0x1A, 0xE1, 0x4D, 0x0B, 0x97, 0x6A, 0xFA,
0xC5, 0x2F, 0xF7, 0xD7, 0x9B, 0xBA, 0x9D, 0xE0,
0xFE, 0xB5, 0x82, 0xD3, 0x39, 0x34, 0xA4, 0xF0,
0x95, 0x4C, 0xC2, 0x36, 0x3B, 0xC7, 0x3F, 0x78,
};

uint8_t aes_ad[] __aligned(4) = {
0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef,
0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef,
0xab, 0xad, 0xda, 0xd2
};

uint8_t aes_nonce[] __aligned(4) = {
0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad
};

uint8_t aes_gcm_128_tag_golden[AES_BLOCK_SIZE] __aligned(4) = {
0xF6, 0xF1, 0x09, 0x17, 0x64, 0x90, 0x06, 0x30,
0xA1, 0x93, 0x65, 0x24, 0x07, 0x77, 0xE7, 0x26,
};

uint8_t aes_gcm_192_tag_golden[AES_BLOCK_SIZE] __aligned(4) = {
0xD4, 0x8F, 0xA5, 0xBE, 0x30, 0x6A, 0xA1, 0xD9,
0x14, 0x23, 0x94, 0x2A, 0xB3, 0x7E, 0x44, 0x9B,
};

uint8_t aes_gcm_256_tag_golden[AES_BLOCK_SIZE] __aligned(4) = {
0x18, 0x32, 0x3B, 0x38, 0xC6, 0x4E, 0xF2, 0x0D,
0xC6, 0xB0, 0xB1, 0xE5, 0xDE, 0x91, 0x9E, 0xFB,
};

static uint8_t aes_decrypt[AES_TEST_DATA_SIZE], aes_encrypt[AES_TEST_DATA_SIZE];
static uint8_t aes_tag[AES_BLOCK_SIZE];

void print_array_value(void *x, uint32_t len)
{
	int32_t i;
	uint8_t* ptr_u8 = (uint8_t*)x;

	/* little-endian */
	printk("\t    ");
	for (i = 0 ; i < len  ; i++) {
		if ((i % 16) == 0) {
			printk("\r\n");
		}
		printk("%02X", ptr_u8[i]);
	}
	printk("\r\n\r\n");
}

static int aes_setup(struct cipher_ctx *ctx, enum cipher_mode opmode,
			int keybitSize, enum cipher_op encrypt)
{
	ctx->ops.cipher_mode = opmode;
	ctx->key.bit_stream = (uint8_t *)aes_key;
	ctx->keylen = keybitSize;
	ctx->flags = CAP_RAW_KEY | CAP_SYNC_OPS | CAP_SEPARATE_IO_BUFS;
	return(cipher_begin_session(aes_dev, ctx, CRYPTO_CIPHER_ALGO_AES,
		opmode, encrypt));
}

static int aes_test(int keysize, enum cipher_mode opmode)
{
	int ret;
	uint8_t *ciphertext_golden = NULL;
	uint8_t *tag_golden = NULL;
	struct cipher_ctx ctx;
	struct cipher_pkt pkt;
	struct cipher_aead_pkt aead_pkt;

	if (opmode == CRYPTO_CIPHER_MODE_ECB) {
		if (keysize == 128) {
			ciphertext_golden = aes_ecb_128_ciphertext_golden;
		} else if (keysize == 192) {
			ciphertext_golden = aes_ecb_192_ciphertext_golden;
		} else {
			ciphertext_golden = aes_ecb_256_ciphertext_golden;
		}
	} else if (opmode == CRYPTO_CIPHER_MODE_CBC) {
		if (keysize == 128) {
			ciphertext_golden = aes_cbc_128_ciphertext_golden;
		} else if (keysize == 192) {
			ciphertext_golden = aes_cbc_192_ciphertext_golden;
		} else {
			ciphertext_golden = aes_cbc_256_ciphertext_golden;
		}
	} else if (opmode == CRYPTO_CIPHER_MODE_CTR) {
		if (keysize == 128) {
			ciphertext_golden = aes_ctr_128_ciphertext_golden;
		} else if (keysize == 192) {
			ciphertext_golden = aes_ctr_192_ciphertext_golden;
		} else {
			ciphertext_golden = aes_ctr_256_ciphertext_golden;
		}
	} else if (opmode == CRYPTO_CIPHER_MODE_GCM) {
		if (keysize == 128) {
			ciphertext_golden = aes_gcm_128_ciphertext_golden;
			tag_golden = aes_gcm_128_tag_golden;
		} else if (keysize == 192) {
			ciphertext_golden = aes_gcm_192_ciphertext_golden;
			tag_golden = aes_gcm_192_tag_golden;
		} else {
			ciphertext_golden = aes_gcm_256_ciphertext_golden;
			tag_golden = aes_gcm_256_tag_golden;
		}
	} else {
		printk("AES FAIL mode:%d\n\r", opmode);
		return AES_TEST_FAIL;
	}

	/* Encryption Test */
	ret = aes_setup(&ctx, opmode, keysize, CRYPTO_CIPHER_OP_ENCRYPT);

	if (ret == 0) {
		pkt.in_buf = aes_plaintext_golden;
		pkt.out_buf = aes_encrypt;
		pkt.in_len = AES_TEST_DATA_SIZE;
		pkt.out_buf_max = AES_TEST_DATA_SIZE;
		pkt.ctx = &ctx;

		if (opmode == CRYPTO_CIPHER_MODE_ECB) {
			ctx.ops.block_crypt_hndlr(&ctx, &pkt);
		} else if (opmode == CRYPTO_CIPHER_MODE_CBC) {
			ctx.ops.cbc_crypt_hndlr(&ctx, &pkt, (uint8_t *)aes_iv);
		} else if (opmode == CRYPTO_CIPHER_MODE_CTR) {
			ctx.ops.ctr_crypt_hndlr(&ctx, &pkt, (uint8_t *)aes_iv);
		} else if (opmode == CRYPTO_CIPHER_MODE_GCM) {
			aead_pkt.pkt = &pkt;
			aead_pkt.ad = (uint8_t *)aes_ad;
			aead_pkt.ad_len = sizeof(aes_ad);
			aead_pkt.tag = (uint8_t *)aes_tag;
			ctx.mode_params.gcm_info.tag_len = sizeof(aes_tag);
			ctx.mode_params.gcm_info.nonce_len = sizeof(aes_nonce);
			ctx.ops.gcm_crypt_hndlr(&ctx, &aead_pkt, (uint8_t *)aes_nonce);
		} else {
			printk("Encrypt FAIL mode:%d\n\r", opmode);
			return AES_TEST_FAIL;
		}
	}else {
		printk("Encrypt FAIL mode:%d size:%d\n\r", opmode, keysize);
		return AES_TEST_FAIL;
	}

	printk("Encryption: \n\r");
	print_array_value(pkt.in_buf, pkt.in_len);
	print_array_value(pkt.out_buf, pkt.out_len);

	if (opmode == CRYPTO_CIPHER_MODE_GCM) {
		print_array_value(aead_pkt.tag, ctx.mode_params.gcm_info.tag_len);

		for (ret = 0; ret < ctx.mode_params.gcm_info.tag_len; ret++) {
			if (aead_pkt.tag[ret] != tag_golden[ret]) {
				printk("Encryption: AES GCM TAG Error!\n\r");
				return AES_TEST_FAIL;
			}
		}
	}

	for (ret = 0; ret < pkt.out_len; ret++) {
		if (aes_encrypt[ret] != ciphertext_golden[ret]) {
			return AES_TEST_FAIL;
		}
	}

	/* Decryption Test */
	memset(aes_tag, 0, sizeof(aes_tag));
	ret = aes_setup(&ctx, opmode, keysize, CRYPTO_CIPHER_OP_DECRYPT);

	if (ret == 0) {
		pkt.in_buf = ciphertext_golden;
		pkt.out_buf = aes_decrypt;
		pkt.in_len = AES_TEST_DATA_SIZE;
		pkt.out_buf_max = AES_TEST_DATA_SIZE;
		pkt.ctx = &ctx;

		if (opmode == CRYPTO_CIPHER_MODE_ECB) {
			ctx.ops.block_crypt_hndlr(&ctx, &pkt);
		} else if (opmode == CRYPTO_CIPHER_MODE_CBC) {
			ctx.ops.cbc_crypt_hndlr(&ctx, &pkt, (uint8_t *)aes_iv);
		} else if (opmode == CRYPTO_CIPHER_MODE_CTR) {
			ctx.ops.ctr_crypt_hndlr(&ctx, &pkt, (uint8_t *)aes_iv);
		} else if (opmode == CRYPTO_CIPHER_MODE_GCM) {
			aead_pkt.pkt = &pkt;
			aead_pkt.ad = (uint8_t *)aes_ad;
			aead_pkt.ad_len = sizeof(aes_ad);
			aead_pkt.tag = (uint8_t *)aes_tag;
			ctx.mode_params.gcm_info.tag_len = sizeof(aes_tag);
			ctx.mode_params.gcm_info.nonce_len = sizeof(aes_nonce);
			ctx.ops.gcm_crypt_hndlr(&ctx, &aead_pkt, (uint8_t *)aes_nonce);
		} else {
			printk("Decrypt FAIL mode:%d\n\r", opmode);
			return AES_TEST_FAIL;
		}
	} else {
		printk("Decrypt FAIL mode:%d size:%d\n\r", opmode, keysize);
		return AES_TEST_FAIL;
	}

	printk("Decryption: \n\r");
	print_array_value(pkt.in_buf, pkt.in_len);
	print_array_value(pkt.out_buf, pkt.out_len);

	if (opmode == CRYPTO_CIPHER_MODE_GCM) {
		print_array_value(aead_pkt.tag, ctx.mode_params.gcm_info.tag_len);

		for (ret = 0; ret < ctx.mode_params.gcm_info.tag_len; ret++) {
			if (aead_pkt.tag[ret] != tag_golden[ret]) {
				printk("Decryption: AES GCM TAG Error!\n\r");
				return AES_TEST_FAIL;
			}
		}
	}

	for (ret = 0; ret < pkt.out_len; ret++) {
		if (aes_decrypt[ret] != aes_plaintext_golden[ret]) {
			return AES_TEST_FAIL;
		}
	}

	return AES_TEST_PASS;
}

static void aes_validation_func(void *dummy1, void *dummy2, void *dummy3)
{
	uint32_t events, keysize, test_result;

	k_event_init(&aes_event);

	while (true) {
		events = k_event_wait(&aes_event, 0xFFF, true, K_FOREVER);

		if (events != AES_TEST_EVENT) {
			printk("[FAIL] AES TEST EVENT is incorrect.(%d)\n\r", events);
			continue;
		}

		memset(aes_decrypt, 0, AES_TEST_DATA_SIZE);
		memset(aes_encrypt, 0, AES_TEST_DATA_SIZE);
		keysize = atoi(arguments[1]);

		if (!strcmp("ecb", arguments[0])) {
			test_result = aes_test(keysize, CRYPTO_CIPHER_MODE_ECB);
		} else if (!strcmp("cbc", arguments[0])) {
			test_result = aes_test(keysize, CRYPTO_CIPHER_MODE_CBC);
		} else if (!strcmp("ctr", arguments[0])) {
			test_result = aes_test(keysize, CRYPTO_CIPHER_MODE_CTR);
		} else if (!strcmp("gcm", arguments[0])) {
			test_result = aes_test(keysize, CRYPTO_CIPHER_MODE_GCM);
		} else {
			test_result = AES_TEST_FAIL;
			printk("[FAIL] mode not support\n\r");
		}

		if (test_result == AES_TEST_PASS) {
			printk("[PASS] AES %s mode key length %d bit\n\r", arguments[0], keysize);
		} else {
			printk("[FAIL] AES %s mode key length %d bit\n\r", arguments[0], keysize);
		}
	}
}

int main(void)
{
	/* Zephyr driver validation */
	printk("Start AES Validation Task\n\r");
	k_thread_create(&temp_id, temp_stack, TASK_STACK_SIZE, aes_validation_func, NULL, NULL,
			NULL, PRIORITY, K_INHERIT_PERMS, K_FOREVER);
	k_thread_name_set(&temp_id, "AES Validation");
	k_thread_start(&temp_id);

	return 0;
}

static int aes_command(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t i;

	for (i = 0; i < argc; i++) {
		strcpy(arguments[i], argv[i]);
	}

	k_event_post(&aes_event, AES_TEST_EVENT);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_aes,
	SHELL_CMD_ARG(ecb, NULL, "aes ecb key_length", aes_command, 2, 0),
	SHELL_CMD_ARG(cbc, NULL, "aes cbc key_length", aes_command, 2, 0),
	SHELL_CMD_ARG(ctr, NULL, "aes ctr key_length", aes_command, 2, 0),
	SHELL_CMD_ARG(gcm, NULL, "aes gcm key_length", aes_command, 2, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(aes, &sub_aes, "AES validation commands", NULL);

