#!/usr/bin/env python3
"""Driver for the CRT exec/spawn probe.

Runs a matrix of (CRT function, spawn mode, path case) and records whether the
child was launched and what errno the call returned otherwise.

    probe.py run --runner exe:PATH/probe.exe --child PATH/child.exe [--wine] --out FILE.md
    probe.py run --runner ctypes:msvcrt.dll --child PATH/child.exe --out FILE.md
    probe.py one DLL API MODE FILE [ARGS...]     (internal: single ctypes call)

Runner "exe" invokes the compiled C probe once per cell. Runner "ctypes" calls
the named DLL's functions from Python (in a fresh interpreter per cell, since a
successful exec replaces the process).
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

APIS = [
    ("execvp", "-"),
    ("wexecvp", "-"),
    ("execv", "-"),
    ("spawnvp", "wait"),
    ("spawnvp", "nowait"),
    ("spawnvp", "overlay"),
    ("wspawnvp", "wait"),
    ("spawnv", "wait"),
]

# (case name, path relative to the probe temp dir or bare name, kind)
# kind: "abs" -> absolute path under tmp, "rel" -> relative to cwd (=tmp), "bare" -> as is
CASES = [
    ("abs-noext-exists", "dir/child", "abs"),
    ("abs-exe-exists", "dir/child.exe", "abs"),
    ("abs-noext-missing", "dir/nothere", "abs"),
    ("abs-exe-missing", "dir/nothere.exe", "abs"),
    ("absdir-missing-noext", "nodir/child", "abs"),
    ("absdir-missing-exe", "nodir/child.exe", "abs"),
    ("rel-noext-exists", "dir/child", "rel"),
    ("rel-exe-exists", "dir/child.exe", "rel"),
    ("bare-noext-onpath", "child", "bare"),
    ("bare-exe-onpath", "child.exe", "bare"),
    ("bare-noext-missing", "nothere", "bare"),
    ("bare-noext-offpath", "child", "bare-offpath"),
]

CHILD_ARGS = ["-version"]
CHILD_EXIT = 37
PROBE_EXIT = 99

ERRNO_NAMES = {0: "0", 2: "ENOENT", 7: "E2BIG", 8: "ENOEXEC", 11: "EAGAIN", 12: "ENOMEM",
               13: "EACCES", 22: "EINVAL"}


def errno_name(e):
    return ERRNO_NAMES.get(e, str(e))


# ---------------------------------------------------------------- ctypes single call

def ctypes_one(dll, api, mode_s, file, extra):
    import ctypes
    crt = ctypes.CDLL(dll)
    intptr = ctypes.c_ssize_t
    cp, wp = ctypes.c_char_p, ctypes.c_wchar_p
    for name in ("_execvp", "_execv"):
        getattr(crt, name).argtypes = [cp, ctypes.POINTER(cp)]
        getattr(crt, name).restype = intptr
    crt._wexecvp.argtypes = [wp, ctypes.POINTER(wp)]
    crt._wexecvp.restype = intptr
    for name in ("_spawnvp", "_spawnv"):
        getattr(crt, name).argtypes = [ctypes.c_int, cp, ctypes.POINTER(cp)]
        getattr(crt, name).restype = intptr
    crt._wspawnvp.argtypes = [ctypes.c_int, wp, ctypes.POINTER(wp)]
    crt._wspawnvp.restype = intptr
    crt._errno.restype = ctypes.POINTER(ctypes.c_int)
    crt._errno.argtypes = []
    doserrno = getattr(crt, "__doserrno")
    doserrno.restype = ctypes.POINTER(ctypes.c_ulong)
    doserrno.argtypes = []
    crt._cwait.argtypes = [ctypes.POINTER(ctypes.c_int), intptr, ctypes.c_int]
    crt._cwait.restype = intptr

    k32 = ctypes.WinDLL("kernel32", use_last_error=True)
    buf = ctypes.create_unicode_buffer(1024)
    k32.GetModuleFileNameW(ctypes.c_void_p(crt._handle), buf, 1024)
    print(f"CRT dll={dll} path={buf.value}")
    for name in ("_execvp", "_wexecvp", "_execv", "_spawnvp", "_wspawnvp", "_spawnv"):
        print(f"MODULE {name}={buf.value}")
    sys.stdout.flush()

    modes = {"wait": 0, "nowait": 1, "overlay": 2, "-": -1}
    mode = modes[mode_s]
    args = [file] + extra
    nargv = (cp * (len(args) + 1))(*[a.encode("mbcs") for a in args], None)
    wargv = (wp * (len(args) + 1))(*args, None)
    nfile = file.encode("mbcs")

    crt._errno()[0] = 0
    doserrno()[0] = 0
    if api == "execvp":
        ret = crt._execvp(nfile, nargv)
    elif api == "wexecvp":
        ret = crt._wexecvp(file, wargv)
    elif api == "execv":
        ret = crt._execv(nfile, nargv)
    elif api == "spawnvp":
        ret = crt._spawnvp(mode, nfile, nargv)
    elif api == "wspawnvp":
        ret = crt._wspawnvp(mode, file, wargv)
    elif api == "spawnv":
        ret = crt._spawnv(mode, nfile, nargv)
    else:
        raise SystemExit(f"bad api {api}")
    e = crt._errno()[0]
    d = doserrno()[0]
    print(f'RESULT api={api} mode={mode_s} ret={ret} errno={e} doserrno={d} strerror="{os.strerror(e) if e else ""}"')
    if ret != -1 and mode == 1:
        status = ctypes.c_int(-1)
        crt._errno()[0] = 0
        w = crt._cwait(ctypes.byref(status), ret, 0)
        print(f"CWAIT ret={w} status={status.value} errno={crt._errno()[0]}")
    sys.stdout.flush()
    sys.exit(PROBE_EXIT)


# ---------------------------------------------------------------- matrix driver

class Runner:
    def __init__(self, spec, wine):
        self.kind, _, self.target = spec.partition(":")
        self.wine = wine
        if self.kind not in ("exe", "ctypes"):
            raise SystemExit(f"bad runner {spec}")
        if self.kind == "exe":
            self.target = os.path.abspath(self.target)

    def win(self, path):
        """Convert a host absolute path to the path the Windows process sees."""
        path = os.path.abspath(path)
        if self.wine:
            return "Z:" + path.replace("/", "\\")
        return path

    def cmd(self, api, mode, file, extra):
        if self.kind == "exe":
            base = ["wine", self.target] if self.wine else [self.target]
            return base + [api, mode, file] + extra
        return [sys.executable, os.path.abspath(__file__), "one", self.target, api, mode, file] + extra


def run_matrix(runner, child_exe, out_path):
    tmp = tempfile.mkdtemp(prefix="crtprobe-")
    d = os.path.join(tmp, "dir")
    os.makedirs(d)
    shutil.copy(child_exe, os.path.join(d, "child.exe"))
    marker = os.path.join(tmp, "marker.txt")

    env_base = dict(os.environ)
    env_base["PROBE_MARKER"] = runner.win(marker)
    sep = ";"
    if runner.wine:
        env_base["WINEDEBUG"] = "-all"

    def env_for(kind):
        env = dict(env_base)
        pathvar = "WINEPATH" if runner.wine else "PATH"
        cur = env.get(pathvar, "")
        if kind == "bare-offpath":
            # make sure dir is not on PATH
            parts = [p for p in cur.split(sep) if os.path.normcase(p) != os.path.normcase(runner.win(d))]
            env[pathvar] = sep.join(parts)
        else:
            env[pathvar] = runner.win(d) + (sep + cur if cur else "")
        return env

    header = []
    rows = []
    for api, mode in APIS:
        for case, rel, kind in CASES:
            if kind == "abs":
                file = runner.win(os.path.join(tmp, rel))
            elif kind == "rel":
                file = rel.replace("/", "\\")
            else:
                file = rel
            if os.path.exists(marker):
                os.remove(marker)
            cmd = runner.cmd(api, mode, file, CHILD_ARGS)
            try:
                p = subprocess.run(cmd, cwd=tmp, env=env_for(kind), stdout=subprocess.PIPE,
                                   stderr=subprocess.STDOUT, timeout=120)
                out = p.stdout.decode("utf-8", "replace")
                rc = p.returncode
            except subprocess.TimeoutExpired as ex:
                out = (ex.stdout or b"").decode("utf-8", "replace")
                rc = "timeout"
            if not header:
                header = [l for l in out.splitlines() if l.startswith(("CRT", "MODULE"))]
            m = re.search(r"^RESULT .*?ret=(-?\d+) errno=(\d+) doserrno=(\d+)", out, re.M)
            cw = re.search(r"^CWAIT ret=(-?\d+) status=(-?\d+)", out, re.M)
            child_line = re.search(r"^CHILD .*$", out, re.M)
            marker_text = ""
            if os.path.exists(marker):
                with open(marker) as f:
                    marker_text = f.read().strip()
            launched = bool(marker_text)
            if m:
                ret, e, dos = int(m.group(1)), int(m.group(2)), int(m.group(3))
            else:
                ret = e = dos = None
            if m is None:
                outcome = "launched(exec)" if launched else f"no RESULT line (rc={rc})"
            elif ret == -1:
                outcome = f"fail {errno_name(e)}" + (" but child ran!" if launched else "")
            elif mode == "wait":
                outcome = "launched" if (ret == CHILD_EXIT and launched) else f"ret={ret} launched={launched}"
            elif mode == "nowait":
                st = int(cw.group(2)) if cw else None
                outcome = "launched" if (st == CHILD_EXIT and launched) else f"cwait={st} launched={launched}"
            else:
                outcome = f"returned ret={ret} launched={launched}"
            rows.append((api, mode, case, file, outcome, ret, e, dos, rc, marker_text, out))
            print(f"{api:9} {mode:8} {case:22} -> {outcome}", flush=True)

    lines = []
    lines.append(f"### runner={runner.kind}:{runner.target}" + (" (wine)" if runner.wine else ""))
    lines.append("")
    lines.append("```")
    lines.extend(header)
    lines.append("```")
    lines.append("")
    lines.append(f"tmp dir: `{runner.win(tmp)}`; `dir\\child.exe` exists; `nodir` does not; PATH includes `dir` except in bare-noext-offpath; cwd is tmp.")
    lines.append("")
    lines.append("| api | mode | case | file | outcome | ret | errno | doserrno | exit |")
    lines.append("|-----|------|------|------|---------|-----|-------|----------|------|")
    for api, mode, case, file, outcome, ret, e, dos, rc, marker_text, out in rows:
        fshow = file.replace(runner.win(tmp), "$T")
        lines.append(f"| {api} | {mode} | {case} | `{fshow}` | {outcome} | {ret if ret is not None else ''} | "
                     f"{errno_name(e) if e is not None else ''} | {dos if dos is not None else ''} | {rc} |")
    lines.append("")
    lines.append("<details><summary>raw output</summary>")
    lines.append("")
    for api, mode, case, file, outcome, ret, e, dos, rc, marker_text, out in rows:
        lines.append(f"#### {api} {mode} {case}")
        lines.append("```")
        lines.append(out.strip().replace(runner.win(tmp), "$T"))
        if marker_text:
            lines.append("MARKER: " + marker_text.replace(runner.win(tmp), "$T"))
        lines.append(f"exit={rc}")
        lines.append("```")
    lines.append("</details>")
    text = "\n".join(lines) + "\n"
    if out_path:
        with open(out_path, "w") as f:
            f.write(text)
        print(f"wrote {out_path}")
    print(text)


def main():
    if len(sys.argv) >= 2 and sys.argv[1] == "one":
        ctypes_one(sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5], sys.argv[6:])
        return
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    r = sub.add_parser("run")
    r.add_argument("--runner", required=True, help="exe:PATH or ctypes:DLL")
    r.add_argument("--child", required=True, help="path to child.exe")
    r.add_argument("--wine", action="store_true")
    r.add_argument("--out")
    a = ap.parse_args()
    run_matrix(Runner(a.runner, a.wine), a.child, a.out)


if __name__ == "__main__":
    main()
