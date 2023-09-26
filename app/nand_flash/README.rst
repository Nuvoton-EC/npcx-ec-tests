.. _nand_flash-tests:

NAND FLASH Controller Test Suite,
###########

Overview
********

This sample demonstrates how to use the flash driver API to control NAND flash.

Depending on the board's overlay, it supports various read/write modes.
To make sure read operation can wrok properly in different mode,
the corrsponding dummy cycle and number of address byte must be set correctly
accroding to the NAND flash datasheet.

Please refer the yaml file below for more information:
1. zephyr\dts\bindings\flash_controller\nuvoton,npcx-fiu-nand.yaml
2. zephyr\dts\bindings\flash_controller\nuvoton,npcx-fiu-qspi.yaml

Flash Support List
******************

Winbond:
	W25N01KVxxIR

Building and Running
********************

This application can be built and executed on npcx4m8f_evb board
with Winbond W25N01KVxxIR NAND flash:

.. zephyr-app-commands::
   :zephyr-app: npcx-tests/app/nand_flash
   :host-os: unix/windows
   :board: npcx4m8f_evb
   :goals: run
   :compact: "nuvoton,npcx-fiu-nand"


Sample Output
=============

.. code-block:: console

    /* Command List */
    nand_flash - NAND Flash validation commands
    Subcommands:
    id      :nand_flash id: read flash id
    erase   :nand_flash erase <addr> <block count>: erase flash
    read    :nand_flash read <addr> <size>: read flash
    write   :nand_flash write <addr> <size>: write flash
    rdst    :nand_flash rdst: read flash status registers
    wrst    :nand_flash wrst <sts1> <sts2>: write flash status registers
    rdlut   :nand_flash rdlut: read lookup table status
    active  :nand_flash active <device>: select active device
    list    :nand_flash list: list all flash devices


    [00:00:00.003,000] <inf> flash_npcx_fiu_nand: JEDEC_ID:0xef 0xaa 0x21
    *** Booting Zephyr OS build zephyr-v3.4.0-3939-gb70984e98c0d ***
    [00:00:00.235,000] <inf> flash: Start CI20 Validation Task
    [00:00:00.235,000] <inf> flash: flash device [0]:nand_w25n01kv@0 is ready

    ec:~$ nand_flash id
    Read NAND FLASH ID
    JEDEC ID = 0xef 0xaa 0x21

    ec:~$ nand_flash rdlut
    Get bad block lut
    Is lut init: YES
    All blocks are good !!

    ec:~$ nand_flash rdst
    READ NAND FLASH STATUS 0~3 (00, 0x19, 00)

    ec:~$ nand_flash erase 0 20
    Erase NAND FLASH
    [PASS] Flash erase succeeded!
    [GO]

    ec:~$ nand_flash write 0 131072
    Write NAND FLASH
    Flash write succeeded!
    [GO]

    ec:~$ nand_flash read 0 131072
    Read NAND FLASH
    Flash Address: 0x0, size: 0x20000
    [PASS] Flash read succeeded!
    [GO]








