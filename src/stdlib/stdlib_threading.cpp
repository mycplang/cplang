#include "stdlib/stdlib.hpp"

namespace cplang {

// Threading module for CP stdlib
// #include'd from stdlib.cpp, already inside namespace cplang

namespace thread_ns {

// ── TLS (global, shared across threads via mutex) ──
static std::mutex tlsMutex;
static std::unordered_map<std::string, Value> tlsStore;

// ── Helper to extract VMObject* from Value ──
// Uses typeTag-based check instead of dynamic_cast for RTTI-independent operation.
namespace detail {
    template<typename T> struct TypeTagFor;
    template<> struct TypeTagFor<VMMutex>     { static constexpr UInt8 value = ObjectHeader::TAG_MUTEX; };
    template<> struct TypeTagFor<VMCondition> { static constexpr UInt8 value = ObjectHeader::TAG_CONDITION; };
    template<> struct TypeTagFor<VMSemaphore> { static constexpr UInt8 value = ObjectHeader::TAG_SEMAPHORE; };
    template<> struct TypeTagFor<VMBarrier>   { static constexpr UInt8 value = ObjectHeader::TAG_BARRIER; };
    template<> struct TypeTagFor<VMAtomicInt> { static constexpr UInt8 value = ObjectHeader::TAG_ATOMIC_INT; };
    template<> struct TypeTagFor<VMRWLock>    { static constexpr UInt8 value = ObjectHeader::TAG_RWLOCK; };
    template<> struct TypeTagFor<VMChannel>   { static constexpr UInt8 value = ObjectHeader::TAG_CHANNEL; };
    template<> struct TypeTagFor<VMFuture>    { static constexpr UInt8 value = ObjectHeader::TAG_FUTURE; };
    template<> struct TypeTagFor<VMThread>    { static constexpr UInt8 value = ObjectHeader::TAG_THREAD; };
}

template<typename T>
T* objCast(const Value& v) {
    if (!v.isPtr() || !v.asPtr()) return nullptr;
    if (v.asPtr()->typeTag != detail::TypeTagFor<T>::value) return nullptr;
    return static_cast<T*>(v.asPtr());
}
template<typename T>
T* objCastOrNil(const Value& v) {
    if (v.isNil() || !v.isPtr() || !v.asPtr()) return nullptr;
    if (v.asPtr()->typeTag != detail::TypeTagFor<T>::value) return nullptr;
    return static_cast<T*>(v.asPtr());
}

// ── Mutex ──
Value mutexCreate_(std::vector<Value>&) {
    VMMutex* m = VMMutex::create();
    VM::current()->trackGC(m);
    return makePtrVal(m);
}
Value mutexLock_(std::vector<Value>& args) {
    VMMutex* m = objCast<VMMutex>(args[0]);
    if (m) { m->mtx.lock(); return Value::Bool(true); }
    return Value::Bool(false);
}
Value mutexUnlock_(std::vector<Value>& args) {
    VMMutex* m = objCast<VMMutex>(args[0]);
    if (m) { m->mtx.unlock(); return Value::Bool(true); }
    return Value::Bool(false);
}
Value mutexTryLock_(std::vector<Value>& args) {
    VMMutex* m = objCast<VMMutex>(args[0]);
    if (m) return Value::Bool(m->mtx.try_lock());
    return Value::Bool(false);
}

// ── Condition Variable ──
Value condCreate_(std::vector<Value>&) {
    VMCondition* c = VMCondition::create();
    VM::current()->trackGC(c);
    return makePtrVal(c);
}

// ── Semaphore ──
Value semCreate_(std::vector<Value>& args) {
    VMSemaphore* s = VMSemaphore::create();
    if (!args.empty() && args[0].isInt()) s->count = args[0].asInt();
    VM::current()->trackGC(s);
    return makePtrVal(s);
}
Value semPost_(std::vector<Value>& args) {
    VMSemaphore* s = objCast<VMSemaphore>(args[0]);
    if (!s) return Value::Bool(false);
    { std::lock_guard<std::mutex> lk(s->mtx); s->count++; }
    s->cv.notify_one();
    return Value::Bool(true);
}
Value semTryWait_(std::vector<Value>& args) {
    VMSemaphore* s = objCast<VMSemaphore>(args[0]);
    if (!s) return Value::Bool(false);
    std::lock_guard<std::mutex> lk(s->mtx);
    if (s->count > 0) { s->count--; return Value::Bool(true); }
    return Value::Bool(false);
}

// ── Barrier ──
Value barrierCreate_(std::vector<Value>& args) {
    VMBarrier* b = VMBarrier::create();
    if (!args.empty() && args[0].isInt()) b->target = args[0].asInt();
    VM::current()->trackGC(b);
    return makePtrVal(b);
}
Value barrierWait_(std::vector<Value>& args) {
    VMBarrier* b = objCast<VMBarrier>(args[0]);
    if (!b) return Value::nil();
    std::unique_lock<std::mutex> lk(b->mtx);
    Int64 gen = b->generation;
    if (++b->current == b->target) {
        b->current = 0; b->generation++;
        lk.unlock(); b->cv.notify_all();
    } else {
        b->cv.wait(lk, [&] { return b->generation != gen; });
    }
    return Value::Bool(true);
}

// ── Atomic Integer ──
Value atomicInt_(std::vector<Value>& args) {
    VMAtomicInt* a = VMAtomicInt::create();
    if (!args.empty() && args[0].isInt()) a->value.store(args[0].asInt());
    VM::current()->trackGC(a);
    return makePtrVal(a);
}
Value atomicLoad_(std::vector<Value>& args) {
    VMAtomicInt* a = objCast<VMAtomicInt>(args[0]);
    if (a) return Value::Int(a->value.load());
    return Value::Int(0);
}
Value atomicAdd_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Int(0);
    VMAtomicInt* a = objCast<VMAtomicInt>(args[0]);
    if (a && args[1].isInt()) return Value::Int(a->value.fetch_add(args[1].asInt()));
    return Value::Int(0);
}
Value atomicStore_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::nil();
    VMAtomicInt* a = objCast<VMAtomicInt>(args[0]);
    if (a && args[1].isInt()) { a->value.store(args[1].asInt()); return Value::Bool(true); }
    return Value::Bool(false);
}
Value atomicExchange_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Int(0);
    VMAtomicInt* a = objCast<VMAtomicInt>(args[0]);
    if (a && args[1].isInt()) return Value::Int(a->value.exchange(args[1].asInt()));
    return Value::Int(0);
}
Value atomicCAS_(std::vector<Value>& args) {
    if (args.size() < 3) return Value::Bool(false);
    VMAtomicInt* a = objCast<VMAtomicInt>(args[0]);
    if (!a || !args[1].isInt() || !args[2].isInt()) return Value::Bool(false);
    Int64 expected = args[1].asInt();
    return Value::Bool(a->value.compare_exchange_strong(expected, args[2].asInt()));
}

