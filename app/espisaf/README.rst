.. <sample>:

eSPI TAF Sample on NPCX4 EVB
##################

Overview
********

This sample demonstrates how to use the eSPI SAF driver API.

Building and Running
********************

Build and flash the sample as follows, changing ``npcx4m8f_evb`` for
your board:

.. zephyr-app-commands::
   :zephyr-app: npcx_tests/app/espisaf
   :host-os: unix/windows
   :board: npcx4m8f_evb
   :goals: run
   :compact:

Sample Output
=============

.. code-block:: console

    # eSPI TAF read stress test
    tag: 0x9 addr: 0x784 len: 23
    queue_number: 1, np_free: 0
    [PASS][SAF]Get Flash Complete  | cycle type: read | tag: 0x9 | length: 0x17 | addr: 0x784 |
    tag: 0xe addr: 0x649 len: 60
    queue_number: 1, np_free: 0
    [PASS][SAF]Get Flash Complete  | cycle type: read | tag: 0xe | length: 0x3c | addr: 0x649 |
    tag: 0x9 addr: 0xea6 len: 8
    queue_number: 1, np_free: 0
    [PASS][SAF]Get Flash Complete  | cycle type: read | tag: 0x9 | length: 0x08 | addr: 0xea6 |

    # eSPI TAF erase test
    queue_number: 1, np_free: 0
    [PASS][SAF]Get Flash Complete  | cycle type: erase | tag: 0x0 | length: 0x03 | addr: 0x0 |

    # eSPI TAF write test
    queue_number: 1, np_free: 0
    [PASS][SAF]Get Flash Complete  | cycle type: write | tag: 0x1 | length: 0x40 | addr: 0x0 |

    # eSPI TAF read test
    queue_number: 1, np_free: 0
    [PASS][SAF]Get Flash Complete  | cycle type: read | tag: 0x2 | length: 0x40 | addr: 0x0 |
    DATA:
    0000 | 02 05 08 0B 0E 11 14 17 1A 1D 20 23 26 29 2C 2F
    0010 | 32 35 38 3B 3E 41 44 47 4A 4D 50 53 56 59 5C 5F
    0020 | 62 65 68 6B 6E 71 74 77 7A 7D 80 83 86 89 8C 8F
    0030 | 92 95 98 9B 9E A1 A4 A7 AA AD B0 B3 B6 B9 BC BF
    [GO]