# CRT exec/spawn probe

Throwaway experiment measuring how the Windows C runtime's `_execvp`/`_spawnvp`
family handles extensionless paths, missing files, and PATH search, comparing the
legacy `msvcrt.dll` against `ucrtbase.dll`. Motivated by the commit
"bitcoin: Fix msvcrt regressions from _execvp to _spawnvp switch" in the
Bitcoin Core `bitcoin.exe` wrapper, which claims that msvcrt `_spawnvp` neither
appends `.exe` nor returns ENOENT for a missing file, while `_execvp` does both.

Files:

- `probe.c`: `probe.exe API MODE FILE [ARGS...]` calls one CRT function and
  prints `RESULT ... ret= errno= doserrno=`. Also prints which DLL each function
  is resolved from.
- `child.c`: the target executable. Prints argv, appends it to the file named by
  `PROBE_MARKER`, exits 37.
- `probe.py`: driver. Runs the matrix of functions x modes x path cases, either
  through `probe.exe` (`--runner exe:PATH`) or by calling the DLL's exports from
  Python via ctypes (`--runner ctypes:msvcrt.dll`). Writes a markdown table.
- `prebuilt/`: `probe.exe` and `child.exe` cross-compiled on Linux with nix
  `x86_64-w64-mingw32-gcc` 15.2.0 / mingw-w64 13.0 (msvcrt), via
  `x86_64-w64-mingw32-gcc -O1 -Wall -Wextra -static -o probe.exe probe.c`.
- `.github/workflows/msvcrt-probe.yml`: runs everything on GitHub-hosted Windows
  runners (ctypes on windows-2022 and windows-2025; MSYS2 MINGW64 and UCRT64
  builds; MSVC native build; the prebuilt nix binaries).

Reproduce locally on Windows:

```
cl /Fe:probe.exe probe.c shell32.lib
cl /Fe:child.exe child.c
py -3 probe.py run --runner exe:probe.exe --child child.exe --out results.md
py -3 probe.py run --runner ctypes:msvcrt.dll --child child.exe --out results-msvcrt.md
py -3 probe.py run --runner ctypes:ucrtbase.dll --child child.exe --out results-ucrt.md
```

On Linux with Wine (harness check only; Wine's msvcrt is a reimplementation):

```
python3 probe.py run --runner exe:prebuilt/probe.exe --child prebuilt/child.exe --wine
```
