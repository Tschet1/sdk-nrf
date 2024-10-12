/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

#if defined(CONFIG_CLOCK_CONTROL_NRF)
#include <nrfx_clock.h>
#endif /* CONFIG_CLOCK_CONTROL_NRF */

#include <mpsl_clock.h>
#include "mpsl_clock_ctrl.h"

/* Variable shared for nrf and nrf2 clock control */
static atomic_t m_hfclk_refcnt;

/* Type use to get information about a clock request status */
struct clock_onoff_state {
	struct onoff_client cli;
	atomic_t m_clk_ready;
	atomic_t m_clk_refcnt;
	struct k_sem sem;
	int clk_req_rsp;
};

static struct clock_onoff_state m_lfclk_state;

static int32_t m_lfclk_release(void);

#define MPSL_LFCLK_REQUEST_WAIT_TIMEOUT_MS 1000

/** @brief LFCLK request callback.
 *
 * The callback function provided to clock control to notify about LFCLK request being finished.
 */
static void lfclk_request_cb(struct onoff_manager *mgr, struct onoff_client *cli, uint32_t state,
			     int res)
{
	struct clock_onoff_state *clock_state = CONTAINER_OF(cli, struct clock_onoff_state, cli);

	clock_state->clk_req_rsp = res;
	k_sem_give(&clock_state->sem);
}

/** @brief Wait for LFCLK to be ready.
 *
 * The function can time out if there is no response from clock control drvier until
 * MPSL_LFCLK_REQUEST_WAIT_TIMEOUT_MS.
 *
 * @note For nRF54H SoC series waiting for LFCLK can't block the system work queue. The nrf2 clock
 *       control driver can return -TIMEDOUT due not handled response from sysctrl.
 *
 * @retval 0 LFCLK is ready.
 * @retval -NRF_EINVAL There were no LFCLK request.
 * @retval -NRF_EFAULT LFCLK request failed.
 */
static int32_t m_lfclk_wait(void)
{
	int32_t err;

	if (atomic_get(&m_lfclk_state.m_clk_ready) == (atomic_val_t) true) {
		return 0;
	}

	/* Check if lfclk has been requested */
	if (atomic_get(&m_lfclk_state.m_clk_refcnt) <= (atomic_val_t)0) {
		return -NRF_EINVAL;
	}

	/* Wait for response from clock control */
	err = k_sem_take(&m_lfclk_state.sem, K_MSEC(MPSL_LFCLK_REQUEST_WAIT_TIMEOUT_MS));
	if (err < 0) {
		/* Do a gracefull cancel of the request, the function release does this
		 * as well as and relase.
		 */
		(void)m_lfclk_release();

		return -NRF_EFAULT;
	}

	if (m_lfclk_state.clk_req_rsp < 0) {
		__ASSERT(false, "LFCLK could not be started, reason: %d",
			 m_lfclk_state.clk_req_rsp);
		/* Possible failure reasons:
		 *  # -ERRTIMEDOUT - nRFS service timeout
		 *  # -EIO - nRFS service error
		 *  # -ENXIO - request rejected
		 * All these mean failure for MPSL.
		 */
		return -NRF_EFAULT;
	}

	atomic_set(&m_lfclk_state.m_clk_ready, (atomic_val_t) true);

	return 0;
}

#if defined(CONFIG_CLOCK_CONTROL_NRF)

static void m_lfclk_calibration_start(void)
{
	if (IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_DRIVER_CALIBRATION)) {
		z_nrf_clock_calibration_force_start();
	}
}

static bool m_lfclk_calibration_is_enabled(void)
{
	if (IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_DRIVER_CALIBRATION)) {
		return true;
	} else {
		return false;
	}
}

static int32_t m_lfclk_request(void)
{
	struct onoff_manager *mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_LF);
	int32_t err;

	sys_notify_init_callback(&m_lfclk_state.cli.notify, lfclk_request_cb);
	(void)k_sem_init(&m_lfclk_state.sem, 0, 1);

	err = onoff_request(mgr, &m_lfclk_state.cli);
	if (err < 0) {
		return err;
	}

	atomic_inc(&m_lfclk_state.m_clk_refcnt);

	return 0;
}

static int32_t m_lfclk_release(void)
{
	struct onoff_manager *mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_LF);
	int32_t err;

	/* In case there is other ongoing request, cancel it. */
	err = onoff_cancel_or_release(mgr, &m_lfclk_state.cli);
	if (err < 0) {
		return err;
	}

	atomic_dec(&m_lfclk_state.m_clk_refcnt);

	return 0;
}

static void m_hfclk_request(void)
{
	/* The z_nrf_clock_bt_ctlr_hf_request doesn't count references to HFCLK,
	 * it is caller responsibility handle requests and releases counting.
	 */
	if (atomic_inc(&m_hfclk_refcnt) > (atomic_val_t)0) {
		return;
	}

	z_nrf_clock_bt_ctlr_hf_request();
}

static void m_hfclk_release(void)
{
	/* The z_nrf_clock_bt_ctlr_hf_request doesn't count references to HFCLK,
	 * it is caller responsibility to not release the clock if there is
	 * other request pending.
	 */
	if (atomic_get(&m_hfclk_refcnt) < (atomic_val_t)1) {
		LOG_WRN("Mismatch between HFCLK request/release");
		return;
	}

	if (atomic_dec(&m_hfclk_refcnt) > (atomic_val_t)1) {
		return;
	}

	z_nrf_clock_bt_ctlr_hf_release();
}

static bool m_hfclk_is_running(void)
{
	if (atomic_get(&m_hfclk_refcnt) > (atomic_val_t)0) {
		nrf_clock_hfclk_t type;

		unsigned int key = irq_lock();

		(void)nrfx_clock_is_running(NRF_CLOCK_DOMAIN_HFCLK, &type);

		irq_unlock(key);

		return ((type == NRF_CLOCK_HFCLK_HIGH_ACCURACY) ? true : false);
	}

	return false;
}

#else
#error "Unsupported clock control"
#endif /* CONFIG_CLOCK_CONTROL_NRF */

static mpsl_clock_lfclk_ctrl_source_t m_nrf_lfclk_ctrl_data = {
	.lfclk_wait = m_lfclk_wait,
	.lfclk_calibration_start = m_lfclk_calibration_start,
	.lfclk_calibration_is_enabled = m_lfclk_calibration_is_enabled,
	.lfclk_request = m_lfclk_request,
	.lfclk_release = m_lfclk_release,
#if defined(CONFIG_CLOCK_CONTROL_NRF_ACCURACY)
	.accuracy_ppm = CONFIG_CLOCK_CONTROL_NRF_ACCURACY,
#else
	.accuracy_ppm = MPSL_LFCLK_ACCURACY_PPM,
#endif /* CONFIG_CLOCK_CONTROL_NRF_ACCURACY */
	.skip_wait_lfclk_started = IS_ENABLED(CONFIG_SYSTEM_CLOCK_NO_WAIT)
};

static mpsl_clock_hfclk_ctrl_source_t m_nrf_hfclk_ctrl_data = {
	.hfclk_request = m_hfclk_request,
	.hfclk_release = m_hfclk_release,
	.hfclk_is_running = m_hfclk_is_running,
	.startup_time_us = CONFIG_MPSL_HFCLK_LATENCY
};

int32_t mpsl_clock_ctrl_init(void)
{
	return mpsl_clock_ctrl_source_register(&m_nrf_lfclk_ctrl_data, &m_nrf_hfclk_ctrl_data);
}

int32_t mpsl_clock_ctrl_uninit(void)
{
	return mpsl_clock_ctrl_source_unregister();
}
