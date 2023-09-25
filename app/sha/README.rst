.. <sample>:

SHA Test Sample
###########

Overview
********

A simple sample that can be used with any :ref:`supported board <boards>` and
test driver capability via the console.

Building and Running
********************

This application can be built and executed on Nuvoton EVBs as follows:

.. zephyr-app-commands::
   :zephyr-app: npcx-tests/sha
   :host-os: unix/windows
   :board: npcx4m8f_evb
   :goals: run
   :compact:

Sample Output
=============

When running the above on a Nuvoton EC, the output may look like the following.

.. code-block:: console

    # CRYPTO_HASH_ALGO_SHA256
    ec:~$ sha set_alg 2
    Using SHA Alg CRYPTO_HASH_ALGO_SHA256
    ec:~$ sha sha_test 7
    Start SHA test...
    SHA alg: CRYPTO_HASH_ALGO_SHA256, index: 0
    SHA alg: CRYPTO_HASH_ALGO_SHA256, index: 1
    SHA alg: CRYPTO_HASH_ALGO_SHA256, index: 2
    SHA alg: CRYPTO_HASH_ALGO_SHA256, index: 3
    SHA alg: CRYPTO_HASH_ALGO_SHA256, index: 4
    SHA alg: CRYPTO_HASH_ALGO_SHA256, index: 5
    SHA alg: CRYPTO_HASH_ALGO_SHA256, index: 6
    SHA alg: CRYPTO_HASH_ALGO_SHA256, index: 7
    [PASS] SHA test completion
    [GO]
    # CRYPTO_HASH_ALGO_SHA384
    ec:~$ sha set_alg 3
    Using SHA Alg CRYPTO_HASH_ALGO_SHA384
    ec:~$ sha sha_test 7
    Start SHA test...
    SHA alg: CRYPTO_HASH_ALGO_SHA384, index: 0
    SHA alg: CRYPTO_HASH_ALGO_SHA384, index: 1
    SHA alg: CRYPTO_HASH_ALGO_SHA384, index: 2
    SHA alg: CRYPTO_HASH_ALGO_SHA384, index: 3
    SHA alg: CRYPTO_HASH_ALGO_SHA384, index: 4
    SHA alg: CRYPTO_HASH_ALGO_SHA384, index: 5
    SHA alg: CRYPTO_HASH_ALGO_SHA384, index: 6
    SHA alg: CRYPTO_HASH_ALGO_SHA384, index: 7
    [PASS] SHA test completion
    [GO]
    # CRYPTO_HASH_ALGO_SHA512
    ec:~$ sha set_alg 4
    Using SHA Alg CRYPTO_HASH_ALGO_SHA512
    ec:~$ sha sha_test 7
    Start SHA test...
    SHA alg: CRYPTO_HASH_ALGO_SHA512, index: 0
    SHA alg: CRYPTO_HASH_ALGO_SHA512, index: 1
    SHA alg: CRYPTO_HASH_ALGO_SHA512, index: 2
    SHA alg: CRYPTO_HASH_ALGO_SHA512, index: 3
    SHA alg: CRYPTO_HASH_ALGO_SHA512, index: 4
    SHA alg: CRYPTO_HASH_ALGO_SHA512, index: 5
    SHA alg: CRYPTO_HASH_ALGO_SHA512, index: 6
    SHA alg: CRYPTO_HASH_ALGO_SHA512, index: 7
    [PASS] SHA test completion
    [GO]
