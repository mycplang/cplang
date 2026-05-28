// ORC JIT 编译器实现
// 当 LLVM 开发库可用时，使用 ORC JIT API
// 否则降级到外部进程模式

#include "jit/orc_jit.hpp"
#include "jit/jit_compiler.hpp"
#include "jit/jit_runtime.hpp"
#include "core/verbose.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>

// 条件编译：只有CPLANG_HAS_LLVM定义时才包含LLVM头文件
#ifdef CPLANG_HAS_LLVM
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetSelect.h>
#endif

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  OrcJIT::Impl - LLVM 内部实现
// ═══════════════════════════════════════════════════════════════════

class OrcJIT::Impl {
public:
    Impl() = default;
    ~Impl() = default;
    
    // 这些方法在 LLVM 可用时实现
    bool initialize(const std::string& /*llvmDir*/) {
#ifdef CPLANG_HAS_LLVM
        // 阶段1：最小可行LLJIT
        VERBOSE(std::cout << "[OrcJIT] LLVM 可用，尝试初始化 LLJIT...\n");
        
        // 初始化LLVM目标
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
        llvm::InitializeNativeTargetAsmParser();
        
        // 创建LLJIT
        auto jit = llvm::orc::LLJITBuilder().create();
        if (!jit) {
            std::cerr << "[OrcJIT] LLJIT 创建失败: " << llvm::toString(jit.takeError()) << "\n";
            return false;
        }
        
        lljit_ = std::move(*jit);
        VERBOSE(std::cout << "[OrcJIT] LLJIT 初始化成功！\n");
        
        // 注册 JIT 运行时辅助函数（如 jit_strcat, jit_printv, jit_table_*）
        addSymbol("jit_strcat", reinterpret_cast<void*>(&jit_strcat));
        addSymbol("jit_printv", reinterpret_cast<void*>(&jit_printv));
        addSymbol("jit_table_create", reinterpret_cast<void*>(&jit_table_create));
        addSymbol("jit_table_get", reinterpret_cast<void*>(&jit_table_get));
        addSymbol("jit_table_set", reinterpret_cast<void*>(&jit_table_set));
        addSymbol("jit_tick", reinterpret_cast<void*>(&jit_tick));
        return true;
#else
        // 无LLVM，返回失败
        VERBOSE(std::cout << "[OrcJIT] LLVM 开发库未配置，ORC JIT 不可用\n");
        VERBOSE(std::cout << "[OrcJIT] 请下载 LLVM 开发包并设置 LLVM_DIR\n");
        return false;
#endif
    }
    
    void* compile(const std::string& ir, const std::string& funcName) {
#ifdef CPLANG_HAS_LLVM
        if (!lljit_) {
            std::cerr << "[OrcJIT] LLJIT 未初始化\n";
            return nullptr;
        }
        
        // Step 1: Parse IR string into LLVM Module
        auto ctx = std::make_unique<llvm::LLVMContext>();
        auto memBuf = llvm::MemoryBuffer::getMemBuffer(ir, funcName + ".ll");
        llvm::SMDiagnostic diag;
        auto module = llvm::parseIR(memBuf->getMemBufferRef(), diag, *ctx);
        if (!module) {
            std::cerr << "[OrcJIT] IR 解析失败: ";
            diag.print(funcName.c_str(), llvm::errs());
            std::cerr << "\n";
            return nullptr;
        }
        
        // 提取 LLVM 解码后的真实函数名（中文名 sanitizeName 后会转义，LLVM 解析时解码）
        std::string realFuncName = funcName;
        for (auto& f : module->functions()) {
            if (!f.isDeclaration()) {
                realFuncName = std::string(f.getName());
                break;
            }
        }
        
        // Step 2: Add module to LLJIT
        auto tsm = llvm::orc::ThreadSafeModule(
            std::move(module),
            llvm::orc::ThreadSafeContext(std::move(ctx))
        );
        auto addErr = lljit_->addIRModule(std::move(tsm));
        if (addErr) {
            // Duplicate definition is benign — function already in LLJIT, just look it up
            std::string errMsg = llvm::toString(std::move(addErr));
            if (errMsg.find("Duplicate definition") == std::string::npos) {
                std::cerr << "[OrcJIT] 模块添加失败: " << errMsg << "\n";
                return nullptr;
            }
            // Fall through to lookup below
        }
        
        // Step 3: Look up the compiled function (use decoded name)
        auto addr = lljit_->lookup(realFuncName);
        if (!addr) {
            std::cerr << "[OrcJIT] 函数 '" << realFuncName << "' 查找失败: "
                      << llvm::toString(addr.takeError()) << "\n";
            return nullptr;
        }
        
        auto ptr = reinterpret_cast<void*>(static_cast<uintptr_t>(addr->getValue()));
        if (!ptr) {
            std::cerr << "[OrcJIT] 函数地址为空\n";
            return nullptr;
        }
        
        VERBOSE(std::cout << "[OrcJIT] 编译成功: " << funcName << " @ 0x"
                  << std::hex << addr->getValue() << std::dec << "\n");
        return ptr;
#else
        return nullptr;
#endif
    }
    
