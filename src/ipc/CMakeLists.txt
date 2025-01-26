# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

# Build local libmultiprocess/ directory, unless an external libmultiprocess
# package is being used instead.
if (NOT WITH_LIBMULTIPROCESS)
  add_subdirectory(libmultiprocess EXCLUDE_FROM_ALL)
  # Apply core_interface compile options to libmultiprocess runtime library.
  target_link_libraries(multiprocess PUBLIC $<BUILD_INTERFACE:core_interface>)
  target_link_libraries(mputil PUBLIC $<BUILD_INTERFACE:core_interface>)
  # Share MP_INCLUDE_DIR variable with parent scope so target_capnp_sources
  # build rules can find the libmultiprocess include directory and avoid
  # "error: Import failed: /mp/proxy.capnp" errors from capnp.
  set(MP_INCLUDE_DIR "${MP_INCLUDE_DIR}" PARENT_SCOPE)
endif()

add_library(bitcoin_ipc STATIC EXCLUDE_FROM_ALL
  capnp/mining.cpp
  capnp/protocol.cpp
  interfaces.cpp
  process.cpp
)

target_capnp_sources(bitcoin_ipc ${PROJECT_SOURCE_DIR}
  capnp/common.capnp
  capnp/echo.capnp
  capnp/init.capnp
  capnp/mining.capnp
)

target_link_libraries(bitcoin_ipc
  PRIVATE
    core_interface
    univalue
)
