// CP语言 平台抽象层 — Windows IOCP IO 多路复用
#ifdef _WIN32

#include "platform/platform.hpp"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <mswsock.h>
#include <vector>
#include <cstring>

// 如果需要 AcceptEx（暂不使用）
#pragma comment(lib, "mswsock.lib")

namespace cplang {
namespace platform {

// ═══════════════════════════════════════════════════════════════════
//  内部结构：每个已注册 socket 的 overlapped 状态
// ═══════════════════════════════════════════════════════════════════
struct IOPollCtx {
    HANDLE  iocp = nullptr;          // IOCP 句柄
    HANDLE  interrupt_event = nullptr; // 用于中断 wait
    int     max_events = 64;

    // per-socket overlapped 缓冲区
    struct SockEntry {
        OVERLAPPED  ov = {};
        WSABUF      buf = {0, nullptr};
        uint64_t    user_data = 0;
        bool        pending = false;
        char        dummy[1];          // 1 字节探测缓冲区
    };
    std::vector<SockEntry>   entries;  // 按 fd 索引，扩容
    CRITICAL_SECTION         lock;
};

// ── 为 socket 投递一个零字节 recv 作为可读探测器 ──
static int postReadProbe(IOPollCtx* ctx, int sock, uint64_t user_data) {
    int idx = sock;
    if (idx < 0) return -1;

    if (idx >= (int)ctx->entries.size()) {
        ctx->entries.resize(idx + 1);
    }
    auto& e = ctx->entries[idx];

    // 如果已有 pending 操作，不需要重复投递
    if (e.pending) return 0;

    memset(&e.ov, 0, sizeof(e.ov));
    e.buf.buf = e.dummy;
    e.buf.len = 1;
    e.user_data = user_data;
    e.pending = true;

    DWORD flags = 0;
    int ret = WSARecv(sock, &e.buf, 1, nullptr, &flags, &e.ov, nullptr);
    if (ret == 0 || WSAGetLastError() == WSA_IO_PENDING) {
        return 0;
    }
    e.pending = false;
    return -1;
}

// ── 创建 IOCP ──
void* iopoll_create(int max_events) {
    auto* ctx = new IOPollCtx();
    ctx->max_events = (max_events > 0) ? max_events : 64;

    ctx->iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    if (!ctx->iocp) {
        delete ctx;
        return nullptr;
    }

    ctx->interrupt_event = CreateEventA(nullptr, FALSE, FALSE, nullptr);
    InitializeCriticalSection(&ctx->lock);

    return ctx;
}

// ── 将 socket 注册到 IOCP ──
int iopoll_add(void* poll, int sock, uint32_t events, uint64_t user_data) {
    if (!poll || sock < 0) return -1;
    auto* ctx = static_cast<IOPollCtx*>(poll);

    EnterCriticalSection(&ctx->lock);

    // 绑定 socket 到 IOCP
    HANDLE h = CreateIoCompletionPort((HANDLE)(intptr_t)sock, ctx->iocp,
                                       (ULONG_PTR)0, 0);
    if (!h) {
        LeaveCriticalSection(&ctx->lock);
        return -1;
    }

    // 设置非阻塞模式
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);

    // 投递可读探测器
    if (events & IOPollEvent::IOPOLL_IN) {
        postReadProbe(ctx, sock, user_data);
    }

    LeaveCriticalSection(&ctx->lock);
    return 0;
}

int iopoll_mod(void* poll, int sock, uint32_t events, uint64_t user_data) {
    // 简化实现：等同于 del + add
    iopoll_del(poll, sock);
    return iopoll_add(poll, sock, events, user_data);
}

int iopoll_del(void* poll, int sock) {
    if (!poll || sock < 0) return -1;
    auto* ctx = static_cast<IOPollCtx*>(poll);

    EnterCriticalSection(&ctx->lock);

    if (sock < (int)ctx->entries.size()) {
        auto& e = ctx->entries[sock];
        if (e.pending) {
            // 取消 pending 的 overlapped 操作
            CancelIo((HANDLE)(intptr_t)sock);
            e.pending = false;
        }
    }

    LeaveCriticalSection(&ctx->lock);
    return 0;
}

// ── 等待 IO 事件 ──
int iopoll_wait(void* poll, IOPollResult* results, int max_results, int timeout_ms) {
    if (!poll) return 0;
    auto* ctx = static_cast<IOPollCtx*>(poll);

    // 限制单次获取数量
    int batch = (max_results < 64) ? max_results : 64;

    OVERLAPPED_ENTRY entries[64];
    ULONG count = 0;

    BOOL ok = GetQueuedCompletionStatusEx(
        ctx->iocp,
        entries,
        batch,
        &count,
        timeout_ms,
        FALSE  // alertable
    );

    if (!ok && count == 0) return 0;

    int n = 0;
    for (ULONG i = 0; i < count && n < max_results; i++) {
        // 跳过中断信号（自定义完成包 lpCompletionKey == 0 && lpOverlapped == nullptr）
        if (entries[i].lpOverlapped == nullptr) continue;

        // 从 overlapped 反向查找 user_data
        OVERLAPPED* ov = entries[i].lpOverlapped;
        uint64_t user_data = 0;
        int sock = -1;

        EnterCriticalSection(&ctx->lock);
        // 线性搜索 entry（socket 数量少时可行）
        for (int j = 0; j < (int)ctx->entries.size(); j++) {
            if (&ctx->entries[j].ov == ov) {
                sock = j;
                user_data = ctx->entries[j].user_data;
                ctx->entries[j].pending = false;
                break;
            }
        }
        LeaveCriticalSection(&ctx->lock);

        if (sock < 0) continue;

        results[n].user_data = user_data;
        results[n].events = IOPollEvent::IOPOLL_IN;  // recv 完成 = 可读
        results[n].error_code = 0;

        // 重新投递探测器
        postReadProbe(ctx, sock, user_data);

        n++;
    }

    return n;
}

// ── 中断 wait ──
void iopoll_interrupt(void* poll) {
    if (!poll) return;
    auto* ctx = static_cast<IOPollCtx*>(poll);
    // Post 一个空的完成包到 IOCP 以唤醒等待线程
    PostQueuedCompletionStatus(ctx->iocp, 0, 0, nullptr);
}

// ── 销毁 ──
void iopoll_destroy(void* poll) {
    if (!poll) return;
    auto* ctx = static_cast<IOPollCtx*>(poll);
    CloseHandle(ctx->iocp);
    CloseHandle(ctx->interrupt_event);
    DeleteCriticalSection(&ctx->lock);
    ctx->entries.clear();
    delete ctx;
}

} // namespace platform
} // namespace cplang

#endif // _WIN32
