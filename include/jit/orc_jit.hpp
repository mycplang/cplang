// ORC JIT 编译器 - LLVM ORC JIT 集成
// 使用 LLVM ORC JIT API 进行内存中编译执行
// 
// 前置条件：
// 1. 安装 LLVM 开发包（clang+llvm-*.tar.xz）
// 2. 设置 LLVM_DIR 环境变量或调用 setLLVMDir()
// 3. CMakeLists.txt 中链接 LLVM 库

#pragma once

#include "common/types.hpp"
#include "vm/vm.hpp"
#include "codegen/llvm_codegen.hpp"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <atomic>

// 前向声明 LLVM 类型（避免在头文件中包含 LLVM 头文件）
namespace llvm {
    class ExecutionSession;
    class RTDyldObjectLinkingLayer;
    class IRCompileLayer;
    class IRTransformLayer;
    class ThreadSafeModule;
    class Module;
    class Function;
    class TargetMachine;
    class DataLayout;
    class Triple;
    class LLJIT;
}

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  JIT 编译结果
// ═══════════════════════════════════════════════════════════════════

struct JITCompiledFunction {
    void* address = nullptr;        // 函数地址
    size_t codeSize = 0;            // 代码大小
    std::string name;               // 函数名
    double compileTimeMs = 0;       // 编译时间
    bool isHot = false;             // 是否热点函数
    int callCount = 0;              // 调用次数
};

// ═══════════════════════════════════════════════════════════════════
//  后台编译任务
// ═══════════════════════════════════════════════════════════════════

struct CompileTask {
    Shared<struct Program> program;
    std::string funcName;
    std::function<void(void*, const std::string&)> callback;
    int priority = 0;
};

// ═══════════════════════════════════════════════════════════════════
//  ORC JIT 编译器
// ═══════════════════════════════════════════════════════════════════

class OrcJIT {
public:
    OrcJIT();
    ~OrcJIT();
    
    // ═══════════════════════════════════════════════════════════════════
    //  初始化
    // ═══════════════════════════════════════════════════════════════════
    
    // 初始化 ORC JIT（需要 LLVM 开发库）
    bool initialize();
    
    // 检查 LLVM 是否可用
    bool isAvailable() const { return available_; }
    
    // 设置 LLVM 安装目录
    void setLLVMDir(const std::string& path);
    
    // ═══════════════════════════════════════════════════════════════════
    //  核心编译接口
    // ═══════════════════════════════════════════════════════════════════
    
    // 从 LLVM IR 字符串编译
    void* compileIR(const std::string& ir, const std::string& funcName);
    
    // 从 AST 编译
    void* compileAST(Shared<Program> program, const std::string& funcName);
    
    // 存储 AST 程序（用于后续热点编译，不立即编译）
    void storeProgram(Shared<Program> program) { program_ = program; }
    
    // 全量预编译所有函数到同一模块（解决跨函数引用）
    void compileAll(Shared<Program> program);
    
    // 从 VMFunction 热点编译（使用之前存储的 AST）
    void* compileHotFunction(VMFunction* func);
    
    // 查找已编译的符号
    void* lookup(const std::string& name);
    
    // 添加全局符号（供 JIT 代码调用）
    bool addSymbol(const std::string& name, void* address);
    
    // ═══════════════════════════════════════════════════════════════════
    //  热点检测和自动编译
    // ═══════════════════════════════════════════════════════════════════
    
    // 记录函数调用
    void recordCall(VMFunction* func);
    
    // 检查是否应该编译
    bool shouldCompile(VMFunction* func) const;
    
    // 获取调用次数
    int getCallCount(VMFunction* func) const;
    
    // 获取编译后的函数
    const JITCompiledFunction* getCompiledFunction(VMFunction* func) const;
    
    // 设置热点阈值
    void setHotThreshold(int threshold) { hotThreshold_ = threshold; }
    
    // ═══════════════════════════════════════════════════════════════════
    //  后台编译
    // ═══════════════════════════════════════════════════════════════════
    
    // 启动后台编译线程
    void startBackgroundThread();
    
    // 停止后台编译线程
    void stopBackgroundThread();
    
    // 提交编译任务
    void submitCompileTask(const CompileTask& task);
    
    // 等待所有编译任务完成
    void waitForCompletion();
    
    // ═══════════════════════════════════════════════════════════════════
    //  配置
    // ═══════════════════════════════════════════════════════════════════
    
    void setOptLevel(int level) { optLevel_ = level; }
    void setEnableProfiling(bool enable) { enableProfiling_ = enable; }
    
    // ═══════════════════════════════════════════════════════════════════
    //  缓存管理
    // ═══════════════════════════════════════════════════════════════════
    
    // 清空编译缓存
    void clearCache();
    
    // 打印统计信息
    void dumpStats() const;
    
private:
    // LLVM 内部实现（PIMPL 模式，避免在头文件中暴露 LLVM 类型）
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    // 状态
    bool available_ = false;
    bool initialized_ = false;
    
    // 配置
    std::string llvmDir_;
    int hotThreshold_ = 100;
    int optLevel_ = 3;
    bool enableProfiling_ = true;
    
    // 热点跟踪
    std::unordered_map<VMFunction*, int> callCounts_;
    std::unordered_map<VMFunction*, JITCompiledFunction> compiledFunctions_;
    std::unordered_set<VMFunction*> hotFunctions_;
    std::unordered_set<VMFunction*> failedFunctions_;  // 已尝试过但编译失败的函数（避免重复尝试）
    
    // 后台编译
    std::thread bgThread_;
    std::queue<CompileTask> taskQueue_;
    std::mutex queueMutex_;
    std::condition_variable queueCv_;
    std::atomic<bool> running_{false};
    
    // 统计
    struct Stats {
        std::atomic<int> totalCompiles{0};
        std::atomic<int> successfulCompiles{0};
        std::atomic<int> failedCompiles{0};
        std::atomic<int> cacheHits{0};
        double totalCompileTimeMs = 0;
    };
    Stats stats_;
    
    // LLVM IR 生成器
    std::unique_ptr<LLVMCodegen> llvmCodegen_;
    
    // 存储的 AST 程序（用于热点编译）
    Shared<Program> program_;
    
    // 全量预编译缓存：函数名 → 入口地址
    std::unordered_map<std::string, void*> fullCompileCache_;
    
    // 内部方法
    void backgroundWorker();
    void* compileInternal(const std::string& ir, const std::string& funcName);
};

// ═══════════════════════════════════════════════════════════════════
//  混合 JIT 策略
//  - 优先使用 ORC JIT（如果可用）
//  - 降级到外部进程模式（如果 LLVM 开发库不可用）
// ═══════════════════════════════════════════════════════════════════
//  HybridJIT 已移至 hybrid_jit.hpp
// ═══════════════════════════════════════════════════════════════════

} // namespace cplang
