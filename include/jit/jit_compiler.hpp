#pragma once

#include "common/types.hpp"
#include "vm/vm.hpp"
#include "codegen/llvm_codegen.hpp"
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  JIT 编译函数指针类型
// ═══════════════════════════════════════════════════════════════════

using JITFuncPtr = void (*)(void);

// ═══════════════════════════════════════════════════════════════════
//  编译后的函数信息
// ═══════════════════════════════════════════════════════════════════

struct CompiledFunction {
    void* machineCode = nullptr;      // 编译后的机器码
    size_t codeSize = 0;             // 代码大小
    std::string llFile;              // LLVM IR 文件路径
    std::string objFile;             // 目标文件路径
    double compileTimeMs = 0;         // 编译耗时
    bool isHot = false;              // 是否热点函数
};

// ═══════════════════════════════════════════════════════════════════
//  JIT 编译器
// ═══════════════════════════════════════════════════════════════════

class JITCompiler {
public:
    JITCompiler();
    ~JITCompiler();

    // ═══════════════════════════════════════════════════════════════════
    //  核心接口
    // ═══════════════════════════════════════════════════════════════════

    // 初始化 JIT 编译器
    bool initialize();

    // 检查函数是否需要 JIT 编译
    bool shouldCompile(VMFunction* func) const;

    // 编译热点函数，返回编译后的函数指针
    void* compile(VMFunction* func);

    // 直接从 AST 编译
    void* compileFromAST(Shared<Program> program, const String& funcName);

    // ═══════════════════════════════════════════════════════════════════
    //  配置接口
    // ═══════════════════════════════════════════════════════════════════

    void setHotThreshold(int threshold);           // 热点阈值（默认100次）
    void setEnableProfiling(bool enable);          // 启用性能分析
    void setOptLevel(int level);                   // 优化级别（0-3）
    void setLLVMDir(const String& path);          // LLVM 安装目录

    // ═══════════════════════════════════════════════════════════════════
    //  运行时接口
    // ═══════════════════════════════════════════════════════════════════

    // 记录函数调用（热点检测）
    void recordCall(VMFunction* func);

    // 获取函数调用次数
    int getCallCount(VMFunction* func) const;

    // 获取函数是否已编译
    bool isCompiled(VMFunction* func) const;

    // ═══════════════════════════════════════════════════════════════════
    //  缓存管理
    // ═══════════════════════════════════════════════════════════════════

    void clearCache();                            // 清空编译缓存
    void dumpStats() const;                       // 打印统计信息
    bool isAvailable() const { return isAvailable_; }  // LLVM 是否可用

private:
    // 内部编译流程
    bool checkLLVM();
    String generateLLVMIR(VMFunction* func);
    bool compileToObjectFile(const String& irFile, const String& objFile);
    void* loadCompiledCode(const String& objFile);
    void freeCompiledCode(void* code, size_t size);

    // 从字节码函数提取程序（用于 LLVM IR 生成）
    Shared<Program> extractProgram(VMFunction* func);

    // 工具函数
    String getTempDir();
    String getFuncName(VMFunction* func);
    int64_t getCurrentTimeMs();

private:
    // 配置
    int hotThreshold_ = 100;
    bool enableProfiling_ = true;
    int optLevel_ = 3;
    String llvmDir_;
    String tempDir_;

    // 状态
    bool isAvailable_ = false;
    bool initialized_ = false;

    // 热点跟踪
    std::unordered_map<VMFunction*, int> callCounts_;
    std::unordered_set<VMFunction*> hotFunctions_;

    // 编译缓存
    std::unordered_map<VMFunction*, CompiledFunction> compiledFunctions_;

    // 统计
    struct Stats {
        int totalCompileRequests = 0;
        int successfulCompiles = 0;
        int failedCompiles = 0;
        double totalCompileTimeMs = 0;
        double avgCompileTimeMs = 0;
    };
    Stats stats_;

    // LLVM IR 生成器（复用现有代码）
    std::unique_ptr<LLVMCodegen> llvmCodegen_;
};

} // namespace cplang
