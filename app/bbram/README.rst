.. bbram-tests:

BBRAM Test Suite,

####################################

Overview
********

This sample demonstrates how to use the :ref:`BBRAM API <bbram_api>`.
Callbacks are registered that will write to the console indicating BBRAM events.
These events indicate BBRAM host interaction. Please refer to [ITBT]
(http://wehqjira01:8090/display/NTCCI/BBRAM+Driver) for more detail.

Building and Running
********************

This application can be built and executed on Nuvoton EVBs as follows:

.. zephyr-app-commands::
   :zephyr-app: npcx-tests/bbram
   :host-os: unix/windows
   :board: npcx9m6f_evb/npcx4m8f_evb/npcxk3m7k_evb
   :goals: run
   :compact:

Sample output
=============

.. code-block:: console

   PECI test
   Note: You are expected to see several interactions including ID and
   temperature retrieval.
