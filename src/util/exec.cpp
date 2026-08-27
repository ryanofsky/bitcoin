// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/exec.h>

#include <util/fs.h>
#ifdef WIN32
#include <util/subprocess.h>
#endif

#include <cstdlib>
#include <string>
#include <system_error>

#ifdef WIN32
#include <process.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace util {
int ExecVp(const char* file, char* const argv[])
{
#ifndef WIN32
    return execvp(file, argv);
#else
    std::vector<std::string> escaped_args;
    for (char* const* arg_ptr{argv}; *arg_ptr; ++arg_ptr) {
        subprocess::util::quote_argument(std::string{*arg_ptr}, escaped_args.emplace_back(), /*force=*/false);
    }

    std::vector<const char*> new_argv;
    new_argv.reserve(escaped_args.size() + 1);
    for (const auto& s : escaped_args) new_argv.push_back(s.c_str());
    new_argv.push_back(nullptr);
    // Use _spawnvp(_P_WAIT) instead of _execvp because on Windows all _exec*
    // variants work by calling CreateProcess to start the child then immediately
    // calling ExitProcess(0) on the parent. Unlike POSIX execvp, which replaces
    // the calling process image (the parent's PID becomes the child), the
    // Windows parent exits before the child finishes. Any shell or test
    // framework waiting on bitcoin.exe sees it exit with code 0 while
    // bitcoind.exe or bitcoin-qt.exe continues running as an orphan.
    //
    // _spawnvp(_P_WAIT) blocks until the child exits, then returns the child's
    // exit code. We forward it via _exit() so ExecVp still never returns on
    // success, matching POSIX behavior from the caller's perspective.
    intptr_t result{_spawnvp(_P_WAIT, file, new_argv.data())};
    if (result == -1) return -1;
    _exit(static_cast<int>(result)); // forward child exit code; never returns
#endif
}

fs::path GetExePath(std::string_view argv0)
{
    // Try to figure out where executable is located. This does a simplified
    // search that won't work perfectly on every platform and doesn't need to,
    // as it is only currently being used in a convenience wrapper binary to try
    // to prioritize locally built or installed executables over system
    // executables.
    const fs::path argv0_path{fs::PathFromString(std::string{argv0})};
    fs::path path{argv0_path};
    std::error_code ec;
#ifndef WIN32
    // If argv0 doesn't contain a path separator, it was invoked from the system
    // PATH and can be searched for there.
    if (!argv0_path.has_parent_path()) {
        if (const char* path_env = std::getenv("PATH")) {
            size_t start{0}, end{0};
            for (std::string_view paths{path_env}; end != std::string_view::npos; start = end + 1) {
                end = paths.find(':', start);
                fs::path candidate = fs::path(paths.substr(start, end - start)) / argv0_path;
                if (fs::is_regular_file(candidate, ec)) {
                    path = candidate;
                    break;
                }
            }
        }
    }
#else
    wchar_t module_path[MAX_PATH];
    if (GetModuleFileNameW(nullptr, module_path, MAX_PATH) > 0) {
        path = fs::path{module_path};
    }
#endif
    return path;
}

} // namespace util