// ── Hardware info ──
Value threadHw_(std::vector<Value>&) { return Value::Int(std::thread::hardware_concurrency()); }
Value threadId_(std::vector<Value>&) {
    auto id = std::this_thread::get_id();
    return Value::Int(static_cast<Int64>(std::hash<std::thread::id>{}(id)));
}
Value threadYield_(std::vector<Value>&) { std::this_thread::yield(); return Value::nil(); }

// ── Future ──
// ── Future 辅助：基于 typeTag 的类型安全提取 ──
static VMFuture* futureCast(const Value& v) {
    if (!v.isPtr() || !v.asPtr()) return nullptr;
    if (v.asPtr()->typeTag != ObjectHeader::TAG_FUTURE) return nullptr;
    return static_cast<VMFuture*>(v.asPtr());
}

Value futureGo_(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    VMFuture* f = VMFuture::create();
    Value fn = args[0];
    std::vector<Value> futArgs;
    for (size_t i = 1; i < args.size(); i++) futArgs.push_back(args[i]);
    VM* mainVM = VM::current();
    f->worker = std::thread([f, fn, futArgs, mainVM]() mutable {
        VM::setCurrent(mainVM);
        try {
            Value result = mainVM->callFunction(fn, futArgs);
            std::lock_guard<std::mutex> lk(f->mtx);
            f->result = result; f->ready = true;
        } catch (...) {
            std::lock_guard<std::mutex> lk(f->mtx);
            f->ready = true;
        }
        f->cv.notify_all();
    });
    f->worker.detach();
    VM::current()->trackGC(f);
    return makePtrVal(f);
}
Value futureIsReady_(std::vector<Value>& args) {
    VMFuture* f = futureCast(args[0]);
    if (!f) return Value::Bool(false);
    std::lock_guard<std::mutex> lk(f->mtx);
    return Value::Bool(f->ready);
}
Value futureGet_(std::vector<Value>& args) {
    VMFuture* f = futureCast(args[0]);
    if (!f) return Value::nil();
    std::unique_lock<std::mutex> lk(f->mtx);
    f->cv.wait(lk, [&] { return f->ready; });
    Value r = f->result;
    lk.unlock();
    // 线程已分离（detach），不再需要 join 等待；结果通过 cv/mutex 同步
    return r;
}

