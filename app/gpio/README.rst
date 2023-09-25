.. _gpio-tests:

GPIO Test Suite,
###########

Overview
********


This sample demonstrates how to use the GPIO driver API.

According to the target board, you can set the gpio group pin and flag to determine
input/output/push-pull/open-drain. In addition, you can also set the pin interrupt
related to the miwu table.


Building and Running
********************
This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: npcx-tests/app/gpio
   :host-os: unix
   :board: npcx4m8f_evb/npcxk3m7k_evb
   :goals: run
   :compact:


Sample Output
=============

.. code-block:: console

    gpio -o 0 1 -oh
    note: GPIO, group 0, pin 1, direction output, flag output high,
    setting gpio01 is output high

