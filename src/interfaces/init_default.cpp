// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/init.h>

#include <util/memory.h>

namespace interfaces {
namespace {
class LocalInitImpl : public LocalInit
{
public:
    LocalInitImpl() : LocalInit(/* exe_name= */ nullptr, /* log_suffix= */ nullptr) {}
};
} // namespace
std::unique_ptr<LocalInit> MakeInit(int argc, char* argv[]) { return MakeUnique<LocalInitImpl>(); }
} // namespace interfaces