    void* lookup(const std::string& name) {
#ifdef CPLANG_HAS_LLVM
        if (!lljit_) {
            std::cerr << "[OrcJIT] LLJIT 未初始化\n";
            return nullptr;
        }
        
        auto addr = lljit_->lookup(name);
        if (!addr) {
            // 打印错误信息但不崩溃
            llvm::Error err = addr.takeError();
            std::cerr << "[OrcJIT] 符号 '" << name << "' 查找失败: "
                      << llvm::toString(std::move(err)) << "\n";
            return nullptr;
        }
        
        void* ptr = reinterpret_cast<void*>(static_cast<uintptr_t>(addr->getValue()));
        VERBOSE(std::cout << "[OrcJIT] 符号 '" << name << "' 查找成功: 0x"
                  << std::hex << addr->getValue() << std::dec << "\n");
        return ptr;
#else
        return nullptr;
#endif
    }
    
    bool addSymbol(const std::string& name, void* address) {
#ifdef CPLANG_HAS_LLVM
        if (!lljit_) {
            std::cerr << "[OrcJIT] LLJIT 未初始化\n";
            return false;
        }
        
        // 创建绝对符号定义
        auto& dylib = lljit_->getMainJITDylib();
        
        // 使用 AbsoluteSymbols 添加符号
        auto& ES = lljit_->getExecutionSession();
        auto symName = ES.intern(name);
        llvm::orc::SymbolMap symbols;
        symbols[symName] = llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr(reinterpret_cast<uintptr_t>(address)),
            llvm::JITSymbolFlags::Exported
        );
        
        llvm::cantFail(dylib.define(llvm::orc::absoluteSymbols(std::move(symbols))));
        
