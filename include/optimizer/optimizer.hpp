// 优化管理器
// 统一管理所有优化 pass

#pragma once
#include "ast/ast.hpp"
#include "common/types.hpp"  // OptLevel
#include "optimizer/constant_folder.hpp"
#include "optimizer/dead_code_eliminator.hpp"
#include "optimizer/function_inliner.hpp"
#include "optimizer/tail_recursion_optimizer.hpp"
#include "optimizer/loop_unroller.hpp"
#include "optimizer/escape_analyzer.hpp"

namespace cplang {

// 优化统计
struct OptStats {
    int constantsFolded = 0;
    int deadCodeRemoved = 0;
    int functionsInlined = 0;
    int tailRecOptimized = 0;
    int loopsUnrolled = 0;
    int stackAllocs = 0;            // 栈上分配数
    int heapAllocsAvoided = 0;     // 避免的堆分配
    int iterations = 0;
};

// 优化管理器
class Optimizer {
public:
    Optimizer(OptLevel level = OptLevel::O2) : level_(level) {}
    
    // 优化整个程序
    Shared<Program> optimize(Shared<Program> program);
    
    // 优化单个语句
    Shared<Stmt> optimizeStmt(Shared<Stmt> stmt);
    
    // 获取统计
    const OptStats& getStats() const { return stats_; }
    
    // 打印统计
    void printStats() const;
    
    // 获取逃逸分析结果
    const ProgramEscapeResult& getEscapeResult() const { return escapeResult_; }
    
private:
    OptLevel level_;
    OptStats stats_;
    ProgramEscapeResult escapeResult_;
    
    ConstantFolder folder_;
    DeadCodeEliminator dce_;
    FunctionInliner inliner_;
    TailRecursionOptimizer tailRec_;
    LoopUnroller unroller_;
    EscapeAnalyzer escape_;
    
    // 单轮优化
    bool runOnePass(Shared<Program> program);
};

} // namespace cplang
