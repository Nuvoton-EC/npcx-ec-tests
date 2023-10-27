/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/kscan.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(main);

#define MAX_MATRIX_KEY_COLS 13
#define MAX_MATRIX_KEY_ROWS 8
#define ROW_IDX_MAX (MAX_MATRIX_KEY_ROWS - 1)

/* Key pressed callback count  = pressed + release callback = 2 */
#define KEY_PRESS_CB_CNT 2

const struct device *const kscan_dev = DEVICE_DT_GET(DT_NODELABEL(kscan_input));
static K_SEM_DEFINE(sem_kscan, 0, 1);
const struct shell *sh_ptr;

enum kbscan_mode {
	KBSCAN_MD_NONE = 0,
	KBSCAN_MD_SCAN,
	KBSCAN_MD_GHOST,
} tst_mode;

enum ghost_mode {
	GHOST_MD_LEFT_UP = 0,
	GHOST_MD_RIGHT_BUTTOM,
	GHOST_MD_MAX,
} ghost_mode;

struct key_event {
	/* Callback count of each key */
	uint8_t cb_cnt;
	/* Key event status (pressed -> released) */
	bool matrix_state[2];
	/* Information for multi key pressed at the same timeup	 */
	uint8_t col;
	uint8_t row;
};

struct kbscan_data {
	/* Event status of each row(KBSIN) */
	struct key_event event[MAX_MATRIX_KEY_ROWS];
	uint8_t scan_col_idx;
	/* Key event count */
	uint8_t key_evt_cnt;
};

struct kbscan_data kbd_data;

static void set_tst_mode(enum kbscan_mode mode)
{
	tst_mode = mode;
}

static void reset_kbscan_data(void)
{
	memset(&kbd_data, 0, sizeof(struct kbscan_data));
}

static void reset_kbscan_event(void)
{
	memset(&kbd_data.event, 0, sizeof(kbd_data.event));
}

static void kb_callback(const struct device *dev, uint32_t row, uint32_t col,
			bool pressed)
{
	struct key_event *p_event;

	ARG_UNUSED(dev);

	if (sh_ptr == NULL) {
		LOG_INF("(CB) row: %d, col: %d, pressed: %d", row, col, pressed);
	} else {
		shell_info(sh_ptr, "(CB) row: %d, col: %d, pressed: %d", row, col, pressed);
	}

	if (tst_mode == KBSCAN_MD_SCAN) {
		p_event = &kbd_data.event[row];

		/* Record each key pressed status */
		p_event->col = col;
		p_event->row = row;
		p_event->matrix_state[p_event->cb_cnt] = pressed;
		p_event->cb_cnt++;

		/* Check release event the same col */
		if (p_event->cb_cnt == KEY_PRESS_CB_CNT &&
		    p_event->col != col) {
			p_event->col = 0xff;
		}

		/* Check operation done condition */
		if ((row == ROW_IDX_MAX && p_event->cb_cnt == KEY_PRESS_CB_CNT) ||
		    kbd_data.scan_col_idx != col) {
			set_tst_mode(KBSCAN_MD_NONE);
			k_sem_give(&sem_kscan);
		}
	} else if (tst_mode == KBSCAN_MD_GHOST) {
		if (kbd_data.key_evt_cnt >= MAX_MATRIX_KEY_ROWS) {
			shell_info(sh_ptr, "[FAIL] key event buffer full");
			return;
		}

		p_event = &kbd_data.event[kbd_data.key_evt_cnt];

		p_event->col = col;
		p_event->row = row;
		p_event->matrix_state[0] = pressed;
		kbd_data.key_evt_cnt++;

		/* 3 key pressed operation = 6 event */
		if (kbd_data.key_evt_cnt >= 6) {
			k_sem_give(&sem_kscan);
		}
	}
}


int main(void)
{
	/* Zephyr driver validation */
	LOG_INF("Start KSCAN Validation Task");

	if (!device_is_ready(kscan_dev)) {
		LOG_ERR("kscan device %s not ready", kscan_dev->name);
		return ENODEV;
	}
	LOG_INF("kscan device: %s is ready", kscan_dev->name);

	kscan_config(kscan_dev, kb_callback);
	kscan_enable_callback(kscan_dev);

	return 0;
}

