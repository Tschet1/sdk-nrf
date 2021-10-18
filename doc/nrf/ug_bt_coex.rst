.. _ug_bt_coex:

Using Bluetooth external radio coexistence
##########################################

.. contents::
   :local:
   :depth: 2

This guide describes how to add support for Bluetooth® coexistence to your application in |NCS|.

.. _ug_bt_coex_overview:

Overview
********

The coexistence feature can be used to reduce radio interference when multiple devices are located close to each other.
This feature puts the Bluetooth stack under the control of a Packet Traffic Arbitrator (PTA) through a three-wire interface.
The feature can only be used with the SoftDevice Controller, and only on nRF52 Series devices.

The implementation is based on :ref:`nrfxlib:bluetooth_coex` which is integrated into nrfxlib's MPSL library.

.. _ug_bt_coex_requirements:

Enabling coexistence and MPSL
*****************************

Make sure that the following Kconfig options are enabled:

   * :kconfig:`CONFIG_MPSL_CX`
   * :kconfig:`CONFIG_MPSL_CX_BT`

.. _ug_bt_coex_config:

Configuring coexistence
***********************

Configuration is set using the devicetree (DTS).
The MPSL coexistence sample :file:`samples/bluetooth/radio_coex_3wire` provides an example of adding the configuration as a devicetree overlay.
For more information about devicetree overlays, see :ref:`zephyr:use-dt-overlays`.

A sample configuration is provided below:

   .. code-block::

      / {
        radio_coex: radio_coex_three_wire {
          status = "okay";
          compatible = "sdc-radio-coex-three-wire";
          req-gpios =     <&gpio0 3 GPIO_ACTIVE_HIGH>; /* P0.03 */
          pri-dir-gpios = <&gpio0 4 GPIO_ACTIVE_HIGH>; /* P0.04 */
          grant-gpios =   <&gpio0 2 GPIO_ACTIVE_HIGH>; /* P0.02 */
          type-delay-us = <8>;
          radio-delay-us = <5>;
          is-rx-active-level = <0>;
          };
      };

Each element can be configured accordingly.
Description of the elements:

   * ``req-gpios``: REQUEST pin - GPIO port number, pin number, and active level.
   * ``pri-dir-gpios``: PRIORITY pin - GPIO port number, pin number, and active level.
   * ``grant-gpios``: GRANT pin - GPIO port number, pin number, and active level.
   * ``type-delay-us``: delay in microseconds between the REQUEST pin being raised and the PRIORITY pin indicating the type of transaction (RX/TX).
   * ``radio-delay-us``: delay in microseconds between the PRIORITY pin indicating the type of transaction and the radio starting on-air activity.
   * ``is-rx-active-level``: if set to 1, RX direction is indicated by asserting the PRIORITY pin at the same level used for indicating a HIGH priority request.

.. note::
   The sum of type_delay_us and radio_delay_us cannot be more than 40 us due to radio ramp-up time.

.. _ug_bt_coex_sample:

Sample application
******************

A sample application can be found at :file:`samples/bluetooth/radio_coex_3wire`.
