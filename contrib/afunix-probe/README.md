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
- `prebuilt/`: binaries cross-compiled on Linux with nix `x86_64-w64-mingw32-g++`
  15.2.0 and 14.3.0 (mingw-w64 13.0, **msvcrt**), via
  `x86_64-w64-mingw32-g++ -std=c++20 -O1 -static -o probe.exe probe.cpp -lws2_32`.
- `.github/workflows/afunix-probe.yml`: MSVC on windows-2022 / windows-2025 /
  windows-2025-vs2026; apt mingw-w64 UCRT and msvcrt cross builds from
  ubuntu-24.04 and an ubuntu:26.04 container, run on windows-2022 and
  windows-2025 together with the prebuilt nix binaries; MSYS2 UCRT64 and
  MINGW64 on both Windows versions. The `collect` job prints every results file.

Reproduce locally on Windows:

```
cl /std:c++20 /EHsc /Fe:probe.exe probe.cpp ws2_32.lib
probe.exe results.md
```
