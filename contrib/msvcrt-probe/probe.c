// Probe the behavior of the Windows CRT exec/spawn family.
//
//   probe.exe API MODE FILE [ARGS...]
//
// API:  execvp | wexecvp | execv | spawnvp | wspawnvp | spawnv
//       createprocess      (CreateProcessA with lpApplicationName=NULL and
//                           FILE as the first command line token)
//       createprocess-app  (CreateProcessA with lpApplicationName=FILE)
// MODE: wait | nowait | overlay   (spawn APIs)   or   -   (exec APIs)
// FILE: passed as the CRT function's file name argument and as argv[0]
//
// Prints one RESULT line if the call returns. A successful exec never returns
// (the process is replaced), so the driver treats "no RESULT line" plus the
// child's marker file as exec success. Exits 99 after a call returns so the
// driver can tell that from the child's own exit code (37).

#define _CRT_SECURE_NO_WARNINGS
#include <errno.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <shellapi.h>

#ifndef _WAIT_CHILD
#define _WAIT_CHILD 0
#endif

static void print_module_of(const char* name, const void* addr)
{
    HMODULE h = NULL;
    char path[MAX_PATH] = "?";
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCSTR)addr, &h)) {
        GetModuleFileNameA(h, path, sizeof(path));
    }
    printf("MODULE %s=%s\n", name, path);
}

// Run FILE through CreateProcessA directly, wait for it, and return its exit
// code, or -1 with the Win32 error in *err. Mirrors what the CRT functions do
// underneath, minus any CRT-side lookup.
static intptr_t do_createprocess(const char* file, char** args, int use_app, unsigned long* err)
{
    char cmdline[4096] = "";
    for (char** a = args; *a; ++a) {
        if (a != args) strcat(cmdline, " ");
        if (strchr(*a, ' ')) { strcat(cmdline, "\""); strcat(cmdline, *a); strcat(cmdline, "\""); }
        else strcat(cmdline, *a);
    }
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    *err = 0;
    if (!CreateProcessA(use_app ? file : NULL, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        *err = GetLastError();
        return -1;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = (DWORD)-1;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return (intptr_t)code;
}

int main(int argc, char** argv)
{
    if (argc < 4) {
        fprintf(stderr, "usage: probe API MODE FILE [ARGS...]\n");
        return 2;
    }
    const char* api = argv[1];
    const char* mode_s = argv[2];
    const char* file = argv[3];
    char** args = argv + 3; // child argv: argv[0] == file, then extra args

    int mode = -1;
    if (!strcmp(mode_s, "wait")) mode = _P_WAIT;
    else if (!strcmp(mode_s, "nowait")) mode = _P_NOWAIT;
    else if (!strcmp(mode_s, "overlay")) mode = _P_OVERLAY;
    else if (strcmp(mode_s, "-")) {
        fprintf(stderr, "bad mode %s\n", mode_s);
        return 2;
    }

    printf("CRT");
#ifdef _UCRT
    printf(" _UCRT=1");
#else
    printf(" _UCRT=0");
#endif
#ifdef __MSVCRT_VERSION__
    printf(" __MSVCRT_VERSION__=0x%x", (unsigned)__MSVCRT_VERSION__);
#endif
#ifdef __MINGW64_VERSION_MAJOR
    printf(" mingw-w64=%d.%d", (int)__MINGW64_VERSION_MAJOR, (int)__MINGW64_VERSION_MINOR);
#endif
#ifdef __GNUC__
    printf(" gcc=%d.%d.%d", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#endif
#ifdef _MSC_VER
    printf(" _MSC_VER=%d", (int)_MSC_VER);
#endif
    printf("\n");
    print_module_of("_execvp", (const void*)&_execvp);
    print_module_of("_wexecvp", (const void*)&_wexecvp);
    print_module_of("_execv", (const void*)&_execv);
    print_module_of("_spawnvp", (const void*)&_spawnvp);
    print_module_of("_wspawnvp", (const void*)&_wspawnvp);
    print_module_of("_spawnv", (const void*)&_spawnv);
    fflush(stdout);

    int wargc = 0;
    wchar_t** wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
    if (!wargv || wargc != argc) {
        fprintf(stderr, "wide argv mismatch: wargc=%d argc=%d\n", wargc, argc);
        return 2;
    }
    const wchar_t* wfile = wargv[3];
    wchar_t** wargs = wargv + 3;

    errno = 0;
    _doserrno = 0;
    intptr_t ret;
    if (!strcmp(api, "execvp")) ret = _execvp(file, (const char* const*)args);
    else if (!strcmp(api, "wexecvp")) ret = _wexecvp(wfile, (const wchar_t* const*)wargs);
    else if (!strcmp(api, "execv")) ret = _execv(file, (const char* const*)args);
    else if (!strcmp(api, "spawnvp")) ret = _spawnvp(mode, file, (const char* const*)args);
    else if (!strcmp(api, "wspawnvp")) ret = _wspawnvp(mode, wfile, (const wchar_t* const*)wargs);
    else if (!strcmp(api, "spawnv")) ret = _spawnv(mode, file, (const char* const*)args);
    else if (!strcmp(api, "createprocess") || !strcmp(api, "createprocess-app")) {
        unsigned long err = 0;
        ret = do_createprocess(file, args, !strcmp(api, "createprocess-app"), &err);
        printf("RESULT api=%s mode=%s ret=%lld errno=0 doserrno=%lu strerror=\"winerr %lu\"\n",
               api, mode_s, (long long)ret, err, err);
        fflush(stdout);
        return 99;
    }
    else {
        fprintf(stderr, "bad api %s\n", api);
        return 2;
    }
    int e = errno;
    unsigned long d = _doserrno;
    printf("RESULT api=%s mode=%s ret=%lld errno=%d doserrno=%lu strerror=\"%s\"\n",
           api, mode_s, (long long)ret, e, d, strerror(e));
    if (ret != -1 && mode == _P_NOWAIT) {
        int status = -1;
        errno = 0;
        intptr_t w = _cwait(&status, ret, _WAIT_CHILD);
        printf("CWAIT ret=%lld status=%d errno=%d\n", (long long)w, status, errno);
    }
    fflush(stdout);
    return 99;
}
