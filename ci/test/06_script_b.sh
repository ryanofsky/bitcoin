#!/usr/bin/env bash
#
# Copyright (c) 2018-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

if [ -n "$QEMU_USER_CMD" ]; then
  BEGIN_FOLD wrap-qemu
  # Generate all binaries, so that they can be wrapped
  DOCKER_EXEC make $MAKEJOBS -C src/secp256k1 VERBOSE=1
  DOCKER_EXEC make $MAKEJOBS -C src/univalue VERBOSE=1
  DOCKER_EXEC "${BASE_ROOT_DIR}/ci/test/wrap-qemu.sh"
  END_FOLD
fi

if [ -n "$USE_VALGRIND" ]; then
  BEGIN_FOLD wrap-valgrind
  DOCKER_EXEC "${BASE_ROOT_DIR}/ci/test/wrap-valgrind.sh"
  END_FOLD
fi

if [ "$RUN_UNIT_TESTS" = "true" ]; then
  BEGIN_FOLD unit-tests
  bash -c "${CI_WAIT}" &  # Print dots in case the unit tests take a long time to run
  DOCKER_EXEC LD_LIBRARY_PATH=$DEPENDS_DIR/$HOST/lib make $MAKEJOBS check VERBOSE=1
  END_FOLD
fi

if [ "$RUN_FUNCTIONAL_TESTS" = "true" ]; then
  BEGIN_FOLD functional-tests
<<<<<<< HEAD
<<<<<<< HEAD
  DOCKER_EXEC LD_LIBRARY_PATH=$DEPENDS_DIR/$HOST/lib ${TEST_RUNNER_ENV} test/functional/test_runner.py --ci $MAKEJOBS --tmpdirprefix "${BASE_SCRATCH_DIR}/test_runner/" --ansi --combinedlogslen=4000 ${TEST_RUNNER_EXTRA} --quiet --failfast
||||||| merged common ancestors
  DOCKER_EXEC test/functional/test_runner.py --ci $MAKEJOBS --tmpdirprefix "${BASE_SCRATCH_DIR}/test_runner/" --ansi --combinedlogslen=4000 ${TEST_RUNNER_EXTRA} --quiet --failfast
=======
  DOCKER_EXEC LD_LIBRARY_PATH=$TRAVIS_BUILD_DIR/depends/$HOST/lib test/functional/test_runner.py --ci $MAKEJOBS --tmpdirprefix "${BASE_SCRATCH_DIR}/test_runner/" --ansi --combinedlogslen=4000 ${TEST_RUNNER_EXTRA} --quiet --failfast
>>>>>>> Set LD_LIBRARY_PATH consistently in travis tests
||||||| merged common ancestors
  DOCKER_EXEC LD_LIBRARY_PATH=$TRAVIS_BUILD_DIR/depends/$HOST/lib test/functional/test_runner.py --ci $MAKEJOBS --tmpdirprefix "${BASE_SCRATCH_DIR}/test_runner/" --ansi --combinedlogslen=4000 ${TEST_RUNNER_EXTRA} --quiet --failfast
=======
  DOCKER_EXEC LD_LIBRARY_PATH=$TRAVIS_BUILD_DIR/depends/$HOST/lib ${TEST_RUNNER_ENV} test/functional/test_runner.py --ci $MAKEJOBS --tmpdirprefix "${BASE_SCRATCH_DIR}/test_runner/" --ansi --combinedlogslen=4000 ${TEST_RUNNER_EXTRA} --quiet --failfast
>>>>>>> Add multiprocess travis configuration
  END_FOLD
fi

if [ "$RUN_FUZZ_TESTS" = "true" ]; then
  BEGIN_FOLD fuzz-tests
<<<<<<< HEAD
  DOCKER_EXEC LD_LIBRARY_PATH=$DEPENDS_DIR/$HOST/lib test/fuzz/test_runner.py ${FUZZ_TESTS_CONFIG} -l DEBUG ${DIR_FUZZ_IN}
||||||| merged common ancestors
  DOCKER_EXEC test/fuzz/test_runner.py ${FUZZ_TESTS_CONFIG} -l DEBUG ${DIR_FUZZ_IN}
=======
  DOCKER_EXEC LD_LIBRARY_PATH=$TRAVIS_BUILD_DIR/depends/$HOST/lib test/fuzz/test_runner.py ${FUZZ_TESTS_CONFIG} -l DEBUG ${DIR_FUZZ_IN}
>>>>>>> Set LD_LIBRARY_PATH consistently in travis tests
  END_FOLD
fi
