// 编译管线实现

#include "compiler/optimized_compiler.hpp"
#include <iostream>
#include <fstream>

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  从源代码编译
// ═══════════════════════════════════════════════════════════════════

CompileResult Compiler::compile(const String& source) {
    CompileResult result;
    
    // 1. 词法分析
    std::vector<Token> tokens = lexer_.tokenize(source);
    if (lexer_.hasError()) {
        result.error = lexer_.errorMessage();
        return result;
    }
    
    // 2. 语法分析
    Shared<Program> program = parser_.parse(tokens);
    if (parser_.hasError()) {
        result.error = parser_.errorMessage();
        return result;
    }
    
    // 3. 语义分析
    if (!semantic_.analyze(program)) {
        result.error = semantic_.errorMessage();
        return result;
    }
    
    // 4. 优化
    program = optimizer_.optimize(program);
    result.optStats = optimizer_.getStats();
    
    // 5. 代码生成
    result.llvmIR = codegen_.generate(program);
    
    result.success = true;
    return result;
}

// ═══════════════════════════════════════════════════════════════════
//  从文件编译
// ═══════════════════════════════════════════════════════════════════

CompileResult Compiler::compileFile(const String& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        CompileResult result;
        result.error = "无法打开文件: " + filename;
        return result;
    }
    
    String source((std::istreambuf_iterator<char>(file)),
                  std::istreambuf_iterator<char>());
    
    return compile(source);
}

} // namespace cplang