// ── RWLock ──
Value rwLockCreate_(std::vector<Value>&) {
    VMRWLock* rw = VMRWLock::create();
    VM::current()->trackGC(rw);
    return makePtrVal(rw);
}
Value rwLockRead_(std::vector<Value>& args) {
    VMRWLock* rw = objCast<VMRWLock>(args[0]);
    if (rw) { rw->smtx.lock_shared(); return Value::Bool(true); }
    return Value::Bool(false);
}
Value rwLockWrite_(std::vector<Value>& args) {
    VMRWLock* rw = objCast<VMRWLock>(args[0]);
    if (rw) { rw->smtx.lock(); return Value::Bool(true); }
    return Value::Bool(false);
}
Value rwLockUnlock_(std::vector<Value>& args) {
    VMRWLock* rw = objCast<VMRWLock>(args[0]);
    if (!rw) return Value::Bool(false);
    rw->smtx.unlock(); return Value::Bool(true);
}

// ── TLS ──
Value tlsSet_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString()) return Value::Bool(false);
    std::string key(args[0].asString()->data, args[0].asString()->length);
    std::lock_guard<std::mutex> lk(tlsMutex);
    tlsStore[key] = args[1];
    return Value::Bool(true);
}
Value tlsGet_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string key(args[0].asString()->data, args[0].asString()->length);
    std::lock_guard<std::mutex> lk(tlsMutex);
    auto it = tlsStore.find(key);
    return (it != tlsStore.end()) ? it->second : Value::nil();
}

// ── Channel ──
Value channelCreate_(std::vector<Value>& args) {
    VMChannel* ch = VMChannel::create();
    if (!args.empty() && args[0].isInt()) ch->capacity = (size_t)args[0].asInt();
    VM::current()->trackGC(ch);
    return makePtrVal(ch);
}
Value channelSend_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Bool(false);
    VMChannel* ch = objCast<VMChannel>(args[0]);
    if (!ch) return Value::Bool(false);
    std::unique_lock<std::mutex> lk(ch->mtx);
    if (ch->closed) return Value::Bool(false);
    if (ch->capacity > 0) ch->notFull.wait(lk, [&]{ return ch->q.size() < ch->capacity || ch->closed; });
    if (ch->closed) return Value::Bool(false);
    ch->q.push(args[1]);
    lk.unlock(); ch->notEmpty.notify_one();
    return Value::Bool(true);
}
Value channelRecv_(std::vector<Value>& args) {
    VMChannel* ch = objCast<VMChannel>(args[0]);
    if (!ch) return Value::nil();
    std::unique_lock<std::mutex> lk(ch->mtx);
    ch->notEmpty.wait(lk, [&]{ return !ch->q.empty() || ch->closed; });
    if (ch->q.empty()) return Value::nil();
    Value v = ch->q.front(); ch->q.pop();
    lk.unlock(); ch->notFull.notify_one();
    return v;
}
Value channelClose_(std::vector<Value>& args) {
    VMChannel* ch = objCast<VMChannel>(args[0]);
    if (!ch) return Value::Bool(false);
    std::lock_guard<std::mutex> lk(ch->mtx);
    ch->closed = true;
    ch->notEmpty.notify_all();
    ch->notFull.notify_all();
    return Value::Bool(true);
}

// ── Channel TryRecv (non-blocking receive) ──
Value channelTryRecv_(std::vector<Value>& args) {
    VMChannel* ch = objCast<VMChannel>(args[0]);
    if (!ch) { VMArray* e = VMArray::create(); e->data = {Value::Bool(false), Value::nil()}; return makeArrayVal(e); }
    std::unique_lock<std::mutex> lk(ch->mtx, std::try_to_lock);
    if (lk.owns_lock() && !ch->q.empty()) {
        Value v = ch->q.front(); ch->q.pop();
        lk.unlock(); ch->notFull.notify_one();
        VMArray* r = VMArray::create();
        r->data = {Value::Bool(true), v};
        return makeArrayVal(r);
    }
    VMArray* e = VMArray::create();
    e->data = {Value::Bool(false), Value::nil()};
    return makeArrayVal(e);
}

