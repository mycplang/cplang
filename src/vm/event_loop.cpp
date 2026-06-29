// CP语言 事件循环实现
#include "vm/event_loop.hpp"
#include <condition_variable>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

namespace cplang {

// ═══════════════════════════════════════════════════════════════
//  构造 & 析构
// ═══════════════════════════════════════════════════════════════

EventLoop::EventLoop() : running_(false) {}

EventLoop::~EventLoop() {
    stop();
}

// ═══════════════════════════════════════════════════════════════
//  生命周期
// ═══════════════════════════════════════════════════════════════

void EventLoop::start() {
    if (running_.load()) return;
    running_.store(true);
    
    // 启动 I/O 工作线程
    ioThread_ = std::thread([this]() {
        while (running_.load()) {
            IOTask task;
            {
                std::unique_lock<std::mutex> lock(ioMutex_);
                ioCv_.wait(lock, [this]() { 
                    return !ioQueue_.empty() || !running_.load(); 
                });
                if (!running_.load()) return;
                if (!ioQueue_.empty()) {
                    task = std::move(ioQueue_.front());
                    ioQueue_.pop();
                }
            }
            if (task) task();
        }
    });
    
    // 启动事件循环线程
    thread_ = std::thread([this]() { loop(); });
}

void EventLoop::stop() {
    running_.store(false);
    cv_.notify_all();
    ioCv_.notify_all();
    
    if (thread_.joinable()) thread_.join();
    if (ioThread_.joinable()) ioThread_.join();
}

// ═══════════════════════════════════════════════════════════════
//  微任务
// ═══════════════════════════════════════════════════════════════

void EventLoop::enqueueMicrotask(Microtask task) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        microtasks_.push(std::move(task));
    }
    cv_.notify_one();
}

void EventLoop::runMicrotasks() {
    std::queue<Microtask> tasks;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::swap(tasks, microtasks_);
    }
    while (!tasks.empty()) {
        tasks.front()();
        tasks.pop();
    }
}

void EventLoop::runMicrotasksSync() {
    runMicrotasks();
    // 继续处理直到队列为空
    while (true) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (microtasks_.empty()) break;
        lock.unlock();
        runMicrotasks();
    }
}

// ═══════════════════════════════════════════════════════════════
//  定时器
// ═══════════════════════════════════════════════════════════════

uint64_t EventLoop::setTimeout(TimerCallback cb, uint64_t delayMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    uint64_t id = nextTimerId_++;
    Timer t;
    t.id = id;
    t.deadline = nowMs() + delayMs;
    t.interval = 0;
    t.callback = std::move(cb);
    timers_.push(t);
    cv_.notify_one();
    return id;
}

uint64_t EventLoop::setInterval(TimerCallback cb, uint64_t intervalMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    uint64_t id = nextTimerId_++;
    Timer t;
    t.id = id;
    t.deadline = nowMs() + intervalMs;
    t.interval = intervalMs;
    t.callback = std::move(cb);
    timers_.push(t);
    cv_.notify_one();
    return id;
}

void EventLoop::clearTimer(uint64_t timerId) {
    // 简化：标记清除（不在优先队列中直接删除，在到期时跳过）
    // 使用 id=0 标记为已取消
    std::lock_guard<std::mutex> lock(mutex_);
    std::priority_queue<Timer> newTimers;
    while (!timers_.empty()) {
        Timer t = timers_.top();
        timers_.pop();
        if (t.id != timerId) newTimers.push(t);
    }
    timers_ = std::move(newTimers);
}

// ═══════════════════════════════════════════════════════════════
//  异步I/O
// ═══════════════════════════════════════════════════════════════

void EventLoop::runAsync(IOTask blockingTask, TimerCallback onComplete) {
    // 在I/O线程执行，完成后把回调放入微任务队列
    {
        std::lock_guard<std::mutex> lock(ioMutex_);
        ioQueue_.push([this, task = std::move(blockingTask), cb = std::move(onComplete)]() {
            task();
            if (cb) {
                enqueueMicrotask([cb = std::move(cb)]() { cb(); });
            }
        });
    }
    ioCv_.notify_one();
}

// ═══════════════════════════════════════════════════════════════
//  工具
// ═══════════════════════════════════════════════════════════════

uint64_t EventLoop::nowMs() {
    static auto start = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::steady_clock::now() - start;
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()
    );
}

uint64_t EventLoop::nextTimerDelayMs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (timers_.empty()) return UINT64_MAX;
    uint64_t now = nowMs();
    if (timers_.top().deadline <= now) return 0;
    return timers_.top().deadline - now;
}

size_t EventLoop::pendingMicrotasks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return microtasks_.size();
}

size_t EventLoop::activeTimers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return timers_.size();
}

// ═══════════════════════════════════════════════════════════════
//  事件循环主循环
// ═══════════════════════════════════════════════════════════════

void EventLoop::loop() {
    while (running_.load()) {
        // 1. 先处理所有微任务
        runMicrotasks();
        
        // 2. 处理到期的定时器
        {
            std::unique_lock<std::mutex> lock(mutex_);
            uint64_t now = nowMs();
            
            // 收集所有到期的定时器
            std::vector<Timer> due;
            while (!timers_.empty() && timers_.top().deadline <= now) {
                due.push_back(timers_.top());
                timers_.pop();
            }
            lock.unlock();
            
            // 执行定时器回调
            for (auto& t : due) {
                if (t.callback) t.callback();
                // 如果是周期定时器，重新加入
                if (t.interval > 0) {
                    setInterval(std::move(t.callback), t.interval);
                }
            }
        }
        
        // 3. 等待下一个定时器或新任务
        {
            std::unique_lock<std::mutex> lock(mutex_);
            uint64_t delay = nextTimerDelayMs();
            if (delay == UINT64_MAX) {
                // 没有定时器，无限等待
                cv_.wait(lock, [this]() { 
                    return !microtasks_.empty() || !running_.load(); 
                });
            } else if (delay > 0) {
                // 等待直到下一个定时器到期
                cv_.wait_for(lock, std::chrono::milliseconds(delay));
            }
            // delay == 0：立即回到循环顶部处理到期定时器
        }
    }
}

} // namespace cplang
