// AF_UNIX stale socket file probe (throwaway experiment, see README.md).
//
// Binds an AF_UNIX socket to a path, abandons it in several ways, and then
// reports what Win32 and std::filesystem say about the leftover file and
// which ways of removing it work. All output is markdown on stdout; if an
// argument is given the same text is also written to that file.
//
//   probe.exe [RESULTS.md]            run everything
//   probe.exe --child-sleep PATH      (internal) bind PATH, print BOUND, sleep
//   probe.exe --child-exit PATH       (internal) bind PATH, print BOUND, ExitProcess
//
// Every probe call is wrapped so an exception or crash in one row does not
// hide the rest.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#if __has_include(<afunix.h>)
#include <afunix.h>
#define PROBE_HAVE_AFUNIX_H 1
#else
#define PROBE_HAVE_AFUNIX_H 0
#define UNIX_PATH_MAX 108
struct sockaddr_un {
    ADDRESS_FAMILY sun_family;
    char sun_path[UNIX_PATH_MAX];
};
#endif
#include <io.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <functional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#ifndef IO_REPARSE_TAG_AF_UNIX
#define IO_REPARSE_TAG_AF_UNIX 0x80000023L
#endif
#ifndef IO_REPARSE_TAG_SYMLINK
#define IO_REPARSE_TAG_SYMLINK 0xA000000CL
#endif
#ifndef IO_REPARSE_TAG_MOUNT_POINT
#define IO_REPARSE_TAG_MOUNT_POINT 0xA0000003L
#endif

namespace fs = std::filesystem;

// ---------------------------------------------------------------- formatting

static std::string g_out;

static std::string fmt(const char* f, ...)
{
    char buf[4096];
    va_list ap;
    va_start(ap, f);
    vsnprintf(buf, sizeof buf, f, ap);
    va_end(ap);
    return buf;
}

static void out(const std::string& s)
{
    g_out += s;
    g_out += '\n';
    fputs(s.c_str(), stdout);
    fputc('\n', stdout);
    fflush(stdout);
}

static std::string narrow(const std::wstring& w)
{
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), s.data(), n, nullptr, nullptr);
    return s;
}

static std::string trim(std::string s)
{
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' ' || s.back() == '.')) s.pop_back();
    for (auto& c : s) if (c == '|') c = '/';
    return s;
}

static std::string win_err(DWORD e)
{
    wchar_t* msg = nullptr;
    DWORD n = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                             nullptr, e, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPWSTR)&msg, 0, nullptr);
    std::string s = fmt("err=%lu", (unsigned long)e);
    if (n && msg) {
        s += " (" + trim(narrow(msg)) + ")";
        LocalFree(msg);
    }
    return s;
}

static std::string errno_str(int e)
{
    return fmt("errno=%d (%s)", e, trim(strerror(e)).c_str());
}

static std::string attrs_str(DWORD a)
{
    if (a == INVALID_FILE_ATTRIBUTES) return "INVALID_FILE_ATTRIBUTES";
    std::string s = fmt("0x%lx", (unsigned long)a);
    struct { DWORD f; const char* n; } flags[] = {
        {FILE_ATTRIBUTE_READONLY, "READONLY"}, {FILE_ATTRIBUTE_HIDDEN, "HIDDEN"},
        {FILE_ATTRIBUTE_SYSTEM, "SYSTEM"}, {FILE_ATTRIBUTE_DIRECTORY, "DIRECTORY"},
        {FILE_ATTRIBUTE_ARCHIVE, "ARCHIVE"}, {FILE_ATTRIBUTE_NORMAL, "NORMAL"},
        {FILE_ATTRIBUTE_REPARSE_POINT, "REPARSE_POINT"}, {FILE_ATTRIBUTE_TEMPORARY, "TEMPORARY"},
        {FILE_ATTRIBUTE_SPARSE_FILE, "SPARSE"}, {FILE_ATTRIBUTE_COMPRESSED, "COMPRESSED"},
        {FILE_ATTRIBUTE_OFFLINE, "OFFLINE"}, {FILE_ATTRIBUTE_NOT_CONTENT_INDEXED, "NOT_INDEXED"},
    };
    const char* sep = " [";
    for (auto& f : flags) if (a & f.f) { s += sep; s += f.n; sep = ","; }
    if (sep[0] == ',') s += "]";
    return s;
}

static std::string tag_str(DWORD t)
{
    if (t == IO_REPARSE_TAG_AF_UNIX) return fmt("0x%lx=IO_REPARSE_TAG_AF_UNIX", (unsigned long)t);
    if (t == IO_REPARSE_TAG_SYMLINK) return fmt("0x%lx=IO_REPARSE_TAG_SYMLINK", (unsigned long)t);
    if (t == IO_REPARSE_TAG_MOUNT_POINT) return fmt("0x%lx=IO_REPARSE_TAG_MOUNT_POINT", (unsigned long)t);
    return fmt("0x%lx", (unsigned long)t);
}

static const char* type_str(fs::file_type t)
{
    switch (t) {
    case fs::file_type::none: return "none";
    case fs::file_type::not_found: return "not_found";
    case fs::file_type::regular: return "regular";
    case fs::file_type::directory: return "directory";
    case fs::file_type::symlink: return "symlink";
    case fs::file_type::block: return "block";
    case fs::file_type::character: return "character";
    case fs::file_type::fifo: return "fifo";
    case fs::file_type::socket: return "socket";
    case fs::file_type::unknown: return "unknown";
#ifdef _WIN32
#if defined(_MSC_VER) && !defined(__clang__) && !defined(__GNUC__)
    case fs::file_type::junction: return "junction";
#endif
#endif
    default: return "?";
    }
}