        VERBOSE(std::cout << "[OrcJIT] 符号 '" << name << "' 添加成功: 0x"
                  << std::hex << reinterpret_cast<uintptr_t>(address) << std::dec << "\n");
        return true;
#else
        return false;
#endif
    }
    
    std::unordered_map<std::string, void*> compileAll(const std::string& ir) {
        std::unordered_map<std::string, void*> result;
#ifdef CPLANG_HAS_LLVM
        if (!lljit_) return result;
        
        auto ctx = std::make_unique<llvm::LLVMContext>();
        auto memBuf = llvm::MemoryBuffer::getMemBuffer(ir, "full.ll");
        llvm::SMDiagnostic diag;
        auto module = llvm::parseIR(memBuf->getMemBufferRef(), diag, *ctx);
        if (!module) {
            diag.print("full", llvm::errs());
            return result;
        }
        
        // 收集所有定义的函数名
        std::vector<std::string> funcNames;
        for (auto& f : module->functions()) {
            if (!f.isDeclaration()) {
                funcNames.push_back(std::string(f.getName()));
            }
        }
        
        auto tsm = llvm::orc::ThreadSafeModule(
            std::move(module),
            llvm::orc::ThreadSafeContext(std::move(ctx))
        );
        auto addErr = lljit_->addIRModule(std::move(tsm));
        if (addErr) {
            std::cerr << "[OrcJIT] 全量编译失败: " << llvm::toString(std::move(addErr)) << "\n";
            return result;
        }
        
        // 查找所有函数符号
        for (const auto& name : funcNames) {
            auto addr = lljit_->lookup(name);
            if (addr) {
                result[name] = reinterpret_cast<void*>(static_cast<uintptr_t>(addr->getValue()));
            }
        }
        
        VERBOSE(std::cout << "[OrcJIT] 全量编译: " << result.size() << " 个函数\n");
#endif
        return result;
    }
    
    void clearCache() {
#ifdef CPLANG_HAS_LLVM
        // 重新创建 LLJIT 实例以清理缓存
        lljit_.reset();
        
        auto jit = llvm::orc::LLJITBuilder().create();
        if (!jit) {
            std::cerr << "[OrcJIT] LLJIT 重建失败: " << llvm::toString(jit.takeError()) << "\n";
            return;
        }
        
        lljit_ = std::move(*jit);
        VERBOSE(std::cout << "[OrcJIT] 缓存已清理，LLJIT 已重建\n");
#endif
    }
    
private:
#ifdef CPLANG_HAS_LLVM
    // LLVM ORC JIT 实例（当 LLVM 可用时使用）
    std::unique_ptr<llvm::orc::LLJIT> lljit_;
#endif
};

// ═══════════════════════════════════════════════════════════════════
//  OrcJIT 实现
// ═══════════════════════════════════════════════════════════════════

OrcJIT::OrcJIT() 
    : impl_(std::make_unique<Impl>())
    , llvmCodegen_(std::make_unique<LLVMCodegen>())
{
}

OrcJIT::~OrcJIT() {
    stopBackgroundThread();
}

bool OrcJIT::initialize() {
    if (initialized_) return available_;
    
    VERBOSE(std::cout << "[OrcJIT] 初始化 ORC JIT...\n");
    
    // 尝试初始化 LLVM ORC JIT
    available_ = impl_->initialize(llvmDir_);
    
    if (available_) {
        VERBOSE(std::cout << "[OrcJIT] ORC JIT 初始化成功\n");
        VERBOSE(std::cout << "[OrcJIT] 编译模式: 内存 JIT\n");
    } else {
        VERBOSE(std::cout << "[OrcJIT] ORC JIT 不可用，请安装 LLVM 开发包\n");
        VERBOSE(std::cout << "[OrcJIT] 下载地址: https://github.com/llvm/llvm-project/releases\n");
        VERBOSE(std::cout << "[OrcJIT] 选择: clang+llvm-*.tar.xz (不是 LLVM-*.exe)\n");
    }
    
    initialized_ = true;
    return available_;
}

void OrcJIT::setLLVMDir(const std::string& path) {
    llvmDir_ = path;
    if (initialized_) {
        available_ = impl_->initialize(llvmDir_);
    }
}

void* OrcJIT::compileIR(const std::string& ir, const std::string& funcName) {
    if (!available_) return nullptr;
    return compileInternal(ir, funcName);
}

void* OrcJIT::compileAST(Shared<Program> program, const std::string& funcName) {
    if (!available_ || !program) return nullptr;
    
    // 保存 program 用于后续热点编译
    program_ = program;
    
    stats_.totalCompiles++;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // 生成 LLVM IR
    OptLevel opt = static_cast<OptLevel>(optLevel_);
    llvmCodegen_->setOptLevel(opt);
    std::string ir = llvmCodegen_->generateIRString(program, opt);
    
    if (ir.empty()) {
        VERBOSE(std::cout << "[OrcJIT] 错误: LLVM IR 生成失败\n");
        stats_.failedCompiles++;
        return nullptr;
    }
    
    // 编译
    void* result = compileInternal(ir, funcName);
    
    if (result) {
        auto endTime = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double, std::milli>(endTime - startTime).count();
        
        stats_.successfulCompiles++;
        stats_.totalCompileTimeMs += elapsed;
        
        VERBOSE(std::cout << "[OrcJIT] 编译成功: " << funcName 
                  << " (耗时: " << std::fixed << std::setprecision(2) << elapsed << "ms)\n");
    } else {
        stats_.failedCompiles++;
    }
    
    return result;
}

