// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <clientversion.h>
#include <util/fs.h>
#include <util/strencodings.h>
#include <util/translation.h>

#include <iostream>
#include <optional>
#include <string>
#include <tinyformat.h>
#include <vector>

#include <unistd.h>

const TranslateFn G_TRANSLATION_FUN{nullptr};

static constexpr auto HELP_USAGE = R"(Usage: %1$s [OPTIONS] COMMAND...

Commands (run help command for more information):
  {gui,daemon,rpc,wallet,test,help}

Options:
  -m, --multiprocess     Run multiprocess binaries bitcoin-node, bitcoin-gui.\n"
  -M, --monolithic       Run monolithic binaries bitcoind, bitcoin-qt. (Default behavior)\n"
  -v, --version          Show version information
  -h, --help             Show this help message

Command overview:
  %1$s gui [ARGS]     Start GUI, equivalent to running 'bitcoin-qt [ARGS]' or 'bitcoin-gui [ARGS]'.
  %1$s daemon [ARGS]  Start daemon, equivalent to running 'bitcoind [ARGS]' or 'bitcoin-node [ARGS]'.
  %1$s rpc [ARGS]     Call RPC method, equivalent to running 'bitcoin-cli -named [ARGS]'.
  %1$s wallet [ARGS]  Call wallet command, equivalent to running 'bitcoin-wallet [ARGS]'.
  %1$s tx [ARGS]      Manipulate hex-encoded transactions, equivalent to running 'bitcoin-tx [ARGS]'.
  %1$s help           Show this help message.
)";

struct CommandLine {
    bool use_multiprocess{false};
    bool show_version{false};
    bool show_help{false};
    std::string_view command;
    std::vector<const char*> args;
};

CommandLine ParseCommandLine(int argc, char* argv[]);
void ExecCommand(const std::vector<const char*>& args, std::string_view argv0);