static std::string ec_str(const std::error_code& ec)
{
    if (!ec) return "ec=0";
    return fmt("ec=%d/%s (%s)", ec.value(), ec.category().name(), trim(ec.message()).c_str());
}

static std::string status_str(const fs::file_status& st)
{
    return type_str(st.type());
}

// Run f() and return its string result, or a description of what it threw.
static std::string guarded(const std::function<std::string()>& f)
{
    try {
        return f();
    } catch (const fs::filesystem_error& e) {
        return fmt("THROW filesystem_error: what=\"%s\" %s", trim(e.what()).c_str(), ec_str(e.code()).c_str());
    } catch (const std::exception& e) {
        return fmt("THROW %s", trim(e.what()).c_str());
    } catch (...) {
        return "THROW unknown";
    }
}

struct Table {
    std::vector<std::pair<std::string, std::string>> rows;
    void add(const std::string& k, const std::function<std::string()>& f) { rows.emplace_back(k, guarded(f)); }
    void add_s(const std::string& k, const std::string& v) { rows.emplace_back(k, v); }
};

// ---------------------------------------------------------------- sockets

static std::string wsa_str(int e)
{
    return fmt("WSA%d (%s)", e, trim(win_err((DWORD)e)).c_str());
}

static std::string sun_path_of(const fs::path& p)
{
    // Same thing bitcoind does: narrow string copied into sun_path.
    std::string s = p.string();
    return s;
}

// Bind an AF_UNIX socket to p. Returns the socket or INVALID_SOCKET, with a
// description in *desc.
static SOCKET bind_socket(const fs::path& p, std::string* desc, bool do_listen = true)
{
    std::string sp = sun_path_of(p);
    if (sp.size() >= UNIX_PATH_MAX) { *desc = "path too long for sun_path"; return INVALID_SOCKET; }
    SOCKET s = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) { *desc = "socket() failed " + wsa_str(WSAGetLastError()); return INVALID_SOCKET; }
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    memcpy(addr.sun_path, sp.c_str(), sp.size() + 1);
    if (::bind(s, (sockaddr*)&addr, sizeof addr) != 0) {
        *desc = "bind() failed " + wsa_str(WSAGetLastError());
        closesocket(s);
        return INVALID_SOCKET;
    }
    if (do_listen && ::listen(s, 1) != 0) {
        *desc = "bind ok, listen() failed " + wsa_str(WSAGetLastError());
        return s;
    }
    *desc = do_listen ? "bind+listen ok" : "bind ok";
    return s;
}

static int child_main(const char* mode, const char* path)
{
    if (strcmp(mode, "--child-remove-all") == 0) {
        std::error_code ec;
        auto n = fs::remove_all(fs::path(path), ec);
        printf("remove_all returned %llu %s\n", (unsigned long long)n, ec_str(ec).c_str());
        fflush(stdout);
        ExitProcess(0);
    }
    WSADATA wd;
    WSAStartup(MAKEWORD(2, 2), &wd);
    std::string desc;
    SOCKET s = bind_socket(fs::path(path), &desc);
    if (s == INVALID_SOCKET) {
        printf("FAIL %s\n", desc.c_str());
        fflush(stdout);
        return 2;
    }
    printf("BOUND\n");
    fflush(stdout);
    if (strcmp(mode, "--child-sleep") == 0) {
        Sleep(INFINITE);
    }
    // --child-exit: leave the socket open; let process exit clean it up.
    ExitProcess(0);
}

static std::wstring g_exe;

// Run this exe as a child that binds p. If kill is true, TerminateProcess it
// after it reports BOUND; otherwise wait for it to exit on its own.
static std::string spawn_child(const fs::path& p, bool kill)
{
    SECURITY_ATTRIBUTES sa{sizeof sa, nullptr, TRUE};
    HANDLE rd = nullptr, wr = nullptr;
    if (!CreatePipe(&rd, &wr, &sa, 0)) return "CreatePipe failed " + win_err(GetLastError());
    SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0);
    std::wstring cmd = L"\"" + g_exe + L"\" " + (kill ? L"--child-sleep" : L"--child-exit") + L" \"" + p.wstring() + L"\"";
    STARTUPINFOW si{};
    si.cb = sizeof si;
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = wr;
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    PROCESS_INFORMATION pi{};
    std::vector<wchar_t> cmdbuf(cmd.begin(), cmd.end());
    cmdbuf.push_back(0);
    if (!CreateProcessW(nullptr, cmdbuf.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi)) {
        std::string e = "CreateProcessW failed " + win_err(GetLastError());
        CloseHandle(rd);
        CloseHandle(wr);
        return e;
    }
    CloseHandle(wr);
    std::string line;
    char c;
    DWORD n;
    while (ReadFile(rd, &c, 1, &n, nullptr) && n == 1) {
        if (c == '\n') break;
        line += c;
    }
    CloseHandle(rd);
    std::string result;
    if (line.rfind("BOUND", 0) != 0) {
        result = "child did not bind: " + trim(line);
    } else if (kill) {
        if (!TerminateProcess(pi.hProcess, 9)) result = "TerminateProcess failed " + win_err(GetLastError());
        else result = "child bound, TerminateProcess ok";
    } else {
        result = "child bound, exited";
    }
    WaitForSingleObject(pi.hProcess, 30000);
    DWORD code = 0;
    GetExitCodeProcess(pi.hProcess, &code);
    result += fmt(", exit code %lu", (unsigned long)code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return result;
}

