.. _pwm-tests:

PWM Test Suite,
###########

Overview
********


This sample demonstrates how to use the PWM driver API.

Depending on the target board, it reads PWM samples
from one or more channels and prints the readings on the console.
If the period and pulse of the PWM used can be obtained,
the desired duty cycle can be output.


Building and Running
********************
This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: npcx-tests/app/pwm
   :host-os: unix
   :board: npcx4m8f_evb/npcxk3m7k_evb
   :goals: run
   :compact:


Sample Output
=============

.. code-block:: console

    pwm set 1 64000 32000 0
    note: PWM, channel 1, period value is 64000, pulse 32000, inverse disable.
