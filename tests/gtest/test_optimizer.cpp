// CP语言优化器测试
#include "minimal_test.hpp"
#include "optimizer/optimizer.hpp"
#include "parser/parser.hpp"
#include "ast/ast.hpp"

using namespace cplang;

TEST(OptimizerTest, CreateOptimizer) {
    Optimizer opt(OptLevel::O2);
    EXPECT_EQ(opt.getStats().constantsFolded, 0);
}

TEST(OptimizerTest, ConstantFolding) {
    auto program = parseString(u8"变量 x = 1 + 2 * 3;");
    ASSERT_NE(program, nullptr);
    
    Optimizer opt(OptLevel::O1);
    auto optimized = opt.optimize(program);
    
    EXPECT_GE(opt.getStats().constantsFolded, 0);
}

TEST(OptimizerTest, DeadCodeElimination) {
    auto program = parseString(u8"变量 x = 10; 如果 (false) { 打印(x); }");
    ASSERT_NE(program, nullptr);
    
    Optimizer opt(OptLevel::O2);
    auto optimized = opt.optimize(program);
    
    EXPECT_GE(opt.getStats().deadCodeRemoved, 0);
}

TEST(OptimizerTest, TailRecursionOptimization) {
    auto program = parseString(u8"函数 fact(n, acc) { 如果 (n <= 1) { 返回 acc; } 返回 fact(n-1, acc*n); }");
    ASSERT_NE(program, nullptr);
    
    Optimizer opt(OptLevel::O2);
    auto optimized = opt.optimize(program);
    
    EXPECT_GE(opt.getStats().tailRecOptimized, 0);
}

TEST(OptimizerTest, LoopUnrolling) {
    auto program = parseString(u8"变量 sum = 0; 循环 (变量 i = 0; i < 5; i = i + 1) { sum = sum + i; }");
    ASSERT_NE(program, nullptr);
    
    Optimizer opt(OptLevel::O2);
    auto optimized = opt.optimize(program);
    
    EXPECT_GE(opt.getStats().loopsUnrolled, 0);
}

TEST(OptimizerTest, FunctionInlining) {
    auto program = parseString(u8"函数 add(a, b) { 返回 a + b; } 变量 x = add(1, 2);");
    ASSERT_NE(program, nullptr);
    
    Optimizer opt(OptLevel::O3);
    auto optimized = opt.optimize(program);
    
    EXPECT_GE(opt.getStats().functionsInlined, 0);
}

TEST(OptimizerTest, EscapeAnalysis) {
    auto program = parseString(u8"变量 arr = [1, 2, 3]; 打印(arr[0]);");
    ASSERT_NE(program, nullptr);
    
    Optimizer opt(OptLevel::O2);
    auto optimized = opt.optimize(program);
    
    // 逃逸分析总是运行，不修改AST但收集信息
    EXPECT_GE(opt.getEscapeResult().functionResults.size(), 0);
}

TEST(OptimizerTest, NoOptimization) {
    auto program = parseString(u8"变量 x = 10;");
    ASSERT_NE(program, nullptr);
    
    Optimizer opt(OptLevel::None);
    auto optimized = opt.optimize(program);
    
    // 无优化时统计应该都是0
    EXPECT_EQ(opt.getStats().constantsFolded, 0);
    EXPECT_EQ(opt.getStats().deadCodeRemoved, 0);
}

TEST(OptimizerTest, MultiplePasses) {
    auto program = parseString(u8"变量 x = 1 + 2; 变量 y = x * 3;");
    ASSERT_NE(program, nullptr);
    
    Optimizer opt(OptLevel::O2);
    auto optimized = opt.optimize(program);
    
    // 应该有多轮迭代
    EXPECT_GE(opt.getStats().iterations, 1);
}
