// HybridJIT — 混合 JIT 策略实现
// 优先使用 ORC JIT（内存中 LLVM JIT），降级到外部进程模式（clang 编译）
#include "jit/hybrid_jit.hpp"
#include "jit/orc_jit.hpp"
#include "jit/jit_compiler.hpp"
#include "vm/vm.hpp"
#include "ast/ast.hpp"
#include "codegen/codegen.hpp"
#include "core/verbose.hpp"
#include <iostream>

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  HybridJIT::ExternalJIT — 外部进程模式（降级方案）
// ═══════════════════════════════════════════════════════════════════

class HybridJIT::ExternalJIT {
public:
    ExternalJIT() : compiler_(std::make_unique<JITCompiler>()) {}

    bool initialize() {
        return compiler_->initialize();
    }

    void* compile(Shared<Program> program, const std::string& funcName) {
        return compiler_->compileFromAST(program, funcName);
    }

    void recordCall(VMFunction* func) {
        compiler_->recordCall(func);
    }

    bool shouldCompile(VMFunction* func) const {
        return compiler_->shouldCompile(func);
    }

    void dumpStats() const {
        compiler_->dumpStats();
    }

private:
    std::unique_ptr<JITCompiler> compiler_;
};

// ═══════════════════════════════════════════════════════════════════
//  HybridJIT 实现
// ═══════════════════════════════════════════════════════════════════

HybridJIT::HybridJIT()
    : orcJit_(std::make_unique<OrcJIT>())
    , externalJit_(std::make_unique<ExternalJIT>())
{
}

HybridJIT::~HybridJIT() = default;

bool HybridJIT::initialize() {
    // 首先尝试 ORC JIT（内存中编译，性能最佳）
    if (orcJit_->initialize()) {
        mode_ = Mode::Orc;
        VERBOSE(std::cout << "[HybridJIT] 使用 ORC JIT 模式（内存中编译）\n");
        return true;
    }

    // 降级到外部进程模式（通过 clang 编译）
    if (externalJit_->initialize()) {
        mode_ = Mode::External;
        VERBOSE(std::cout << "[HybridJIT] 使用外部进程模式（clang 编译）\n");
        return true;
    }

    mode_ = Mode::None;
    VERBOSE(std::cout << "[HybridJIT] JIT 不可用，将使用字节码解释执行\n");
    return false;
}

void* HybridJIT::compile(Shared<Program> program, const std::string& funcName) {
    switch (mode_) {
        case Mode::Orc:
            return orcJit_->compileAST(program, funcName);
        case Mode::External:
            return externalJit_->compile(program, funcName);
        default:
            return nullptr;
    }
}

void HybridJIT::storeProgram(Shared<Program> program) {
    switch (mode_) {
        case Mode::Orc:
            orcJit_->storeProgram(program);
            break;
        case Mode::External:
            // 外部进程模式不需要存储 AST
            break;
        default:
            break;
    }
}

void HybridJIT::compileAll(Shared<Program> program) {
    switch (mode_) {
        case Mode::Orc:
            orcJit_->compileAll(program);
            break;
        case Mode::External:
            VERBOSE(std::cerr << "[HybridJIT] 外部进程模式不支持全量预编译，请使用 ORC JIT\n");
            break;
        default:
            break;
    }
}

void HybridJIT::recordCall(VMFunction* func) {
    switch (mode_) {
        case Mode::Orc:
            orcJit_->recordCall(func);
            break;
        case Mode::External:
            externalJit_->recordCall(func);
            break;
        default:
            break;
    }
}

bool HybridJIT::shouldCompile(VMFunction* func) const {
    switch (mode_) {
        case Mode::Orc:
            return orcJit_->shouldCompile(func);
        case Mode::External:
            return externalJit_->shouldCompile(func);
        default:
            return false;
    }
}

void HybridJIT::setHotThreshold(int threshold) {
    switch (mode_) {
        case Mode::Orc:
            orcJit_->setHotThreshold(threshold);
            break;
        case Mode::External:
            // 外部进程模式暂不支持热点阈值
            break;
        default:
            break;
    }
}

void* HybridJIT::compileHotFunction(VMFunction* func) {
    switch (mode_) {
        case Mode::Orc:
            return orcJit_->compileHotFunction(func);
        case Mode::External:
            return nullptr;  // 外部进程模式暂不支持热点编译
        default:
            return nullptr;
    }
}

void HybridJIT::dumpStats() const {
    switch (mode_) {
        case Mode::Orc:
            orcJit_->dumpStats();
            break;
        case Mode::External:
            externalJit_->dumpStats();
            break;
        default:
            VERBOSE(std::cout << "[HybridJIT] JIT 未初始化\n");
            break;
    }
}

} // namespace cplang