// ── Channel Select (poll multiple channels with optional timeout) ──
// channels: array of channel objects
// timeoutMs: optional timeout in milliseconds (-1 = infinite, 0 = non-blocking)
// Returns: [index, value] or empty array on timeout
Value channelSelect_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) { VMArray* e = VMArray::create(); return makeArrayVal(e); }
    auto channels = args[0].asArray();
    Int64 timeoutMs = (args.size() > 1 && args[1].isInt()) ? args[1].asInt() : -1;
    if (channels->data.empty()) { VMArray* e = VMArray::create(); return makeArrayVal(e); }

    // Build channel list
    std::vector<VMChannel*> chans;
    for (auto& v : channels->data) {
        chans.push_back(objCast<VMChannel>(v));
    }

    auto start = std::chrono::steady_clock::now();
    while (true) {
        for (size_t i = 0; i < chans.size(); i++) {
            auto* ch = chans[i];
            if (!ch) continue;
            std::unique_lock<std::mutex> lk(ch->mtx, std::try_to_lock);
            if (lk.owns_lock() && !ch->q.empty() && !ch->closed) {
                Value v = ch->q.front(); ch->q.pop();
                lk.unlock(); ch->notFull.notify_one();
                VMArray* r = VMArray::create();
                r->data = {Value::Int((Int64)i), v};
                return makeArrayVal(r);
            }
        }
        if (timeoutMs >= 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count();
            if (elapsed >= timeoutMs) { VMArray* e = VMArray::create(); return makeArrayVal(e); }
        }
        if (timeoutMs == 0) { VMArray* e = VMArray::create(); return makeArrayVal(e); } // non-blocking
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}
Value threadCreate_(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    VMThread* t = VMThread::create();
    if (!t) return Value::nil();
    Value fn = args[0];
    std::vector<Value> targs;
    for (size_t i = 1; i < args.size(); i++) targs.push_back(args[i]);
    VM* mainVM = VM::current();
    t->th = std::thread([fn, targs, mainVM]() mutable {
        VM::setCurrent(mainVM);
        try { mainVM->callFunction(fn, targs); }
        catch (...) {}
    });
    VM::current()->trackGC(t);
    return makePtrVal(t);
}
Value threadJoin_(std::vector<Value>& args) {
    if (args.empty()) return Value::Bool(false);
    auto* t = objCast<VMThread>(args[0]);
    if (!t) return Value::Bool(false);
    if (t->th.joinable()) { t->th.join(); return Value::Bool(true); }
    return Value::Bool(false);
}
Value threadDetach_(std::vector<Value>& args) {
    if (args.empty()) return Value::Bool(false);
    auto* t = objCast<VMThread>(args[0]);
    if (!t) return Value::Bool(false);
    if (t->th.joinable()) { t->th.detach(); return Value::Bool(true); }
    return Value::Bool(false);
}

// ── JThread (auto-join on GC, uses std::thread with join-on-destroy) ──
struct VMJThread : VMObject {
    std::thread th;
    VMJThread() { typeTag = TAG_USERDATA; }
    ~VMJThread() override { if (th.joinable()) th.join(); }
    VMJThread(const VMJThread&) = delete;
    VMJThread& operator=(const VMJThread&) = delete;
    static VMJThread* create() {
        auto* p = new VMJThread();
        p->size = sizeof(VMJThread);
        return p;
    }
};

Value jthreadNew_(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    auto* t = VMJThread::create();
    if (!t) return Value::nil();
    Value fn = args[0];
    std::vector<Value> targs;
    for (size_t i = 1; i < args.size(); i++) targs.push_back(args[i]);
    VM* mainVM = VM::current();
    t->th = std::thread([fn, targs, mainVM]() mutable {
        VM::setCurrent(mainVM);
        try { mainVM->callFunction(fn, targs); }
        catch (...) {}
    });
    VM::current()->trackGC(t);
    return makePtrVal(t);
}
Value jthreadJoin_(std::vector<Value>& args) {
    if (args.empty()) return Value::Bool(false);
    if (!args[0].isPtr() || !args[0].asPtr()) return Value::Bool(false);
    if (args[0].asPtr()->typeTag != ObjectHeader::TAG_USERDATA) return Value::Bool(false);
    auto* t = static_cast<VMJThread*>(args[0].asPtr());
    if (!t) return Value::Bool(false);
    if (t->th.joinable()) { t->th.join(); return Value::Bool(true); }
    return Value::Bool(false);
}

} // namespace thread_ns