int main(int argc, char* argv[])
{
    try {
        CommandLine cmd{ParseCommandLine(argc, argv)};
        if (cmd.show_version) {
            tfm::format(std::cout, "%s version %s\n%s", CLIENT_NAME, FormatFullVersion(), FormatParagraph(LicenseInfo()));
            return EXIT_SUCCESS;
        }

        std::vector<const char*> args;
        if (cmd.show_help || cmd.command == "help" || cmd.command.empty()) {
            tfm::format(std::cout, HELP_USAGE, argv[0]);
            return cmd.command.empty() ? EXIT_FAILURE : EXIT_SUCCESS;
        } else if (cmd.command == "gui") {
            args.emplace_back(cmd.use_multiprocess ? "qt/bitcoin-gui" : "qt/bitcoin-qt");
        } else if (cmd.command == "daemon") {
            args.emplace_back(cmd.use_multiprocess ? "bitcoin-node" : "bitcoind");
        } else if (cmd.command == "rpc") {
            args.emplace_back("bitcoin-cli");
            args.emplace_back("-named");
        } else if (cmd.command == "wallet") {
            args.emplace_back("bitcoin-wallet");
        } else if (cmd.command == "tx") {
            args.emplace_back("bitcoin-tx");
        } else if (cmd.command == "test") { // undocumented internal command
            args.emplace_back("test/test_bitcoin");
        } else if (cmd.command == "mine") { // undocumented, used by tests
            args.emplace_back("bitcoin-mine");
        } else if (cmd.command == "util") { // undocumented, used by tests
            args.emplace_back("bitcoin-util");
        } else {
            throw std::runtime_error(strprintf("Unrecognized command: '%s'", cmd.command));
        }
        if (!args.empty()) {
            args.insert(args.end(), cmd.args.begin(), cmd.args.end());
            ExecCommand(args, argv[0]);
        }
    } catch (const std::exception& e) {
        tfm::format(std::cerr, "Error: %s\nTry '%s --help' for more information.\n", e.what(), argv[0]);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

CommandLine ParseCommandLine(int argc, char* argv[])
{
    CommandLine cmd;
    cmd.args.reserve(argc);
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (!cmd.command.empty()) {
            cmd.args.emplace_back(argv[i]);
        } else if (arg == "-m" || arg == "--multiprocess") {
            cmd.use_multiprocess = true;
        } else if (arg == "-M" || arg == "--monolithic") {
            cmd.use_multiprocess = false;
        } else if (arg == "-v" || arg == "--version") {
            cmd.show_version = true;
        } else if (arg == "-h" || arg == "--help") {
            cmd.show_help = true;
        } else if (arg.starts_with("-")) {
            throw std::runtime_error(strprintf("Unknown option: %s", arg));
        } else if (!arg.empty()) {
            cmd.command = arg;
        }
    }
    return cmd;
}

//! Execute the specified bitcoind, bitcoin-qt or other command line in `args`
//! using src, bin and libexec directory paths relative to this executable, where
//! the path to this executable is specified in `wrapper_argv0`.
//!
//! @param args Command line arguments to execute, where first argument should
//!             be a relative path to a bitcoind, bitcoin-qt or other executable
//!             that will be located on the PATH or relative to this executable.
//!
//! @param wrapper_argv0 String containing first command line argument passed to
//!                      main() to run the current executable. This is used to
//!                      help determine the path to the current executable and
//!                      how to look for new executables.
//
//! @note This function doesn't currently print anything but can be debugged
//! from the command line using strace or dtrace like:
//!
//!     strace -e trace=execve -s 10000 build/src/bitcoin ...
//!     dtrace -n 'proc:::exec-success { trace(curpsinfo->pr_psargs); }' -c ...
void ExecCommand(const std::vector<const char*>& args, std::string_view wrapper_argv0)
{
    // Construct argument string for execvp
    std::vector<const char*> exec_args{args};
    exec_args.emplace_back(nullptr);

    // Try to call execvp with given exe path.
    auto try_exec = [&](fs::path exe_path, bool allow_notfound = true) {
        std::string exe_path_str{fs::PathToString(exe_path)};
        exec_args[0] = exe_path_str.c_str();
        if (execvp(exec_args[0], (char*const*)exec_args.data()) == -1) {
            if (allow_notfound && errno == ENOENT) return false;
            throw std::system_error(errno, std::system_category(), strprintf("execvp failed to execute '%s'", exec_args[0]));
        }
        return true; // Will not actually be reached if execvp succeeds
    };

    // Whether to use system PATH variable to locate wrapper executable and
    // search for other executables. Only use PATH if wrapper executable was
    // invoked using the PATH, to avoid unintentionally launching system
    // executables in a local build.
    // (https://github.com/bitcoin/bitcoin/pull/31375#discussion_r1861814807)
    const bool search_system_path(wrapper_argv0.find('/') == std::string_view::npos);

    // Try to figure out where wrapper executable is located. This is a
    // simplified search that won't work perfectly on every platform and doesn't
    // need to, as it is only trying to prioritize locally built or installed
    // executables over system executables. We may want to add options to
    // override this behavior in the future, though.
    const fs::path wrapper_argv0_path{fs::PathFromString(std::string{wrapper_argv0})};
    fs::path wrapper_path{wrapper_argv0_path};
    std::error_code ec;
    if (search_system_path) {
        if (const char* path_env = std::getenv("PATH")) {
            size_t start{0}, end{0};
            for (std::string_view paths{path_env}; end != std::string_view::npos; start = end + 1) {
                end = paths.find(':', start);
                fs::path candidate = fs::path(paths.substr(start, end - start)) / wrapper_argv0_path;
                if (fs::is_regular_file(candidate, ec)) {
                    wrapper_path = candidate;
                    break;
                }
            }
        }
    }

    // Try to resolve any symlinks and figure out actual directory containing the wrapper executable.
    fs::path wrapper_dir{fs::weakly_canonical(wrapper_path, ec)};
    if (wrapper_dir.empty()) wrapper_dir = wrapper_path; // Restore previous path if weakly_canonical failed.
    wrapper_dir = wrapper_dir.parent_path();
    const fs::path arg0{fs::PathFromString(args[0])};

    // If wrapper is in a CMake build tree, first look for target executable
    // relative to it.
    (wrapper_dir.filename() == "src" && try_exec(wrapper_dir / arg0)) ||
    // Otherwise if wrapper is installed in a bin/ directory, first look for
    // target executable in libexec/
    (wrapper_dir.filename() == "bin" && try_exec(fs::path{wrapper_dir.parent_path()} / "libexec" / arg0.filename())) ||
    // Otherwise look for target executable next to current wrapper
    try_exec(wrapper_dir / arg0.filename(), search_system_path) ||
    // Otherwise just look on the system path.
    (search_system_path && try_exec(arg0.filename(), false));
};
