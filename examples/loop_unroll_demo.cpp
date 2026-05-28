// 循环展开演示
// 展示循环展开的效果

#include <iostream>
#include <chrono>
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic_analyzer.hpp"
#include "optimizer/loop_unroller.hpp"
#include "codegen/llvm_codegen.hpp"

using namespace cplang;

// 性能测试
void benchmarkUnroll() {
    std::cout << "\n╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║        性能对比                                      ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n\n";
    
    const int N = 100000000;
    
    // 未展开版本
    auto start1 = std::chrono::high_resolution_clock::now();
    volatile int sum1 = 0;
    for (int i = 0; i < N; i++) {
        sum1 += i;
    }
    auto end1 = std::chrono::high_resolution_clock::now();
    auto time1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1).count();
    
    // 手动展开版本（展开因子4）
    auto start2 = std::chrono::high_resolution_clock::now();
    volatile int sum2 = 0;
    for (int i = 0; i < N; i += 4) {
        sum2 += i;
        sum2 += i + 1;
        sum2 += i + 2;
        sum2 += i + 3;
    }
    auto end2 = std::chrono::high_resolution_clock::now();
    auto time2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2).count();
    
    std::cout << "未展开版本: " << time1 << " ms\n";
    std::cout << "展开版本:   " << time2 << " ms\n";
    std::cout << "加速比:     " << (double)time1 / time2 << "x\n";
}

int main() {
    std::cout << "╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║        循环展开优化演示                              ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n\n";
    
    // 测试用例
    std::vector<std::pair<String, String>> tests = {
        {"小循环（4次）", R"(
            对于 整数 i = 0; i < 4; i = i + 1 则 {
                打印(i);
            }
        )"},
        {"中等循环（8次）", R"(
            对于 整数 i = 0; i < 8; i = i + 1 则 {
                sum = sum + i;
            }
        )"},
        {"大循环（100次）", R"(
            对于 整数 i = 0; i < 100; i = i + 1 则 {
                arr[i] = i;
            }
        )"},
        {"嵌套循环", R"(
            对于 整数 i = 0; i < 4; i = i + 1 则 {
                对于 整数 j = 0; j < 4; j = j + 1 则 {
                    sum = sum + i * j;
                }
            }
        )"}
    };
    
    LoopUnroller unroller;
    
    for (const auto& test : tests) {
        std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << "【" << test.first << "】\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        // 编译
        Lexer lexer;
        auto tokens = lexer.tokenize(test.second);
        
        Parser parser;
        auto program = parser.parse(tokens);
        
        // 查找循环
        for (const auto& stmt : program->statements) {
            if (auto forStmt = std::dynamic_pointer_cast<ForStmt>(stmt)) {
                unroller.printAnalysis(forStmt);
                
                auto info = unroller.analyzeLoop(forStmt);
                if (unroller.shouldUnroll(info)) {
                    int factor = unroller.computeUnrollFactor(info);
                    std::cout << "\n展开前:\n";
                    std::cout << "  for (i = 0; i < " << info.tripCount << "; i++) { ... }\n";
                    
                    std::cout << "\n展开后（因子 " << factor << "）:\n";
                    std::cout << "  for (i = 0; i < " << (info.tripCount / factor) << "; i += " << factor << ") {\n";
                    std::cout << "    // 循环体 x " << factor << "\n";
                    std::cout << "  }\n";
                }
                break;
            }
        }
    }
    
    // 性能对比
    benchmarkUnroll();
    
    // 展开策略说明
    std::cout << "\n\n╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║        展开策略                                      ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "完全展开（小循环）:\n";
    std::cout << "  - 循环次数 ≤ 8\n";
    std::cout << "  - 展开为顺序语句\n";
    std::cout << "  - 消除所有循环开销\n\n";
    
    std::cout << "部分展开（中等循环）:\n";
    std::cout << "  - 循环次数 8-64\n";
    std::cout << "  - 展开因子 4-8\n";
    std::cout << "  - 减少循环开销 75-87.5%\n\n";
    
    std::cout << "不展开（大循环）:\n";
    std::cout << "  - 循环次数 > 64\n";
    std::cout << "  - 代码膨胀过大\n";
    std::cout << "  - 可能降低指令缓存命中率\n";
    
    std::cout << "\n完成！\n";
    return 0;
}
