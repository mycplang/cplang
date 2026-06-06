// 循环展开优化器实现

#include "optimizer/loop_unroller.hpp"
#include "core/verbose.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <initializer_list>

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  循环分析
// ═══════════════════════════════════════════════════════════════════

LoopInfo LoopUnroller::analyzeLoop(Shared<Stmt> stmt) const {
    LoopInfo info;
    info.isCountable = false;
    info.tripCount = -1;
    
    if (auto forStmt = std::dynamic_pointer_cast<ForStmt>(stmt)) {
        // for (var = start; var < end; var += step) { body }
        // init 是 VarDeclStmt，从中提取循环变量名和起始值
        if (auto varDecl = std::dynamic_pointer_cast<VarDeclStmt>(forStmt->init)) {
            info.loopVar = varDecl->name;
            info.start = varDecl->init;
        }
        info.end = forStmt->condition;
        info.step = forStmt->update;
        info.body = forStmt->body;
        
        // 尝试计算循环次数
        if (auto startLit = std::dynamic_pointer_cast<LiteralExpr>(info.start)) {
            if (auto endLit = std::dynamic_pointer_cast<LiteralExpr>(info.end)) {
                try {
                    int startVal = static_cast<int>(std::get<Int64>(startLit->value));
                    int endVal = static_cast<int>(std::get<Int64>(endLit->value));
                    
                    // 假设步长为1
                    if (endVal > startVal) {
                        info.tripCount = endVal - startVal;
                        info.isCountable = true;
                    }
                } catch (...) {
                    // 无法解析
                }
            }
        }
    } else if (auto whileStmt = std::dynamic_pointer_cast<WhileStmt>(stmt)) {
        info.body = whileStmt->body;
        // while 循环难以静态分析
        info.isCountable = false;
    }
    
    return info;
}

// ═══════════════════════════════════════════════════════════════════
//  大小估算
// ═══════════════════════════════════════════════════════════════════

int LoopUnroller::estimateSize(Shared<Stmt> stmt) const {
    if (!stmt) return 0;
    
    if (auto block = std::dynamic_pointer_cast<BlockStmt>(stmt)) {
        int size = 0;
        for (const auto& s : block->statements) {
            size += estimateSize(s);
        }
        return size;
    }
    
    if (auto expr = std::dynamic_pointer_cast<ExprStmt>(stmt)) {
        return estimateExprSize(expr->expr);
    }
    
    if (auto ifStmt = std::dynamic_pointer_cast<IfStmt>(stmt)) {
        return 1 + estimateExprSize(ifStmt->condition) +
               estimateSize(ifStmt->thenBranch) +
               estimateSize(ifStmt->elseBranch);
    }
    
    if (auto forStmt = std::dynamic_pointer_cast<ForStmt>(stmt)) {
        int initSize = 0;
        if (auto varDecl = std::dynamic_pointer_cast<VarDeclStmt>(forStmt->init)) {
            initSize = estimateExprSize(varDecl->init);
        } else if (auto exprStmt = std::dynamic_pointer_cast<ExprStmt>(forStmt->init)) {
            initSize = estimateExprSize(exprStmt->expr);
        }
        return 2 + initSize +
               estimateExprSize(forStmt->condition) +
               estimateExprSize(forStmt->update) +
               estimateSize(forStmt->body);
    }
    
    if (auto whileStmt = std::dynamic_pointer_cast<WhileStmt>(stmt)) {
        return 1 + estimateExprSize(whileStmt->condition) +
               estimateSize(whileStmt->body);
    }
    
    return 1;
}

int LoopUnroller::estimateExprSize(Shared<Expr> expr) const {
    if (!expr) return 0;
    
    if (auto binary = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
        return 1 + estimateExprSize(binary->left) + estimateExprSize(binary->right);
    }
    
    if (auto unary = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
        return 1 + estimateExprSize(unary->operand);
    }
    
    if (auto call = std::dynamic_pointer_cast<CallExpr>(expr)) {
        int size = 1;
        for (const auto& arg : call->arguments) {
            size += estimateExprSize(arg);
        }
        return size;
    }
    
    return 1;
}

// ═══════════════════════════════════════════════════════════════════
//  展开决策
// ═══════════════════════════════════════════════════════════════════

bool LoopUnroller::shouldUnroll(const LoopInfo& info) const {
    // 不展开不可计数循环
    if (!info.isCountable) return false;
    
    // 不展开循环次数未知的循环
    if (info.tripCount <= 0) return false;
    
    // 不展开太多次循环（代码膨胀）
    if (info.tripCount > maxUnrollFactor_ * 4) return false;
    
    // 不展开太大的循环体
    int bodySize = estimateSize(info.body);
    if (bodySize > maxBodySize_) return false;
    
    return true;
}

