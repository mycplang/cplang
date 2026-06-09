// CP语言 平台抽象层 — Windows 实现
#ifdef _WIN32

#include "platform/platform.hpp"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winhttp.h>
#include <io.h>
#include <direct.h>
#include <fcntl.h>
#include <shlobj.h>
#include <cstring>
#include <cstdio>
#include <conio.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winhttp.lib")

namespace cplang {
namespace platform {

// ── 系统信息 ──

const char* os_name() { return "windows"; }

int cpu_count() {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwNumberOfProcessors;
}

void random_seed(unsigned int seed) { srand(seed); }

// ── 文件 I/O ──

void* file_open(const char* path, const char* mode) {
    DWORD access = 0, disposition = 0;
    if (strchr(mode, 'w')) { access = GENERIC_WRITE; disposition = CREATE_ALWAYS; }
    else { access = GENERIC_READ; disposition = OPEN_EXISTING; }
    if (strchr(mode, '+')) access |= GENERIC_READ | GENERIC_WRITE;
    HANDLE h = CreateFileA(path, access, FILE_SHARE_READ, NULL,
                           disposition, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        // 回退到标准 C fopen
        return (void*)fopen(path, mode);
    }
    return h;
}

int file_read(void* f, char* buf, int len) {
    HANDLE h = (HANDLE)f;
    DWORD r;
    if (ReadFile(h, buf, (DWORD)len, &r, NULL)) return (int)r;
    return -1;
}

int file_write(void* f, const char* buf, int len) {
    HANDLE h = (HANDLE)f;
    DWORD w;
    if (WriteFile(h, buf, (DWORD)len, &w, NULL)) return (int)w;
    return -1;
}

void file_close(void* f) { CloseHandle((HANDLE)f); }

bool file_exists(const char* path) {
    return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

bool file_delete(const char* path) {
    return DeleteFileA(path) != 0;
}

int64_t file_size(void* f) {
    LARGE_INTEGER li;
    if (GetFileSizeEx((HANDLE)f, &li)) return li.QuadPart;
    return -1;
}

bool file_is_dir(const char* path) {
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

std::string file_cwd() {
    char buf[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, buf);
    return buf;
}

bool file_chdir(const char* path) {
    return SetCurrentDirectoryA(path) != 0;
}

bool file_mkdir(const char* path) {
    return CreateDirectoryA(path, NULL) != 0;
}

std::string file_temp_dir() {
    char buf[MAX_PATH];
    GetTempPathA(MAX_PATH, buf);
    return buf;
}

// ── 网络 ──

static bool net_ready = false;

bool net_init() {
    if (net_ready) return true;
    WSADATA wsa;
    net_ready = (WSAStartup(MAKEWORD(2,2), &wsa) == 0);
    return net_ready;
}

void net_cleanup() {
    if (net_ready) { WSACleanup(); net_ready = false; }
}

int net_last_error() { return WSAGetLastError(); }

const char* net_error_str(int err) {
    static char buf[256];
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, buf, sizeof(buf), NULL);
    return buf;
}

int sock_create(int af, int type, int proto) {
    net_init();
    return (int)socket(af, type, proto);
}

int sock_connect(int sock, const char* host, int port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)port);
    inet_pton(AF_INET, host, &addr.sin_addr);
    return connect((SOCKET)sock, (sockaddr*)&addr, sizeof(addr));
}

int sock_bind(int sock, int port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)port);
    addr.sin_addr.s_addr = INADDR_ANY;
    return bind((SOCKET)sock, (sockaddr*)&addr, sizeof(addr));
}

int sock_listen(int sock, int backlog) { return listen((SOCKET)sock, backlog); }

int sock_accept(int sock) {
    return (int)accept((SOCKET)sock, NULL, NULL);
}

int sock_recv(int sock, char* buf, int len) {
    return recv((SOCKET)sock, buf, len, 0);
}

int sock_send(int sock, const char* buf, int len) {
    return send((SOCKET)sock, buf, len, 0);
}

int sock_close(int sock) { return closesocket((SOCKET)sock); }