// Run this exe with the given mode and argument, capture its first output line,
// kill it if it takes longer than timeout_ms.
static std::string run_child_with_timeout(const wchar_t* mode, const fs::path& arg, DWORD timeout_ms)
{
    SECURITY_ATTRIBUTES sa{sizeof sa, nullptr, TRUE};
    HANDLE rd = nullptr, wr = nullptr;
    if (!CreatePipe(&rd, &wr, &sa, 0)) return "CreatePipe failed " + win_err(GetLastError());
    SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0);
    std::wstring cmd = L"\"" + g_exe + L"\" " + mode + L" \"" + arg.wstring() + L"\"";
    STARTUPINFOW si{};
    si.cb = sizeof si;
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = wr;
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    PROCESS_INFORMATION pi{};
    std::vector<wchar_t> cmdbuf(cmd.begin(), cmd.end());
    cmdbuf.push_back(0);
    if (!CreateProcessW(nullptr, cmdbuf.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi)) {
        std::string e = "CreateProcessW failed " + win_err(GetLastError());
        CloseHandle(rd);
        CloseHandle(wr);
        return e;
    }
    CloseHandle(wr);
    DWORD w = WaitForSingleObject(pi.hProcess, timeout_ms);
    std::string result;
    if (w == WAIT_TIMEOUT) {
        TerminateProcess(pi.hProcess, 9);
        WaitForSingleObject(pi.hProcess, 10000);
        result = fmt("HUNG: no exit after %lu ms, killed", (unsigned long)timeout_ms);
    } else {
        DWORD code = 0;
        GetExitCodeProcess(pi.hProcess, &code);
        result = fmt("exit code %lu", (unsigned long)code);
    }
    // Drain whatever the child printed (non-blocking: the write end is closed now).
    std::string outp;
    char buf[512];
    DWORD n;
    DWORD avail = 0;
    while (PeekNamedPipe(rd, nullptr, 0, nullptr, &avail, nullptr) && avail > 0 && ReadFile(rd, buf, sizeof buf, &n, nullptr) && n > 0) outp.append(buf, n);
    CloseHandle(rd);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    if (!outp.empty()) result += "; output: " + trim(outp);
    return result;
}

