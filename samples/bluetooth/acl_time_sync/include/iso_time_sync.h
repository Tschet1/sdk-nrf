/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ISO_TIME_SYNC_H__
#define ISO_TIME_SYNC_H__

/** Definitions for the ISO time sync sample.
 *
 * This file contains common definitions and API declarations
 * used by the sample.
 */

#include <stdint.h>
#include <stdbool.h>


void central_start(void);

void peripheral_start(void);

/** Obtain the current Bluetooth controller time.
 *
 * The ISO timestamps are based upon this clock.
 *
 * @retval The current controller time.
 */
uint64_t controller_time_us_get(void);

/** Sets the controller to trigger a PPI event at the given timestamp.
 *
 * @param timestamp_us The timestamp where it will trigger.
 */
void controller_time_trigger_set(uint64_t timestamp_us);

/** Get the address of the event that will trigger.
 *
 * @retval The address of the event that will trigger.
 */
uint32_t controller_time_trigger_event_addr_get(void);

/** Initialize the module handling timed toggling of an LED.
 *
 * @retval 0 on success, failure otherwise.
 */
int timed_led_toggle_init(void);

/** Toggle the led to the give value at the given timestamp.
 *
 * @param value The LED value.
 * @param timestamp_us The time when the led will be set.
 *                     The time is specified in controller clock units.
 */
void timed_led_toggle_trigger_at(uint8_t value, uint32_t timestamp_us);

#endif