int LoopUnroller::computeUnrollFactor(const LoopInfo& info) const {
    if (!shouldUnroll(info)) return 1;
    
    // 计算最优展开因子
    // 目标：展开后代码大小适中，且循环次数减少
    
    int bodySize = estimateSize(info.body);
    int tripCount = info.tripCount;
    
    // 基于循环体大小计算
    int sizeBasedFactor = maxBodySize_ / std::max(bodySize, 1);
    
    // 基于循环次数计算
    int tripBasedFactor = tripCount;
    
    // 取最小值，但不超过最大因子
    int factor = std::min({sizeBasedFactor, tripBasedFactor, maxUnrollFactor_}, std::less<int>{});
    
    // 确保因子是循环次数的约数（简化处理）
    if (tripCount % factor != 0 && factor < tripCount) {
        // 找到最接近的约数
        for (int f = factor; f >= 1; f--) {
            if (tripCount % f == 0) {
                factor = f;
                break;
            }
        }
    }
    
    return std::max(factor, 1);
}

// ═══════════════════════════════════════════════════════════════════
//  语句克隆
// ═══════════════════════════════════════════════════════════════════

Shared<Expr> LoopUnroller::cloneExpr(Shared<Expr> expr, const String& var, int offset) {
    if (!expr) return nullptr;
    
    // 标识符 - 如果是循环变量，替换为 var + offset
    if (auto id = std::dynamic_pointer_cast<IdentifierExpr>(expr)) {
        if (id->name == var && offset != 0) {
            // 创建 var + offset
            auto newExpr = std::make_shared<BinaryExpr>();
            newExpr->op = TokenType::OP_PLUS;
            
            auto varExpr = std::make_shared<IdentifierExpr>();
            varExpr->name = var;
            newExpr->left = varExpr;
            
            auto offsetLit = std::make_shared<LiteralExpr>();
            offsetLit->value = (Int64)offset;
            newExpr->right = offsetLit;
            
            return newExpr;
        }
        
        // 否则直接复制
        auto newId = std::make_shared<IdentifierExpr>();
        newId->name = id->name;
        return newId;
    }
    
    // 字面量
    if (auto lit = std::dynamic_pointer_cast<LiteralExpr>(expr)) {
        auto newLit = std::make_shared<LiteralExpr>();
        newLit->value = lit->value;
        return newLit;
    }
    
    // 二元表达式
    if (auto binary = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
        auto newBinary = std::make_shared<BinaryExpr>();
        newBinary->op = binary->op;
        newBinary->left = cloneExpr(binary->left, var, offset);
        newBinary->right = cloneExpr(binary->right, var, offset);
        return newBinary;
    }
    
    // 一元表达式
    if (auto unary = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
        auto newUnary = std::make_shared<UnaryExpr>();
        newUnary->op = unary->op;
        newUnary->operand = cloneExpr(unary->operand, var, offset);
        return newUnary;
    }
    
    // 调用表达式
    if (auto call = std::dynamic_pointer_cast<CallExpr>(expr)) {
        auto newCall = std::make_shared<CallExpr>();
        newCall->callee = cloneExpr(call->callee, var, offset);
        for (const auto& arg : call->arguments) {
            newCall->arguments.push_back(cloneExpr(arg, var, offset));
        }
        return newCall;
    }
    
    return expr;
}

Shared<Stmt> LoopUnroller::cloneStmt(Shared<Stmt> stmt, const String& var, int offset) {
    if (!stmt) return nullptr;
    
    // 表达式语句
    if (auto expr = std::dynamic_pointer_cast<ExprStmt>(stmt)) {
        auto newStmt = std::make_shared<ExprStmt>();
        newStmt->expr = cloneExpr(expr->expr, var, offset);
        return newStmt;
    }
    
    // 块语句
    if (auto block = std::dynamic_pointer_cast<BlockStmt>(stmt)) {
        auto newBlock = std::make_shared<BlockStmt>();
        for (const auto& s : block->statements) {
            newBlock->statements.push_back(cloneStmt(s, var, offset));
        }
        return newBlock;
    }
    
    // If 语句
    if (auto ifStmt = std::dynamic_pointer_cast<IfStmt>(stmt)) {
        auto newIf = std::make_shared<IfStmt>();
        newIf->condition = cloneExpr(ifStmt->condition, var, offset);
        newIf->thenBranch = cloneStmt(ifStmt->thenBranch, var, offset);
        newIf->elseBranch = cloneStmt(ifStmt->elseBranch, var, offset);
        return newIf;
    }
    
    // Return 语句
    if (auto ret = std::dynamic_pointer_cast<ReturnStmt>(stmt)) {
        auto newRet = std::make_shared<ReturnStmt>();
        newRet->value = cloneExpr(ret->value, var, offset);
        return newRet;
    }
    
    return stmt;
}

// ═══════════════════════════════════════════════════════════════════
//  循环展开
// ═══════════════════════════════════════════════════════════════════

Shared<BlockStmt> LoopUnroller::buildUnrolledBody(
    Shared<Stmt> body,
    const String& loopVar,
    int factor
) {
    auto result = std::make_shared<BlockStmt>();
    
    // 展开为 factor 个连续的循环体
    for (int i = 0; i < factor; i++) {
        auto clonedBody = cloneStmt(body, loopVar, i);
        result->statements.push_back(clonedBody);
    }
    
    return result;
}

