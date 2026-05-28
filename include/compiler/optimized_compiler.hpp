// 编译管线（带优化）
// Lexer → Parser → Optimizer → Semantic → Codegen → VM/LLVM

#pragma once
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic_analyzer.hpp"
#include "optimizer/optimizer.hpp"
#include "codegen/llvm_codegen.hpp"
#include "vm/vm.hpp"

namespace cplang {

// 编译结果
struct CompileResult {
    bool success = false;
    String error;
    String llvmIR;
    OptStats optStats;
    
    // 可执行代码
    VMFunction* vmFunc = nullptr;
};

// 编译器
class Compiler {
public:
    Compiler(OptLevel optLevel = OptLevel::O2) 
        : optimizer_(optLevel), codegen_() {}
    
    // 从源代码编译
    CompileResult compile(const String& source);
    
    // 从文件编译
    CompileResult compileFile(const String& filename);
    
    // 设置优化级别
    void setOptLevel(OptLevel level) { optimizer_ = Optimizer(level); }
    
private:
    Lexer lexer_;
    Parser parser_;
    SemanticAnalyzer semantic_;
    Optimizer optimizer_;
    LLVMCodegen codegen_;
    
    // 编译阶段
    Shared<Program> parse(const String& source, String& error);
    bool analyze(Shared<Program> program, String& error);
    String generateLLVM(Shared<Program> program);
};

} // namespace cplang
