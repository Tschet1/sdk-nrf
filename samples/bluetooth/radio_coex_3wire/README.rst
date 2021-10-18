.. _bluetooth_radio_coex_3wire_sample:

Bluetooth: External radio coexistence using three-wire interface
################################################################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to use :ref:`Bluetooth® external radio coexistence <nrfxlib:bluetooth_coex>`.

Overview
********

The application sets up an advertiser, and implements a rudimentary PTA in order to test coexistence.
User enters ``g`` to grant or ``d`` to deny requests from Bluetooth.
Since this is a basic implementation, the decision to grant or deny does not consider priority or direction of the requests.

* When requests from Bluetooth are granted, packets will be transmitted.
* When requests from Bluetooth are denied, no packets will be transmitted.
* Number of packets transmitted is tracked by counting RADIO->READY event.
* The total number of transmitted packets will be printed every second.

The application generates a grant signal on pin APP_GRANT_GPIO_PIN.
This pin has to be connected to the pin defined by the grant-gpios property in the DTS using a jumper.

* On ``nrf52840dk_nrf52840``, these are pins P0.26 and P0.02.

The application senses request signal on pin APP_REQ_GPIO_PIN.
This pin has to be connected to the pin defined by the req-gpios property in the DTS using a jumper.

* On ``nrf52840dk_nrf52840``, these are pins P0.28 and P0.03.

Requirements
************

The sample supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf52840dk_nrf52840

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/radio_coex_3wire`

.. include:: /includes/build_and_run.txt

Testing
=======

1. Connect pin APP_GRANT_GPIO_PIN to pin grant-gpios on your development kit.
#. Connect pin APP_REQ_GPIO_PIN to pin req-gpios on your development kit.
#. Build and program the development kit.
#. |connect_terminal|
#. Watch the number of packets transmitted.
#. Enter ``d`` to deny packet transmission.
#. Enter ``g`` to grant packet transmission.

Dependencies
************

This sample uses the following `sdk-nrfxlib`_ libraries:

* :ref:`nrfxlib:mpsl`
* :ref:`nrfxlib:softdevice_controller`

The sample also uses drivers from `nrfx`_ libraries.

The sample uses the following Zephyr libraries:

* ``include/console/console.h``
* :ref:`zephyr:bluetooth_api`
