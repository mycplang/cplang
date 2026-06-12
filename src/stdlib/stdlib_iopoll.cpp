// CP语言 标准库 — IO 多路复用 (IOCP / epoll)
// 
// 架构: C++ 后台线程 (IOCP/epoll wait) + Channel 投递事件到 CP 侧
// CP 语言 API:
//   循环 = 启动事件循环(最大事件数)
//   事件循环注册(循环, socket)
//   事件循环注册读写(循环, socket)
//   事件 = 事件循环接收(循环)
//   事件 = 事件循环接收等待(循环, 超时毫秒)
//   事件循环停止(循环)

#include "stdlib/stdlib.hpp"
#include "platform/platform.hpp"
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  内部上下文：管理 IOCP/epoll 句柄 + 后台线程 + 事件队列
// ═══════════════════════════════════════════════════════════════════

namespace iopoll_ns {

struct PollContext {
    void*              poll_handle = nullptr;
    std::thread*       worker = nullptr;
    std::atomic<bool>  running{false};
    int                max_events = 1024;

    // 事件队列：后台线程写入，CP 侧读取
    std::queue<VMTable*> event_queue;
    std::mutex           queue_mtx;
    std::condition_variable queue_cv;
};

// ── 辅助：创建事件表 ──
static VMTable* makeEvent(int fd, uint32_t events, int error_code) {
    VMTable* t = VMTable::create();
    t->set(Value::String(VMString::create("fd")),     Value::Int(fd));
    t->set(Value::String(VMString::create("events")), Value::Int((Int64)events));
    t->set(Value::String(VMString::create("error")),  Value::Int(error_code));
    return t;
}

// ── 后台工作线程 ──
static void pollWorker(PollContext* ctx) {
    std::vector<platform::IOPollResult> results(ctx->max_events);

    while (ctx->running.load(std::memory_order_relaxed)) {
        int n = platform::iopoll_wait(ctx->poll_handle,
                                       results.data(),
                                       ctx->max_events,
                                       100);  // 100ms 超时，便于检查 running

        if (n <= 0) continue;

        // 批量入队
        {
            std::lock_guard<std::mutex> lk(ctx->queue_mtx);
            for (int i = 0; i < n; i++) {
                if (results[i].user_data == 0) continue;
                ctx->event_queue.push(
                    makeEvent((int)results[i].user_data,
                              results[i].events,
                              results[i].error_code));
            }
        }
        ctx->queue_cv.notify_one();
    }
}

// ── 全局 PollContext 表（CP 侧通过整数 handle 索引）──
static std::mutex g_ctx_mutex;
static std::vector<PollContext*> g_contexts;
static int g_next_handle = 1;

static int storeCtx(PollContext* ctx) {
    std::lock_guard<std::mutex> lk(g_ctx_mutex);
    int h = g_next_handle++;
    if (h >= (int)g_contexts.size()) {
        g_contexts.resize(h + 1, nullptr);
    }
    g_contexts[h] = ctx;
    return h;
}

static PollContext* getCtx(int handle) {
    std::lock_guard<std::mutex> lk(g_ctx_mutex);
    if (handle < 0 || handle >= (int)g_contexts.size()) return nullptr;
    return g_contexts[handle];
}

static void removeCtx(int handle) {
    std::lock_guard<std::mutex> lk(g_ctx_mutex);
    if (handle >= 0 && handle < (int)g_contexts.size()) {
        g_contexts[handle] = nullptr;
    }
}

// ═══════════════════════════════════════════════════════════════════
//  CP 语言 API 实现
// ═══════════════════════════════════════════════════════════════════

// 启动事件循环(最大事件数) → 返回循环句柄(int)
Value iopollCreate_(std::vector<Value>& args) {
    auto* ctx = new PollContext();

    if (!args.empty() && args[0].isInt()) {
        ctx->max_events = (int)args[0].asInt();
        if (ctx->max_events < 1) ctx->max_events = 1;
    }

    ctx->poll_handle = platform::iopoll_create(ctx->max_events);
    if (!ctx->poll_handle) {
        delete ctx;
        VM::current()->raiseError("启动事件循环失败: 无法创建 IO 多路复用句柄");
        return Value::nil();
    }

    int handle = storeCtx(ctx);
    if (handle < 0) {
        platform::iopoll_destroy(ctx->poll_handle);
        delete ctx;
        VM::current()->raiseError("启动事件循环失败: 内部错误");
        return Value::nil();
    }

    // 启动后台线程
    ctx->running.store(true);
    try {
        ctx->worker = new std::thread(pollWorker, ctx);
    } catch (...) {
        ctx->running.store(false);
        platform::iopoll_destroy(ctx->poll_handle);
        removeCtx(handle);
        delete ctx;
        VM::current()->raiseError("启动事件循环失败: 无法创建后台线程");
        return Value::nil();
    }

    return Value::Int(handle);
}

// 事件循环注册(循环, socket)
Value iopollRegister_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Bool(false);

    auto* ctx = getCtx((int)args[0].asInt());
    if (!ctx) return Value::Bool(false);

    int sock = (int)args[1].asInt();
    if (sock < 0) return Value::Bool(false);

    int ret = platform::iopoll_add(ctx->poll_handle, sock,
                                    platform::IOPollEvent::IOPOLL_IN,
                                    (uint64_t)sock);
    return Value::Bool(ret == 0);
}

