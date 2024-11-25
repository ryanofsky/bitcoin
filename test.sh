#!/usr/bin/env bash

set -x
set -e

if [ ! -e build ]; then
  CMAKE_PREFIX_PATH=$HOME/work/mp/build/prefix:$CMAKE_PREFIX_PATH cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_BENCH=ON -DBUILD_FUZZ_BINARY=ON -DBUILD_GUI=ON -DBUILD_KERNEL_LIB=ON -DBUILD_UTIL_CHAINSTATE=ON -DWERROR=ON -DWITH_BDB=ON -DWITH_MINIUPNPC=ON -DWITH_ZMQ=ON -DWITH_MULTIPROCESS=ON
fi
make -j12 -k -C build bitcoin_ipc bitcoin_ipc_test
make -j12 -k -C build
cmake --build build --target test_bitcoin && src/test/test_bitcoin -l test_suite -t ipc_tests

#BITCOIND=$PWD/build/src/bitcoin-node build/test/functional/test_runner.py
BITCOIN_CMD="$PWD/build/src/bitcoin -m" build/test/functional/feature_config_args.py
BITCOIN_CMD="$PWD/build/src/bitcoin -m" build/test/functional/tool_signet_miner.py
BITCOIN_CMD="$PWD/build/src/bitcoin -m" build/test/functional/tool_wallet.py
build/test/functional/interface_ipc_mining.py
