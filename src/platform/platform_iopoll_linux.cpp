// CP语言 平台抽象层 — Linux/Android epoll IO 多路复用
#if defined(__linux__) || defined(__ANDROID__) || defined(__APPLE__)

#include "platform/platform.hpp"
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

namespace cplang {
namespace platform {

// ═══════════════════════════════════════════════════════════════════
//  内部结构：epoll 上下文
// ═══════════════════════════════════════════════════════════════════
struct IOPollCtx {
    int  epfd = -1;       // epoll 文件描述符
    int  eventfd = -1;    // 用于 iopoll_interrupt 的事件文件描述符
    int  max_events = 64;
};

// ── 创建 epoll ──
void* iopoll_create(int max_events) {
    auto* ctx = new IOPollCtx();
    ctx->max_events = (max_events > 0) ? max_events : 64;

    ctx->epfd = epoll_create1(EPOLL_CLOEXEC);
    if (ctx->epfd < 0) {
        delete ctx;
        return nullptr;
    }

    // 创建 eventfd 用于中断 epoll_wait
    ctx->eventfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (ctx->eventfd < 0) {
        close(ctx->epfd);
        delete ctx;
        return nullptr;
    }

    // 将 eventfd 加入 epoll（用于中断信号）
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.u64 = (uint64_t)-1;  // 特殊标记：中断信号
    epoll_ctl(ctx->epfd, EPOLL_CTL_ADD, ctx->eventfd, &ev);

    return ctx;
}

// ── 将 socket 注册到 epoll ──
int iopoll_add(void* poll, int sock, uint32_t events, uint64_t user_data) {
    if (!poll || sock < 0) return -1;
    auto* ctx = static_cast<IOPollCtx*>(poll);

    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLRDHUP | EPOLLERR | EPOLLHUP;  // 始终监听错误/挂断
    if (events & IOPollEvent::IOPOLL_IN)  ev.events |= EPOLLIN;
    if (events & IOPollEvent::IOPOLL_OUT) ev.events |= EPOLLOUT;
    ev.data.u64 = user_data;

    int ret = epoll_ctl(ctx->epfd, EPOLL_CTL_ADD, sock, &ev);
    return ret;
}

int iopoll_mod(void* poll, int sock, uint32_t events, uint64_t user_data) {
    if (!poll || sock < 0) return -1;
    auto* ctx = static_cast<IOPollCtx*>(poll);

    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLRDHUP | EPOLLERR | EPOLLHUP;
    if (events & IOPollEvent::IOPOLL_IN)  ev.events |= EPOLLIN;
    if (events & IOPollEvent::IOPOLL_OUT) ev.events |= EPOLLOUT;
    ev.data.u64 = user_data;

    int ret = epoll_ctl(ctx->epfd, EPOLL_CTL_MOD, sock, &ev);
    return ret;
}

int iopoll_del(void* poll, int sock) {
    if (!poll || sock < 0) return -1;
    auto* ctx = static_cast<IOPollCtx*>(poll);

    struct epoll_event ev;  // epoll_ctl DEL 时 ev 可忽略 (Linux >= 2.6.9)
    memset(&ev, 0, sizeof(ev));
    int ret = epoll_ctl(ctx->epfd, EPOLL_CTL_DEL, sock, &ev);
    return ret;
}

// ── 等待 IO 事件 ──
int iopoll_wait(void* poll, IOPollResult* results, int max_results, int timeout_ms) {
    if (!poll) return 0;
    auto* ctx = static_cast<IOPollCtx*>(poll);

    int batch = (max_results < 64) ? max_results : 64;

    struct epoll_event evs[64];
    int nfds = epoll_wait(ctx->epfd, evs, batch, timeout_ms);
    if (nfds < 0) {
        if (errno == EINTR) return 0;  // 被信号中断，返回空
        return -1;
    }

    int n = 0;
    for (int i = 0; i < nfds && n < max_results; i++) {
        // 过滤中断信号
        if (evs[i].data.u64 == (uint64_t)-1) {
            // 消费 eventfd 中的数据（防止反复触发）
            uint64_t val;
            ssize_t r = read(ctx->eventfd, &val, sizeof(val));
            (void)r;
            continue;
        }

        results[n].user_data = evs[i].data.u64;
        results[n].events = 0;
        if (evs[i].events & EPOLLIN)  results[n].events |= IOPollEvent::IOPOLL_IN;
        if (evs[i].events & EPOLLOUT) results[n].events |= IOPollEvent::IOPOLL_OUT;
        if (evs[i].events & EPOLLERR) results[n].events |= IOPollEvent::IOPOLL_ERR;
        if (evs[i].events & EPOLLHUP) results[n].events |= IOPollEvent::IOPOLL_HUP;
        if (evs[i].events & EPOLLRDHUP) results[n].events |= IOPollEvent::IOPOLL_HUP;
        results[n].error_code = 0;
        n++;
    }

    return n;
}

// ── 中断 wait ──
void iopoll_interrupt(void* poll) {
    if (!poll) return;
    auto* ctx = static_cast<IOPollCtx*>(poll);
    if (ctx->eventfd >= 0) {
        uint64_t val = 1;
        ssize_t w = write(ctx->eventfd, &val, sizeof(val));
        (void)w;
    }
}

// ── 销毁 ──
void iopoll_destroy(void* poll) {
    if (!poll) return;
    auto* ctx = static_cast<IOPollCtx*>(poll);
    if (ctx->epfd >= 0) close(ctx->epfd);
    if (ctx->eventfd >= 0) close(ctx->eventfd);
    delete ctx;
}

} // namespace platform
} // namespace cplang

#endif // __linux__ || __ANDROID__ || __APPLE__
