// 优化管理器实现

#include "optimizer/optimizer.hpp"
#include "core/verbose.hpp"
#include <iostream>

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  优化整个程序
// ═══════════════════════════════════════════════════════════════════

Shared<Program> Optimizer::optimize(Shared<Program> program) {
    if (!program || level_ == OptLevel::None) {
        return program;
    }
    
    stats_ = OptStats();
    
    // 迭代优化直到收敛
    bool changed = true;
    int maxIterations = 10;  // 防止无限循环
    
    while (changed && stats_.iterations < maxIterations) {
        changed = runOnePass(program);
        stats_.iterations++;
    }
    
    return program;
}

// ═══════════════════════════════════════════════════════════════════
//  单轮优化
// ═══════════════════════════════════════════════════════════════════

bool Optimizer::runOnePass(Shared<Program> program) {
    bool changed = false;
    
    // 1. 常量折叠
    if (level_ >= OptLevel::O1) {
        int before = folder_.getFoldCount();
        
        for (auto& stmt : program->statements) {
            stmt = folder_.foldStmt(stmt);
        }
        
        int after = folder_.getFoldCount();
        if (after > before) {
            changed = true;
            stats_.constantsFolded += (after - before);
        }
    }
    
    // 2. 死代码消除
    if (level_ >= OptLevel::O2) {
        int before = dce_.getRemovedCount();
        
        std::vector<Shared<Stmt>> newStmts;
        for (auto& stmt : program->statements) {
            Shared<Stmt> optimized = dce_.eliminate(stmt);
            if (optimized) {
                newStmts.push_back(optimized);
            }
        }
        
        int after = dce_.getRemovedCount();
        if (after > before) {
            changed = true;
            stats_.deadCodeRemoved += (after - before);
            program->statements = newStmts;
        }
    }
    
    // 3. 尾递归优化
    if (level_ >= OptLevel::O2) {
        int before = tailRec_.getOptCount();
        
        program = tailRec_.optimizeProgram(program);
        
        int after = tailRec_.getOptCount();
        if (after > before) {
            changed = true;
            stats_.tailRecOptimized += (after - before);
        }
    }
    
    // 4. 循环展开
    if (level_ >= OptLevel::O2) {
        int before = unroller_.getUnrollCount();
        
        program = unroller_.unrollProgram(program);
        
        int after = unroller_.getUnrollCount();
        if (after > before) {
            changed = true;
            stats_.loopsUnrolled += (after - before);
        }
    }
    
    // 5. 逃逸分析
    if (level_ >= OptLevel::O2) {
        // 逃逸分析不修改 AST，但提供信息给代码生成器
        escapeResult_ = escape_.analyze(program);
        stats_.stackAllocs += escapeResult_.totalStackAlloc;
        stats_.heapAllocsAvoided += escapeResult_.totalStackSaved;
    }
    
    // 6. 函数内联
    if (level_ >= OptLevel::O3) {
        int before = inliner_.getInlineCount();
        
        program = inliner_.inlineFunctions(program);
        
        int after = inliner_.getInlineCount();
        if (after > before) {
            changed = true;
            stats_.functionsInlined += (after - before);
        }
    }
    
    // 7. JIT 编译准备 — 已提升至 CLI 层
    // optimizer 保持纯 AST 变换器角色
    // JIT 集成由高层编排层负责 (src/cli/main.cpp)
    
    return changed;
}

// ═══════════════════════════════════════════════════════════════════
//  优化单个语句
// ═══════════════════════════════════════════════════════════════════

Shared<Stmt> Optimizer::optimizeStmt(Shared<Stmt> stmt) {
    if (!stmt || level_ == OptLevel::None) {
        return stmt;
    }
    
    // 常量折叠
    stmt = folder_.foldStmt(stmt);
    
    // 死代码消除
    stmt = dce_.eliminate(stmt);
    
    return stmt;
}

// ═══════════════════════════════════════════════════════════════════
//  打印统计
// ═══════════════════════════════════════════════════════════════════

void Optimizer::printStats() const {
    if (!cplang::verboseEnabled()) return;
        std::cout << "=== 优化统计 ===" << std::endl;
        std::cout << "常量折叠: " << stats_.constantsFolded << " 次" << std::endl;
        std::cout << "死代码消除: " << stats_.deadCodeRemoved << " 处" << std::endl;
        std::cout << "尾递归优化: " << stats_.tailRecOptimized << " 个函数" << std::endl;
        std::cout << "循环展开: " << stats_.loopsUnrolled << " 个循环" << std::endl;
        std::cout << "栈上分配: " << stats_.stackAllocs << " 个" << std::endl;
        std::cout << "避免堆分配: " << stats_.heapAllocsAvoided << " 个" << std::endl;
        std::cout << "函数内联: " << stats_.functionsInlined << " 次" << std::endl;
        std::cout << "迭代次数: " << stats_.iterations << std::endl;
}

} // namespace cplang