.. _adc-tests:

ADC Test Suite,
###########

Overview
********


This sample demonstrates how to use the ADC driver API.

Depending on the target board, it compares the measured voltage input to either a programmable
threshold or a pair of thresholds from one or more channels and prints the readings on the console.


Building and Running
********************
This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: npcx-tests/app/adc_cmp
   :host-os: unix
   :board: npcx4m8f_evb/npcxk3m7k_evb
   :goals: run
   :compact:


Sample Output
=============

.. code-block:: console

    adc cfg 0 0
    note: ADC channel 0 and a threshold event is generated when the measured voltage is higher than
          the threshold level.
    adc cfg 0 1
    note: ADC channel 0 and a threshold event is generated when the measured voltage is lower than
          or equal to the threshold level.


