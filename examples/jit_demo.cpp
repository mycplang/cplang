// JIT 编译演示
// 展示 JIT 编译的效果

#include <iostream>
#include <chrono>
#include "jit/jit_compiler.hpp"
#include "codegen/codegen.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic_analyzer.hpp"
#include "vm/vm.hpp"

using namespace cplang;

int main() {
    std::cout << "╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║        JIT 编译演示                                 ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n\n";
    
    // 测试函数：斐波那契
    std::string fibSource = R"(
        函数 fib(n) {
            如果 (n <= 1) { 返回 n; }
            返回 fib(n - 1) + fib(n - 2);
        }
    )";
    
    std::cout << "测试函数: fib(n)\n";
    std::cout << "──────────────────────────────────────────────────\n\n";
    
    // 先编译到字节码
    Lexer lexer(fibSource);
    Parser parser(&lexer);
    auto program = parser.parse();
    if (!program || parser.hasError()) {
        std::cout << "解析失败\n";
        return 1;
    }
    
    SemanticAnalyzer analyzer;
    if (!analyzer.analyze(program)) {
        std::cout << "语义分析失败\n";
        return 1;
    }
    
    VM vm;
    Codegen codegen(&vm, &analyzer);
    VMFunction* func = codegen.compile(program);
    if (!func) {
        std::cout << "代码生成失败\n";
        return 1;
    }
    
    std::cout << "字节码编译成功\n\n";
    
    // 初始化 JIT
    JITCompiler jit;
    if (!jit.initialize()) {
        std::cout << "JIT 初始化失败（LLVM 开发库未配置）\n";
        std::cout << "回退到字节码执行\n";
        return 0;
    }
    
    std::cout << "JIT 初始化成功\n";
    
    // 尝试 JIT 编译
    auto start = std::chrono::high_resolution_clock::now();
    void* jitFunc = jit.compile(func);
    auto end = std::chrono::high_resolution_clock::now();
    auto compileTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    if (jitFunc) {
        std::cout << "JIT 编译成功!\n";
        std::cout << "  编译时间: " << compileTime << " ms\n";
        std::cout << "  函数地址: " << jitFunc << "\n\n";
    } else {
        std::cout << "JIT 编译失败，回退到字节码执行\n";
    }
    
    // 打印统计
    jit.dumpStats();
    
    std::cout << "\n完成！\n";
    return 0;
}