/**
 * Fixed one column(KSOUT) and test each row(KSIN) key status.
 *
 * Example:
 * Select KSOUT as 5.
 * KBIN:0 KBOUT:5
 * KBIN:1 KBOUT:5
 * KBIN:2 KBOUT:5
 * KBIN:3 KBOUT:5
 * KBIN:4 KBOUT:5
 * KBIN:5 KBOUT:5
 * KBIN:6 KBOUT:5
 * KBIN:7 KBOUT:5
 */
static int kscan_scan_handler(const struct shell *shell, size_t argc, char **argv)
{
	char *eptr;
	uint8_t col_idx = 0;
	struct key_event *p_event;

	sh_ptr = shell;

	/* Convert integer from string */
	col_idx = strtoul(argv[1], &eptr, 0);
	if (*eptr != '\0') {
		shell_error(shell, "Invalid argument, '%s' is not an integer", argv[1]);
		return -EINVAL;
	}

	/* Check input KSOUT index correct */
	if (col_idx > (MAX_MATRIX_KEY_COLS-1)) {
		shell_error(shell, "Invalid KSOUT index %d(0~%d)",
			    col_idx, MAX_MATRIX_KEY_COLS-1);
		return -EINVAL;
	}

	reset_kbscan_data();
	kbd_data.scan_col_idx = col_idx;


	shell_info(shell, "Test Scan Mode, row_lim: %d, cb_cnt_lim: %d\r\n",
		   ROW_IDX_MAX, KEY_PRESS_CB_CNT);

	while (true) {
		reset_kbscan_event();
		set_tst_mode(KBSCAN_MD_SCAN);

		/* Wait until KBSIN reach lmiit in callback */
		k_sem_take(&sem_kscan, K_FOREVER);

		/* KBSIN reach limit in callback, check all key pressed status */
		for (int row = 0; row < MAX_MATRIX_KEY_ROWS; row++) {
			/* Get KBSIN event result */
			p_event = &kbd_data.event[row];

			/* Each press operation include 1 pressed and 1 released callback */
			if (p_event->cb_cnt != 0x2 ||
			    p_event->col != col_idx ||
			    p_event->row != row ||
			    p_event->matrix_state[0] != true ||
			    p_event->matrix_state[1] != false) {
				/* Print result */
				shell_info(shell, "Expected: row=%d, col=%d", row, col_idx);
				shell_info(shell, "Record:   row=%d, col=%d cb_cnt=%d, sts=%d %d",
				           p_event->row, p_event->col, p_event->cb_cnt,
				           p_event->matrix_state[0], p_event->matrix_state[1]);

				shell_error(shell, "[FAIL] Scan test");

				return -ENODEV;
			}
		}

		shell_info(shell, "[PASS] KBSOUT: %d, KBSIN 0~%d\r\n[GO]\r\n",
			   col_idx, (MAX_MATRIX_KEY_ROWS - 1));

		col_idx++;
		kbd_data.scan_col_idx = col_idx;

		if (col_idx >= MAX_MATRIX_KEY_COLS) {
			break;
		}
	}

	shell_info(shell, "[PASS] Scan test");

	return 0;
}

/* Compare 2 key event is the same key (pressed and released) in multi key test */
bool chk_multi_key_evt_the_same(struct key_event *p_evt1,
				  struct key_event *p_evt2)
{
	if (p_evt1->col == p_evt2->col &&
	    p_evt1->row == p_evt2->row &&
	    p_evt1->matrix_state[0] == true &&
	    p_evt2->matrix_state[0] == false) {
		return true;
	}

