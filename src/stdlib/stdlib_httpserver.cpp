// CP语言 HTTP 服务端标准库 — 基于 Mongoose (修复安全停止)
#ifdef _WIN32
#include <malloc.h>  // for _alloca
#endif
#define MG_ENABLE_HTTP 1
#define MG_ENABLE_HTTP_WS 1
#include "third_party/mongoose.c"
#include "stdlib/stdlib.hpp"
#include "vm/vm_class.hpp"
#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <atomic>

namespace cplang {

struct ServerCtx {
    struct mg_mgr* mgr;
    std::thread  thread;
    std::atomic<bool> running{true};
    char* rootDir = nullptr; // 静态文件服务根目录
};

static std::map<int, ServerCtx*> g_servers;
static std::mutex g_mutex;

// ── 静态文件服务 ──
static void fileHandler(struct mg_connection* c, int ev, void* ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message* hm = (struct mg_http_message*)ev_data;
        struct mg_http_serve_opts opts = {};
        opts.root_dir = (const char*)c->fn_data;
        mg_http_serve_dir(c, hm, &opts);
    }
}

// ── API 服务 ──
static void apiHandler(struct mg_connection* c, int ev, void* ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message* hm = (struct mg_http_message*)ev_data;
        std::string uri(hm->uri.buf, hm->uri.len);
        std::string method(hm->method.buf, hm->method.len);
        std::string body(hm->body.buf, hm->body.len);
        std::string resp = "{\"path\":\"" + uri + "\",\"method\":\"" + method + "\"}";
        mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s\n", resp.c_str());
    }
}

// ── 事件循环（可安全停止）──
static void eventLoop(ServerCtx* ctx) {
    while (ctx->running.load(std::memory_order_relaxed)) {
        mg_mgr_poll(ctx->mgr, 50); // 50ms 轮询，更快响应停止
    }
}
// 回调服务器事件循环（声明于 CallbackCtx 定义之后）

// ═══════════════════════════════════════════════════════════
//  服务.文件(端口, 目录) → 真/假
// ═══════════════════════════════════════════════════════════

Value httpserver::startFileServer(std::vector<Value>& args) {
    VM* vm = VM::current();
    if (!vm || args.size() < 2 || !args[0].isInt()) return Value::Bool(false);
    int port = (int)args[0].asInt();
    std::string root = args[1].toString();

    std::lock_guard<std::mutex> lk(g_mutex);
    if (g_servers.count(port)) return Value::Bool(false);

    auto ctx = new ServerCtx();
    ctx->mgr = new mg_mgr();
    mg_mgr_init(ctx->mgr);

    char addr[64];
    snprintf(addr, sizeof(addr), "http://0.0.0.0:%d", port);
    char* rootCopy = new char[root.size() + 1];
    strcpy(rootCopy, root.c_str());
    ctx->rootDir = rootCopy;

    mg_http_listen(ctx->mgr, addr, fileHandler, rootCopy);

    ctx->thread = std::thread(eventLoop, ctx);
    g_servers[port] = ctx;
    return Value::Bool(true);
}

// ═══════════════════════════════════════════════════════════
//  服务.API(端口) → 真/假
// ═══════════════════════════════════════════════════════════

Value httpserver::startAPIServer(std::vector<Value>& args) {
    VM* vm = VM::current();
    if (!vm || args.empty() || !args[0].isInt()) return Value::Bool(false);
    int port = (int)args[0].asInt();

    std::lock_guard<std::mutex> lk(g_mutex);
    if (g_servers.count(port)) return Value::Bool(false);

    auto ctx = new ServerCtx();
    ctx->mgr = new mg_mgr();
    mg_mgr_init(ctx->mgr);

    char addr[64];
    snprintf(addr, sizeof(addr), "http://0.0.0.0:%d", port);
    mg_http_listen(ctx->mgr, addr, apiHandler, nullptr);

    ctx->thread = std::thread(eventLoop, ctx);
    g_servers[port] = ctx;
    return Value::Bool(true);
}

// ═══════════════════════════════════════════════════════════
//  服务.停止(端口) → 真/假（安全停止）
// ═══════════════════════════════════════════════════════════

// ═══════════════════════════════════════════════════════════
//  v0.5.0: 回调服务器 — CP 函数处理 HTTP 请求
// ═══════════════════════════════════════════════════════════

struct CallbackCtx {
    struct mg_mgr* mgr;
    std::thread thread;
    std::atomic<bool> running{true};
    VM* vm;
    Value callback;
    std::mutex mtx;
};

static std::map<int, CallbackCtx*> g_callbackServers;

// 回调服务器事件循环
static void callbackEventLoop(CallbackCtx* ctx) {
    while (ctx->running.load(std::memory_order_relaxed)) {
        mg_mgr_poll(ctx->mgr, 50);
    }
}

