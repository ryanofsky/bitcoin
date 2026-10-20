# AF_UNIX stale socket file probe

Throwaway experiment for bitcoin/bitcoin#32387: `ProcessImpl::bind` removes a
stale AF_UNIX socket file before binding, using

```cpp
if (fs::symlink_status(path).type() == fs::file_type::socket
    || (fs::is_regular_file(path) && fs::file_size(path) == 0)) fs::remove(path);
```

which on Windows CI either throws from `status` (MSVC) or evaluates false
(MinGW UCRT cross build), so the restart in `interface_ipc_init.py` fails with
WSAEADDRINUSE. This probe reports, per toolchain and Windows version, what
Win32 and every relevant `std::filesystem` call say about the leftover file,
and which removal calls work.

Files:

- `probe.cpp`: `probe.exe [RESULTS.md]` creates leftovers of each kind
  (closesocket in-process, child killed, child exited, plus controls: missing
  path, empty regular file, still-bound socket), prints an inspection table
  for each, then a removal x leftover-kind matrix with the rebind result.
  Internally re-executes itself as `--child-sleep`/`--child-exit` children.
- `prebuilt/`: binaries cross-compiled on Linux. `probe-nix-gcc14.3-ucrt.exe` is
  built in Bitcoin Core's own `contrib/devtools/shell-win64-cross.nix` shell (GCC
  14.3.0, mingw-w64 13.0.0, **UCRT**), the exact toolchain of the "Windows-cross
  to x86_64" CI job. The `-msvcrt` ones use nix `x86_64-w64-mingw32-g++` 15.2.0
  and 14.3.0 (mingw-w64 13.0, msvcrt). All via
  `$CXX -std=c++20 -O1 -static -o probe.exe probe.cpp -lws2_32 -lversion`.
- `.github/workflows/afunix-probe.yml`: MSVC on windows-2022 / windows-2025 /
  windows-2025-vs2026; apt mingw-w64 msvcrt cross builds (Ubuntu has no UCRT mingw-w64 package) from
  ubuntu-24.04 and an ubuntu:26.04 container, run on windows-2022 and
  windows-2025 together with the prebuilt nix binaries; MSYS2 UCRT64 and
  MINGW64 on both Windows versions. The `collect` job prints every results file.

Reproduce locally on Windows:

```
cl /std:c++20 /EHsc /Fe:probe.exe probe.cpp ws2_32.lib version.lib
probe.exe results.md
```

`errprobe.cpp` (`errprobe.exe [RESULTS.md]`) prints the exact `std::errc`
`message()`/`what()` strings, the `system_category` string for `WSAECONNREFUSED`,
`GetFileAttributesW` and `connect()` results for a stale socket file, a live
listener, a missing file and paths whose parent directory is missing. Built and
run by every job; `prebuilt/errprobe-*.exe` are the nix UCRT (GCC 14.3) and
msvcrt (GCC 15.2) cross builds.
