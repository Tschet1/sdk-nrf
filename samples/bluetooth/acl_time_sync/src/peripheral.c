/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** CIS peripheral implementation for the time sync sample
 *
 * This file implements a device that acts as a CIS peripheral.
 * The peripheral can either be configured as a transmitter or a receiver.
 */

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <sdc_hci_vs.h>
#include <sdc_hci.h>

#include "iso_time_sync.h"

static bool configured_for_tx;

static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		printk("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	struct bt_conn_info info;
	if (bt_conn_get_info(conn, &info)) {
		printk("Failed getting conn info\n");
		return;
	}

	uint16_t conn_interval_us = BT_CONN_INTERVAL_TO_US(info.le.interval);

	k_sleep(K_MSEC(100));

	uint16_t conn_handle = 0;
	err = bt_hci_get_conn_handle(conn, &conn_handle);

	const sdc_hci_cmd_vs_get_conn_event_anchor_t params = {
		.conn_handle = conn_handle,
	};
	sdc_hci_cmd_vs_get_conn_event_anchor_return_t return_params;

	err = hci_vs_sdc_get_conn_event_anchor(&params, &return_params);

	if (err) {
		printk("Failed to get conn anchor (%u)\n", err);
		return;
	}

	const uint16_t start_conn_evt = 10;

	const uint16_t conn_evt_til_start = start_conn_evt - return_params.conn_event_counter;

	timed_led_toggle_trigger_at(1, return_params.timestamp_us + conn_evt_til_start * conn_interval_us);

	printk("Read conn evt: %u, local time %u", return_params.conn_event_counter, return_params.timestamp_us);
	printk("Connected: %s\n", addr);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected from %s (reason 0x%02x)\n", addr, reason);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

void peripheral_start()
{
	int err;

	bt_conn_cb_register(&conn_callbacks);

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("CIS peripheral started advertising\n");
}