static void callbackHandler(struct mg_connection* c, int ev, void* ev_data) {
    if (ev != MG_EV_HTTP_MSG) return;
    auto* ctx = (CallbackCtx*)c->fn_data;
    if (!ctx || !ctx->vm) return;

    struct mg_http_message* hm = (struct mg_http_message*)ev_data;

    std::lock_guard<std::mutex> lk(ctx->mtx);

    // 临时设置 thread_local VM（mongoose 线程独立于主线程）
    VM* savedVM = VM::current();
    VM::setCurrent(ctx->vm);

    // 构建请求表
    VM* vm = ctx->vm;
    VMTable* reqTbl = VMTable::create();
    Value reqTable = Value::Table(reqTbl);

    std::string method(hm->method.buf, hm->method.len);
    std::string path(hm->uri.buf, hm->uri.len);
    std::string body(hm->body.buf, hm->body.len);

    reqTbl->set(Value::String(VMString::create("method")), Value::String(VMString::create(method.c_str())));
    reqTbl->set(Value::String(VMString::create("path")),   Value::String(VMString::create(path.c_str())));
    reqTbl->set(Value::String(VMString::create("body")),   Value::String(VMString::create(body.c_str())));

    // 解析查询参数
    VMTable* queryTbl = VMTable::create();
    Value query = Value::Table(queryTbl);
    size_t qpos = path.find('?');
    if (qpos != std::string::npos) {
        std::string qs = path.substr(qpos + 1);
        reqTbl->set(Value::String(VMString::create("query_string")), Value::String(VMString::create(qs.c_str())));
        size_t start = 0;
        while (start < qs.size()) {
            size_t eq = qs.find('=', start);
            size_t amp = qs.find('&', start);
            if (amp == std::string::npos) amp = qs.size();
            if (eq != std::string::npos && eq < amp) {
                std::string k = qs.substr(start, eq - start);
                std::string v = qs.substr(eq + 1, amp - eq - 1);
                queryTbl->set(Value::String(VMString::create(k.c_str())), Value::String(VMString::create(v.c_str())));
            }
            start = amp + 1;
        }
    }
    reqTbl->set(Value::String(VMString::create("query")), query);

    // 调用 CP 回调
    std::vector<Value> callArgs = {reqTable};
    Value result = vm->callFunction(ctx->callback, callArgs);

    // 构建响应
    int status = 200;
    std::string contentType = "text/plain; charset=utf-8";
    std::string respBody;

    if (result.isTable()) {
        auto* tbl = result.asTable();
        Value sv = tbl->get(Value::String(VMString::create("status")));
        if (sv.isInt()) status = (int)sv.asInt();
        Value cv = tbl->get(Value::String(VMString::create("content_type")));
        if (cv.isString()) contentType = cv.toString();
        Value bv = tbl->get(Value::String(VMString::create("body")));
        if (bv.isString()) respBody = bv.toString();
    } else if (result.isString()) {
        respBody = result.toString();
    }

    // 发送响应
    char hdr[256];
    snprintf(hdr, sizeof(hdr), "Content-Type: %s\r\n", contentType.c_str());
    mg_http_reply(c, status, hdr, "%s", respBody.c_str());

    VM::setCurrent(savedVM);
}

Value httpserver::startCallbackServer(std::vector<Value>& args) {
    VM* vm = VM::current();
    if (!vm || args.size() < 2 || !args[0].isInt()) return Value::Bool(false);
    int port = (int)args[0].asInt();
    Value cb = args[1];
    if (!cb.isFunction() && !cb.isClosure()) return Value::Bool(false);

    std::lock_guard<std::mutex> lk(g_mutex);
    if (g_callbackServers.count(port)) return Value::Bool(false);

    auto ctx = new CallbackCtx();
    ctx->vm = vm;
    ctx->callback = cb;
    ctx->mgr = new mg_mgr();
    mg_mgr_init(ctx->mgr);

    char addr[64];
    snprintf(addr, sizeof(addr), "http://0.0.0.0:%d", port);
    mg_http_listen(ctx->mgr, addr, callbackHandler, ctx);

    ctx->thread = std::thread(callbackEventLoop, ctx);
    g_callbackServers[port] = ctx;
    return Value::Bool(true);
}

Value httpserver::stop(std::vector<Value>& args) {
    if (args.empty() || !args[0].isInt()) return Value::Bool(false);
    int port = (int)args[0].asInt();

    std::lock_guard<std::mutex> lk(g_mutex);

    // 尝试停止普通服务器
    auto it = g_servers.find(port);
    if (it != g_servers.end()) {
        ServerCtx* ctx = it->second;
        ctx->running.store(false);
        if (ctx->thread.joinable()) ctx->thread.join();
        mg_mgr_free(ctx->mgr);
        delete ctx->mgr;
        delete[] ctx->rootDir;
        delete ctx;
        g_servers.erase(it);
        return Value::Bool(true);
    }

    // 尝试停止回调服务器
    auto cbIt = g_callbackServers.find(port);
    if (cbIt != g_callbackServers.end()) {
        CallbackCtx* ctx = cbIt->second;
        ctx->running.store(false);
        if (ctx->thread.joinable()) ctx->thread.join();
        mg_mgr_free(ctx->mgr);
        delete ctx->mgr;
        delete ctx;
        g_callbackServers.erase(cbIt);
        return Value::Bool(true);
    }

    return Value::Bool(false);
}

void StdLib::registerHTTPServer(VM* vm) {
    registerFunction(vm, "http_start_file",     httpserver::startFileServer);
    registerFunction(vm, "http_start_api",      httpserver::startAPIServer);
    registerFunction(vm, "http_start_callback", httpserver::startCallbackServer);
    registerFunction(vm, "http_stop",           httpserver::stop);
    registerAlias(vm, "服务.文件",     "http_start_file");
    registerAlias(vm, "服务.API",      "http_start_api");
    registerAlias(vm, "服务.回调",     "http_start_callback");
    registerAlias(vm, "服务.停止",     "http_stop");
}

} // namespace cplang
