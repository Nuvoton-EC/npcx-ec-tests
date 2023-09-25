.. _DMA-tests:

DMA Test Suite,
###########

Overview
********


This sample demonstrates how to use the DMA driver API.

Verify NPCX4 DMA Function.
Please refer `dma_npcx_api_example` for simplest usage.
- channel_direction - HW support M2M, M2P, P2M
	block mode: RAM to RAM, RAM to/from SPI flash
	demand mode:
	eSPI_SIF, AES, SHA to/from RAM or from SPI flash
- block_size - number of bytes to be transferred
- source_address
- dest_address
- source_data_size - transfer data size for source
	1/2/4/16 : 1B/1W/1DW/Burst Mode with DW
- dest_data_size - transfer data size for dest
are the minimal requirments to use DMA api.

Validation item
- read flash
- move data from ram to ram
- power down
- data transfer in parallel

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
    # api example for other module reference to use dma
    dma c0

    # dma power down test
    dma c1 gpd

    # dma data transfer in parallel with different device and channel test
    # case 1 ~ 4 different device with different channel
    # case 5 ~ 6 same device with different channel
    dma c2 para 1
    dma c2 para 2
    ...
    dma c2 para 6

    # dma read data from flash and transfer to ram
    dma c3 read 0 0
    note: 1st parameter, 0 is internal flash, 1 is external flash.
          2nd parameter, 0 is GDMAMemPool, 1 is code ram

   # dma transfer data from ram to ram within device 0
    dma c3 ram 0 0
    note: 1st parameter, 0 MRAM to GDMApool, 1 is GDMApool to MRAM
          2nd parameter, 0 is channel one, 1 is channel two.