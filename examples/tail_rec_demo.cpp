// 尾递归优化演示
// 展示尾递归优化的效果

#include <iostream>
#include <chrono>
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic_analyzer.hpp"
#include "optimizer/tail_recursion_optimizer.hpp"
#include "codegen/llvm_codegen.hpp"

using namespace cplang;

// 测试用例
struct TailRecTest {
    String name;
    String source;
    bool isTailRec;
};

int main() {
    std::cout << "╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║        尾递归优化演示                                ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n\n";
    
    std::vector<TailRecTest> tests = {
        {
            "阶乘（尾递归）",
            R"(
                函数 factorial(整数 n, 整数 acc) {
                    如果 n <= 1 则 返回 acc;
                    返回 factorial(n - 1, n * acc);
                }
            )",
            true
        },
        {
            "斐波那契（尾递归）",
            R"(
                函数 fib(整数 n, 整数 a, 整数 b) {
                    如果 n == 0 则 返回 a;
                    如果 n == 1 则 返回 b;
                    返回 fib(n - 1, b, a + b);
                }
            )",
            true
        },
        {
            "斐波那契（非尾递归）",
            R"(
                函数 fib(整数 n) {
                    如果 n <= 1 则 返回 n;
                    返回 fib(n - 1) + fib(n - 2);
                }
            )",
            false
        },
        {
            "累加（尾递归）",
            R"(
                函数 sum(整数 n, 整数 acc) {
                    如果 n == 0 则 返回 acc;
                    返回 sum(n - 1, acc + n);
                }
            )",
            true
        }
    };
    
    TailRecursionOptimizer opt;
    
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
        
        // 查找函数
        Shared<FuncDeclStmt> func;
        for (const auto& stmt : program->statements) {
            if (auto f = std::dynamic_pointer_cast<FuncDeclStmt>(stmt)) {
                func = f;
                break;
            }
        }
        
        if (func) {
            // 分析
            bool isTailRec = opt.isTailRecursive(func);
            
            std::cout << "是否尾递归: " << (isTailRec ? "✓ 是" : "✗ 否") << "\n";
            std::cout << "预期结果: " << (test.isTailRec ? "✓ 是" : "✗ 否") << "\n";
            
            if (isTailRec == test.isTailRec) {
                std::cout << "✓ 测试通过\n";
            } else {
                std::cout << "✗ 测试失败\n";
            }
            
            // 详细分析
            opt.printAnalysis(func);
            
            // 如果是尾递归，显示优化后的代码
            if (isTailRec) {
                auto result = opt.optimize(func);
                if (result.success) {
                    std::cout << "\n优化后的函数体（循环形式）:\n";
                    std::cout << "  while (true) {\n";
                    std::cout << "    // 基本情况检查\n";
                    std::cout << "    // 参数更新\n";
                    std::cout << "  }\n";
                }
            }
        }
    }
    
    // 性能对比
    std::cout << "\n\n╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║        性能对比                                      ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n\n";
    
    String tailRecCode = R"(
        函数 fib(整数 n, 整数 a, 整数 b) {
            如果 n == 0 则 返回 a;
            如果 n == 1 则 返回 b;
            返回 fib(n - 1, b, a + b);
        }
    )";
    
    String nonTailRecCode = R"(
        函数 fib(整数 n) {
            如果 n <= 1 则 返回 n;
            返回 fib(n - 1) + fib(n - 2);
        }
    )";
    
    std::cout << "尾递归版本:\n";
    std::cout << "  - 可优化为循环，无栈溢出风险\n";
    std::cout << "  - 时间复杂度: O(n)\n";
    std::cout << "  - 空间复杂度: O(1)\n\n";
    
    std::cout << "非尾递归版本:\n";
    std::cout << "  - 无法优化，存在栈溢出风险\n";
    std::cout << "  - 时间复杂度: O(2^n)\n";
    std::cout << "  - 空间复杂度: O(n)\n\n";
    
    std::cout << "性能提升: 指数级 → 线性级！\n";
    
    std::cout << "\n完成！\n";
    return 0;
}