// Delete everything in dir with Win32 calls only (fs::remove_all can hang, see part 4).
static std::string win32_cleanup(const fs::path& dir)
{
    std::string s;
    WIN32_FIND_DATAW d{};
    HANDLE h = FindFirstFileW((dir / L"*").c_str(), &d);
    if (h == INVALID_HANDLE_VALUE) return "FindFirstFileW failed " + win_err(GetLastError());
    do {
        if (wcscmp(d.cFileName, L".") == 0 || wcscmp(d.cFileName, L"..") == 0) continue;
        fs::path f = dir / d.cFileName;
        BOOL ok = (d.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? RemoveDirectoryW(f.c_str()) : DeleteFileW(f.c_str());
        if (!ok) s += narrow(d.cFileName) + ": " + win_err(GetLastError()) + "; ";
    } while (FindNextFileW(h, &d));
    FindClose(h);
    if (!RemoveDirectoryW(dir.c_str())) s += "RemoveDirectoryW: " + win_err(GetLastError());
    return s.empty() ? "ok" : s;
}

// ---------------------------------------------------------------- leftover kinds

enum class Leftover { CloseSocket, ChildKilled, ChildExit, LiveBound, EmptyFile, Missing };

static const char* leftover_name(Leftover k)
{
    switch (k) {
    case Leftover::CloseSocket: return "closesocket";
    case Leftover::ChildKilled: return "child-killed";
    case Leftover::ChildExit: return "child-exit";
    case Leftover::LiveBound: return "live-bound";
    case Leftover::EmptyFile: return "empty-file";
    case Leftover::Missing: return "missing";
    }
    return "?";
}

static const char* leftover_desc(Leftover k)
{
    switch (k) {
    case Leftover::CloseSocket: return "this process: socket(AF_UNIX), bind, listen, closesocket";
    case Leftover::ChildKilled: return "child process binds+listens, parent TerminateProcess()es it";
    case Leftover::ChildExit: return "child process binds+listens, then ExitProcess(0) without closesocket";
    case Leftover::LiveBound: return "this process: bind+listen, socket still open (control)";
    case Leftover::EmptyFile: return "ordinary empty regular file (control)";
    case Leftover::Missing: return "path that does not exist (control)";
    }
    return "?";
}

static SOCKET g_live = INVALID_SOCKET;

// Create the leftover at p (which must not exist). Returns a description.
static std::string make_leftover(Leftover k, const fs::path& p)
{
    std::string desc;
    switch (k) {
    case Leftover::CloseSocket: {
        SOCKET s = bind_socket(p, &desc);
        if (s != INVALID_SOCKET) {
            int r = closesocket(s);
            desc += r == 0 ? ", closesocket ok" : ", closesocket failed " + wsa_str(WSAGetLastError());
        }
        return desc;
    }
    case Leftover::ChildKilled: return spawn_child(p, true);
    case Leftover::ChildExit: return spawn_child(p, false);
    case Leftover::LiveBound: {
        if (g_live != INVALID_SOCKET) closesocket(g_live);
        g_live = bind_socket(p, &desc);
        return desc;
    }
    case Leftover::EmptyFile: {
        HANDLE h = CreateFileW(p.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h == INVALID_HANDLE_VALUE) return "CreateFileW failed " + win_err(GetLastError());
        CloseHandle(h);
        return "created";
    }
    case Leftover::Missing: return "nothing created";
    }
    return "?";
}

// ---------------------------------------------------------------- inspection

static std::string inspect_handle(HANDLE h)
{
    std::string s;
    FILE_ATTRIBUTE_TAG_INFO ti{};
    if (GetFileInformationByHandleEx(h, FileAttributeTagInfo, &ti, sizeof ti)) {
        s += "FileAttributeTagInfo: attrs=" + attrs_str(ti.FileAttributes) + " tag=" + tag_str(ti.ReparseTag);
    } else {
        s += "GetFileInformationByHandleEx(FileAttributeTagInfo) failed " + win_err(GetLastError());
    }
    LARGE_INTEGER sz{};
    if (GetFileSizeEx(h, &sz)) s += fmt("; GetFileSizeEx=%lld", (long long)sz.QuadPart);
    else s += "; GetFileSizeEx failed " + win_err(GetLastError());
    BY_HANDLE_FILE_INFORMATION bi{};
    if (GetFileInformationByHandle(h, &bi)) s += fmt("; GetFileInformationByHandle: attrs=%s size=%lu", attrs_str(bi.dwFileAttributes).c_str(), (unsigned long)bi.nFileSizeLow);
    else s += "; GetFileInformationByHandle failed " + win_err(GetLastError());
    return s;
}

static std::string create_file_probe(const fs::path& p, DWORD access, DWORD flags)
{
    HANDLE h = CreateFileW(p.c_str(), access, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, flags, nullptr);
    if (h == INVALID_HANDLE_VALUE) return "FAIL " + win_err(GetLastError());
    std::string s = "ok; " + inspect_handle(h);
    CloseHandle(h);
    return s;
}

static Table inspect(const fs::path& p)
{
    Table t;
    t.add("GetFileAttributesW", [&] {
        DWORD a = GetFileAttributesW(p.c_str());
        if (a == INVALID_FILE_ATTRIBUTES) return "FAIL " + win_err(GetLastError());
        return attrs_str(a);
    });
    t.add("GetFileAttributesExW", [&] {
        WIN32_FILE_ATTRIBUTE_DATA d{};
        if (!GetFileAttributesExW(p.c_str(), GetFileExInfoStandard, &d)) return "FAIL " + win_err(GetLastError());
        return "attrs=" + attrs_str(d.dwFileAttributes) + fmt(" size=%lu", (unsigned long)d.nFileSizeLow);
    });
    t.add("FindFirstFileW", [&] {
        WIN32_FIND_DATAW d{};
        HANDLE h = FindFirstFileW(p.c_str(), &d);
        if (h == INVALID_HANDLE_VALUE) return "FAIL " + win_err(GetLastError());
        FindClose(h);
        std::string s = "attrs=" + attrs_str(d.dwFileAttributes) + fmt(" size=%lu", (unsigned long)d.nFileSizeLow);
        if (d.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) s += " dwReserved0=" + tag_str(d.dwReserved0);
        else s += fmt(" dwReserved0=0x%lx(n/a)", (unsigned long)d.dwReserved0);
        return s;
    });
    t.add("CreateFileW(0, BACKUP_SEMANTICS) [MSVC status() style]", [&] { return create_file_probe(p, 0, FILE_FLAG_BACKUP_SEMANTICS); });
    t.add("CreateFileW(FILE_READ_ATTRIBUTES, BACKUP_SEMANTICS)", [&] { return create_file_probe(p, FILE_READ_ATTRIBUTES, FILE_FLAG_BACKUP_SEMANTICS); });
    t.add("CreateFileW(FILE_READ_ATTRIBUTES, BACKUP_SEMANTICS/OPEN_REPARSE_POINT)", [&] { return create_file_probe(p, FILE_READ_ATTRIBUTES, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT); });
    t.add("CreateFileW(GENERIC_READ, 0)", [&] { return create_file_probe(p, GENERIC_READ, 0); });
    t.add("_wstat64", [&] {
        struct __stat64 st{};
        errno = 0;
        int r = _wstat64(p.c_str(), &st);
        if (r != 0) return fmt("ret=%d ", r) + errno_str(errno);
        return fmt("ret=0 st_mode=0%o%s%s st_size=%lld", (unsigned)st.st_mode,
                   (st.st_mode & _S_IFREG) ? " S_IFREG" : "", (st.st_mode & _S_IFDIR) ? " S_IFDIR" : "",
                   (long long)st.st_size);
    });
    // _wstat64 resolved at run time from each CRT DLL, independent of which CRT
    // this binary was linked against (static UCRT, ucrtbase.dll or msvcrt.dll).
    for (const wchar_t* dll : {L"ucrtbase.dll", L"api-ms-win-crt-filesystem-l1-1-0.dll", L"msvcrt.dll"}) {
        t.add("_wstat64 via GetProcAddress(" + narrow(dll) + ")", [&, dll] {
            HMODULE m = LoadLibraryW(dll);
            if (!m) return "LoadLibraryW failed " + win_err(GetLastError());
            typedef int(__cdecl * Fn)(const wchar_t*, struct __stat64*);
            Fn fn = (Fn)(void*)GetProcAddress(m, "_wstat64");
            if (!fn) return "GetProcAddress failed " + win_err(GetLastError());
            struct __stat64 st{};
            int r = fn(p.c_str(), &st);
            if (r != 0) {
                // errno lives in the DLL's CRT, not necessarily ours; read it there.
                typedef int*(__cdecl * ErrnoFn)();
                ErrnoFn ef = (ErrnoFn)(void*)GetProcAddress(m, "_errno");
                int e = ef ? *ef() : -1;
                return fmt("ret=%d ", r) + (e >= 0 ? errno_str(e) : std::string("errno=? (no _errno export)")) + fmt(" GetLastError=%lu", (unsigned long)GetLastError());
            }
            return fmt("ret=0 st_mode=0%o%s%s st_size=%lld", (unsigned)st.st_mode,
                       (st.st_mode & _S_IFREG) ? " S_IFREG" : "", (st.st_mode & _S_IFDIR) ? " S_IFDIR" : "",
                       (long long)st.st_size);
        });
    }
    t.add("_waccess(0)", [&] {
        errno = 0;
        int r = _waccess(p.c_str(), 0);
        return r == 0 ? std::string("ret=0") : fmt("ret=%d ", r) + errno_str(errno);
    });
    // std::filesystem
    t.add("fs::symlink_status(p)", [&] { return status_str(fs::symlink_status(p)); });
    t.add("fs::symlink_status(p, ec)", [&] { std::error_code ec; auto st = fs::symlink_status(p, ec); return status_str(st) + " " + ec_str(ec); });
    t.add("fs::status(p)", [&] { return status_str(fs::status(p)); });
    t.add("fs::status(p, ec)", [&] { std::error_code ec; auto st = fs::status(p, ec); return status_str(st) + " " + ec_str(ec); });
    t.add("fs::exists(p)", [&] { return std::string(fs::exists(p) ? "true" : "false"); });
    t.add("fs::exists(p, ec)", [&] { std::error_code ec; bool b = fs::exists(p, ec); return std::string(b ? "true " : "false ") + ec_str(ec); });
    t.add("fs::is_regular_file(p)", [&] { return std::string(fs::is_regular_file(p) ? "true" : "false"); });
    t.add("fs::is_regular_file(p, ec)", [&] { std::error_code ec; bool b = fs::is_regular_file(p, ec); return std::string(b ? "true " : "false ") + ec_str(ec); });
    t.add("fs::is_regular_file(fs::symlink_status(p, ec))", [&] { std::error_code ec; bool b = fs::is_regular_file(fs::symlink_status(p, ec)); return std::string(b ? "true " : "false ") + ec_str(ec); });
    t.add("fs::is_symlink(p, ec)", [&] { std::error_code ec; bool b = fs::is_symlink(p, ec); return std::string(b ? "true " : "false ") + ec_str(ec); });
    t.add("fs::is_socket(p, ec)", [&] { std::error_code ec; bool b = fs::is_socket(p, ec); return std::string(b ? "true " : "false ") + ec_str(ec); });
    t.add("fs::is_other(p, ec)", [&] { std::error_code ec; bool b = fs::is_other(p, ec); return std::string(b ? "true " : "false ") + ec_str(ec); });
    t.add("fs::is_directory(p, ec)", [&] { std::error_code ec; bool b = fs::is_directory(p, ec); return std::string(b ? "true " : "false ") + ec_str(ec); });
    t.add("fs::file_size(p)", [&] { return fmt("%llu", (unsigned long long)fs::file_size(p)); });
    t.add("fs::file_size(p, ec)", [&] { std::error_code ec; auto n = fs::file_size(p, ec); return fmt("%llu ", (unsigned long long)n) + ec_str(ec); });
    t.add("fs::last_write_time(p, ec)", [&] { std::error_code ec; auto ft = fs::last_write_time(p, ec); (void)ft; return ec_str(ec); });
    t.add("directory_iterator entry", [&] {
        std::error_code ec;
        for (fs::directory_iterator it(p.parent_path(), ec); !ec && it != fs::directory_iterator(); it.increment(ec)) {
            if (it->path().filename() != p.filename()) continue;
            std::error_code e1, e2, e3, e4;
            auto ss = it->symlink_status(e1);
            auto st = it->status(e2);
            bool reg = it->is_regular_file(e3);
            auto sz = it->file_size(e4);
            return fmt("found; symlink_status=%s %s; status=%s %s; is_regular_file=%s %s; file_size=%llu %s",
                       status_str(ss).c_str(), ec_str(e1).c_str(), status_str(st).c_str(), ec_str(e2).c_str(),
                       reg ? "true" : "false", ec_str(e3).c_str(), (unsigned long long)sz, ec_str(e4).c_str());
        }
        return "not found in parent " + ec_str(ec);
    });
    // The exact expression from bitcoin ProcessImpl::bind (with the WIN32 branch).
    t.add("OLD CHECK: symlink_status(p).type()==socket || (is_regular_file(p) && file_size(p)==0)", [&] {
        std::string trace;
        fs::file_type ty;
        try {
            ty = fs::symlink_status(p).type();
        } catch (const fs::filesystem_error& e) {
            return fmt("THROW in symlink_status(p): what=\"%s\" %s", trim(e.what()).c_str(), ec_str(e.code()).c_str());
        }
        trace += fmt("symlink_status=%s", type_str(ty));
        if (ty == fs::file_type::socket) return "TRUE via symlink_status==socket [" + trace + "]";
        bool reg;
        try {
            reg = fs::is_regular_file(p);
        } catch (const fs::filesystem_error& e) {
            return fmt("THROW in is_regular_file(p): what=\"%s\" %s [%s]", trim(e.what()).c_str(), ec_str(e.code()).c_str(), trace.c_str());
        }
        trace += fmt(", is_regular_file=%s", reg ? "true" : "false");
        if (!reg) return "FALSE [" + trace + "]";
        unsigned long long sz;
        try {
            sz = fs::file_size(p);
        } catch (const fs::filesystem_error& e) {
            return fmt("THROW in file_size(p): what=\"%s\" %s [%s]", trim(e.what()).c_str(), ec_str(e.code()).c_str(), trace.c_str());
        }
        trace += fmt(", file_size=%llu", sz);
        return std::string(sz == 0 ? "TRUE via regular&&size0 [" : "FALSE [") + trace + "]";
    });
    t.add("ALT CHECK A: ec-form symlink_status type in {socket,regular,unknown} (no follow)", [&] {
        std::error_code ec;
        auto st = fs::symlink_status(p, ec);
        auto ty = st.type();
        bool hit = ty == fs::file_type::socket || ty == fs::file_type::regular || ty == fs::file_type::unknown;
        return fmt("%s [symlink_status=%s %s]", hit ? "TRUE" : "FALSE", type_str(ty), ec_str(ec).c_str());
    });
    t.add("ALT CHECK B: fs::exists(fs::symlink_status(p, ec))", [&] {
        std::error_code ec;
        bool b = fs::exists(fs::symlink_status(p, ec));
        return std::string(b ? "TRUE " : "FALSE ") + ec_str(ec);
    });
    return t;
}

// ---------------------------------------------------------------- removal

enum class Removal { None, FsRemove, FsRemoveEc, DeleteFile, WRemove, Candidate };

static const char* removal_name(Removal r)
{
    switch (r) {
    case Removal::None: return "(no removal)";
    case Removal::FsRemove: return "fs::remove(p)";
    case Removal::FsRemoveEc: return "fs::remove(p, ec)";
    case Removal::DeleteFile: return "DeleteFileW(p)";
    case Removal::WRemove: return "_wremove(p)";
    case Removal::Candidate: return "candidate: FindFirstFileW tag==AF_UNIX then DeleteFileW";
    }
    return "?";
}

static std::string do_removal(Removal r, const fs::path& p)
{
    switch (r) {
    case Removal::None: return "-";
    case Removal::FsRemove: return guarded([&] { return std::string(fs::remove(p) ? "true" : "false"); });
    case Removal::FsRemoveEc: return guarded([&] { std::error_code ec; bool b = fs::remove(p, ec); return std::string(b ? "true " : "false ") + ec_str(ec); });
    case Removal::DeleteFile: {
        if (DeleteFileW(p.c_str())) return "ok";
        return "FAIL " + win_err(GetLastError());
    }
    case Removal::WRemove: {
        errno = 0;
        int rc = _wremove(p.c_str());
        return rc == 0 ? std::string("ret=0") : fmt("ret=%d ", rc) + errno_str(errno);
    }
    case Removal::Candidate: {
        // The IsSocketFile() + DeleteFileW() code from bitcoin pr/ipc-win.
        WIN32_FIND_DATAW data;
        HANDLE find{FindFirstFileW(p.c_str(), &data)};
        if (find == INVALID_HANDLE_VALUE) return "FindFirstFileW failed " + win_err(GetLastError()) + " -> not a socket, skipped";
        FindClose(find);
        bool is_sock = (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) && data.dwReserved0 == IO_REPARSE_TAG_AF_UNIX;
        if (!is_sock) return "not a socket (" + attrs_str(data.dwFileAttributes) + "), skipped";
        if (DeleteFileW(p.c_str())) return "is socket, DeleteFileW ok";
        return "is socket, DeleteFileW FAIL " + win_err(GetLastError());
    }
    }
    return "?";
}

static std::string exists_now(const fs::path& p)
{
    DWORD a = GetFileAttributesW(p.c_str());
    if (a == INVALID_FILE_ATTRIBUTES) {
        DWORD e = GetLastError();
        if (e == ERROR_FILE_NOT_FOUND || e == ERROR_PATH_NOT_FOUND) return "gone";
        return "GetFileAttributesW " + win_err(e);
    }
    return "still exists (" + attrs_str(a) + ")";
}

// ---------------------------------------------------------------- environment

static std::string os_version()
{
    typedef LONG(WINAPI * RtlGetVersionFn)(PRTL_OSVERSIONINFOW);
    RTL_OSVERSIONINFOW vi{};
    vi.dwOSVersionInfoSize = sizeof vi;
    HMODULE nt = GetModuleHandleW(L"ntdll.dll");
    auto fn = nt ? (RtlGetVersionFn)(void*)GetProcAddress(nt, "RtlGetVersion") : nullptr;
    if (fn && fn(&vi) == 0) return fmt("%lu.%lu build %lu", (unsigned long)vi.dwMajorVersion, (unsigned long)vi.dwMinorVersion, (unsigned long)vi.dwBuildNumber);
    return "unknown";
}

static std::string compiler_desc()
{
    std::string s;
#if defined(_MSC_FULL_VER) && !defined(__GNUC__)
    s += fmt("MSVC _MSC_FULL_VER=%d _MSC_VER=%d", (int)_MSC_FULL_VER, (int)_MSC_VER);
#ifdef _MSVC_STL_UPDATE
    s += fmt(" _MSVC_STL_UPDATE=%d", (int)_MSVC_STL_UPDATE);
#endif
#ifdef _MSVC_STL_VERSION
    s += fmt(" _MSVC_STL_VERSION=%d", (int)_MSVC_STL_VERSION);
#endif
#endif
#if defined(__GNUC__) && !defined(__clang__)
    s += fmt("GCC %d.%d.%d", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#ifdef __MINGW64_VERSION_STR
    s += fmt(" mingw-w64 %s", __MINGW64_VERSION_STR);
#endif
#ifdef _GLIBCXX_RELEASE
    s += fmt(" libstdc++ release %d (__GLIBCXX__=%ld)", (int)_GLIBCXX_RELEASE, (long)__GLIBCXX__);
#endif
#endif
#ifdef __clang__
    s += fmt(" clang %s", __clang_version__);
#endif
#ifdef _UCRT
    s += " CRT=UCRT(_UCRT defined)";
#else
#ifdef __MSVCRT_VERSION__
    s += fmt(" CRT=msvcrt(__MSVCRT_VERSION__=0x%x)", (unsigned)__MSVCRT_VERSION__);
#else
    s += " CRT=UCRT(MSVC)";
#endif
#endif
    s += fmt(" afunix.h=%s", PROBE_HAVE_AFUNIX_H ? "yes" : "no");
    s += fmt(" __cplusplus=%ld", (long)__cplusplus);
    return s;
}

static std::string dll_version(const wchar_t* name)
{
    wchar_t sysdir[MAX_PATH];
    GetSystemDirectoryW(sysdir, MAX_PATH);
    std::wstring path = std::wstring(sysdir) + L"\\" + name;
    DWORD dummy = 0;
    DWORD n = GetFileVersionInfoSizeW(path.c_str(), &dummy);
    if (!n) return narrow(name) + ": " + win_err(GetLastError());
    std::vector<char> buf(n);
    if (!GetFileVersionInfoW(path.c_str(), 0, n, buf.data())) return narrow(name) + ": " + win_err(GetLastError());
    VS_FIXEDFILEINFO* ffi = nullptr;
    UINT len = 0;
    if (!VerQueryValueW(buf.data(), L"\\", (void**)&ffi, &len) || !ffi) return narrow(name) + ": no VS_FIXEDFILEINFO";
    return narrow(name) + fmt(" %u.%u.%u.%u", HIWORD(ffi->dwFileVersionMS), LOWORD(ffi->dwFileVersionMS), HIWORD(ffi->dwFileVersionLS), LOWORD(ffi->dwFileVersionLS));
}

static std::string loaded_crts()
{
    std::string s;
    for (const wchar_t* n : {L"msvcrt.dll", L"ucrtbase.dll", L"vcruntime140.dll", L"msvcp140.dll", L"libstdc++-6.dll", L"libgcc_s_seh-1.dll"}) {
        if (GetModuleHandleW(n)) { s += narrow(n); s += " "; }
    }
    return s.empty() ? "(none of msvcrt/ucrtbase/vcruntime140/msvcp140/libstdc++ loaded)" : s;
}

// ---------------------------------------------------------------- main

static void print_table(const Table& t)
{
    out("| check | result |");
    out("|---|---|");
    for (auto& r : t.rows) out("| " + r.first + " | " + r.second + " |");
    out("");
}

int main(int argc, char** argv)
{
    if (argc == 3 && (strcmp(argv[1], "--child-sleep") == 0 || strcmp(argv[1], "--child-exit") == 0 || strcmp(argv[1], "--child-remove-all") == 0)) {
        return child_main(argv[1], argv[2]);
    }
    const char* outfile = argc >= 2 ? argv[1] : nullptr;

    wchar_t exe[MAX_PATH];
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    g_exe = exe;

    WSADATA wd;
    int wsa = WSAStartup(MAKEWORD(2, 2), &wd);

    wchar_t tmp[MAX_PATH];
    GetTempPathW(MAX_PATH, tmp);
    fs::path base = fs::path(tmp) / fmt("afunix-probe-%lu", (unsigned long)GetCurrentProcessId());
    std::error_code ec;
    fs::create_directories(base, ec);

    out("# AF_UNIX stale socket file probe");
    out("");
    out("| item | value |");
    out("|---|---|");
    out("| compiler | " + compiler_desc() + " |");
    out("| CRT DLLs loaded | " + loaded_crts() + " |");
    out("| system CRT DLL versions | " + dll_version(L"ucrtbase.dll") + "; " + dll_version(L"msvcrt.dll") + " |");
    out("| Windows | " + os_version() + " |");
    out(fmt("| WSAStartup | %d |", wsa));
    out("| base dir | " + base.string() + " |");
    {
        wchar_t fsname[64] = L"";
        wchar_t root[MAX_PATH];
        GetVolumePathNameW(base.c_str(), root, MAX_PATH);
        GetVolumeInformationW(root, nullptr, 0, nullptr, nullptr, nullptr, fsname, 64);
        out("| volume filesystem | " + narrow(fsname) + " |");
    }
    out("");

    int counter = 0;
    auto fresh = [&](const char* tag) { return base / fmt("%s-%d.sock", tag, counter++); };

    // Part 1: inspection of each kind of leftover.
    out("## 1. Inspection of leftover paths");
    out("");
    for (Leftover k : {Leftover::Missing, Leftover::EmptyFile, Leftover::LiveBound, Leftover::CloseSocket, Leftover::ChildKilled, Leftover::ChildExit}) {
        fs::path p = fresh(leftover_name(k));
        out(fmt("### 1.%s: %s", leftover_name(k), leftover_desc(k)));
        out("");
        out("setup: " + make_leftover(k, p) + "  ");
        out("path: `" + p.string() + "`");
        out("");
        print_table(inspect(p));
        if (k == Leftover::LiveBound) {
            out("rebind while still bound: " + guarded([&] { std::string d; SOCKET s = bind_socket(p, &d, false); if (s != INVALID_SOCKET) closesocket(s); return d; }));
            out("");
            if (g_live != INVALID_SOCKET) { closesocket(g_live); g_live = INVALID_SOCKET; }
        }
    }

    // Part 2: removal + rebind, each on a fresh leftover.
    out("## 2. Removal and rebind");
    out("");
    out("Each cell: removal result; whether the path still exists afterwards; result of a new bind() to the same path (then listen; the new socket is closed and the file deleted before the next cell).");
    out("");
    Leftover kinds[] = {Leftover::CloseSocket, Leftover::ChildKilled, Leftover::ChildExit, Leftover::EmptyFile};
    std::string hdr = "| removal |";
    std::string sep = "|---|";
    for (Leftover k : kinds) { hdr += std::string(" ") + leftover_name(k) + " |"; sep += "---|"; }
    out(hdr);
    out(sep);
    for (Removal r : {Removal::None, Removal::FsRemove, Removal::FsRemoveEc, Removal::DeleteFile, Removal::WRemove, Removal::Candidate}) {
        std::string line = std::string("| ") + removal_name(r) + " |";
        for (Leftover k : kinds) {
            fs::path p = fresh(leftover_name(k));
            std::string setup = make_leftover(k, p);
            std::string cell;
            if (setup.find("fail") != std::string::npos || setup.find("not bind") != std::string::npos) {
                cell = "SETUP FAILED: " + setup;
            } else {
                // Separate statements: operands of + are unsequenced, and MSVC
                // evaluated exists_now() before do_removal() in round 1.
                std::string rem = do_removal(r, p);
                std::string after = exists_now(p);
                std::string d;
                SOCKET s = bind_socket(p, &d, true);
                if (s != INVALID_SOCKET) closesocket(s);
                cell = "remove: " + rem + "; then: " + after + "; rebind: " + d;
            }
            // cleanup for next cell
            DeleteFileW(p.c_str());
            line += " " + cell + " |";
        }
        out(line);
    }
    out("");

    // Part 3: does a leftover survive across many create/remove cycles (sanity), and
    // does removing via DeleteFileW then rebinding leave the same kind of file?
    out("## 3. Rebind after DeleteFileW, repeated 3x on one path");
    out("");
    {
        fs::path p = fresh("cycle");
        std::string s;
        for (int i = 0; i < 3; ++i) {
            std::string d;
            SOCKET sk = bind_socket(p, &d, true);
            s += fmt("round %d: %s", i, d.c_str());
            if (sk != INVALID_SOCKET) closesocket(sk);
            s += std::string("; DeleteFileW: ") + (DeleteFileW(p.c_str()) ? "ok" : "FAIL " + win_err(GetLastError()));
            s += "; " + exists_now(p) + "  ";
            out(s);
            s.clear();
        }
        out("");
    }

    // Part 4: fs::remove_all on a directory holding a stale socket file, in a
    // child process with a timeout (round 1 hung here on windows-2022 + UCRT).
    out("## 4. fs::remove_all(dir) with a stale socket file inside (child process, 20 s timeout)");
    out("");
    {
        fs::path dir = base / "rmall";
        fs::create_directories(dir, ec);
        std::string setup = make_leftover(Leftover::CloseSocket, dir / "stale.sock");
        out("setup: " + setup + "  ");
        out("result: " + run_child_with_timeout(L"--child-remove-all", dir, 20000) + "  ");
        out("dir afterwards: " + exists_now(dir) + "  ");
        out("socket afterwards: " + exists_now(dir / "stale.sock"));
        out("");
        win32_cleanup(dir);
    }

    out(fmt("cleanup (Win32): %s", win32_cleanup(base).c_str()));

    if (outfile) {
        FILE* f = fopen(outfile, "wb");
        if (f) {
            fwrite(g_out.data(), 1, g_out.size(), f);
            fclose(f);
        }
    }
    return 0;
}
