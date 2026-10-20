// Throwaway probe: how do std::filesystem and Win32 APIs see a stale AF_UNIX
// socket file on Windows? See .github/workflows/afunix-probe-o55.yml.
//
// Usage: probe.exe [LABEL]          run all scenarios, print markdown tables
//        probe.exe child PATH MODE  (internal) bind PATH, then MODE=exit|kill

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#if __has_include(<afunix.h>)
#include <afunix.h>
#else
#define UNIX_PATH_MAX 108
struct sockaddr_un { ADDRESS_FAMILY sun_family; char sun_path[UNIX_PATH_MAX]; };
#endif
#include <sys/stat.h>
#include <io.h>
#include <winver.h>
#include <stdio.h>

#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <system_error>
#include <vector>

#ifndef IO_REPARSE_TAG_AF_UNIX
#define IO_REPARSE_TAG_AF_UNIX 0x80000023L
#endif

namespace fs = std::filesystem;

static std::string hex(unsigned long v)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "0x%lx", v);
    return buf;
}

static std::string win_err_msg(DWORD err)
{
    char buf[512] = {0};
    DWORD n = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, err, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), buf, sizeof(buf), nullptr);
    std::string s(buf, n);
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ' || s.back() == '.')) s.pop_back();
    return s;
}

static std::string clean(std::string s)
{
    for (char& c : s) if (c == '|' || c == '\n' || c == '\r') c = ' ';
    return s;
}

static std::string ec_str(const std::error_code& ec)
{
    if (!ec) return "ec=0";
    return "ec=" + std::to_string(ec.value()) + "/" + ec.category().name() + " \"" + clean(ec.message()) + "\"";
}

static const char* ftype(fs::file_type t)
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
    default: return "other";
    }
}

// Run a probe and turn any exception into a description.
static std::string cell(const std::function<std::string()>& f)
{
    try {
        return f();
    } catch (const fs::filesystem_error& e) {
        return "THROW filesystem_error \"" + clean(e.what()) + "\" " + ec_str(e.code());
    } catch (const std::system_error& e) {
        return "THROW system_error \"" + clean(e.what()) + "\" " + ec_str(e.code());
    } catch (const std::exception& e) {
        return "THROW \"" + clean(e.what()) + "\"";
    } catch (...) {
        return "THROW (unknown)";
    }
}

static std::wstring widen(const std::string& s)
{
    int n = MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(n ? n - 1 : 0, L'\0');
    if (n) MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, w.data(), n);
    return w;
}

static std::string attr_str(DWORD a)
{
    if (a == INVALID_FILE_ATTRIBUTES) {
        DWORD e = GetLastError();
        return "INVALID err=" + std::to_string(e) + " \"" + win_err_msg(e) + "\"";
    }
    std::string s = hex(a);
    struct { DWORD f; const char* n; } flags[] = {
        {FILE_ATTRIBUTE_READONLY, "RO"}, {FILE_ATTRIBUTE_HIDDEN, "HID"}, {FILE_ATTRIBUTE_SYSTEM, "SYS"},
        {FILE_ATTRIBUTE_DIRECTORY, "DIR"}, {FILE_ATTRIBUTE_ARCHIVE, "ARCH"}, {FILE_ATTRIBUTE_NORMAL, "NORMAL"},
        {FILE_ATTRIBUTE_TEMPORARY, "TEMP"}, {FILE_ATTRIBUTE_SPARSE_FILE, "SPARSE"},
        {FILE_ATTRIBUTE_REPARSE_POINT, "REPARSE"}, {FILE_ATTRIBUTE_COMPRESSED, "COMPR"},
        {FILE_ATTRIBUTE_NOT_CONTENT_INDEXED, "NCI"},
    };
    std::string names;
    for (auto& f : flags) if (a & f.f) names += (names.empty() ? "" : "+") + std::string(f.n);
    return s + " (" + names + ")";
}

