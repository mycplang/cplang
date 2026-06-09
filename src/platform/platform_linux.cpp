// CP语言 平台抽象层 — Linux/Android 实现 (POSIX)
#if defined(__linux__) || defined(__ANDROID__) || defined(__APPLE__)

#include "platform/platform.hpp"
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <dlfcn.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <climits>

namespace cplang {
namespace platform {

// ── 系统信息 ──

const char* os_name() {
#ifdef __ANDROID__
    return "android";
#elif __APPLE__
    return "macos";
#else
    return "linux";
#endif
}

int cpu_count() {
    return (int)sysconf(_SC_NPROCESSORS_ONLN);
}

void random_seed(unsigned int seed) { srand(seed); }

// ── 文件 I/O ──

void* file_open(const char* path, const char* mode) {
    return (void*)fopen(path, mode);
}

int file_read(void* f, char* buf, int len) {
    return (int)fread(buf, 1, len, (FILE*)f);
}

int file_write(void* f, const char* buf, int len) {
    return (int)fwrite(buf, 1, len, (FILE*)f);
}

void file_close(void* f) { fclose((FILE*)f); }

bool file_exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0;
}

bool file_delete(const char* path) {
    return remove(path) == 0;
}

int64_t file_size(void* f) {
    long pos = ftell((FILE*)f);
    fseek((FILE*)f, 0, SEEK_END);
    long sz = ftell((FILE*)f);
    fseek((FILE*)f, pos, SEEK_SET);
    return sz;
}

bool file_is_dir(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

std::string file_cwd() {
    char buf[PATH_MAX];
    return getcwd(buf, sizeof(buf)) ? buf : "";
}

bool file_chdir(const char* path) {
    return chdir(path) == 0;
}

bool file_mkdir(const char* path) {
    return mkdir(path, 0755) == 0;
}

std::string file_temp_dir() {
    const char* tmp = getenv("TMPDIR");
    if (!tmp) tmp = "/tmp";
    return tmp;
}

// ── 网络 ──

bool net_init() { return true; }

void net_cleanup() {}

int net_last_error() { return errno; }

const char* net_error_str(int err) {
    return strerror(err);
}

int sock_create(int af, int type, int proto) {
    return socket(af, type, proto);
}

int sock_connect(int sock, const char* host, int port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    // 先尝试直接解析 IP
    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        // 回退到 DNS 解析（使用 getaddrinfo 替代废弃的 gethostbyname）
        struct addrinfo hints, *res = nullptr;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        char portStr[16];
        snprintf(portStr, sizeof(portStr), "%d", port);
        if (getaddrinfo(host, portStr, &hints, &res) != 0 || !res) return -1;
        memcpy(&addr.sin_addr, &((struct sockaddr_in*)res->ai_addr)->sin_addr, sizeof(addr.sin_addr));
        freeaddrinfo(res);
    }
    return connect(sock, (struct sockaddr*)&addr, sizeof(addr));
}

int sock_bind(int sock, int port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    addr.sin_addr.s_addr = INADDR_ANY;
    return bind(sock, (struct sockaddr*)&addr, sizeof(addr));
}

int sock_listen(int sock, int backlog) { return listen(sock, backlog); }

int sock_accept(int sock) {
    return accept(sock, NULL, NULL);
}

int sock_recv(int sock, char* buf, int len) {
    return (int)recv(sock, buf, len, 0);
}

int sock_send(int sock, const char* buf, int len) {
    return (int)send(sock, buf, len, 0);
}

int sock_close(int sock) { return close(sock); }

std::string http_get(const char* url) {
    // 简易 HTTP GET（原生 socket 实现，无外部依赖）
    // 解析 URL: http://host:port/path
    const char* p = url;
    bool isHttps = false;
    if (strncmp(p, "https://", 8) == 0) { isHttps = true; p += 8; }
    else if (strncmp(p, "http://", 7) == 0) { p += 7; }
    else return ""; // 仅支持 http/https

    // 提取 host
    const char* hostStart = p;
    const char* slash = strchr(p, '/');
    const char* colon = strchr(p, ':');
    std::string host;
    int port = isHttps ? 443 : 80;

    if (colon && (!slash || colon < slash)) {
        host.assign(hostStart, colon - hostStart);
        port = atoi(colon + 1);
    } else if (slash) {
        host.assign(hostStart, slash - hostStart);
    } else {
        host = hostStart;
    }

    const char* path = slash ? slash : "/";

    // 创建 socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return "";

    if (sock_connect(sock, host.c_str(), port) < 0) {
        close(sock);
        return "";
    }

    // 构造 HTTP 请求
    char req[4096];
    snprintf(req, sizeof(req),
        "GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: CPLang/1.0\r\nConnection: close\r\n\r\n",
        path, host.c_str());
    send(sock, req, strlen(req), 0);

    // 接收响应
    std::string result;
    char buf[4096];
    int n;
    while ((n = recv(sock, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[n] = 0;
        result += buf;
    }
    close(sock);

    // 跳过 HTTP 头（找到 \r\n\r\n）
    size_t headerEnd = result.find("\r\n\r\n");
    if (headerEnd != std::string::npos) {
        result = result.substr(headerEnd + 4);
    }

    return result;
}

// ── 线程 ──

struct PosixThreadArg { void (*fn)(void*); void* arg; };
static void* posixThreadProc(void* p) {
    auto* a = (PosixThreadArg*)p;
    a->fn(a->arg);
    delete a;
    return nullptr;
}

void* thread_create(void (*fn)(void*), void* arg) {
    auto* ta = new PosixThreadArg{fn, arg};
    pthread_t* t = new pthread_t;
    if (pthread_create(t, NULL, posixThreadProc, ta) != 0) {
        delete t; delete ta;
        return nullptr;
    }
    return t;
}

void thread_join(void* thread) {
    pthread_t* t = (pthread_t*)thread;
    pthread_join(*t, NULL);
    delete t;
}

void thread_detach(void* thread) {
    pthread_t* t = (pthread_t*)thread;
    pthread_detach(*t);
    delete t;
}

void thread_sleep_ms(int ms) { usleep(ms * 1000); }

void* mutex_create() {
    pthread_mutex_t* m = new pthread_mutex_t;
    pthread_mutex_init(m, NULL);
    return m;
}
void mutex_lock(void* m)   { pthread_mutex_lock((pthread_mutex_t*)m); }
void mutex_unlock(void* m) { pthread_mutex_unlock((pthread_mutex_t*)m); }
void mutex_destroy(void* m){ pthread_mutex_destroy((pthread_mutex_t*)m); delete (pthread_mutex_t*)m; }

// ── 动态库加载 ──

void* dl_open(const char* path) { return dlopen(path, RTLD_LAZY); }
void* dl_sym(void* lib, const char* name) { return dlsym(lib, name); }
void dl_close(void* lib) { dlclose(lib); }

// ── 进程 ──

int proc_getpid() { return (int)getpid(); }

std::string proc_exe_path() {
    char buf[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len > 0) { buf[len] = 0; return buf; }
    return "";
}

bool proc_exec(const char* cmd, std::string& output) {
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return false;
    char buf[4096];
    while (fgets(buf, sizeof(buf), pipe)) {
        output += buf;
    }
    int ret = pclose(pipe);
    return ret != -1;
}

// ── 控制台 ──

void console_set_utf8() {
    // Linux 默认 UTF-8，无需设置
}

bool console_is_tty() {
    return isatty(STDOUT_FILENO);
}

int console_getch() {
    // 非阻塞读取一个字符
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    int ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

} // namespace platform
} // namespace cplang

#endif // __linux__ || __ANDROID__ || __APPLE__