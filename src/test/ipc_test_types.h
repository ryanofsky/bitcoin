// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_IPC_TEST_TYPES_H
#define BITCOIN_TEST_IPC_TEST_TYPES_H

#include <ipc/capnp/common-types.h>
#include <ipc/capnp/mining-types.h>
#include <test/ipc_test.capnp.h>

namespace mp {
// Custom serialization for CustomStruct.
void CustomBuildMessage(InvokeContext& invoke_context,
                        const CustomStruct& src,
                        gen::CustomStruct::Builder&& builder);
void CustomReadMessage(InvokeContext& invoke_context,
                       const gen::CustomStruct::Reader& reader,
                       CustomStruct& dest);
} // namespace mp

#endif // BITCOIN_TEST_IPC_TEST_TYPES_H
