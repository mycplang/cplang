// 优化演示程序
// 展示常量折叠和死代码消除的效果

#include <iostream>
#include <chrono>
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic_analyzer.hpp"
#include "optimizer/optimizer.hpp"
#include "codegen/llvm_codegen.hpp"

using namespace cplang;

// 测试用例
struct TestCase {
    String name;
    String source;
};

// 性能计时
template<typename Func>
double measureTime(Func func, int iterations = 1) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        func();
    }
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count() / iterations;
}

int main() {
    std::cout << "╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║        CPLANG 优化器演示                             ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n\n";
    
    // 测试用例
    std::vector<TestCase> tests = {
        {
            "常量折叠",
            R"(
                常量 a = 3 + 4 * 2;
                常量 b = 100 / 5;
                常量 c = a + b;
                打印;
            )"
        },
        {
            "死代码消除",
            R"(
                如果 假 则 {
                    打印(1);  // 这段代码会被删除
                }
                如果 真 则 {
                    打印(2);  // 这段代码会保留
                }
            )"
        },
        {
            "复杂表达式折叠",
            R"(
                常量 x = (1 + 2) * (3 + 4);
                常量 y = 100 - 50 + 25;
                常量 z = x + y;
                打印;
            )"
        }
    };
    
    // 测试不同优化级别
    for (int level = 0; level <= 3; level++) {
        std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << "优化级别: O" << level << "\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        OptLevel optLevel = static_cast<OptLevel>(level);
        
        for (const auto& test : tests) {
            std::cout << "\n【" << test.name << "】\n";
            
            // 编译
            Lexer lexer;
            auto tokens = lexer.tokenize(test.source);
            
            Parser parser;
            auto program = parser.parse(tokens);
            
            SemanticAnalyzer semantic;
            semantic.analyze(program);
            
            // 优化
            Optimizer optimizer(optLevel);
            auto optProgram = optimizer.optimize(program);
            
            // 打印统计
            auto stats = optimizer.getStats();
            std::cout << "  常量折叠: " << stats.constantsFolded << " 次\n";
            std::cout << "  死代码消除: " << stats.deadCodeRemoved << " 处\n";
            std::cout << "  迭代次数: " << stats.iterations << "\n";
            
            // 生成 LLVM IR
            LLVMCodegen codegen;
            String ir = codegen.generate(optProgram);
            
            // 显示部分 IR
            std::cout << "  生成的 LLVM IR:\n";
            std::istringstream iss(ir);
            String line;
            int lineCount = 0;
            while (std::getline(iss, line) && lineCount < 15) {
                std::cout << "    " << line << "\n";
                lineCount++;
            }
        }
    }
    
    // 性能对比
    std::cout << "\n\n╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║        性能对比                                      ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n\n";
    
    String perfTest = R"(
        函数 compute() {
            常量 a = 1 + 2 + 3 + 4 + 5;
            常量 b = 10 * 10;
            常量 c = a + b;
            返回 c;
        }
    )";
    
    for (int level = 0; level <= 3; level++) {
        OptLevel optLevel = static_cast<OptLevel>(level);
        
        double time = measureTime([&]() {
            Lexer lexer;
            auto tokens = lexer.tokenize(perfTest);
            
            Parser parser;
            auto program = parser.parse(tokens);
            
            SemanticAnalyzer semantic;
            semantic.analyze(program);
            
            Optimizer optimizer(optLevel);
            optimizer.optimize(program);
            
            LLVMCodegen codegen;
            codegen.generate(program);
        }, 100);
        
        std::cout << "O" << level << ": " << time << " ms\n";
    }
    
    std::cout << "\n完成！\n";
    return 0;
}
