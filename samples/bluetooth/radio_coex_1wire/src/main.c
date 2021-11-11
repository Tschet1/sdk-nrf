/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>
#include <sys/printk.h>
#include <sys/__assert.h>

#include <bluetooth/bluetooth.h>

#include <hal/nrf_radio.h>
#include <hal/nrf_timer.h>
#include <helpers/nrfx_gppi.h>
#include <nrfx_timer.h>
#include <nrfx_gpiote.h>
#include <nrfx_ppi.h>

#define STACKSIZE             CONFIG_MAIN_STACK_SIZE
#define THREAD_PRIORITY       K_LOWEST_APPLICATION_THREAD_PRIO
#define M_COUNTER             NRF_TIMER1

//static const uint32_t btn_gpio_pin = DT_GPIO_PIN(DT_ALIAS(sw0), gpios);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

static void print_welcome_message(void)
{
	printk("-----------------------------------------------------\n");
	printk("This sample illustrates the 1Wire coex interface     \n");
	printk("The number of radio request every second is printed  \n");
	printk("continuously. Press button 1 on the devkit to deny   \n");
	printk("the communication. \n");
	printk("-----------------------------------------------------\n");
}

static void console_print_thread(void)
{
	uint32_t * task_capture = (uint32_t *)nrf_timer_task_address_get(M_COUNTER, nrf_timer_capture_task_get(0));
	uint32_t * task_clear = (uint32_t *)nrf_timer_task_address_get(M_COUNTER, NRF_TIMER_TASK_CLEAR);
	while (1) {
		*task_capture = 1;

		printk("Number of radio events last second: %d\n", nrf_timer_cc_get(M_COUNTER, 0));

		*task_clear = 1;

		k_sleep(K_MSEC(1000));
	}
}

static nrf_ppi_channel_t allocate_gppi_channel(void)
{
	nrf_ppi_channel_t channel;

	if (nrfx_ppi_channel_alloc(&channel) != NRFX_SUCCESS) {
		__ASSERT(false, "(D)PPI channel allocation error");
	}
	return channel;
}

static void setup_radio_event_counter(void)
{
	/* This function sets up a timer as a counter to count radio events. */
	nrf_timer_mode_set(M_COUNTER, NRF_TIMER_MODE_LOW_POWER_COUNTER);

	nrf_ppi_channel_t channel = allocate_gppi_channel();

	nrfx_gppi_channel_endpoints_setup(channel,
		nrf_radio_event_address_get(NRF_RADIO, NRF_RADIO_EVENT_READY),

		nrf_timer_task_address_get(M_COUNTER, NRF_TIMER_TASK_COUNT));
	nrfx_ppi_channel_enable(channel);
}

void main(void)
{
	printk("Starting Radio Coex Demo 1wire on board %s\n", CONFIG_BOARD);

	if (bt_enable(NULL)) {
		printk("Bluetooth init failed");
		return;
	}
	printk("Bluetooth initialized\n");

	setup_radio_event_counter();

	if (bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad), NULL, 0)) {
		printk("Advertising failed to start");
		return;
	}
	printk("Advertising started\n");

	print_welcome_message();

	while (1) {
		k_sleep(K_MSEC(100));
	}
}

K_THREAD_DEFINE(console_print_thread_id, STACKSIZE, console_print_thread,
		NULL, NULL, NULL, THREAD_PRIORITY, 0, 0);
