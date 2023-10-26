.. _adc-tests:

PSL Test Suite
###########

Overview
********


This sample demonstrates how to use the PSL driver API.



Building and Running
********************
This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: npcx-tests/app/psl
   :host-os: unix
   :board: npcx4m8f_evb
   :goals: run
   :compact:


Sample Output
=============

.. code-block:: console
    psl c1 init
    note: Selects the PSL_INn signal to the corresponding pins.
    psl c1 test
    note: Configure detection settings of PSL_IN and set PSL_OUT to inactive state.

