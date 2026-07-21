#!/usr/bin/env bash
#
# Copyright (c) 2025-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_win64
export CI_IMAGE_NAME_TAG="mirror.gcr.io/debian:trixie"  # Check that https://packages.debian.org/trixie/g++-mingw-w64-ucrt64 can cross-compile
export HOST=x86_64-w64-mingw32ucrt
export PACKAGES="g++-mingw-w64-ucrt64"  # TEMP: dropped nsis (only needed for deploy/installer target)
export RUN_UNIT_TESTS=false
export RUN_FUNCTIONAL_TESTS=false
export GOAL="install"  # TEMP: was "deploy" (NSIS installer) but deploy target requires GUI; use install to just build binaries
export DEP_OPTS="NO_QT=1 NO_WALLET=1"  # TEMP: skip Qt/wallet in depends build to speed up Windows IPC debugging
export BITCOIN_CONFIG="\
  --preset=dev-mode \
  -DWITH_USDT=OFF \
  -DREDUCE_EXPORTS=ON \
  -DCMAKE_CXX_FLAGS='-Wno-error=maybe-uninitialized' \
  -DENABLE_WALLET=OFF \
  -DBUILD_GUI=OFF \
"
