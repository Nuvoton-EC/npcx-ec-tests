.. _peci-sample:

PECI Interface
####################################

Overview
********

This sample demonstrates how to use the :ref:`PECI API <peci_api>`.
Callbacks are registered that will write to the console indicating PECI events.
These events indicate PECI host interaction.

Building and Running
********************

This application can be built and executed on Nuvoton EVBs as follows:

.. zephyr-app-commands::
   :zephyr-app: npcx-tests/peci
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
