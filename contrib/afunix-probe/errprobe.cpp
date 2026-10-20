// Error-string probe for ProcessImpl::connect on Windows (see README.md).
//
//   errprobe.exe [RESULTS.md]
//
// Prints, per toolchain: the exact what()/message() strings for
// std::errc::connection_refused and no_such_file_or_directory, the
// system_category string for WSAECONNREFUSED and whether it compares equal to
// errc::connection_refused, GetFileAttributesW results for a stale AF_UNIX
// socket file / a missing file / paths whose parent directory is missing, and
// the WSA error from connect() to each of those paths.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <afunix.h>

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <system_error>

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
    g_out += s + "\n";
    fputs((s + "\n").c_str(), stdout);
    fflush(stdout);
}

// Quote a string so whitespace, case and line endings are visible in markdown.
static std::string q(const std::string& s)
{
    std::string r = "`\"";
    for (unsigned char c : s) {
        if (c == '\r') r += "\\r";
        else if (c == '\n') r += "\\n";
        else if (c == '\t') r += "\\t";
        else if (c == '|') r += "\\|";
        else if (c == '`') r += "'";
        else if (c < 0x20 || c == 0x7f) r += fmt("\\x%02x", c);
        else r += (char)c;
    }
    return r + "\"`";
}

static std::string narrow(const std::wstring& w)
{
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), s.data(), n, nullptr, nullptr);
    return s;
}

static std::string compiler_desc()
{
    std::string s;
#if defined(_MSC_FULL_VER) && !defined(__GNUC__)
    s += fmt("MSVC _MSC_FULL_VER=%d", (int)_MSC_FULL_VER);
#ifdef _MSVC_STL_UPDATE
    s += fmt(" STL %d", (int)_MSVC_STL_UPDATE);
#endif
#ifdef _DLL
    s += " /MD";
#else
    s += " /MT";
#endif
#endif
#if defined(__GNUC__) && !defined(__clang__)
    s += fmt("GCC %d.%d.%d", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#ifdef __MINGW64_VERSION_STR
    s += fmt(" mingw-w64 %s", __MINGW64_VERSION_STR);
#endif
#ifdef _GLIBCXX_RELEASE
    s += fmt(" libstdc++ %d", (int)_GLIBCXX_RELEASE);
#endif
#endif
#ifdef _UCRT
    s += " UCRT";
#elif defined(__MSVCRT_VERSION__)
    s += " msvcrt";
#endif
    return s;
}

static std::string os_version()
{
    typedef LONG(WINAPI * RtlGetVersionFn)(PRTL_OSVERSIONINFOW);
    RTL_OSVERSIONINFOW vi{};
    vi.dwOSVersionInfoSize = sizeof vi;
    HMODULE nt = GetModuleHandleW(L"ntdll.dll");
    auto fn = nt ? (RtlGetVersionFn)(void*)GetProcAddress(nt, "RtlGetVersion") : nullptr;
    if (fn && fn(&vi) == 0) return fmt("build %lu", (unsigned long)vi.dwBuildNumber);
    return "unknown";
}

static std::string attrs(const std::wstring& p)
{
    SetLastError(0);
    DWORD a = GetFileAttributesW(p.c_str());
    DWORD e = GetLastError();
    if (a == INVALID_FILE_ATTRIBUTES) return fmt("INVALID_FILE_ATTRIBUTES, GetLastError=%lu", (unsigned long)e);
    return fmt("0x%lx, GetLastError=%lu", (unsigned long)a, (unsigned long)e);
}

static const char* wsa_name(int e)
{
    switch (e) {
    case 0: return "0";
    case WSAECONNREFUSED: return "WSAECONNREFUSED";
    case WSAEADDRINUSE: return "WSAEADDRINUSE";
    case WSAEADDRNOTAVAIL: return "WSAEADDRNOTAVAIL";
    case WSAEINVAL: return "WSAEINVAL";
    case WSAEACCES: return "WSAEACCES";
    case WSAENETUNREACH: return "WSAENETUNREACH";
    case WSAEHOSTUNREACH: return "WSAEHOSTUNREACH";
    case WSAETIMEDOUT: return "WSAETIMEDOUT";
    case WSAEFAULT: return "WSAEFAULT";
    case WSAEAFNOSUPPORT: return "WSAEAFNOSUPPORT";
    case WSAENOTSOCK: return "WSAENOTSOCK";
    default: return "?";
    }
}

static std::string connect_to(const std::string& path)
{
    if (path.size() >= UNIX_PATH_MAX) return "path too long";
    SOCKET s = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) return fmt("socket() failed %d", WSAGetLastError());
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    memcpy(addr.sun_path, path.c_str(), path.size() + 1);
    std::string r;
    if (::connect(s, (sockaddr*)&addr, sizeof addr) == 0) {
        r = "connect() ok (live listener)";
    } else {
        int e = WSAGetLastError();
        r = fmt("connect() failed %d=%s", e, wsa_name(e));
    }
    closesocket(s);
    return r;
}

