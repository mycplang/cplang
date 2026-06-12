// CP语言 平台抽象层 — 跨平台 API 统一接口
// Windows: 用 Win32 API 实现
// Linux/Android: 用 POSIX API 实现
#pragma once
#include <cstdint>
#include <string>

// 跨平台网络类型 (AF_INET, sockaddr_in, htons, inet_pton 等)
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winhttp.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dlfcn.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <termios.h>
#endif

namespace cplang {
namespace platform {

// ═══════════════════════════════════════════════
//  系统信息
// ═══════════════════════════════════════════════

const char*   os_name();         // "windows" / "linux" / "android"
int           cpu_count();       // 逻辑 CPU 数量
void          random_seed(unsigned int seed);

// ═══════════════════════════════════════════════
//  文件 I/O
// ═══════════════════════════════════════════════

void*         file_open(const char* path, const char* mode);
int           file_read(void* f, char* buf, int len);
int           file_write(void* f, const char* buf, int len);
void          file_close(void* f);
bool          file_exists(const char* path);
bool          file_delete(const char* path);
int64_t       file_size(void* f);
bool          file_is_dir(const char* path);
std::string   file_cwd();                    // 当前工作目录
bool          file_chdir(const char* path);
bool          file_mkdir(const char* path);
std::string   file_temp_dir();               // 系统临时目录

// ═══════════════════════════════════════════════
//  网络
// ═══════════════════════════════════════════════

bool          net_init();                    // WinSock 初始化 / Linux 空操作
void          net_cleanup();                 // WinSock 清理
int           net_last_error();              // 最后一次错误码
const char*   net_error_str(int err);

int           sock_create(int af, int type, int proto);
int           sock_connect(int sock, const char* host, int port);
int           sock_bind(int sock, int port);
int           sock_listen(int sock, int backlog);
int           sock_accept(int sock);
int           sock_recv(int sock, char* buf, int len);
int           sock_send(int sock, const char* buf, int len);
int           sock_close(int sock);

// HTTP 下载（简单 GET，返回 body）
std::string   http_get(const char* url);

// ═══════════════════════════════════════════════
//  线程
// ═══════════════════════════════════════════════

void*         thread_create(void (*fn)(void*), void* arg);
void          thread_join(void* thread);
void          thread_detach(void* thread);
void          thread_sleep_ms(int ms);

void*         mutex_create();
void          mutex_lock(void* m);
void          mutex_unlock(void* m);
void          mutex_destroy(void* m);

// ═══════════════════════════════════════════════
//  动态库加载
// ═══════════════════════════════════════════════

void*         dl_open(const char* path);
void*         dl_sym(void* lib, const char* name);
void          dl_close(void* lib);

// ═══════════════════════════════════════════════
//  进程
// ═══════════════════════════════════════════════

int           proc_getpid();                 // 当前进程 ID
std::string   proc_exe_path();               // 当前可执行文件路径
bool          proc_exec(const char* cmd, std::string& output);  // 执行命令并捕获输出

// ═══════════════════════════════════════════════
//  控制台/TTY
// ═══════════════════════════════════════════════

void          console_set_utf8();            // 设置控制台 UTF-8 输出
bool          console_is_tty();              // 是否为交互终端
int           console_getch();               // 读取一个按键（无缓冲）

// ═══════════════════════════════════════════════
//  IO 多路复用 (IOCP / epoll)
// ═══════════════════════════════════════════════

// 事件类型标志（与 OS 无关的统一枚举）
enum IOPollEvent : uint32_t {
    IOPOLL_IN  = 1 << 0,   // 可读
    IOPOLL_OUT = 1 << 1,   // 可写
    IOPOLL_ERR = 1 << 2,   // 错误
    IOPOLL_HUP = 1 << 3,   // 挂断
};

struct IOPollResult {
    uint64_t    user_data;   // 用户自定义数据
    uint32_t    events;      // 触发的事件（IOPollEvent 组合）
    int         error_code;  // 错误码（0 = 无错误）
};

// 创建 IO 多路复用句柄（Windows: IOCP / Linux: epoll）
void*        iopoll_create(int max_events);
int          iopoll_add(void* poll, int sock, uint32_t events, uint64_t user_data);
int          iopoll_mod(void* poll, int sock, uint32_t events, uint64_t user_data);
int          iopoll_del(void* poll, int sock);
int          iopoll_wait(void* poll, IOPollResult* results, int max_results, int timeout_ms);
void         iopoll_interrupt(void* poll);
void         iopoll_destroy(void* poll);

} // namespace platform
} // namespace cplang