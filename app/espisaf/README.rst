.. _board_npck_evb:

UART Shell Sample on NPCK EVB
##################

Overview
********

This sample is the first application used for testing NPCX drivers on Zephyr

Building and Running
********************

Build and flash the sample as follows, changing ``npck3m7f_evb`` for
your board:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/ci20_testbench
   :board: npck3m7f_evb
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

    >Task1 shell_fprintf 11111111111111111111111111111111111111<n
    >Task2 printk 22222222222222222222----deadlock---------<
