// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/init.h>

#include <chainparams.h>
#include <interfaces/capnp/ipc.h>
#include <interfaces/chain.h>
#include <interfaces/wallet.h>
#include <key.h>
#include <random.h>
#include <util/system.h>

namespace interfaces {
namespace capnp {
std::string GlobalArgsNetwork();
} // namespace capnp

namespace {
class LocalInitImpl : public LocalInit
{
public:
    LocalInitImpl(int argc, char* argv[]) : LocalInit(/* exe_name */ "bitcoin-wallet", /* log_suffix= */ ".wallet")
    {
        m_protocol = capnp::MakeCapnpProtocol(m_exe_name, *this);
        m_process = MakeIpcProcess(argc, argv, m_exe_name, *m_protocol);
    }
    std::unique_ptr<ChainClient> makeWalletClient(Chain& chain, std::vector<std::string> wallet_filenames) override
    {
        return MakeWalletClient(chain, gArgs, std::move(wallet_filenames));
    }
    void initProcess() override
    {
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
        SelectParams(interfaces::capnp::GlobalArgsNetwork());
        // TODO in future PR: Maybe add AppInitBasicSetup signal handler calls

        // FIXME pasted from InitLogging. This should be fixed by serializing
        // loginstance state and initializing members from parent process, not from
        // reparsing config again here.
        LogInstance().m_print_to_file = !gArgs.IsArgNegated("-debuglogfile");
        LogInstance().m_file_path = AbsPathForConfigVal(gArgs.GetArg("-debuglogfile", DEFAULT_DEBUGLOGFILE));
        if (m_log_suffix && LogInstance().m_file_path.string() != "/dev/null") {
            LogInstance().m_file_path += m_log_suffix;
        }
        LogInstance().m_print_to_console = gArgs.GetBoolArg("-printtoconsole", !gArgs.GetBoolArg("-daemon", false));
        LogInstance().m_log_timestamps = gArgs.GetBoolArg("-logtimestamps", DEFAULT_LOGTIMESTAMPS);
        LogInstance().m_log_time_micros = gArgs.GetBoolArg("-logtimemicros", DEFAULT_LOGTIMEMICROS);
        LogInstance().m_log_threadnames = gArgs.GetBoolArg("-logthreadnames", DEFAULT_LOGTHREADNAMES);

        // FIXME dedup log category code with AppInitParameterInteraction
        if (gArgs.IsArgSet("-debug")) {
            // Special-case: if -debug=0/-nodebug is set, turn off debugging messages
            const std::vector<std::string> categories = gArgs.GetArgs("-debug");

            if (std::none_of(categories.begin(), categories.end(),
                             [](std::string cat){return cat == "0" || cat == "none";})) {
                for (const auto& cat : categories) {
                    LogInstance().EnableCategory(cat);
                }
            }
        }
        for (const std::string& cat : gArgs.GetArgs("-debugexclude")) {
            LogInstance().DisableCategory(cat);
        }

        // TODO in future PR: Refactor bitcoin startup code, dedup this with AppInitMain.
        if (!LogInstance().StartLogging()) {
            throw std::runtime_error(
                strprintf("Could not open wallet debug log file '%s'", LogInstance().m_file_path.string()));
        }
    }
};
} // namespace

std::unique_ptr<LocalInit> MakeInit(int argc, char* argv[]) { return MakeUnique<LocalInitImpl>(argc, argv); }
} // namespace interfaces