static int wsa_bind(const std::string& path, SOCKET* out_sock, bool do_listen)
{
    SOCKET s = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) return -WSAGetLastError();
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);
    if (::bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        int e = WSAGetLastError();
        closesocket(s);
        return e;
    }
    if (do_listen && ::listen(s, 5) != 0) {
        int e = WSAGetLastError();
        closesocket(s);
        return 100000 + e;
    }
    *out_sock = s;
    return 0;
}

static std::string bind_result(int r)
{
    if (r == 0) return "OK";
    if (r < 0) return "socket() failed WSA " + std::to_string(-r);
    if (r >= 100000) return "listen failed WSA " + std::to_string(r - 100000);
    return "FAIL WSA " + std::to_string(r) + " \"" + win_err_msg(r) + "\"";
}

static int child_main(const std::string& path, const std::string& mode)
{
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    SOCKET s;
    int r = wsa_bind(path, &s, true);
    if (mode == "kill") Sleep(INFINITE);
    // Exit without closesocket() or WSACleanup().
    ExitProcess(r == 0 ? 0 : 1);
}

static std::string g_exe;
static std::string g_root;
static int g_counter = 0;

// Run this executable as a child that binds PATH then exits or gets killed.
static std::string run_child(const std::string& path, const std::string& mode)
{
    std::string cmd = "\"" + g_exe + "\" child \"" + path + "\" " + mode;
    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    if (!CreateProcessA(nullptr, cmd.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
        return "CreateProcess failed " + std::to_string(GetLastError());
    }
    std::string note;
    if (mode == "kill") {
        std::wstring w = widen(path);
        for (int i = 0; i < 500 && GetFileAttributesW(w.c_str()) == INVALID_FILE_ATTRIBUTES; ++i) Sleep(10);
        Sleep(200);
        TerminateProcess(pi.hProcess, 9);
    }
    WaitForSingleObject(pi.hProcess, 10000);
    DWORD code = 0;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    Sleep(100);
    return "child exit code " + std::to_string(code);
}

struct Leftover {
    std::string path;
    SOCKET open_sock = INVALID_SOCKET; // for the "open" scenario
    std::string note;
};

static const std::vector<std::string> SCENARIOS = {"closed", "child-exit", "child-killed", "still-open", "empty-regular-file", "missing"};

// Create a fresh leftover socket path for SCENARIO.
static Leftover make_leftover(const std::string& scenario)
{
    Leftover l;
    std::string dir = g_root + "\\" + scenario + "-" + std::to_string(++g_counter);
    CreateDirectoryA(dir.c_str(), nullptr);
    l.path = dir + "\\ipcinit.sock";
    if (scenario == "closed" || scenario == "still-open") {
        SOCKET s;
        int r = wsa_bind(l.path, &s, true);
        l.note = "bind " + bind_result(r);
        if (r == 0) {
            if (scenario == "closed") {
                closesocket(s);
            } else {
                l.open_sock = s;
            }
        }
    } else if (scenario == "empty-regular-file") {
        HANDLE h = CreateFileA(l.path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        l.note = h == INVALID_HANDLE_VALUE ? "create FAIL " + std::to_string(GetLastError()) : "created empty regular file";
        if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
    } else if (scenario == "missing") {
        l.note = "nothing created";
    } else {
        l.note = run_child(l.path, scenario == "child-exit" ? "exit" : "kill");
    }
    return l;
}

static void finish_leftover(Leftover& l)
{
    if (l.open_sock != INVALID_SOCKET) closesocket(l.open_sock);
    l.open_sock = INVALID_SOCKET;
}

// Candidate fix: is P an AF_UNIX socket reparse point?
static bool is_afunix_reparse(const std::string& p)
{
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(widen(p).c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return false;
    FindClose(h);
    return (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) && fd.dwReserved0 == (DWORD)IO_REPARSE_TAG_AF_UNIX;
}

using Row = std::pair<std::string, std::function<std::string(const std::string&)>>;

static std::vector<Row> query_rows()
{
    std::vector<Row> rows;
    auto add = [&](std::string name, std::function<std::string(const std::string&)> f) { rows.emplace_back(std::move(name), std::move(f)); };

    add("GetFileAttributesW", [](const std::string& p) { return attr_str(GetFileAttributesW(widen(p).c_str())); });
    add("FindFirstFileW attrs / dwReserved0", [](const std::string& p) {
        WIN32_FIND_DATAW fd;
        HANDLE h = FindFirstFileW(widen(p).c_str(), &fd);
        if (h == INVALID_HANDLE_VALUE) { DWORD e = GetLastError(); return "FAIL err=" + std::to_string(e) + " \"" + win_err_msg(e) + "\""; }
        FindClose(h);
        std::string tag = hex(fd.dwReserved0);
        if (fd.dwReserved0 == (DWORD)IO_REPARSE_TAG_AF_UNIX) tag += "=AF_UNIX";
        return attr_str(fd.dwFileAttributes) + " tag=" + tag + " size=" + std::to_string(((unsigned long long)fd.nFileSizeHigh << 32) | fd.nFileSizeLow);
    });
    add("CreateFileW(OPEN_REPARSE_POINT) + FileAttributeTagInfo", [](const std::string& p) {
        HANDLE h = CreateFileW(widen(p).c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        if (h == INVALID_HANDLE_VALUE) { DWORD e = GetLastError(); return "open FAIL err=" + std::to_string(e) + " \"" + win_err_msg(e) + "\""; }
        FILE_ATTRIBUTE_TAG_INFO info{};
        std::string r;
        if (GetFileInformationByHandleEx(h, FileAttributeTagInfo, &info, sizeof(info))) {
            r = attr_str(info.FileAttributes) + " tag=" + hex(info.ReparseTag) + (info.ReparseTag == (DWORD)IO_REPARSE_TAG_AF_UNIX ? "=AF_UNIX" : "");
        } else {
            r = "query FAIL err=" + std::to_string(GetLastError());
        }
        LARGE_INTEGER sz;
        if (GetFileSizeEx(h, &sz)) r += " size=" + std::to_string(sz.QuadPart);
        CloseHandle(h);
        return r;
    });
    add("CreateFileW(follow, BACKUP_SEMANTICS)", [](const std::string& p) {
        HANDLE h = CreateFileW(widen(p).c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        if (h == INVALID_HANDLE_VALUE) { DWORD e = GetLastError(); return "FAIL err=" + std::to_string(e) + " \"" + win_err_msg(e) + "\""; }
        CloseHandle(h);
        return std::string("opened OK");
    });
    add("_wstat64", [](const std::string& p) {
        struct _stat64 st;
        errno = 0;
        int r = _wstat64(widen(p).c_str(), &st);
        if (r != 0) { int e = errno; return "FAIL errno=" + std::to_string(e) + " \"" + clean(strerror(e)) + "\""; }
        return "OK st_mode=" + hex(st.st_mode) + ((st.st_mode & _S_IFMT) == _S_IFREG ? " (REG)" : (st.st_mode & _S_IFMT) == _S_IFDIR ? " (DIR)" : (st.st_mode & _S_IFMT) == _S_IFCHR ? " (CHR)" : " (?)") + " st_size=" + std::to_string(st.st_size);
    });

    add("fs::symlink_status(p).type()", [](const std::string& p) { return cell([&] { return std::string(ftype(fs::symlink_status(fs::path(p)).type())); }); });
    add("fs::symlink_status(p, ec).type()", [](const std::string& p) { return cell([&] { std::error_code ec; auto t = fs::symlink_status(fs::path(p), ec).type(); return std::string(ftype(t)) + " " + ec_str(ec); }); });
    add("fs::status(p).type()", [](const std::string& p) { return cell([&] { return std::string(ftype(fs::status(fs::path(p)).type())); }); });
    add("fs::status(p, ec).type()", [](const std::string& p) { return cell([&] { std::error_code ec; auto t = fs::status(fs::path(p), ec).type(); return std::string(ftype(t)) + " " + ec_str(ec); }); });
    add("fs::is_regular_file(p)", [](const std::string& p) { return cell([&] { return std::string(fs::is_regular_file(fs::path(p)) ? "true" : "false"); }); });
    add("fs::is_regular_file(p, ec)", [](const std::string& p) { return cell([&] { std::error_code ec; bool b = fs::is_regular_file(fs::path(p), ec); return std::string(b ? "true " : "false ") + ec_str(ec); }); });
    add("fs::is_socket(p)", [](const std::string& p) { return cell([&] { return std::string(fs::is_socket(fs::path(p)) ? "true" : "false"); }); });
    add("fs::is_other(p)", [](const std::string& p) { return cell([&] { return std::string(fs::is_other(fs::path(p)) ? "true" : "false"); }); });
    add("fs::is_symlink(p)", [](const std::string& p) { return cell([&] { return std::string(fs::is_symlink(fs::path(p)) ? "true" : "false"); }); });
    add("fs::exists(p)", [](const std::string& p) { return cell([&] { return std::string(fs::exists(fs::path(p)) ? "true" : "false"); }); });
    add("fs::exists(p, ec)", [](const std::string& p) { return cell([&] { std::error_code ec; bool b = fs::exists(fs::path(p), ec); return std::string(b ? "true " : "false ") + ec_str(ec); }); });
    add("fs::exists(fs::symlink_status(p))", [](const std::string& p) { return cell([&] { return std::string(fs::exists(fs::symlink_status(fs::path(p))) ? "true" : "false"); }); });
    add("fs::file_size(p)", [](const std::string& p) { return cell([&] { return std::to_string(fs::file_size(fs::path(p))); }); });
    add("fs::file_size(p, ec)", [](const std::string& p) { return cell([&] { std::error_code ec; auto s = fs::file_size(fs::path(p), ec); return std::to_string((long long)s) + " " + ec_str(ec); }); });
    add("directory_entry: symlink_status / status", [](const std::string& p) {
        return cell([&] {
            std::string r;
            for (const auto& de : fs::directory_iterator(fs::path(p).parent_path())) {
                std::error_code ec1, ec2;
                auto t1 = de.symlink_status(ec1).type();
                auto t2 = de.status(ec2).type();
                r += de.path().filename().string() + ": " + ftype(t1) + " " + ec_str(ec1) + " / " + ftype(t2) + " " + ec_str(ec2);
            }
            return r;
        });
    });
    add("OLD EXPR: symlink_status(p).type()==socket", [](const std::string& p) { return cell([&] { return std::string(fs::symlink_status(fs::path(p)).type() == fs::file_type::socket ? "true" : "false"); }); });
    add("OLD EXPR: is_regular_file(p) && file_size(p)==0", [](const std::string& p) { return cell([&] { return std::string((fs::is_regular_file(fs::path(p)) && fs::file_size(fs::path(p)) == 0) ? "true" : "false"); }); });
    add("OLD EXPR: full (incl. create_directories(parent))", [](const std::string& p) {
        return cell([&] {
            fs::path path = p;
            if (path.has_parent_path()) fs::create_directories(path.parent_path());
            bool r = fs::symlink_status(path).type() == fs::file_type::socket || (fs::is_regular_file(path) && fs::file_size(path) == 0);
            return std::string(r ? "true (would remove)" : "false (would not remove)");
        });
    });
    add("CAND: FindFirstFileW REPARSE && tag==AF_UNIX", [](const std::string& p) { return std::string(is_afunix_reparse(p) ? "true" : "false"); });
    add("CAND: symlink_status(p,ec)==socket || (status(p,ec) fails 1920 && symlink_status exists)", [](const std::string& p) {
        return cell([&] {
            std::error_code ec1, ec2;
            auto st = fs::symlink_status(fs::path(p), ec1);
            fs::status(fs::path(p), ec2);
            bool r = st.type() == fs::file_type::socket || (fs::exists(st) && ec2.value() == 1920 && ec2.category() == std::system_category());
            return std::string(r ? "true" : "false");
        });
    });
    return rows;
}

static std::vector<Row> removal_rows()
{
    std::vector<Row> rows;
    auto add = [&](std::string name, std::function<std::string(const std::string&)> f) { rows.emplace_back(std::move(name), std::move(f)); };
    add("(no removal)", [](const std::string&) { return std::string("-"); });
    add("fs::remove(p)", [](const std::string& p) { return cell([&] { return std::string(fs::remove(fs::path(p)) ? "returned true" : "returned false"); }); });
    add("fs::remove(p, ec)", [](const std::string& p) { return cell([&] { std::error_code ec; bool b = fs::remove(fs::path(p), ec); return std::string(b ? "true " : "false ") + ec_str(ec); }); });
    add("fs::remove_all(p, ec)", [](const std::string& p) { return cell([&] { std::error_code ec; auto n = fs::remove_all(fs::path(p), ec); return std::to_string((long long)n) + " " + ec_str(ec); }); });
    add("DeleteFileW", [](const std::string& p) {
        if (DeleteFileW(widen(p).c_str())) return std::string("OK");
        DWORD e = GetLastError();
        return "FAIL err=" + std::to_string(e) + " \"" + win_err_msg(e) + "\"";
    });
    add("CAND: if FindFirstFileW AF_UNIX tag -> fs::remove(p)", [](const std::string& p) { return cell([&] { if (!is_afunix_reparse(p)) return std::string("not detected"); return std::string(fs::remove(fs::path(p)) ? "removed true" : "removed false"); }); });
    add("CAND: if FindFirstFileW AF_UNIX tag -> DeleteFileW", [](const std::string& p) { if (!is_afunix_reparse(p)) return std::string("not detected"); return std::string(DeleteFileW(widen(p).c_str()) ? "OK" : "FAIL " + std::to_string(GetLastError())); });
    add("_wunlink", [](const std::string& p) {
        if (_wunlink(widen(p).c_str()) == 0) return std::string("OK");
        int e = errno;
        return "FAIL errno=" + std::to_string(e) + " \"" + clean(strerror(e)) + "\"";
    });
    return rows;
}

static std::string os_version()
{
    typedef LONG(WINAPI * RtlGetVersionFn)(OSVERSIONINFOW*);
    auto fn = reinterpret_cast<RtlGetVersionFn>(reinterpret_cast<void*>(GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlGetVersion")));
    OSVERSIONINFOW v{};
    v.dwOSVersionInfoSize = sizeof(v);
    if (fn) fn(&v);
    DWORD ubr = 0, sz = sizeof(ubr);
    RegGetValueA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "UBR", RRF_RT_REG_DWORD, nullptr, &ubr, &sz);
    return std::to_string(v.dwMajorVersion) + "." + std::to_string(v.dwMinorVersion) + "." + std::to_string(v.dwBuildNumber) + "." + std::to_string(ubr);
}

static std::string module_version(const char* name)
{
    HMODULE m = GetModuleHandleA(name);
    if (!m) return std::string(name) + " not loaded";
    char path[MAX_PATH];
    GetModuleFileNameA(m, path, MAX_PATH);
    DWORD dummy;
    DWORD sz = GetFileVersionInfoSizeA(path, &dummy);
    std::vector<char> buf(sz);
    VS_FIXEDFILEINFO* fi = nullptr;
    UINT len = 0;
    if (!sz || !GetFileVersionInfoA(path, 0, sz, buf.data()) || !VerQueryValueA(buf.data(), "\\", reinterpret_cast<void**>(&fi), &len)) return std::string(path) + " (no version)";
    return std::string(path) + " " + std::to_string(HIWORD(fi->dwFileVersionMS)) + "." + std::to_string(LOWORD(fi->dwFileVersionMS)) + "." + std::to_string(HIWORD(fi->dwFileVersionLS)) + "." + std::to_string(LOWORD(fi->dwFileVersionLS));
}

static std::string toolchain()
{
    std::string s;
#if defined(_MSC_VER)
    s += "MSVC _MSC_FULL_VER=" + std::to_string(_MSC_FULL_VER);
#endif
#if defined(__clang__)
    s += " clang " __clang_version__;
#elif defined(__GNUC__)
    s += "GCC " __VERSION__;
#endif
#if defined(__GLIBCXX__)
    s += " __GLIBCXX__=" + std::to_string(__GLIBCXX__);
#endif
#if defined(_MSVC_STL_UPDATE)
    s += " _MSVC_STL_UPDATE=" + std::to_string(_MSVC_STL_UPDATE);
#endif
#if defined(__MINGW64_VERSION_STR)
    s += " mingw-w64 " __MINGW64_VERSION_STR;
#endif
#if defined(_UCRT)
    s += " _UCRT";
#endif
#if defined(__MSVCRT_VERSION__)
    s += " __MSVCRT_VERSION__=" + hex(__MSVCRT_VERSION__);
#endif
    return s;
}

int main(int argc, char** argv)
{
    if (argc >= 4 && std::string(argv[1]) == "child") return child_main(argv[2], argv[3]);
    std::string label = argc >= 2 ? argv[1] : "probe";

    char exe[MAX_PATH];
    GetModuleFileNameA(nullptr, exe, MAX_PATH);
    g_exe = exe;
    char tmp[MAX_PATH];
    GetTempPathA(MAX_PATH, tmp);
    g_root = std::string(tmp) + "afp-" + std::to_string(GetCurrentProcessId());
    CreateDirectoryA(g_root.c_str(), nullptr);

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    printf("## %s\n\n", label.c_str());
    printf("- OS: %s\n- Toolchain: %s\n- CRT DLL: %s; %s\n- Root: %s\n\n", os_version().c_str(), toolchain().c_str(), module_version("ucrtbase.dll").c_str(), module_version("msvcrt.dll").c_str(), g_root.c_str());

    // Table 1: queries on a fresh leftover for each scenario.
    auto qrows = query_rows();
    std::map<std::string, std::vector<std::string>> qres;
    std::vector<std::string> notes;
    for (const auto& sc : SCENARIOS) {
        Leftover l = make_leftover(sc);
        notes.push_back(sc + ": " + l.note + " path=" + l.path);
        for (const auto& row : qrows) qres[sc].push_back(row.second(l.path));
        finish_leftover(l);
    }
    printf("### Queries on leftover path\n\n");
    for (const auto& n : notes) printf("- %s\n", n.c_str());
    printf("\n| query |");
    for (const auto& sc : SCENARIOS) printf(" %s |", sc.c_str());
    printf("\n|---|");
    for (size_t i = 0; i < SCENARIOS.size(); ++i) printf("---|");
    printf("\n");
    for (size_t i = 0; i < qrows.size(); ++i) {
        printf("| %s |", qrows[i].first.c_str());
        for (const auto& sc : SCENARIOS) printf(" %s |", qres[sc][i].c_str());
        printf("\n");
    }

    // Table 2: removal on a fresh leftover per (method, scenario), then
    // whether the file is gone and whether a new bind succeeds.
    auto rrows = removal_rows();
    printf("\n### Removal on fresh leftover, then GetFileAttributesW + rebind\n\n");
    printf("| method |");
    for (const auto& sc : SCENARIOS) printf(" %s |", sc.c_str());
    printf("\n|---|");
    for (size_t i = 0; i < SCENARIOS.size(); ++i) printf("---|");
    printf("\n");
    for (const auto& row : rrows) {
        printf("| %s |", row.first.c_str());
        for (const auto& sc : SCENARIOS) {
            Leftover l = make_leftover(sc);
            std::string r = row.second(l.path);
            DWORD a = GetFileAttributesW(widen(l.path).c_str());
            std::string after = a == INVALID_FILE_ATTRIBUTES ? "gone" : "still exists " + hex(a);
            SOCKET s;
            int b = wsa_bind(l.path, &s, true);
            if (b == 0) closesocket(s);
            finish_leftover(l);
            printf(" %s; after: %s; rebind: %s |", r.c_str(), after.c_str(), bind_result(b).c_str());
            fflush(stdout);
        }
        printf("\n");
    }
    printf("\n");
    return 0;
}