LoopUnrollResult LoopUnroller::unrollFor(Shared<ForStmt> forStmt, int factor) {
    LoopUnrollResult result;
    result.unrollFactor = factor;
    
    if (!forStmt || factor <= 1) {
        result.success = false;
        result.error = "无效参数";
        return result;
    }
    
    auto info = analyzeLoop(forStmt);
    
    if (!shouldUnroll(info)) {
        result.success = false;
        result.error = "不适合展开";
        return result;
    }
    
    // 构建展开后的循环体（for 循环没有 varName，需要简化处理）
    auto unrolledBody = forStmt->body;
    
    // 创建新的 for 循环（减少迭代次数）
    auto newFor = std::make_shared<ForStmt>();
    newFor->init = forStmt->init;
    
    // 调整结束条件
    if (auto endLit = std::dynamic_pointer_cast<LiteralExpr>(forStmt->condition)) {
        try {
            int endVal = static_cast<int>(std::get<Int64>(endLit->value));
            int newEnd = endVal / factor;
            
            auto newEndLit = std::make_shared<LiteralExpr>();
            newEndLit->value = (Int64)newEnd;
            newFor->condition = newEndLit;
        } catch (...) {
            newFor->condition = forStmt->condition;
        }
    } else {
        newFor->condition = forStmt->condition;
    }
    
    // 调整步长
    if (auto stepLit = std::dynamic_pointer_cast<LiteralExpr>(forStmt->update)) {
        try {
            int stepVal = static_cast<int>(std::get<Int64>(stepLit->value));
            int newStep = stepVal * factor;
            
            auto newStepLit = std::make_shared<LiteralExpr>();
            newStepLit->value = (Int64)newStep;
            newFor->update = newStepLit;
        } catch (...) {
            newFor->update = forStmt->update;
        }
    } else {
        newFor->update = forStmt->update;
    }
    
    newFor->body = unrolledBody;
    
    result.unrolledLoop = newFor;
    result.iterationsUnrolled = factor;
    result.success = true;
    unrollCount_++;
    
    return result;
}

LoopUnrollResult LoopUnroller::unrollWhile(Shared<WhileStmt> /*whileStmt*/, int /*factor*/) {
    LoopUnrollResult result;
    result.success = false;
    result.error = "while 循环暂不支持展开";
    return result;
}

// ═══════════════════════════════════════════════════════════════════
//  公共接口
// ═══════════════════════════════════════════════════════════════════

Shared<Stmt> LoopUnroller::unroll(Shared<Stmt> stmt) {
    if (!stmt) return stmt;
    
    if (auto forStmt = std::dynamic_pointer_cast<ForStmt>(stmt)) {
        auto info = analyzeLoop(forStmt);
        int factor = computeUnrollFactor(info);
        
        if (factor > 1) {
            auto result = unrollFor(forStmt, factor);
            if (result.success) {
                return result.unrolledLoop;
            }
        }
    }
    
    // 递归处理块语句
    if (auto block = std::dynamic_pointer_cast<BlockStmt>(stmt)) {
        auto newBlock = std::make_shared<BlockStmt>();
        for (const auto& s : block->statements) {
            newBlock->statements.push_back(unroll(s));
        }
        return newBlock;
    }
    
    // 递归处理 if 语句
    if (auto ifStmt = std::dynamic_pointer_cast<IfStmt>(stmt)) {
        auto newIf = std::make_shared<IfStmt>();
        newIf->condition = ifStmt->condition;
        newIf->thenBranch = unroll(ifStmt->thenBranch);
        newIf->elseBranch = unroll(ifStmt->elseBranch);
        return newIf;
    }
    
    return stmt;
}

Shared<Program> LoopUnroller::unrollProgram(Shared<Program> program) {
    if (!program) return program;
    
    for (auto& stmt : program->statements) {
        // 处理函数体内的循环
        if (auto func = std::dynamic_pointer_cast<FuncDeclStmt>(stmt)) {
            func->body = std::dynamic_pointer_cast<BlockStmt>(unroll(func->body));
        }
    }
    
    return program;
}

// ═══════════════════════════════════════════════════════════════════
//  分析打印
// ═══════════════════════════════════════════════════════════════════

void LoopUnroller::printAnalysis(Shared<Stmt> loop) const {
    if (!cplang::verboseEnabled()) return;
        std::cout << "=== 循环展开分析 ===" << std::endl;

        auto info = analyzeLoop(loop);

        std::cout << "循环变量: " << info.loopVar << std::endl;
        std::cout << "是否可计数: " << (info.isCountable ? "是" : "否") << std::endl;

        if (info.isCountable) {
            std::cout << "循环次数: " << info.tripCount << std::endl;
        }

        int bodySize = estimateSize(info.body);
        std::cout << "循环体大小: " << bodySize << std::endl;

        bool should = shouldUnroll(info);
        std::cout << "是否应该展开: " << (should ? "是" : "否") << std::endl;

        if (should) {
            int factor = computeUnrollFactor(info);
            std::cout << "建议展开因子: " << factor << std::endl;
        }

        std::cout << "已展开次数: " << unrollCount_ << std::endl;
}

} // namespace cplang