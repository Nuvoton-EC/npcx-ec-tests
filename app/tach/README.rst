.. _smbm-tests:

Tachometer Sensor Module Driver Test Suite,
###########

Overview
********


This sample demonstrates how to use the Tachometer sensor module driver API.

According to the devicetree's PWM selection, the sample will use Tachometer
sensor module driver API and PWM driver API to drive a FAN device, and then
measuring the RPM value of the FAN device.

Building and Running
********************
This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: npcx-tests/app/tach
   :host-os: unix
   :board: npcx4m8f_evb/npcxk3m7k_evb
   :goals: run
   :compact:


Sample Output
=============

.. code-block:: console

    ec:~$ tach test
    <inf> main: Device_Set : pwm0, Channel : 0
    <dbg> tach_npcx: tach_npcx_is_underflow: port A is underflow 1, port b is underflow 0
    <dbg> tach_npcx: tach_npcx_is_underflow: port A is underflow 0, port b is underflow 0
    <dbg> tach_npcx: tach_npcx_is_captured: port A is captured 1, port b is captured 0
    <inf> main: value = 7710
    <inf> main: [PASS] TACH test Ok
    <inf> main: [GO]