std::string http_get(const char* url) {
    std::string result;
    HINTERNET hSession = WinHttpOpen(L"CPLang/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return result;

    // 解析 URL
    std::string urlStr(url);
    const char* p = strstr(url, "://");
    if (p) p += 3; else p = url;
    const char* path = strchr(p, '/');
    std::string host(p, path ? (size_t)(path - p) : strlen(p));
    std::string uri(path ? path : "/");
    int port = INTERNET_DEFAULT_HTTP_PORT;

    int wlen = MultiByteToWideChar(CP_UTF8, 0, host.c_str(), -1, NULL, 0);
    std::wstring whost(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, host.c_str(), -1, &whost[0], wlen);

    HINTERNET hConnect = WinHttpConnect(hSession, whost.c_str(), port, 0);
    if (hConnect) {
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET",
            std::wstring(uri.begin(), uri.end()).c_str(),
            NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
        if (hRequest) {
            if (WinHttpSendRequest(hRequest, NULL, 0, NULL, 0, 0, 0) &&
                WinHttpReceiveResponse(hRequest, NULL)) {
                DWORD avail;
                while (WinHttpQueryDataAvailable(hRequest, &avail) && avail > 0) {
                    std::string chunk(avail, 0);
                    DWORD read;
                    if (WinHttpReadData(hRequest, &chunk[0], avail, &read))
                        result.append(chunk.c_str(), read);
                }
            }
            WinHttpCloseHandle(hRequest);
        }
        WinHttpCloseHandle(hConnect);
    }
    WinHttpCloseHandle(hSession);
    return result;
}

// ── 线程 ──

struct WinThreadArg { void (*fn)(void*); void* arg; };
static DWORD WINAPI winThreadProc(LPVOID p) {
    auto* a = (WinThreadArg*)p;
    a->fn(a->arg);
    delete a;
    return 0;
}

void* thread_create(void (*fn)(void*), void* arg) {
    auto* ta = new WinThreadArg{fn, arg};
    return CreateThread(NULL, 0, winThreadProc, ta, 0, NULL);
}

void thread_join(void* thread) {
    WaitForSingleObject((HANDLE)thread, INFINITE);
    CloseHandle((HANDLE)thread);
}

void thread_detach(void* thread) { CloseHandle((HANDLE)thread); }

void thread_sleep_ms(int ms) { Sleep(ms); }

void* mutex_create() { return (void*)CreateMutexA(NULL, FALSE, NULL); }
void mutex_lock(void* m) { WaitForSingleObject((HANDLE)m, INFINITE); }
void mutex_unlock(void* m) { ReleaseMutex((HANDLE)m); }
void mutex_destroy(void* m) { CloseHandle((HANDLE)m); }

// ── 动态库加载 ──

void* dl_open(const char* path) { return (void*)LoadLibraryA(path); }
void* dl_sym(void* lib, const char* name) { return (void*)GetProcAddress((HMODULE)lib, name); }
void dl_close(void* lib) { FreeLibrary((HMODULE)lib); }

// ── 进程 ──

int proc_getpid() { return GetCurrentProcessId(); }

std::string proc_exe_path() {
    char buf[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, buf, MAX_PATH);
    return (len > 0) ? std::string(buf, len) : "";
}

bool proc_exec(const char* cmd, std::string& output) {
    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    HANDLE hRead, hWrite;
    if (!CreatePipe(&hRead, &hWrite, &sa, 4096)) return false;
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si = { sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    PROCESS_INFORMATION pi;

    char cmdBuf[4096];
    strncpy(cmdBuf, cmd, sizeof(cmdBuf) - 1);
    cmdBuf[sizeof(cmdBuf)-1] = 0;

    bool ok = CreateProcessA(NULL, cmdBuf, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
    CloseHandle(hWrite);
    if (!ok) { CloseHandle(hRead); return false; }

    char buf[256]; DWORD r;
    while (ReadFile(hRead, buf, sizeof(buf)-1, &r, NULL) && r > 0) {
        buf[r] = 0; output += buf;
    }
    CloseHandle(hRead);
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

// ── 控制台 ──

void console_set_utf8() {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
}

bool console_is_tty() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    return GetConsoleMode(h, &mode) != 0;
}

int console_getch() {
    // 设置控制台为原始模式 读取一个字符
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    DWORD oldMode;
    GetConsoleMode(h, &oldMode);
    SetConsoleMode(h, 0);
    int ch = _getch();
    SetConsoleMode(h, oldMode);
    return ch;
}

} // namespace platform
} // namespace cplang

#endif // _WIN32