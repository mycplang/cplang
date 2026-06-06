#pragma once
#include "vm/vm_types.hpp"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <functional>

namespace cplang { class HybridJIT; }

namespace cplang {

class VM {
    friend class Debugger;  // 调试器需要访问 frames_, globals_ 等内部成员

public:
    static constexpr int MAX_REGISTERS = 256;
    static constexpr int MAX_STACK    = 65536;
    static constexpr int GC_THRESHOLD  = 1024 * 1024;

    VM();
    ~VM();
    bool loadModule(VMFunction* func);
    void registerGlobal(const char* name, Value val);
    void registerNative(const char* name, VMNativeFunc::Fn fn);
    void registerNativeAlias(const char* aliasName, const char* originalName);

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
    Int64 totalInstructions() const { return instructionCount_; }
    Int64 gcCount() const { return gcCount_; }
    void setTraceExec(bool v) { traceExec_ = v; }
    VMString* internString(const char* s, UInt32 len);
    VMString* internString(const std::string& s);
    void setCompiler(void* compiler) { compiler_ = compiler; }

    void setJIT(HybridJIT* jit) { jit_ = jit; }
    HybridJIT* getJIT() const { return jit_; }

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

    bool doImport(const std::string& filename);
    void setLastImportedFunc(VMFunction* f) { lastImportedFunc_ = f; }

    Value callFunction(Value func, std::vector<Value>& args);
    static VM* current() { return currentVM_; }

    // 获取当前执行上下文的信息（供 source_location 标准库使用）
    int getCurrentLine() const;
    std::string getCurrentSourceFile() const;
    std::string getCurrentFunction() const;
    static void setCurrent(VM* vm) { currentVM_ = vm; }

    std::function<bool(const std::string&)> importCallback;
    void trackGC(VMObject* obj);

private:
    bool run(ExecContext* ctx);
    bool callNative(VMNativeFunc* nf, int argc, Value* argv, Value* result);
    void gc();
    void gcMarkRoots();
    void gcMarkObject(VMObject* obj);
    void gcMarkValue(const Value& v);
    void gcSweepPhase();
    void gcCleanup();

    std::vector<Value>               stack_;
    Value*                          top_ = nullptr;
    std::vector<CallFrame>          frames_;
    static constexpr int MAX_GLOBAL_SLOTS = 65535;
    std::vector<Value>               globalSlots_;
    std::unordered_map<std::string, UInt16> globalNameToSlot_;
    UInt16                          nextGlobalSlot_ = 0;
    std::unordered_map<std::string, Value> globals_;
    VMObject*                       allObjects_ = nullptr;
    std::unordered_map<std::string, VMString*> stringTable_;
    size_t                          gcAllocated_ = 0;
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
    static thread_local VM*         currentVM_;

    std::unordered_set<int>         breakpoints_;
    bool                            debugPaused_ = false;
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
