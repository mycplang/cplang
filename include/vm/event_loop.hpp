// CP语言 事件循环 — 非阻塞异步调度
// 支持：定时器、微任务、异步I/O回调
#pragma once
#include "common/types.hpp"
#include <functional>
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <chrono>

namespace cplang {

class EventLoop {
public:
    using Microtask = std::function<void()>;
    using TimerCallback = std::function<void()>;
    using IOTask = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // ── 生命周期 ──
    void start();   // 在新线程启动事件循环
    void stop();    // 停止事件循环
    bool isRunning() const { return running_.load(); }
    
    // ── 微任务（Promise回调） ──
    void enqueueMicrotask(Microtask task);
    void runMicrotasks();            // 在当前线程执行所有微任务
    void runMicrotasksSync();        // 阻塞直到微任务队列清空
    
    // ── 定时器 ──
    /// 延迟指定毫秒后执行回调，返回定时器ID
    uint64_t setTimeout(TimerCallback cb, uint64_t delayMs);
    /// 每隔指定毫秒执行回调，返回定时器ID  
    uint64_t setInterval(TimerCallback cb, uint64_t intervalMs);
    /// 清除定时器
    void clearTimer(uint64_t timerId);
    
    // ── 异步I/O ──
    /// 在后台线程执行阻塞I/O，完成后在当前线程调度回调
    void runAsync(IOTask blockingTask, TimerCallback onComplete);
    
    // ── 工具 ──
    /// 获取当前单调时间（毫秒）
    static uint64_t nowMs();
    /// 下一次定时器到期还需等待多久（毫秒），无定时器返回 UINT64_MAX
    uint64_t nextTimerDelayMs() const;
    /// 待处理的微任务数量
    size_t pendingMicrotasks() const;
    /// 活跃定时器数量
    size_t activeTimers() const;

private:
    void loop();  // 事件循环主函数

    struct Timer {
        uint64_t id;
        uint64_t deadline;   // 绝对到期时间 (nowMs())
        uint64_t interval;   // 0 = 一次性, >0 = 重复间隔
        TimerCallback callback;
        
        // 优先队列：最早到期者优先
        bool operator<(const Timer& other) const {
            return deadline > other.deadline;  // 最小堆
        }
    };

    std::atomic<bool>        running_;
    std::thread               thread_;
    
    mutable std::mutex        mutex_;
    std::condition_variable   cv_;
    
    std::priority_queue<Timer> timers_;
    std::queue<Microtask>     microtasks_;
    uint64_t                  nextTimerId_ = 1;
    
    // I/O线程池（简化：单线程）
    std::thread               ioThread_;
    std::queue<IOTask>        ioQueue_;
    std::mutex                ioMutex_;
    std::condition_variable   ioCv_;
};

} // namespace cplang
