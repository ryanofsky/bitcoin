// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <fs.h>
#include <interfaces/init.h>
#include <interfaces/ipc.h>
#include <interfaces/wallet.h>
#include <ipc/context.h>
#include <key.h>
#include <logging.h>
#include <pubkey.h>
#include <random.h>
#include <util/memory.h>
#include <util/system.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace interfaces {
class Chain;
} // namespace interfaces

namespace ipc {
namespace capnp {
std::string GlobalArgsNetwork();
} // namespace capnp
} // namespace ipc

namespace init {
namespace {
const char* EXE_NAME = "bitcoin-wallet";

class BitcoinWalletInit : public interfaces::Init
{
public:
    BitcoinWalletInit(int argc, char* argv[]) : m_ipc(interfaces::MakeIpc(argc, argv, EXE_NAME, *this))
    {
        m_ipc->context().init_process = [] {
            // TODO in future PR: Refactor bitcoin startup code, dedup this with AppInitSanityChecks
            RandomInit();
            ECC_Start();
            static ECCVerifyHandle globalVerifyHandle;
            // TODO in future PR: Refactor bitcoin startup code, dedup this with InitSanityCheck
            if (!ECC_InitSanityCheck()) {
                throw std::runtime_error("Elliptic curve cryptography sanity check failure. Aborting.");
            }
            if (!Random_SanityCheck()) {
                throw std::runtime_error("OS cryptographic RNG sanity check failure. Aborting.");
            }
            // TODO in future PR: Refactor bitcoin startup code, dedup this with AppInit.
            SelectParams(ipc::capnp::GlobalArgsNetwork());
            // TODO in future PR: Maybe add AppInitBasicSetup signal handler calls

            // FIXME pasted from InitLogging. This should be fixed by serializing
            // loginstance state and initializing members from parent process, not from
            // reparsing config again here.
            LogInstance().m_print_to_file = !gArgs.IsArgNegated("-debuglogfile");
            LogInstance().m_file_path = AbsPathForConfigVal(gArgs.GetArg("-debuglogfile", DEFAULT_DEBUGLOGFILE));
            if (LogInstance().m_file_path.string() != "/dev/null") {
                LogInstance().m_file_path += ".wallet";
            }
            LogInstance().m_print_to_console =
                gArgs.GetBoolArg("-printtoconsole", !gArgs.GetBoolArg("-daemon", false));
            LogInstance().m_log_timestamps = gArgs.GetBoolArg("-logtimestamps", DEFAULT_LOGTIMESTAMPS);
            LogInstance().m_log_time_micros = gArgs.GetBoolArg("-logtimemicros", DEFAULT_LOGTIMEMICROS);
            LogInstance().m_log_threadnames = gArgs.GetBoolArg("-logthreadnames", DEFAULT_LOGTHREADNAMES);

            // FIXME dedup log category code with AppInitParameterInteraction
            if (gArgs.IsArgSet("-debug")) {
                // Special-case: if -debug=0/-nodebug is set, turn off debugging messages
                const std::vector<std::string> categories = gArgs.GetArgs("-debug");
                if (std::none_of(categories.begin(), categories.end(),
                                 [](std::string cat) { return cat == "0" || cat == "none"; })) {
                    for (const auto& cat : categories) {
                        LogInstance().EnableCategory(cat);
                    }
                }
            }
            for (const std::string& cat : gArgs.GetArgs("-debugexclude")) {
                LogInstance().DisableCategory(cat);
            }

            // TODO in future PR: Refactor bitcoin startup code, dedup this with AppInitMain.
            LogInstance().StartLogging();
        };
    }
    std::unique_ptr<interfaces::WalletClient> makeWalletClient(interfaces::Chain& chain) override
    {
        return MakeWalletClient(chain, gArgs);
    }
    interfaces::Ipc* ipc() override { return m_ipc.get(); }
    std::unique_ptr<interfaces::Ipc> m_ipc;
};
} // namespace
} // namespace init

namespace interfaces {
std::unique_ptr<Init> MakeWalletInit(int argc, char* argv[], int& exit_status)
{
    auto init = MakeUnique<init::BitcoinWalletInit>(argc, argv);
    // Check if bitcoin-wallet is being invoked as an IPC server. If so, then
    // bypass normal execution and just respond to requests over the IPC
    // channel and finally return null.
    if (init->m_ipc->serveProcess(init::EXE_NAME, argc, argv, exit_status)) {
        return nullptr;
    }
    return init;
}
} // namespace interfaces