void OrcJIT::compileAll(Shared<Program> program) {
    if (!available_ || !program) return;
    
    program_ = program;
    
    OptLevel opt = static_cast<OptLevel>(optLevel_);
    llvmCodegen_->setOptLevel(opt);
    std::string ir = llvmCodegen_->generateIRString(program, opt);
    
    if (ir.empty()) {
        VERBOSE(std::cout << "[OrcJIT] 全量编译: IR 生成失败\n");
        return;
    }
    
    fullCompileCache_ = impl_->compileAll(ir);
}

void* OrcJIT::lookup(const std::string& name) {
    return impl_->lookup(name);
}

bool OrcJIT::addSymbol(const std::string& name, void* address) {
    return impl_->addSymbol(name, address);
}

void* OrcJIT::compileInternal(const std::string& ir, const std::string& funcName) {
    return impl_->compile(ir, funcName);
}

// ═══════════════════════════════════════════════════════════════════
//  热点检测
// ═══════════════════════════════════════════════════════════════════

void OrcJIT::recordCall(VMFunction* func) {
    if (!func || !enableProfiling_) return;
    callCounts_[func]++;
}

bool OrcJIT::shouldCompile(VMFunction* func) const {
    if (!func || !available_) return false;
    // 已尝试过但编译失败的函数，不再尝试
    if (failedFunctions_.count(func)) return false;
    auto it = callCounts_.find(func);
    return it != callCounts_.end() && it->second >= hotThreshold_;
}

int OrcJIT::getCallCount(VMFunction* func) const {
    auto it = callCounts_.find(func);
    return it != callCounts_.end() ? it->second : 0;
}

const JITCompiledFunction* OrcJIT::getCompiledFunction(VMFunction* func) const {
    auto it = compiledFunctions_.find(func);
    return it != compiledFunctions_.end() ? &it->second : nullptr;
}

void* OrcJIT::compileHotFunction(VMFunction* func) {
    if (!func || !available_ || !program_) return nullptr;
    
    // 已经尝试过但编译失败的函数，不再重试
    if (failedFunctions_.count(func)) {
        return nullptr;
    }
    
    // 从 VMFunction 获取函数名
    std::string funcName;
    if (func->name && func->name->length > 0) {
        funcName = std::string(func->name->data, func->name->length);
    }
    if (funcName.empty()) return nullptr;
    

    
    // 检查全量预编译缓存
    // Try raw name first (compileAll stores decoded LLVM names), then sanitized
    auto cacheIt = fullCompileCache_.find(funcName);
    String safeName = llvmCodegen_->sanitizeName(funcName);
    if (cacheIt == fullCompileCache_.end()) {
        cacheIt = fullCompileCache_.find(safeName);
    }
    if (cacheIt != fullCompileCache_.end()) {
        return cacheIt->second;
    }
    
    // 只为目标函数生成 IR（避免重复定义 main 等其他函数）
    OptLevel opt = static_cast<OptLevel>(optLevel_);
    llvmCodegen_->setOptLevel(opt);
    llvmCodegen_->resetNativeCallFlag();
    std::string ir = llvmCodegen_->generateSingleFunctionIRString(program_, funcName, opt);
    
    if (llvmCodegen_->hasNativeCalls()) {
        // 函数包含 native 调用，暂不支持 JIT 编译
        // 标记为失败，避免重复尝试，同时 VM 可正常执行 bytecode 版本
        failedFunctions_.insert(func);
        stats_.failedCompiles++;
        stats_.totalCompiles++;
        return nullptr;
    }
    
    if (ir.empty()) {
        VERBOSE(std::cout << "[OrcJIT] 错误: 函数 '" << funcName << "' 未找到\n");
        failedFunctions_.insert(func);
        stats_.failedCompiles++;
        stats_.totalCompiles++;
        return nullptr;
    }
    
    // 使用 sanitized 函数名查找（中文函数名被转义了）
    void* entry = compileInternal(ir, safeName);
    if (entry) {
        stats_.successfulCompiles++;
        stats_.totalCompiles++;
        JITCompiledFunction cf;
        cf.address = entry;
        cf.name = funcName;
        cf.isHot = true;
        compiledFunctions_[func] = cf;
        hotFunctions_.insert(func);
        VERBOSE(std::cout << "[OrcJIT] 热点编译: " << funcName 
                  << " (调用次数: " << callCounts_[func] << ")\n");
    } else {
        failedFunctions_.insert(func);
        stats_.failedCompiles++;
        stats_.totalCompiles++;
    }
    return entry;
}

