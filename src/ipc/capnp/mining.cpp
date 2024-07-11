// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <ipc/capnp/mining-types.h>

namespace mp {
void CustomReadMessage(InvokeContext& invoke_context,
                       const ipc::capnp::messages::BlockTemplate::Reader& reader,
                       std::unique_ptr<node::CBlockTemplate>& dest)
{
    // FIXME
}

void CustomBuildMessage(InvokeContext& invoke_context,
                        const std::unique_ptr<node::CBlockTemplate>& block,
                        ipc::capnp::messages::BlockTemplate::Builder&& builder)
{
    // FIXME
}

void CustomReadMessage(InvokeContext& invoke_context,
                       const ipc::capnp::messages::BlockValidationState::Reader& reader,
                       BlockValidationState& dest)
{
    // FIXME
}

void CustomBuildMessage(InvokeContext& invoke_context,
                        const BlockValidationState& state,
                        ipc::capnp::messages::BlockValidationState::Builder&& builder)
{
    // FIXME
}
} // namespace mp
