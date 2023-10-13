.. _wdt-tests:

WDT Test Suite,
###########

Overview
********


This sample demonstrates how to use the watchdog driver API.


The test case provides watchdog installation, setup, and waiting for a reset.
A test is done by checking if the state has changed, and the test should be
finished with no WDT callback. Another test is checking if the state has
changed, then check the test value to see if an interrupt occurred. The last
test is checking the wrong WDT window max parameter.


Building and Running
********************
This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: npcx-tests/app/wdt
   :host-os: unix
   :board: npcxk3m7k_evb
   :goals: run
   :compact:


Sample Output
=============

.. code-block:: console

    Running TESTSUITE wdt_basic_test_suite
    ===================================================================
    START - test_wdt
    Testcase: test_wdt_no_callback
    Waiting to restart MCU
    Running TESTSUITE wdt_basic_test_suite
    ===================================================================
    START - test_wdt
    Testcase: test_wdt_no_callback
    Testcase passed
    Testcase: test_wdt_callback_1
    Waiting to restart MCU
    Running TESTSUITE wdt_basic_test_suite
    ===================================================================
    START - test_wdt
    Testcase: test_wdt_callback_1
    Testcase passed
    Testcase: test_wdt_bad_window_max
     PASS - test_wdt in 0.009 seconds
    ===================================================================
    TESTSUITE wdt_basic_test_suite succeeded
    ------ TESTSUITE SUMMARY START ------
    SUITE PASS - 100.00% [wdt_basic_test_suite]: pass = 1, fail = 0, skip = 0, total = 1 duration = 0.009 seconds
     - PASS - [wdt_basic_test_suite.test_wdt] duration = 0.009 seconds
    ------ TESTSUITE SUMMARY END ------
    ===================================================================
    PROJECT EXECUTION SUCCESSFUL


