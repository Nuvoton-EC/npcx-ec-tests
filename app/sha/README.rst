.. <sample>:

Test <Add your description here>.
###########

Overview
********

A simple sample that can be used with any :ref:`supported board <boards>` and
test driver capability via the console.

Building and Running
********************

This application can be built and executed on Nuvoton EVBs as follows:

.. zephyr-app-commands::
   :zephyr-app: npcx-tests/<your folder>
   :host-os: unix/windows
   :board: npcx9m6f_evb/npcx4m8f_evb/npcxk3m7k_evb
   :goals: run
   :compact:

Sample Output
=============

When running the above on a Nuvoton EC, the output of each command may look
like the following.

.. code-block:: console

<add your console test code>
ec:~$ test go
test go go go!!
