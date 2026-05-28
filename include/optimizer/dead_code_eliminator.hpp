// 死代码消除优化器
// 移除永远不会执行的代码

#pragma once
#include "ast/ast.hpp"
#include "optimizer/constant_folder.hpp"
#include <vector>

namespace cplang {

class DeadCodeEliminator {
public:
    DeadCodeEliminator() : removedCount_(0) {}
    
    // 消除死代码
    Shared<Stmt> eliminate(Shared<Stmt> stmt);
    
    // 处理语句块
    std::vector<Shared<Stmt>> eliminateBlock(const std::vector<Shared<Stmt>>& stmts);
    
    // 统计
    int getRemovedCount() const { return removedCount_; }
    
private:
    int removedCount_;
    ConstantFolder folder_;
    
    // 检查条件是否为常量
    ConstValue checkCondition(Shared<Expr> cond);
};

} // namespace cplang
