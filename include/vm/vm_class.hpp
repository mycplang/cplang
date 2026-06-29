#pragma once
#include "vm/vm_types.hpp"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <functional>
#include <memory>
#include "vm/event_loop.hpp"

namespace cplang { class HybridJIT; }
namespace cplang { class EventLoop; }

namespace cplang {

class VM {
    friend class Debugger;  // 调试器需要访问 frames_, globals_ 等内部成员

public:
    static constexpr int MAX_REGISTERS = 256;
    static constexpr int MAX_STACK    = 65536;
    static constexpr int NURSERY_THRESHOLD  = 256 * 1024;      // 256KB minor GC
    static constexpr int GC_THRESHOLD       = 8 * 1024 * 1024; // 8MB major GC
    static constexpr int PROMOTE_AFTER      = 2;               // promote after 2 minor GC survivals

    VM();
    ~VM();
    bool loadModule(VMFunction* func);
    void registerGlobal(const char* name, Value val);
    void registerNative(const char* name, VMNativeFunc::Fn fn);
    void registerNativeAlias(const char* aliasName, const char* originalName);
    void setGlobal(const char* name, Value v);

    Int32 getOrCreateGlobalSlot(const char* name);
    Int32 getGlobalSlot(const char* name) const {
        auto it = globalNameToSlot_.find(name);
        if (it != globalNameToSlot_.end()) return it->second;
        return -1;
    }
    Int32 getGlobalSlot(const char* name) {
        auto it = globalNameToSlot_.find(name);
        if (it != globalNameToSlot_.end()) return it->second;
        return -1;
    }
    Value* getGlobalBySlot(UInt16 slot);

    const std::string& error() const { return error_; }
    void raiseError(const std::string& msg);
    void raiseError(const char* msg);
    bool hasError() const { return !error_.empty(); }
    void resetExecutionState();  // 重置执行状态（REPL错误恢复用）
    Int64 totalInstructions() const { return instructionCount_; }
    Int64 gcCount() const { return gcCount_; }
    void setTraceExec(bool v) { traceExec_ = v; }
    VMString* internString(const char* s, UInt32 len);
    VMString* internString(const std::string& s);
    void setCompiler(void* compiler) { compiler_ = compiler; }

    void setJIT(HybridJIT* jit) { jit_ = jit; }
    HybridJIT* getJIT() const { return jit_; }
    void setDebugServer(class DebugServer* ds) { debugServer_ = ds; }

    void setBreakpoint(int line) { breakpoints_.insert(line); }
    void removeBreakpoint(int line) { breakpoints_.erase(line); }
    bool hasBreakpoint(int line) const { return breakpoints_.count(line) > 0; }
    void debugContinue() { debugPaused_ = false; debugStepMode_ = false; }
    void debugStepOver() { debugPaused_ = false; debugStepMode_ = true; debugStepDepth_ = callDepth_; }
    void debugStop() { debugStop_ = true; debugPaused_ = false; }
    bool isDebugPaused() const { return debugPaused_; }
    int  debugCurrentLine() const { return debugCurrentLine_; }
    std::string debugCallStack() const;
    std::string debugLocals() const;
    Value debugGetVariable(const std::string& name) const;

    std::vector<std::string> getGlobalSlotNames() const;
    void refreshGlobalSlots();
    void prepareSlot(const std::string& name, UInt16 slot);
    size_t getSlotCount() const { return globalNameToSlot_.size(); }
    const auto& getSlotMap() const { return globalNameToSlot_; }

    bool doImport(const std::string& filename);
    void setLastImportedFunc(VMFunction* f) { lastImportedFunc_ = f; }

    Value callFunction(Value func, std::vector<Value>& args);
    static VM* current() { return currentVM_; }
    
    // 生成器支持（P9.1）
    Value resumeGenerator(VMGenerator* gen, const Value& sendValue = Value::nil());
    
    // ===== 协程/异步 支持（P9.3） =====
    
    // 创建新承诺
    VMPromise* createPromise();
    
    // 解决承诺
    void resolvePromise(VMPromise* promise, const Value& value);
    
    // 拒绝承诺
    void rejectPromise(VMPromise* promise, const Value& reason);
    
    // 等待承诺（暂停当前协程，承诺完成后恢复）
    Value awaitPromise(VMPromise* promise);
    
    // 调度器：微任务队列
    std::vector<std::pair<Value, Value>> microtaskQueue_;  // (callback, arg)
    
