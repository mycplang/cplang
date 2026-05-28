// 函数内联演示
// 展示内联优化的效果

#include <iostream>
#include <chrono>
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic_analyzer.hpp"
#include "optimizer/function_inliner.hpp"
#include "codegen/llvm_codegen.hpp"

using namespace cplang;

// 测试用例
struct InlineTest {
    String name;
    String source;
    int expectedInline;
};

int main() {
    std::cout << "╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║        函数内联优化演示                              ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n\n";
    
    std::vector<InlineTest> tests = {
        {
            "简单小函数",
            R"(
                函数 add(整数 a, 整数 b) {
                    返回 a + b;
                }
                函数 main() {
                    整数 x = add(1, 2);
                    整数 y = add(3, 4);
                    返回 x + y;
                }
            )",
            2  // 预期内联 2 次
        },
        {
            "嵌套调用",
            R"(
                函数 square(整数 x) {
                    返回 x * x;
                }
                函数 double(整数 n) {
                    返回 n * 2;
                }
                函数 main() {
                    整数 r = square(double(3));
                    返回 r;
                }
            )",
            2  // square 和 double 都内联
        },
        {
            "大函数不内联",
            R"(
                函数 large(整数 x) {
                    整数 a = x + 1;
                    整数 b = a * 2;
                    整数 c = b + 3;
                    整数 d = c - 4;
                    整数 e = d * 5;
                    整数 f = e + 6;
                    整数 g = f - 7;
                    整数 h = g + 8;
                    整数 i = h * 9;
                    整数 j = i + 10;
                    返回 j;
                }
                函数 main() {
                    返回 large(100);
                }
            )",
            0  // 函数太大，不内联
        },
        {
            "多次调用同一函数",
            R"(
                函数 inc(整数 x) {
                    返回 x + 1;
                }
                函数 main() {
                    整数 a = inc(1);
                    整数 b = inc(2);
                    整数 c = inc(3);
                    整数 d = inc(4);
                    整数 e = inc(5);
                    返回 a + b + c + d + e;
                }
            )",
            5  // 5 次调用都内联
        }
    };
    
    InlineConfig config;
    config.maxASTSize = 15;  // 最大 15 个 AST 节点
    config.maxCallDepth = 3;
    
    for (const auto& test : tests) {
        std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << "【" << test.name << "】\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        // 编译
        Lexer lexer;
        auto tokens = lexer.tokenize(test.source);
        
        Parser parser;
        auto program = parser.parse(tokens);
        
        SemanticAnalyzer semantic;
        semantic.analyze(program);
        
        // 内联优化
        FunctionInliner inliner(config);
        auto optProgram = inliner.inlineFunctions(program);
        
        // 打印分析
        inliner.printAnalysis();
        
        std::cout << "\n预期内联: " << test.expectedInline << " 次\n";
        std::cout << "实际内联: " << inliner.getInlineCount() << " 次\n";
        
        if (inliner.getInlineCount() == test.expectedInline) {
            std::cout << "✓ 测试通过\n";
        } else {
            std::cout << "✗ 测试失败\n";
        }
        
        // 显示优化后的代码
        std::cout << "\n优化后的 LLVM IR:\n";
        LLVMCodegen codegen;
        String ir = codegen.generate(optProgram);
        std::cout << ir.substr(0, 500) << "...\n";
    }
    
    // 性能对比
    std::cout << "\n\n╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║        性能对比                                      ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n\n";
    
    String perfTest = R"(
        函数 add(整数 a, 整数 b) { 返回 a + b; }
        函数 main() {
            整数 sum = 0;
            整数 i = 0;
            当 i < 1000000 时 {
                sum = add(sum, i);
                i = i + 1;
            }
            返回 sum;
        }
    )";
    
    // 无内联
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100; i++) {
        Lexer lexer;
        auto tokens = lexer.tokenize(perfTest);
        Parser parser;
        auto program = parser.parse(tokens);
        SemanticAnalyzer semantic;
        semantic.analyze(program);
        LLVMCodegen codegen;
        codegen.generate(program);
    }
    auto end = std::chrono::high_resolution_clock::now();
    double timeNoInline = std::chrono::duration<double, std::milli>(end - start).count() / 100;
    
    // 有内联
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100; i++) {
        Lexer lexer;
        auto tokens = lexer.tokenize(perfTest);
        Parser parser;
        auto program = parser.parse(tokens);
        SemanticAnalyzer semantic;
        semantic.analyze(program);
        FunctionInliner inliner(config);
        inliner.inlineFunctions(program);
        LLVMCodegen codegen;
        codegen.generate(program);
    }
    end = std::chrono::high_resolution_clock::now();
    double timeWithInline = std::chrono::duration<double, std::milli>(end - start).count() / 100;
    
    std::cout << "无内联: " << timeNoInline << " ms\n";
    std::cout << "有内联: " << timeWithInline << " ms\n";
    std::cout << "提升: " << (timeNoInline / timeWithInline) << "x\n";
    
    std::cout << "\n完成！\n";
    return 0;
}
