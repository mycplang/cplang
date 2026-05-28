// ═══════════════════════════════════════════════════════════════════
//  Redis 客户端（纯 WinSock RESP 协议实现，零外部依赖）
// ═══════════════════════════════════════════════════════════════════

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <cstring>

namespace redis_ns {

// ─── RESP 编码器 ────────────────────────────────────────────────
static std::string respEncode(const std::string& cmd) {
    // 格式: *<argc>\r\n$<len>\r\n<arg>\r\n...
    std::vector<std::string> args;
    size_t pos = 0, start = 0;
    bool inQuote = false;
    while (pos <= cmd.size()) {
        char c = (pos < cmd.size()) ? cmd[pos] : ' ';
        if (inQuote) {
            if (c == '"') { inQuote = false; }
        } else if (c == '"') {
            inQuote = true;
        } else if (c == ' ' || c == '\0' || pos == cmd.size()) {
            if (pos > start) {
                args.push_back(cmd.substr(start, pos - start));
            }
            start = pos + 1;
        }
        pos++;
    }

    std::string out;
    out += "*" + std::to_string(args.size()) + "\r\n";
    for (auto& a : args) {
        out += "$" + std::to_string(a.size()) + "\r\n" + a + "\r\n";
    }
    return out;
}

// ─── RESP 解码器（递归） ─────────────────────────────────────────
static Value respDecode(const char*& p, VM* vm) {
    if (!p || !*p) return Value::nil();
    
    char type = *p++;
    switch (type) {
    case '+': { // Simple String
        const char* start = p;
        while (*p && *p != '\r') p++;
        std::string s(start, p - start);
        if (*p == '\r') p++;
        if (*p == '\n') p++;
        return makeStringVal(VMString::create(s));
    }
    case '-': { // Error
        const char* start = p;
        while (*p && *p != '\r') p++;
        std::string s(start, p - start);
        if (*p == '\r') p++;
        if (*p == '\n') p++;
        VMTable* err = VMTable::create();
        err->set(makeStringVal(VMString::create("_err")), makeStringVal(VMString::create(s)));
        return makeTableVal(err);
    }
    case ':': { // Integer
        Int64 val = 0;
        int sign = 1;
        if (*p == '-') { sign = -1; p++; }
        while (*p >= '0' && *p <= '9') { val = val * 10 + (*p - '0'); p++; }
        if (*p == '\r') p++;
        if (*p == '\n') p++;
        return Value::Int(val * sign);
    }
    case '$': { // Bulk String
        int len = 0;
        while (*p >= '0' && *p <= '9') { len = len * 10 + (*p - '0'); p++; }
        if (*p == '\r') p++;
        if (*p == '\n') p++;
        if (len < 0) { return Value::nil(); }
        std::string s(p, len);
        p += len;
        if (*p == '\r') p++;
        if (*p == '\n') p++;
        return makeStringVal(VMString::create(s));
    }
    case '*': { // Array
        int count = 0;
        while (*p >= '0' && *p <= '9') { count = count * 10 + (*p - '0'); p++; }
        if (*p == '\r') p++;
        if (*p == '\n') p++;
        if (count < 0) return Value::nil();
        VMArray* arr = VMArray::create();
        for (int i = 0; i < count; i++) {
            arr->data.push_back(respDecode(p, vm));
        }
        return makeArrayVal(arr);
    }
    default:
        return Value::nil();
    }
}

// ─── Redis 连接句柄（SOCKET）存储 ────────────────────────────────
static bool g_wsaInit_redis = false;

static bool initWinsock() {
    if (g_wsaInit_redis) return true;
    WSADATA wsaData;
    g_wsaInit_redis = (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0);
    return g_wsaInit_redis;
}

static SOCKET getSock(Value& v) {
    if (!v.isTable()) return INVALID_SOCKET;
    VMTable* t = v.asTable();
    Value k = makeStringVal(VMString::create("_redis"));
    if (!t->has(k)) return INVALID_SOCKET;
    return (SOCKET)(intptr_t)t->get(k).asInt();
}

static void setSock(VMTable* tbl, SOCKET s) {
    tbl->set(makeStringVal(VMString::create("_redis")),
              Value::Int(static_cast<Int64>((intptr_t)s)));
}

// ─── Redis 连接/断开 ────────────────────────────────────────────
Value connect_(std::vector<Value>& args) {
    if (!initWinsock()) return Value::nil();
    const char* host = (args.size() >= 1 && args[0].isString()) 
        ? args[0].asString()->data : "127.0.0.1";
    int port = (args.size() >= 2 && args[1].isInt()) 
        ? static_cast<int>(args[1].asInt()) : 6379;

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) return Value::nil();

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<u_short>(port));
    inet_pton(AF_INET, host, &addr.sin_addr);

    // 连接超时 3 秒
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
    connect(sock, (sockaddr*)&addr, sizeof(addr));

    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(sock, &wfds);
    timeval tv = {3, 0};
    if (select(0, nullptr, &wfds, nullptr, &tv) <= 0) {
        closesocket(sock);
        return Value::nil();
    }

    mode = 0;
    ioctlsocket(sock, FIONBIO, &mode);

    VMTable* tbl = VMTable::create();
    setSock(tbl, sock);
    return makeTableVal(tbl);
}