static SOCKET bind_at(const std::string& path, std::string* err)
{
    SOCKET s = ::socket(AF_UNIX, SOCK_STREAM, 0);
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    memcpy(addr.sun_path, path.c_str(), path.size() + 1);
    if (::bind(s, (sockaddr*)&addr, sizeof addr) != 0 || ::listen(s, 1) != 0) {
        *err = fmt("bind/listen failed %d", WSAGetLastError());
        closesocket(s);
        return INVALID_SOCKET;
    }
    return s;
}

int main(int argc, char** argv)
{
    WSADATA wd;
    WSAStartup(MAKEWORD(2, 2), &wd);

    out("# errprobe: " + compiler_desc() + ", Windows " + os_version());
    out("");
    out("| item | value |");
    out("|---|---|");

    // 1, 2: std::errc strings
    for (auto [name, e] : {std::pair{"connection_refused", std::errc::connection_refused}, std::pair{"no_such_file_or_directory", std::errc::no_such_file_or_directory}}) {
        std::error_code ec = std::make_error_code(e);
        out(fmt("| make_error_code(errc::%s).message() | ", name) + q(ec.message()) + fmt(" (value %d, category %s) |", ec.value(), ec.category().name()));
        out(fmt("| system_error(make_error_code(errc::%s)).what() | ", name) + q(std::system_error(ec).what()) + " |");
    }
    // 3: WSAECONNREFUSED through system_category
    {
        std::error_code ec(WSAECONNREFUSED, std::system_category());
        std::error_condition cond = ec.default_error_condition();
        out("| system_error(WSAECONNREFUSED, system_category()).what() | " + q(std::system_error(ec).what()) + " |");
        out("| error_code(WSAECONNREFUSED, system_category()) == errc::connection_refused | " + std::string(ec == std::errc::connection_refused ? "true" : "false") + fmt(" (default_error_condition: value %d, category %s) |", cond.value(), cond.category().name()));
        out("| system_category().message(WSAECONNREFUSED) | " + q(std::system_category().message(WSAECONNREFUSED)) + " |");
        std::error_code ec2(ERROR_FILE_NOT_FOUND, std::system_category());
        out("| error_code(ERROR_FILE_NOT_FOUND, system_category()) == errc::no_such_file_or_directory | " + std::string(ec2 == std::errc::no_such_file_or_directory ? "true" : "false") + " |");
        out("| system_error(ERROR_FILE_NOT_FOUND, system_category()).what() | " + q(std::system_error(ec2).what()) + " |");
    }

    // 4, 5: paths
    wchar_t tmpw[MAX_PATH];
    GetTempPathW(MAX_PATH, tmpw);
    std::wstring base = std::wstring(tmpw) + L"errprobe-" + std::to_wstring(GetCurrentProcessId());
    CreateDirectoryW(base.c_str(), nullptr);
    std::string base8 = narrow(base);

    std::string stale = base8 + "\\stale.sock";
    std::string live = base8 + "\\live.sock";
    std::string regular = base8 + "\\regular.txt";
    std::string missing = base8 + "\\missing.sock";
    std::string noparent_bs = "C:\\var\\empty\\notexist\\x.sock";
    std::string noparent_fs = "/var/empty/notexist/test_bitcoin.sock";
    std::string noparent_fs_drive = "C:/var/empty/notexist/test_bitcoin.sock";

    std::string err;
    SOCKET s = bind_at(stale, &err);
    std::string stale_setup = s == INVALID_SOCKET ? err : "bind+listen+closesocket";
    if (s != INVALID_SOCKET) closesocket(s);
    SOCKET ls = bind_at(live, &err);
    std::string live_setup = ls == INVALID_SOCKET ? err : "bind+listen, still open";
    {
        HANDLE h = CreateFileW((base + L"\\regular.txt").c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
    }
    out("| GetFileAttributesW C:\\var\\empty exists? | " + attrs(L"C:\\var\\empty") + " |");
    out("| GetFileAttributesW C:\\var exists? | " + attrs(L"C:\\var") + " |");
    out("");

    out("| path | setup | GetFileAttributesW | connect() |");
    out("|---|---|---|---|");
    struct Row { const char* name; std::string path; std::string setup; } rows[] = {
        {"stale AF_UNIX socket file", stale, stale_setup},
        {"live listener", live, live_setup},
        {"empty regular file", regular, "created"},
        {"missing file, existing dir", missing, "-"},
        {"parent dir missing, backslashes", noparent_bs, "-"},
        {"parent dir missing, forward slashes, no drive", noparent_fs, "-"},
        {"parent dir missing, forward slashes, with drive", noparent_fs_drive, "-"},
    };
    for (auto& r : rows) {
        std::wstring wp(r.path.begin(), r.path.end());
        out(fmt("| %s `%s` | %s | ", r.name, r.path.c_str(), r.setup.c_str()) + attrs(wp) + " | " + connect_to(r.path) + " |");
    }
    out("");

    if (ls != INVALID_SOCKET) closesocket(ls);
    for (auto& p : {stale, live, regular}) DeleteFileW(std::wstring(p.begin(), p.end()).c_str());
    RemoveDirectoryW(base.c_str());

    if (argc >= 2) {
        FILE* f = fopen(argv[1], "wb");
        if (f) {
            fwrite(g_out.data(), 1, g_out.size(), f);
            fclose(f);
        }
    }
    return 0;
}
