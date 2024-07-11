/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** CIS central implementation for the time sync sample
 *
 * This file implements a device that acts as a CIS central.
 * The central can either be configured as a transmitter or a receiver.
 *
 * It connects to as many peripherals as it has available ISO channels.
 */

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <sdc_hci_vs.h>
#include <sdc_hci.h>

#include "iso_time_sync.h"

#define ADV_NAME_STR_MAX_LEN (sizeof(CONFIG_BT_DEVICE_NAME))

static bool configured_for_tx;

static K_SEM_DEFINE(sem_connected, 0, 1);

static void scan_start(void);

static bool adv_data_parse_cb(struct bt_data *data, void *user_data)
{
	char *name = user_data;
	uint8_t len;

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
		len = MIN(data->data_len, ADV_NAME_STR_MAX_LEN - 1);
		memcpy(name, data->data, len);
		name[len] = '\0';
		return false;
	default:
		return true;
	}
}

static void scan_recv(const struct bt_le_scan_recv_info *info, struct net_buf_simple *buf)
{
	char name_str[ADV_NAME_STR_MAX_LEN] = {0};

	bt_data_parse(buf, adv_data_parse_cb, name_str);

	if (strncmp(name_str, CONFIG_BT_DEVICE_NAME, ADV_NAME_STR_MAX_LEN) != 0) {
		return;
	}

	if (bt_le_scan_stop()) {
		return;
	}

	struct bt_conn *conn;
	int err = bt_conn_le_create(info->addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT,
				    &conn);
	if (err) {
		printk("Create conn to %s failed (%d)\n", name_str, err);
	}
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
};

static void scan_start(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE_CONTINUOUS, NULL);
	if (err == -EALREADY) {
		/** If the central is RXing, both the ISO and the ACL
		 * disconnection callbacks try to enable the scanner.
		 * If the ISO channel disconnects before the ACL
		 * connection, the application will attempt to enable
		 * the scanner again.
		 */
		printk("Scanning did not start because it has already started (err %d)\n", err);
	} else if (err) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("CIS Central started scanning\n");
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		printk("Failed to connect to %s (%u)\n", addr, err);

		bt_conn_unref(conn);

		scan_start();
		return;
	}

	struct bt_conn_info info;
	if (bt_conn_get_info(conn, &info)) {
		printk("Failed getting conn info\n");
		return;
	}

	uint16_t conn_interval_us = BT_CONN_INTERVAL_TO_US(info.le.interval);
	printk("Conn interval: %u us\n", conn_interval_us);

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

	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

	bt_conn_unref(conn);

	scan_start();
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

void central_start()
{
	bt_le_scan_cb_register(&scan_callbacks);
	bt_conn_cb_register(&conn_callbacks);

	scan_start();

	printk("CIS central started scanning for peripheral(s)\n");
}
