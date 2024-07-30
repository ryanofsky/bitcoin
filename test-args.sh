#!/usr/bin/env bash
set -x
set -e
make -j12 -k -C src test/test_bitcoin && src/test/test_bitcoin -l test_suite -t argsman_tests
make -j12 -k -C src test/test_bitcoin && src/test/test_bitcoin -l test_suite -t getarg_tests
make -j12 -k -C src test/test_bitcoin && src/test/test_bitcoin -l test_suite -t logging_tests
make -j12 -k -C src test/test_bitcoin && src/test/test_bitcoin -l test_suite -t settings_tests
make -j12 -k -C src bitcoind && test/functional/feature_config_args.py
