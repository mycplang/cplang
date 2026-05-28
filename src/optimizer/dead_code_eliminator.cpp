// 死代码消除优化器实现

#include "optimizer/dead_code_eliminator.hpp"

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  条件检查
// ═══════════════════════════════════════════════════════════════════

ConstValue DeadCodeEliminator::checkCondition(Shared<Expr> cond) {
    return folder_.tryEval(cond);
}

// ═══════════════════════════════════════════════════════════════════
//  消除语句块中的死代码
// ═══════════════════════════════════════════════════════════════════

std::vector<Shared<Stmt>> DeadCodeEliminator::eliminateBlock(const std::vector<Shared<Stmt>>& stmts) {
    std::vector<Shared<Stmt>> result;
    bool afterReturn = false;  // return 之后的代码
    
    for (const auto& stmt : stmts) {
        if (afterReturn) {
            // return 之后的代码永远不会执行
            removedCount_++;
            continue;
        }
        
        Shared<Stmt> optimized = eliminate(stmt);
        if (optimized) {
            result.push_back(optimized);
            
            // 检查是否是 return
            if (std::dynamic_pointer_cast<ReturnStmt>(optimized)) {
                afterReturn = true;
            }
        } else {
            removedCount_++;
        }
    }
    
    return result;
}

// ═══════════════════════════════════════════════════════════════════
//  消除单条语句中的死代码
// ═══════════════════════════════════════════════════════════════════

Shared<Stmt> DeadCodeEliminator::eliminate(Shared<Stmt> stmt) {
    if (!stmt) return nullptr;
    
    // if 语句优化
    if (auto ifStmt = std::dynamic_pointer_cast<IfStmt>(stmt)) {
        ConstValue cond = checkCondition(ifStmt->condition);
        
        if (cond.isBool()) {
            // 条件是常量
            if (cond.boolVal) {
                // 条件永远为真，只保留 then 分支
                removedCount_++;
                return ifStmt->thenBranch;
            } else {
                // 条件永远为假
                if (ifStmt->elseBranch) {
                    // 只保留 else 分支
                    removedCount_++;
                    return ifStmt->elseBranch;
                } else {
                    // 整个 if 可以删除
                    removedCount_++;
                    return nullptr;
                }
            }
        }
        
        // 条件不是常量，递归处理分支
        ifStmt->thenBranch = eliminate(ifStmt->thenBranch);
        if (ifStmt->elseBranch) {
            ifStmt->elseBranch = eliminate(ifStmt->elseBranch);
        }
        return ifStmt;
    }
    
    // while 语句优化
    if (auto whileStmt = std::dynamic_pointer_cast<WhileStmt>(stmt)) {
        ConstValue cond = checkCondition(whileStmt->condition);
        
        if (cond.isBool() && !cond.boolVal) {
            // 条件永远为假，整个循环可以删除
            removedCount_++;
            return nullptr;
        }
        
        // 递归处理循环体
        whileStmt->body = eliminate(whileStmt->body);
        return whileStmt;
    }
    
    // for 语句优化
    if (auto forStmt = std::dynamic_pointer_cast<ForStmt>(stmt)) {
        if (forStmt->condition) {
            ConstValue cond = checkCondition(forStmt->condition);
            if (cond.isBool() && !cond.boolVal) {
                // 条件永远为假，整个循环可以删除
                removedCount_++;
                return nullptr;
            }
        }
        
        // 递归处理
        if (forStmt->init) forStmt->init = eliminate(forStmt->init);
        if (forStmt->body) forStmt->body = eliminate(forStmt->body);
        if (forStmt->update) {
            // update 是表达式，包装为语句处理
        }
        return forStmt;
    }
    
    // 块语句
    if (auto block = std::dynamic_pointer_cast<BlockStmt>(stmt)) {
        block->statements = eliminateBlock(block->statements);
        if (block->statements.empty()) {
            removedCount_++;
            return nullptr;
        }
        return block;
    }
    
    return stmt;
}

} // namespace cplang