Value close_(std::vector<Value>& args) {
    if (args.empty()) return Value::Bool(false);
    SOCKET sock = getSock(args[0]);
    if (sock == INVALID_SOCKET) return Value::Bool(false);
    closesocket(sock);
    if (args[0].isTable()) args[0].asTable()->remove(makeStringVal(VMString::create("_redis")));
    return Value::Bool(true);
}

Value isOpen_(std::vector<Value>& args) {
    if (args.empty()) return Value::Bool(false);
    SOCKET sock = getSock(args[0]);
    if (sock == INVALID_SOCKET) return Value::Bool(false);
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(sock, &rfds);
    timeval tv = {0, 0};
    return Value::Bool(select(0, &rfds, nullptr, nullptr, &tv) >= 0);
}

Value errMsg_(std::vector<Value>& args) {
    if (args.empty()) return makeStringVal(VMString::create("no handle"));
    SOCKET sock = getSock(args[0]);
    if (sock == INVALID_SOCKET) return makeStringVal(VMString::create("not connected"));
    return makeStringVal(VMString::create("check redisCommand result"));
}

// ─── 核心：执行 Redis 命令 ──────────────────────────────────────
static bool sendAll(SOCKET s, const char* data, int len) {
    int sent = 0;
    while (sent < len) {
        int n = send(s, data + sent, len - sent, 0);
        if (n <= 0) return false;
        sent += n;
    }
    return true;
}

Value command_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::nil();
    SOCKET sock = getSock(args[0]);
    if (sock == INVALID_SOCKET) return Value::nil();

    std::string cmd(args[1].asString()->data, args[1].asString()->length);
    std::string req = respEncode(cmd);
    if (!sendAll(sock, req.c_str(), (int)req.size())) return Value::nil();

    // 读取响应（最多 64KB）
    char buf[65536] = {};
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(sock, &rfds);
    timeval tv = {5, 0};
    if (select(0, &rfds, nullptr, nullptr, &tv) <= 0) return Value::nil();

    int n = recv(sock, buf, sizeof(buf) - 1, 0);
    if (n <= 0) return Value::nil();
    buf[n] = '\0';

    const char* p = buf;
    return respDecode(p, nullptr);
}

// ─── 批量命令（Pipeline） ────────────────────────────────────────
Value pipeline_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::nil();
    SOCKET sock = getSock(args[0]);
    if (sock == INVALID_SOCKET) return Value::nil();

    // args[1] 是命令数组
    if (!args[1].isArray()) return Value::nil();
    VMArray* cmds = args[1].asArray();

    // 把所有命令一次性发出去
    for (auto& c : cmds->data) {
        if (!c.isString()) continue;
        std::string cmd(c.asString()->data, c.asString()->length);
        std::string req = respEncode(cmd);
        if (!sendAll(sock, req.c_str(), (int)req.size())) return Value::nil();
    }

    // 读取所有响应
    VMArray* results = VMArray::create();
    for (size_t i = 0; i < cmds->data.size(); i++) {
        if (!cmds->data[i].isString()) {
            results->data.push_back(Value::nil());
            continue;
        }
        char buf[65536] = {};
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(sock, &rfds);
        timeval tv = {5, 0};
        if (select(0, &rfds, nullptr, nullptr, &tv) <= 0) {
            results->data.push_back(Value::nil());
            continue;
        }
        int n = recv(sock, buf, sizeof(buf) - 1, 0);
        if (n <= 0) {
            results->data.push_back(Value::nil());
            continue;
        }
        buf[n] = '\0';
        const char* p = buf;
        results->data.push_back(respDecode(p, nullptr));
    }
    return makeArrayVal(results);
}

} // namespace redis_ns

// ═══════════════════════════════════════════════════════════════════
//  注册函数
// ═══════════════════════════════════════════════════════════════════

void StdLib::registerRedis(VM* vm) {
    registerFunction(vm, "redisConnect",      redis_ns::connect_);
    registerFunction(vm, "redisCommand",      redis_ns::command_);
    registerFunction(vm, "redisPipeline",     redis_ns::pipeline_);
    registerFunction(vm, "redisClose",        redis_ns::close_);
    registerFunction(vm, "redisIsOpen",       redis_ns::isOpen_);
    registerFunction(vm, "redisErrMsg",       redis_ns::errMsg_);
    registerAlias(vm, "Redis连接",            "redisConnect");
    registerAlias(vm, "Redis命令",            "redisCommand");
    registerAlias(vm, "Redis管道",            "redisPipeline");
    registerAlias(vm, "Redis关闭",            "redisClose");
    registerAlias(vm, "Redis是否打开",        "redisIsOpen");
    registerAlias(vm, "Redis错误",            "redisErrMsg");
}
