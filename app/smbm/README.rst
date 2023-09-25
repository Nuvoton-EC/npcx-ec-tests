.. _smbm-tests:

SMB Controller Test Suite,
###########

Overview
********


This sample demonstrates how to use the SMB controller driver API.

Depending on the board's overlay, it supports a different number of multiple
SMB ports. According to the devicetree's port selection, the sample shows the
transaction between controller and device with a dedicated SMB port. And this
is the case of using the SMB controller driver API.


Building and Running
********************
This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: npcx-tests/app/smbm
   :host-os: unix
   :board: npcx4m8f_evb/npcxk3m7k_evb
   :goals: run
   :compact:


Sample Output
=============

.. code-block:: console

    <inf> main: smb device io_i2c_ctrl4_porta is ready
    ec:~$ smb cmd1 1
    <inf> main: Handle CMD1 with 1
    <inf> main:  -    SIM TMP100: temp reg 00: 801a, temp is 26 deg

    <inf> main: [GO]

    smb cmd1 2
    <inf> main: Handle CMD1 with 2
    <inf> main:  -    SIM TMP100: conf reg 01: 80

    <inf> main:  - 2: verify i2c_write_read passed

    <inf> main: [GO]

    ec:~$ smb cmd1 3
    <inf> main: Handle CMD1 with 3
    <inf> main: [GO]

    ec:~$ smb cmd1 4
    <inf> main: Handle CMD1 with 4
    <inf> main:  -    SIM TMP100: conf reg 01: e0

    <inf> main: [GO]

    ec:~$ smb cmd1 5
    <inf> main: Handle CMD1 with 5
    <inf> main:  - 3: verify i2c_write passed

    <inf> main: [GO]

    ec:~$ smb cmd1 6
    <inf> main: Handle CMD1 with 6
    <inf> main: [GO]

    ec:~$ smb cmd1 7
    <inf> main: Handle CMD1 with 7
    <inf> main:  -    SIM TMP100: conf reg 01: 80

    <inf> main:  - 4: verify i2c_read passed

    <inf> main: [GO]

    ec:~$ smb cmd1 8
    <inf> main: Handle CMD1 with 8
    <inf> main:  -    SIM TMP100: T-hi reg 03: 5000, t-hi is 80 deg

    <inf> main: [GO]

    ec:~$ smb cmd1 9
    <inf> main: Handle CMD1 with 9
    <inf> main:  -    SIM TMP100: T-lo reg 02: 4b00, t-hi is 75 deg

    <inf> main: [GO]

    ec:~$ smb cmd1 10
    <inf> main: Handle CMD1 with 10
    <inf> main: [GO]

    ec:~$ smb cmd1 11
    <inf> main: Handle CMD1 with 11
    <inf> main:  -    T-hi reg : 51 80

    <inf> main:  - 5: verify i2c_read suspend passed

    <inf> main: [GO]

    ec:~$ smb cmd1 12
    <inf> main: Handle CMD1 with 12
    <inf> main: [GO]

    ec:~$ smb cmd1 13
    <inf> main: Handle CMD1 with 13
    <inf> main:  -    SIM TMP100: T-hi reg: 5000, t-hi is 80 deg

    <inf> main:  - 6: verify i2c_write suspend passed

    <inf> main: [GO]



