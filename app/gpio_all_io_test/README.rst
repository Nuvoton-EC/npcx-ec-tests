.. _gpio_all_io_test-tests:

GPIO all io Test Suite,
###########

Overview
********


This example demonstrates GPIO driver API automated testing.

According to the target board, when the program is executed,
a pin will be set as the trigger pin for starting verification.
Whenever a trigger signal is received, the pin will be tested for each flag in sequence.


Building and Running
********************
This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: npcx-tests/app/gpio_aii_io_test
   :host-os: unix
   :board: npcx4m8f_evb/npcxk3m7k_evb
   :goals: run
   :compact:


Sample Output
=============

.. code-block:: console
    n/a

