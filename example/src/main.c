/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>

static int gpio_command(const struct shell *shell, size_t argc, char **argv)
{
	/* Do something for this command */
	shell_info(shell, "test go go go!!");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_test,
	SHELL_CMD_ARG(go, NULL, "test go", gpio_command, 1, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(test, &sub_test, "test validation commands", NULL);
