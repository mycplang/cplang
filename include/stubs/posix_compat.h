// CP语言 POSIX 网络兼容层
// 将 WinSock API 映射到 POSIX sockets
#pragma once

#ifdef __linux__

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include <cstring>
#include <cstdint>
#include <ctime>

// ── 类型映射 ──
typedef int SOCKET;
#define INVALID_SOCKET  (-1)
#define SOCKET_ERROR    (-1)

// ── 函数映射 ──
inline int closesocket(SOCKET s) { return close(s); }
inline int ioctlsocket(SOCKET s, long cmd, u_long* argp) {
    if (cmd == 0x8004667E) { // FIONBIO
        int flags = fcntl(s, F_GETFL, 0);
        if (*argp) flags |= O_NONBLOCK; else flags &= ~O_NONBLOCK;
        return fcntl(s, F_SETFL, flags);
    }
    return -1;
}
#define FIONBIO 0x8004667E

// WSAGetLastError → errno
inline int WSAGetLastError() { return errno; }
#define WSAEWOULDBLOCK EAGAIN

// WSAStartup / WSACleanup → nop
inline int WSAStartup(unsigned short, void*) { return 0; }
inline int WSACleanup() { return 0; }

// inet_pton 在某些老系统需要额外处理
#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

// ── Windows 文件 API 映射 ──
#include <cstdio>
#include <cstdlib>

inline int fopen_s(FILE** f, const char* name, const char* mode) {
    *f = fopen(name, mode);
    return *f ? 0 : errno;
}

inline int localtime_s(struct tm* tm, const time_t* t) {
    return localtime_r(t, tm) ? 0 : errno;
}

#define _dupenv_s(ptr, len, name)  (*(ptr) = getenv(name) ? strdup(getenv(name)) : nullptr, *(len) = *(ptr) ? strlen(*(ptr)) : 0, 0)
#define _putenv_s(name, value)     (setenv(name, value, 1) == 0 ? 0 : errno)

inline FILE* _popen(const char* cmd, const char* mode) { return popen(cmd, mode); }
inline int _pclose(FILE* f) { return pclose(f); }

// ── 进程 API ──
#include <sys/types.h>
#include <unistd.h>
#define GetCurrentProcessId() ((unsigned long)getpid())

#define MAX_PATH 4096
typedef unsigned long DWORD;

inline DWORD GetModuleFileNameA(void*, char* buf, DWORD size) {
    ssize_t n = readlink("/proc/self/exe", buf, size - 1);
    if (n < 0) return 0;
    buf[n] = 0;
    return (DWORD)n;
}

// ── 时间 API ──
#include <sys/time.h>
struct SYSTEMTIME { unsigned short wYear, wMonth, wDayOfWeek, wDay, wHour, wMinute, wSecond, wMilliseconds; };
inline void GetLocalTime(SYSTEMTIME* st) {
    time_t t = time(nullptr);
    struct tm* tm = localtime(&t);
    st->wYear = tm->tm_year + 1900;
    st->wMonth = tm->tm_mon + 1;
    st->wDay = tm->tm_mday;
    st->wHour = tm->tm_hour;
    st->wMinute = tm->tm_min;
    st->wSecond = tm->tm_sec;
    st->wMilliseconds = 0;
}

// ── 文件 I/O ──
#include <unistd.h>
#define STD_OUTPUT_HANDLE ((unsigned long)-11)
#define STD_ERROR_HANDLE  ((unsigned long)-12)
typedef void* HANDLE;
inline HANDLE GetStdHandle(unsigned long n) { return (HANDLE)(uintptr_t)n; }
inline int WriteFile(HANDLE h, const void* data, unsigned long len, unsigned long* written, void*) {
    int fd = (uintptr_t)h == (uintptr_t)-11 ? STDOUT_FILENO : STDERR_FILENO;
    ssize_t n = write(fd, data, len);
    *written = n > 0 ? (unsigned long)n : 0;
    return n >= 0;
}

// ── 宽字符 (stub) ──
typedef char16_t WCHAR;
typedef const WCHAR* LPCWSTR;

// CP_UTF8 and MultiByteToWideChar stubs
#define CP_UTF8 65001

// ── 动态库加载 ──
#include <dlfcn.h>
#define HMODULE void*
inline HMODULE LoadLibraryA(const char* path) { return dlopen(path, RTLD_LAZY); }
inline void FreeLibrary(HMODULE h) { if(h) dlclose(h); }
inline void* GetProcAddress(HMODULE h, const char* name) { return dlsym(h, name); }

#endif // __linux__

// Windows POSIX 兼容 (MSVC)
#ifdef _WIN32
#define popen  _popen
#define pclose _pclose
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#endif
