.. _adc-tests:

ADC Test Suite,
###########

Overview
********


This sample demonstrates how to use the ADC driver API.

Depending on the target board, it reads ADC samples from one or more channels
and prints the readings on the console. If voltage of the used reference can
be obtained, the raw readings are converted to millivolts.


Building and Running
********************
This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: npcx-tests/app/adc
   :host-os: unix
   :board: npcx4m8f_evb/npck3m7k_evb
   :goals: run
   :compact:


Sample Output
=============

.. code-block:: console

    adc c2 1800 1
    note: ADC, channel 1, input value is 1800mv, check the output date
    conversion to mV is match.