// 事件循环注册读写(循环, socket)
Value iopollRegisterRW_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Bool(false);

    auto* ctx = getCtx((int)args[0].asInt());
    if (!ctx) return Value::Bool(false);

    int sock = (int)args[1].asInt();
    if (sock < 0) return Value::Bool(false);

    int ret = platform::iopoll_add(ctx->poll_handle, sock,
                                    platform::IOPollEvent::IOPOLL_IN |
                                    platform::IOPollEvent::IOPOLL_OUT,
                                    (uint64_t)sock);
    return Value::Bool(ret == 0);
}

// 事件循环注销(循环, socket)
Value iopollUnregister_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Bool(false);

    auto* ctx = getCtx((int)args[0].asInt());
    if (!ctx) return Value::Bool(false);

    int sock = (int)args[1].asInt();
    if (sock < 0) return Value::Bool(false);

    int ret = platform::iopoll_del(ctx->poll_handle, sock);
    return Value::Bool(ret == 0);
}

// 事件循环接收(循环) — 非阻塞获取一个事件，无事件返回 nil
Value iopollRecv_(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();

    auto* ctx = getCtx((int)args[0].asInt());
    if (!ctx) return Value::nil();

    std::unique_lock<std::mutex> lk(ctx->queue_mtx, std::try_to_lock);
    if (!lk.owns_lock() || ctx->event_queue.empty()) {
        return Value::nil();
    }

    VMTable* ev = ctx->event_queue.front();
    ctx->event_queue.pop();

    // 将 VMTable 所有权转移给 VM（trackGC）
    VM::current()->trackGC(ev);
    return Value::Table(ev);
}

// 事件循环接收等待(循环, 超时毫秒) — 阻塞直到有事件或超时
Value iopollRecvWait_(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();

    auto* ctx = getCtx((int)args[0].asInt());
    if (!ctx) return Value::nil();

    int timeout_ms = -1;
    if (args.size() > 1 && args[1].isInt()) {
        timeout_ms = (int)args[1].asInt();
    }

    std::unique_lock<std::mutex> lk(ctx->queue_mtx);

    if (timeout_ms < 0) {
        // 无限等待
        ctx->queue_cv.wait(lk, [ctx] {
            return !ctx->event_queue.empty() || !ctx->running.load();
        });
    } else if (timeout_ms > 0) {
        // 带超时等待
        bool got = ctx->queue_cv.wait_for(lk,
            std::chrono::milliseconds(timeout_ms),
            [ctx] { return !ctx->event_queue.empty(); });
        if (!got) return Value::nil();  // 超时返回 nil
    }

    if (ctx->event_queue.empty()) return Value::nil();

    VMTable* ev = ctx->event_queue.front();
    ctx->event_queue.pop();

    VM::current()->trackGC(ev);
    return Value::Table(ev);
}

// 事件循环停止(循环)
Value iopollStop_(std::vector<Value>& args) {
    if (args.empty()) return Value::Bool(false);

    int handle = (int)args[0].asInt();
    auto* ctx = getCtx(handle);
    if (!ctx) return Value::Bool(false);

    // 1. 停止标志
    ctx->running.store(false);

    // 2. 唤醒阻塞的 IOCP/epoll
    platform::iopoll_interrupt(ctx->poll_handle);

    // 3. 唤醒阻塞在 queue_cv 上的线程
    ctx->queue_cv.notify_all();

    // 4. 等待后台线程退出
    if (ctx->worker) {
        ctx->worker->join();
        delete ctx->worker;
        ctx->worker = nullptr;
    }

    // 5. 销毁平台句柄
    platform::iopoll_destroy(ctx->poll_handle);

    // 6. 清空事件队列
    {
        std::lock_guard<std::mutex> lk(ctx->queue_mtx);
        while (!ctx->event_queue.empty()) {
            VMTable* ev = ctx->event_queue.front();
            ctx->event_queue.pop();
            // 事件表可能已注册 GC，需要让 VM 跟踪
            VM::current()->trackGC(ev);
        }
    }

    // 7. 清理
    removeCtx(handle);
    delete ctx;

    return Value::Bool(true);
}

} // namespace iopoll_ns

// ═══════════════════════════════════════════════════════════════════
//  注册到 VM
// ═══════════════════════════════════════════════════════════════════

void StdLib::registerIOPoll(VM* vm) {
    using namespace iopoll_ns;

    registerFunction(vm, "iopoll_create",        iopollCreate_);
    registerFunction(vm, "iopoll_register",      iopollRegister_);
    registerFunction(vm, "iopoll_register_rw",   iopollRegisterRW_);
    registerFunction(vm, "iopoll_unregister",    iopollUnregister_);
    registerFunction(vm, "iopoll_recv",          iopollRecv_);
    registerFunction(vm, "iopoll_recv_wait",     iopollRecvWait_);
    registerFunction(vm, "iopoll_stop",          iopollStop_);

    // 中文别名
    registerAlias(vm, "启动事件循环",       "iopoll_create");
    registerAlias(vm, "事件循环注册",       "iopoll_register");
    registerAlias(vm, "事件循环注册读写",   "iopoll_register_rw");
    registerAlias(vm, "事件循环注销",       "iopoll_unregister");
    registerAlias(vm, "事件循环接收",       "iopoll_recv");
    registerAlias(vm, "事件循环接收等待",   "iopoll_recv_wait");
    registerAlias(vm, "事件循环停止",       "iopoll_stop");
}

} // namespace cplang
