#!/usr/bin/env bash

#export LC_ALL=C

set -x
set -e

#prefix="--prefix=$PWD/depends/x86_64-pc-linux-gnu"
cf="-ftemplate-backtrace-limit=0"
cc=gcc
cxx=g++
opts=
#cc=clang
#cxx=clang++
#cf="-Winconsistent-missing-override -Wdelete-non-abstract-non-virtual-dtor"
lf=""
#cf="$cf -fsanitize=address -fno-omit-frame-pointer"
#lf="$lf -fsanitize=address"
#opts="--with-sanitizers=address"

if test -n "$R"; then
./autogen.sh
PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$HOME/work/mp/build/prefix/lib64/pkgconfig" \
./configure CC="$cc" CXX="$cxx" CXXFLAGS="$cf" LDFLAGS="$lf" $prefix --enable-multiprocess=yes --enable-debug --enable-werror --enable-external-signer --with-boost-libdir=$BOOST_LIBDIR --with-qt-bindir=$QTBASE_BIN:$QTTOOLS_BIN  --enable-experimental-util-chainstate --enable-experimental-util-chainstate $opts
fi
OPT=()
OPT+=(-ex "dir /home/russ/Downloads/tmp/db5.3-5.3.28/debian")
#OPT+=(-ex "b fnop")
#OPT+=(-ex "b interfaces::capnp::LoggingErrorHandler::taskFailed")
#OPT+=(-ex "b interfaces::capnp::BreakPoint")
#OPT+=(-ex "b capnp::(anonymous namespace)::newNullCap()")
#OPT+=(-ex "set follow-fork-mode child")
OPT+=(-ex "catch throw")
kill -9 $(pgrep -f bitcoin-node) $(pgrep -f bitcoin-wallet) $(pgrep -f bitcoin-gui) $(pgrep -f gdb) || true
stty sane
time make -j12 -k -C src bitcoin-node bitcoin-gui bitcoin-wallet qt/bitcoin-qt bitcoin-cli
#exit 0

#export ASAN_OPTIONS=abort_on_error=1
#export STOP="node wallet"
#export STOP=wallet
export BITCOIN_NODE=$PWD/src/bitcoin-node

sudo sysctl kernel.core_pattern=core
ulimit -c

#src/bitcoin-node -regtest -printtoconsole -debug
#src/bitcoin-gui -regtest -printtoconsole -debug

echo src/bitcoin-gui -regtest -printtoconsole -debug -ipcconnect=unix
echo src/bitcoin-wallet -regtest -ipcconnect=unix info
#STOP=node gdb -ex run --args src/bitcoin-gui -regtest -printtoconsole -debug -ipcconnect=auto
#gdb -ex run --args src/bitcoin-node -regtest -printtoconsole -debug -ipcbind=unix
#exit 0

#gdb "${OPT[@]}" -ex run --args src/bitcoin-node -regtest -printtoconsole -debug
#gdb "${OPT[@]}" -ex run --args src/bitcoin-gui -regtest -printtoconsole -debug
#gdb "${OPT[@]}" -ex run --args src/qt/bitcoin-qt -regtest -printtoconsole -debug

#NOTIME=1 BITCOIND=$PWD/bitcoin-gdb-screen test/functional/wallet_hd.py
#NOTIME=1 BITCOIND=$PWD/bitcoin-gdb-screen test/functional/wallet_labels.py
#NOTIME=1 BITCOIND=$PWD/bitcoin-gdb-screen test/functional/wallet_keypool.py
#NOTIME=1 BITCOIND=$PWD/bitcoin-gdb-screen test/functional/wallet_listtransactions.py --nocleanup
#NOTIME=1 BITCOIND=$PWD/bitcoin-gdb-screen test/functional/mempool_limit.py
#BITCOIND=$PWD/src/bitcoin-node strace -o strace -ff test/functional/mempool_limit.py
#BITCOIND=$PWD/src/bitcoin-node test/functional/tool_wallet.py --nocleanup
#BITCOIND=$PWD/src/bitcoin-node test/functional/test_runner.py --nocleanup test/functional/tool_wallet.py
#BITCOIND=$PWD/src/bitcoin-node test/functional/wallet_hd.py --nocleanup
#BITCOIND=$PWD/src/bitcoin-node test/functional/wallet_labels.py --nocleanup
#BITCOIND=$PWD/src/bitcoin-node test/functional/interface_bitcoin_cli.py --nocleanup
#BITCOIND=$PWD/src/bitcoin-node test/functional/rpc_rawtransaction.py --nocleanup
#BITCOIND=$PWD/src/bitcoin-node test/functional/rpc_signrawtransaction.py --nocleanup
#BITCOIND=$PWD/src/bitcoin-node test/functional/wallet_resendwallettransactions.py
#BITCOIND=$PWD/src/bitcoin-node test/functional/feature_logging.py
#BITCOIND=$PWD/src/bitcoin-node test/functional/feature_block.py --nocleanup
#BITCOIND=$PWD/src/bitcoin-node test/functional/wallet_groups.py --legacy-wallet
#BITCOIND=$PWD/src/bitcoin-node test/functional/wallet_resendwallettransactions.py --descriptors
#BITCOIND=$PWD/src/bitcoin-node test/functional/wallet_resendwallettransactions.py --legacy-wallet
#BITCOIND=$PWD/src/bitcoin-node test/functional/rpc_fundrawtransaction.py
#BITCOIND=$PWD/src/bitcoin-node test/functional/wallet_multiwallet.py
#BITCOIND=$PWD/src/bitcoin-node test/functional/wallet_backup.py --legacy-wallet
#BITCOIND=$PWD/src/bitcoin-node test/functional/wallet_fast_rescan.py --descriptors
# expected to fail unless contrib/signer/miner binary is built
#BITCOIND=$PWD/src/bitcoin-node test/functional/tool_signet_miner.py --descriptors
#BITCOIND=$PWD/src/bitcoin-node test/functional/feature_config_args.py
#BITCOIND=$PWD/src/bitcoin-node test/functional/wallet_importdescriptors.py --descriptors
#BITCOIND=$PWD/src/bitcoin-node test/functional/wallet_transactiontime_rescan.py --legacy-wallet
#BITCOIND=$PWD/src/bitcoin-node test/functional/tool_wallet.py --legacy-wallet --bdbro
BITCOIND=$PWD/src/bitcoin-node test/functional/test_runner.py
#rm -rf test/cache && BITCOIND=$PWD/bitcoin-gdb-screen test/functional/create_cache.py --cachedir=test/cache --configfile=test/config.ini

#test/functional/combine_logs.py $(ls -1trd $TMPDIR/test* | tail -n1) -c | less -r

# git ls-files src/ipc/capnp/*.{h,cpp} | xargs clang-format -i

#gdb -ex 'b std::runtime_error::runtime_error' -ex run --args src/bitcoin-gui -regtest -printtoconsole -debug -ipcbind=unix

make -j12 -k -C src test/test_bitcoin && src/test/test_bitcoin -l test_suite -t ipc_tests
