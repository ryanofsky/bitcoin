#!/usr/bin/env bash

export LC_ALL=C

set -x

if [ "$C" = 1 ]; then
    sudo chown -R russ:russ .
    rm -rf qa-assets
    git clean -dfx
    exit
fi

MAKEJOBS=-j3
RUN_UNIT_TESTS=true
RUN_FUNCTIONAL_TESTS=true
RUN_FUZZ_TESTS=false
DOCKER_NAME_TAG=ubuntu:18.04
BOOST_TEST_RANDOM=1$TRAVIS_BUILD_ID
CCACHE_SIZE=100M
CCACHE_TEMPDIR=/tmp/.ccache-temp
CCACHE_COMPRESS=1
CCACHE_DIR=$HOME/.ccache
BASE_OUTDIR=$TRAVIS_BUILD_DIR/out
SDK_URL=https://bitcoincore.org/depends-sources/sdks
WINEDEBUG=fixme-all
DOCKER_PACKAGES="build-essential libtool autotools-dev automake pkg-config bsdmainutils curl git ca-certificates ccache"
CACHE_ERR_MSG="Error! Initial build successful, but not enough time remains to run later build stages and tests. Please manually re-run this job by using the travis restart button or asking a bitcoin maintainer to restart. The next run should not time out because the build cache has been saved."
HOST=x86_64-unknown-linux-gnu
PACKAGES="cmake python3"
DEP_OPTS="MULTIPROCESS=1"
GOAL="install"
BITCOIN_CONFIG=""
TEST_RUNNER_ENV="BITCOIND=bitcoin-node"
TRAVIS_BUILD_DIR=$PWD
travis_retry() { "$@"; }
. .travis/test_03_before_install.sh &&
. .travis/test_04_install.sh &&
. .travis/test_05_before_script.sh &&
. .travis/test_06_script_a.sh &&
. .travis/test_06_script_b.sh
