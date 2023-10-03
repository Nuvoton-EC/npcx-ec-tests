.. _DMA-tests:

DMA Test Suite,
###########

Overview
********


This sample demonstrates how to use the DMA driver API.

Verify NPCX4 DMA Function.
Please refer `dma_npcx_api_example` for simplest usage.

* channel_direction - HW support M2M, M2P, P2M
    - block mode: RAM to RAM, RAM to/from SPI flash
    - demand mode: eSPI_SIF, AES, SHA to/from RAM or from SPI flash
* block_size - number of bytes to be transferred
* source_address / dest_address - transfer data source / destination
* source_data_size - transfer data size for source
* dest_data_size - transfer data size for destination
are the minimal requirments to use DMA api.

Note: The data size is limited to one of the following.
1/2/4/16 : 1B/1W/1DW/Burst Mode with DW

Validation item

- transfer data from flash to ram
- transfer data from ram to ram
- power down
- transfer data in async

Building and Running
********************
Build and flash the sample as follows, changing ``npcx4m8f_evb`` for
your board:

.. zephyr-app-commands::
   :zephyr-app: npcx-tests/app/dma
   :host-os: unix
   :board: npcx4m8f_evb
   :goals: run
   :compact:


Sample Output
=============

.. code-block:: console

    # choose which device and channel
    dma init $1 $2
    note: $1: device 0 or 1
          $2: channel 0 or 1
    # api example for other module reference to use dma
    dma c0

    # dma power down test
    dma c1 gpd

    # dma transfer data in parallel with different device and channel test
    # case 1 ~ 4 different device with different channel
    # case 5 ~ 6 same device with different channel
    dma c2 para 1
    dma c2 para 2
    ...
    dma c2 para 6

    # dma transfer data from flash to ram
    dma c3 flash $1 $2 $3
    note: $1: 0 is internal flash, 1 is external flash.
          $2: 0 is GDMAMemPool, 1 is code ram
          $3: 1/2/4/16 : 1B/1W/1DW/Burst Mode with DW

   # dma transfer data from ram to ram
    dma c3 ram $1 $2
    note: $1: 0 is static area to Code ram, 1 is Code ram to static area
          $2: 1/2/4/16 : 1B/1W/1DW/Burst Mode with DW