// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/init.h>
#include <util/memory.h>

namespace interfaces {
std::unique_ptr<Init> MakeWalletInit(int argc, char* argv[], int& exit_status) { return MakeUnique<Init>(); }
} // namespace interfaces