    // 调度器：运行所有微任务
    void runMicrotasks();
    
    // 调度器：添加微任务
    void enqueueMicrotask(const Value& callback, const Value& arg = Value::nil());
    
    // 当前运行的协程（用于 await 时暂停）
    VMGenerator* currentCoroutine_ = nullptr;

    // 获取当前执行上下文的信息（供 source_location 标准库使用）
    int getCurrentLine() const;
    std::string getCurrentSourceFile() const;
    std::string getCurrentFunction() const;
    static void setCurrent(VM* vm) { currentVM_ = vm; }

    std::function<bool(const std::string&)> importCallback;
    void trackGC(VMObject* obj);
    
    // 分代GC写入屏障：当old对象存储young引用时调用
    void gcWriteBarrier(VMObject* container, const Value& newVal);

    // 事件循环
    EventLoop* getEventLoop();
    void startEventLoop();
    void stopEventLoop();

private:
    bool run(ExecContext* ctx);
    bool callNative(VMNativeFunc* nf, int argc, Value* argv, Value* result);
    
    // 旧GC（全堆标记-清除，升级为 major GC）
    void gc();
    // 新生代GC（仅回收young gen）
    void gcMinor();
    // 全堆GC（回收所有代）
    void gcMajor();
    
    void gcMarkRoots();
    void gcMarkObject(VMObject* obj);
    void gcMarkValue(const Value& v);
    void gcSweepPhase();
    void gcSweepYoungPhase();  // 仅清扫新生代
    void gcCleanup();
    
    // 帮助函数
    bool isPointerToYoung(const Value& v) const;

    std::vector<Value>               stack_;
    Value*                          top_ = nullptr;
    std::vector<CallFrame>          frames_;
    static constexpr int MAX_GLOBAL_SLOTS = 65535;
    std::vector<Value>               globalSlots_;
    std::unordered_map<std::string, UInt16> globalNameToSlot_;
    UInt16                          nextGlobalSlot_ = 0;
    std::unordered_map<std::string, Value> globals_;
    VMObject*                       allObjects_   = nullptr;  // 所有对象链表（old gen）
    VMObject*                       youngObjects_ = nullptr;  // 新生代链表
    std::unordered_map<std::string, VMString*> stringTable_;
    size_t                          gcAllocated_   = 0;      // old gen已分配
    size_t                          youngAllocated_= 0;      // young gen已分配
    std::unordered_set<VMObject*>   rememberedSet_;          // 写屏障记录集
    Int64                           gcCount_ = 0;
    bool                            gcRunning_ = false;
    bool                            traceExec_ = false;
    Int64                           instructionCount_ = 0;
    std::string                     error_;
    void*                           compiler_ = nullptr;
    HybridJIT*                      jit_ = nullptr;
    int                             moduleExecDepth_ = 0;
    VMFunction*                     lastImportedFunc_ = nullptr;
    std::vector<HandlerFrame>       handlerStack_;
    ExecContext*                    currentCtx_ = nullptr;
    Int32                           callDepth_ = 0;
    class DebugServer*              debugServer_ = nullptr;  // 调试服务器（TCP）
    static thread_local VM*         currentVM_;

    std::unordered_set<int>         breakpoints_;
    bool                            debugPaused_ = false;
    std::unique_ptr<EventLoop>      eventLoop_;
    bool                            debugStepMode_ = false;
    int                             debugStepDepth_ = 0;
    bool                            debugStop_ = false;
    int                             debugCurrentLine_ = 0;
    ExecContext                     savedDebugCtx_;
};

inline size_t ValueHash::operator()(const Value& v) const {
    if (v.isDouble()) {
        Float64 f = v.asFloat();
        if (f == 0.0) f = 0.0;
        uint64_t raw;
        std::memcpy(&raw, &f, sizeof(Float64));
        return std::hash<uint64_t>{}(raw);
    }
    if (v.isString()) {
        auto* s = v.asString();
        if (!s || !s->data) return 0;
        size_t h = 14695981039346656037ULL;
        for (UInt32 i = 0; i < s->length; i++)
            h = (h ^ (unsigned char)s->data[i]) * 1099511628211ULL;
        return h;
    }
    if (v.isInt64()) {
        // 对装箱 Int64 取其数值哈希，而非指针地址
        return std::hash<Int64>{}(v.asInt());
    }
    return std::hash<uint64_t>{}(v.raw());
}

} // namespace cplang