// ═══════════════════════════════════════════════════════════════════
//  后台编译
// ═══════════════════════════════════════════════════════════════════

void OrcJIT::startBackgroundThread() {
    if (running_) return;
    running_ = true;
    bgThread_ = std::thread(&OrcJIT::backgroundWorker, this);
}

void OrcJIT::stopBackgroundThread() {
    if (!running_) return;
    running_ = false;
    queueCv_.notify_all();
    if (bgThread_.joinable()) {
        bgThread_.join();
    }
}

void OrcJIT::submitCompileTask(const CompileTask& task) {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        taskQueue_.push(task);
    }
    queueCv_.notify_one();
}

void OrcJIT::waitForCompletion() {
    std::unique_lock<std::mutex> lock(queueMutex_);
    queueCv_.wait(lock, [this] { return taskQueue_.empty(); });
}

void OrcJIT::backgroundWorker() {
    while (running_) {
        CompileTask task;
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            queueCv_.wait(lock, [this] { return !taskQueue_.empty() || !running_; });
            
            if (!running_) break;
            
            task = taskQueue_.front();
            taskQueue_.pop();
        }
        
        // 执行编译
        void* result = compileAST(task.program, task.funcName);
        
        // 调用回调
        if (task.callback) {
            task.callback(result, task.funcName);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
//  缓存管理
// ═══════════════════════════════════════════════════════════════════

void OrcJIT::clearCache() {
    impl_->clearCache();
    compiledFunctions_.clear();
    failedFunctions_.clear();
    callCounts_.clear();
    hotFunctions_.clear();
}

void OrcJIT::dumpStats() const {
    VERBOSE(
    std::cout << "\n╔══════════════════════════════════════════════╗\n";
    std::cout << "║           ORC JIT 统计                      ║\n";
    std::cout << "╠══════════════════════════════════════════════╣\n";
    std::cout << "║  模式:         " << (available_ ? "ORC JIT" : "不可用") << "                   ║\n";
    std::cout << "║  热点阈值:     " << hotThreshold_ << " 次                      ║\n";
    std::cout << "║  优化级别:     O" << optLevel_ << "                            ║\n";
    std::cout << "╠══════════════════════════════════════════════╣\n";
    std::cout << "║  编译请求:     " << stats_.totalCompiles.load() << "                        ║\n";
    std::cout << "║  成功编译:     " << stats_.successfulCompiles.load() << "                        ║\n";
    std::cout << "║  失败编译:     " << stats_.failedCompiles.load() << "                        ║\n";
    std::cout << "║  缓存命中:     " << stats_.cacheHits.load() << "                        ║\n";
    if (stats_.successfulCompiles > 0) {
        double avg = stats_.totalCompileTimeMs / stats_.successfulCompiles;
        std::cout << "║  平均耗时:     " << std::fixed << std::setprecision(2) << avg << " ms                ║\n";
    }
    std::cout << "╚══════════════════════════════════════════════╝\n\n";
    );
}

} // namespace cplang