void StdLib::registerThreading(VM* vm) {
    using namespace thread_ns;
    registerFunction(vm, "mutexCreate",     mutexCreate_);
    registerFunction(vm, "mutexLock",       mutexLock_);
    registerFunction(vm, "mutexUnlock",     mutexUnlock_);
    registerFunction(vm, "mutexTryLock",    mutexTryLock_);
    registerAlias(vm, "互斥创建",           "mutexCreate");
    registerAlias(vm, "互斥加锁",           "mutexLock");
    registerAlias(vm, "互斥解锁",           "mutexUnlock");
    registerAlias(vm, "互斥尝试锁",         "mutexTryLock");

    registerFunction(vm, "condCreate",      condCreate_);
    registerAlias(vm, "条件创建",           "condCreate");

    registerFunction(vm, "semCreate",       semCreate_);
    registerFunction(vm, "semPost",         semPost_);
    registerFunction(vm, "semTryWait",      semTryWait_);
    registerAlias(vm, "信号量创建",         "semCreate");
    registerAlias(vm, "信号量释放",         "semPost");
    registerAlias(vm, "信号量尝试等待",     "semTryWait");

    registerFunction(vm, "barrierCreate",   barrierCreate_);
    registerFunction(vm, "barrierWait",     barrierWait_);
    registerAlias(vm, "屏障创建",           "barrierCreate");
    registerAlias(vm, "屏障等待",           "barrierWait");

    registerFunction(vm, "atomicInt",       atomicInt_);
    registerFunction(vm, "atomicLoad",      atomicLoad_);
    registerFunction(vm, "atomicAdd",       atomicAdd_);
    registerFunction(vm, "atomicStore",     atomicStore_);
    registerFunction(vm, "atomicExchange",  atomicExchange_);
    registerFunction(vm, "atomicCAS",       atomicCAS_);
    registerAlias(vm, "原子整数",           "atomicInt");
    registerAlias(vm, "原子加载",           "atomicLoad");
    registerAlias(vm, "原子增加",           "atomicAdd");
    registerAlias(vm, "原子存储",           "atomicStore");
    registerAlias(vm, "原子交换",           "atomicExchange");

    registerFunction(vm, "threadHw",        threadHw_);
    registerFunction(vm, "threadId",        threadId_);
    registerFunction(vm, "threadYield",     threadYield_);
    registerFunction(vm, "threadCreate",    threadCreate_);
    registerFunction(vm, "threadJoin",      threadJoin_);
    registerFunction(vm, "threadDetach",    threadDetach_);
    registerFunction(vm, "jthreadNew",      jthreadNew_);
    registerFunction(vm, "jthreadJoin",     jthreadJoin_);
    registerAlias(vm, "线程HW",             "threadHw");
    registerAlias(vm, "线程ID",             "threadId");
    registerAlias(vm, "线程让步",           "threadYield");
    registerAlias(vm, "线程创建",           "threadCreate");
    registerAlias(vm, "线程等待",           "threadJoin");
    registerAlias(vm, "线程分离",           "threadDetach");
    registerAlias(vm, "自动线程创建",       "jthreadNew");
    registerAlias(vm, "自动线程等待",       "jthreadJoin");

    registerFunction(vm, "futureGo",        futureGo_);
    registerFunction(vm, "futureIsReady",   futureIsReady_);
    registerFunction(vm, "futureGet",       futureGet_);
    registerAlias(vm, "异步执行",           "futureGo");
    registerAlias(vm, "异步就绪",           "futureIsReady");
    registerAlias(vm, "异步结果",           "futureGet");

    registerFunction(vm, "rwLockCreate",    rwLockCreate_);
    registerFunction(vm, "rwLockRead",      rwLockRead_);
    registerFunction(vm, "rwLockWrite",     rwLockWrite_);
    registerFunction(vm, "rwLockUnlock",    rwLockUnlock_);
    registerAlias(vm, "读写锁创建",         "rwLockCreate");
    registerAlias(vm, "读写锁读",           "rwLockRead");
    registerAlias(vm, "读写锁写",           "rwLockWrite");
    registerAlias(vm, "读写锁解锁",         "rwLockUnlock");

    registerFunction(vm, "tlsSet",          tlsSet_);
    registerFunction(vm, "tlsGet",          tlsGet_);
    registerAlias(vm, "线程局部存储",       "tlsSet");
    registerAlias(vm, "线程局部获取",       "tlsGet");

    registerFunction(vm, "channelCreate",   channelCreate_);
    registerFunction(vm, "channelSend",     channelSend_);
    registerFunction(vm, "channelRecv",     channelRecv_);
    registerFunction(vm, "channelClose",    channelClose_);
    registerFunction(vm, "channelTryRecv",  channelTryRecv_);
    registerFunction(vm, "channelSelect",   channelSelect_);
    registerAlias(vm, "通道创建",           "channelCreate");
    registerAlias(vm, "通道发送",           "channelSend");
    registerAlias(vm, "通道接收",           "channelRecv");
    registerAlias(vm, "通道关闭",           "channelClose");
    registerAlias(vm, "通道尝试接收",       "channelTryRecv");
    registerAlias(vm, "通道选择",           "channelSelect");
}

} // namespace cplang