	return false;
}
/**
 * @brief Check whether ghost key is pressed
 *
 *  KEY pressed event sequence:  KEY_1 -> KEY_2 -> KEY_3
 *  KEY released event sequence: KEY_4 -> KEY_5 -> KEY_6
 *
 *  KEY_1 (pressed) = KEY_4 (released)
 *  KEY_3 (pressed) = KEY_5 (released)
 *  KEY_3 (pressed) = KEY_6 (released)
 *
 * < Left-Up 3 Keys >
 *    COL_0   COL_1   COL_2
 *  +-----------------------+
 *  | KEY_1 | KEY_3 |       | ROW_0
 *  |       |       |       |
 *  +-------+-------+-------+
 *  | KEY_2 | Ghost |       | ROW_1
 *  |       |  Key  |       |
 *  +-------+-------+-------+
 *  |       |       |       | ROW_2
 *  |       |       |       |
 *  +-------+-------+-------+
 *
 * < Right-Bottom 3 Keys >
 *    COL_0   COL_1   COL_2
 *  +-----------------------+
 *  |       |       |       | ROW_0
 *  |       |       |       |
 *  +-------+-------+-------+
 *  |       | Ghost | KEY_2 | ROW_1
 *  |       |  Key  |       |
 *  +-------+-------+-------+
 *  |       | KEY_1 | KEY_3 | ROW_2
 *  |       |       |       |
 *  +-------+-------+-------+
 *
 */

static int kscan_ghost_handler(const struct shell *shell, size_t argc, char **argv)
{
	char *eptr;
	struct key_event *p_evt[6];

	/* 0: Left-up 3 key.
	   1: Right-bottom 3 key.
	   2: 4-key pressed
	 */
	uint8_t mode = 0;

	sh_ptr = shell;

	/* Convert integer from string */
	mode = strtoul(argv[1], &eptr, 0);
	if (*eptr != '\0') {
		shell_error(shell, "Invalid argument, '%s' is not an integer", argv[1]);
		return -EINVAL;
	}

	if (mode >= GHOST_MD_MAX) {
		shell_info(shell, "ghost mode not support: %d", mode);
		return -EINVAL;
	}

	set_tst_mode(KBSCAN_MD_GHOST);
	reset_kbscan_data();

	/* Wait until 3 key pressed operation finish */
	k_sem_take(&sem_kscan, K_FOREVER);

	/* Reserve more time for ghost key callback if it is triggered */
	k_sleep(K_MSEC(300));

	/* 3 key pressed operation = 6 callback events */
	/* Ghost key operation 7th and 8th callback should not triggered */
	shell_info(shell, "key_evt_cnt:%d", kbd_data.key_evt_cnt);
	if (kbd_data.key_evt_cnt == 0x0 ||
	    kbd_data.key_evt_cnt > 6) {
		shell_info(shell, "[FAIL] ghost key count");
		return -EINVAL;
	}

	/* Show result */
	for (int i = 0; i < kbd_data.key_evt_cnt; i++) {
		p_evt[i] = &kbd_data.event[i];

		shell_info(shell, "col:%d, row%d, press:%d",
			    p_evt[i]->col, p_evt[i]->row, p_evt[i]->matrix_state[0]);
	}

	/* Check 3 key pressed/released event are the same key */
	if (chk_multi_key_evt_the_same(p_evt[0], p_evt[3]) == true &&
	    chk_multi_key_evt_the_same(p_evt[1], p_evt[4]) == true &&
	    chk_multi_key_evt_the_same(p_evt[2], p_evt[5]) == true) {

		/* Check left-up 3 key */
		if (mode == GHOST_MD_LEFT_UP) {
			/* Check key_4, key_5 the same col */
			/* Check key_4, key_6 the same row */
			if (p_evt[3]->col == p_evt[4]->col &&
			    p_evt[3]->row == p_evt[5]->row) {
				shell_info(shell, "[PASS] Ghost key left-up");
				shell_info(shell, "[GO]");
				return 0;
			}

		/* Check right-bottom 3 key */
		} else if (mode == GHOST_MD_RIGHT_BUTTOM) {
			/* Check key_5, key_6 the same col */
			/* Check key_4, key_6 the same row */
			if (p_evt[4]->col == p_evt[5]->col &&
			    p_evt[3]->row == p_evt[5]->row) {
				shell_info(shell, "[PASS] Ghost key right-bottom");
				shell_info(shell, "[GO]");
				return 0;
			}
		}
	}

	shell_info(shell, "[FAIL] Ghost key test");

	return -EINVAL;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_kscan,
	SHELL_CMD_ARG(scan, NULL, "kscan scan <col_idx>", kscan_scan_handler, 2, 0),
	SHELL_CMD_ARG(ghost, NULL, "kscan ghost <0: left-up 3-key, 1: right-bottom 3-key",
		      kscan_ghost_handler, 2, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(kscan, &sub_kscan, "kscan validation commands", NULL);
