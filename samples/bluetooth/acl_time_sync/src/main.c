/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <ctype.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/console/console.h>
#include "iso_time_sync.h"

enum role {
	CENTRAL = 'c',
	PERIPHERAL = 'p',
};

#define CONSOLE_INTEGER_INVALID 0xFFFFFFFFU

static enum role role_select(void)
{
	enum role role;

	while (true) {
		printk("Choose role - central (c) / peripheral (p)");
		role = console_getchar();
		printk("%c\n", role);

		switch (role) {
		case CENTRAL:
		case PERIPHERAL:
			return role;
		default:
			printk("Invalid role selected\n");
		}
	}
}

int main(void)
{
	int err;
	enum role role;

	console_init();

	printk("Bluetooth ISO Time Sync Demo\n");

	err = timed_led_toggle_init();
	if (err != 0) {
		printk("Error failed to init LED device for toggling\n");
		return err;
	}

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	role = role_select();
	if (role == CENTRAL) {
		central_start();
	} else {
		peripheral_start();
	}

	return 0;
}
