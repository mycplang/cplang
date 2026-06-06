// 尾递归优化器实现

#include "optimizer/tail_recursion_optimizer.hpp"
#include "core/verbose.hpp"
#include <iostream>

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  辅助函数
// ═══════════════════════════════════════════════════════════════════

Shared<CallExpr> TailRecursionOptimizer::getCallExpr(Shared<Expr> expr) {
    if (!expr) return nullptr;
    return std::dynamic_pointer_cast<CallExpr>(expr);
}

bool TailRecursionOptimizer::referencesParam(
    Shared<Expr> expr,
    const String& paramName,
    const std::unordered_set<String>& otherParams
) {
    if (!expr) return false;
    
    // 标识符
    if (auto id = std::dynamic_pointer_cast<IdentifierExpr>(expr)) {
        if (id->name == paramName) return true;
        // 如果引用其他参数，间接引用
        if (otherParams.count(id->name)) return true;
        return false;
    }
    
    // 二元表达式
    if (auto binary = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
        return referencesParam(binary->left, paramName, otherParams) ||
               referencesParam(binary->right, paramName, otherParams);
    }
    
    // 一元表达式
    if (auto unary = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
        return referencesParam(unary->operand, paramName, otherParams);
    }
    
    // 调用表达式
    if (auto call = std::dynamic_pointer_cast<CallExpr>(expr)) {
        if (referencesParam(call->callee, paramName, otherParams)) return true;
        for (const auto& arg : call->arguments) {
            if (referencesParam(arg, paramName, otherParams)) return true;
        }
    }
    
    return false;
}

std::vector<Shared<ReturnStmt>> TailRecursionOptimizer::collectReturns(Shared<Stmt> stmt) const {
    std::vector<Shared<ReturnStmt>> returns;
    
    if (!stmt) return returns;
    
    if (auto ret = std::dynamic_pointer_cast<ReturnStmt>(stmt)) {
        returns.push_back(ret);
    } else if (auto block = std::dynamic_pointer_cast<BlockStmt>(stmt)) {
        for (const auto& s : block->statements) {
            auto subReturns = collectReturns(s);
            returns.insert(returns.end(), subReturns.begin(), subReturns.end());
        }
    } else if (auto ifStmt = std::dynamic_pointer_cast<IfStmt>(stmt)) {
        auto thenReturns = collectReturns(ifStmt->thenBranch);
        auto elseReturns = collectReturns(ifStmt->elseBranch);
        returns.insert(returns.end(), thenReturns.begin(), thenReturns.end());
        returns.insert(returns.end(), elseReturns.begin(), elseReturns.end());
    }
    
    return returns;
}

// ═══════════════════════════════════════════════════════════════════
//  尾调用分析
// ═══════════════════════════════════════════════════════════════════

TailCallInfo TailRecursionOptimizer::analyzeTailCall(
    Shared<FuncDeclStmt> func,
    Shared<Expr> expr
) {
    TailCallInfo info;
    
    if (!expr) return info;
    
    // 检查是否是函数调用
    auto call = getCallExpr(expr);
    if (!call) return info;
    
    // 检查是否是调用自己
    if (auto id = std::dynamic_pointer_cast<IdentifierExpr>(call->callee)) {
        if (id->name == func->name) {
            info.isTailCall = true;
            info.funcName = func->name;
            info.args = call->arguments;
            
            // 分析参数是否递归引用
            std::unordered_set<String> otherParams;
            for (const auto& param : func->params) {
                otherParams.insert(param.first);
            }
            
            for (const auto& arg : call->arguments) {
                bool isRec = false;
                for (const auto& param : func->params) {
                    if (referencesParam(arg, param.first, otherParams)) {
                        isRec = true;
                        break;
                    }
                }
                info.argIsRecursive.push_back(isRec);
            }
        }
    }
    
    return info;
}

// ═══════════════════════════════════════════════════════════════════
//  检查函数是否是尾递归
// ═══════════════════════════════════════════════════════════════════

bool TailRecursionOptimizer::isTailRecursive(Shared<FuncDeclStmt> func) {
    auto returns = collectReturns(func->body);
    
    for (const auto& ret : returns) {
        if (!ret->value) continue;
        
        // 检查 return 的值是否是递归调用
        auto tcInfo = analyzeTailCall(func, ret->value);
        if (tcInfo.isTailCall) {
            return true;
        }
    }
    
    return false;
}

// ═══════════════════════════════════════════════════════════════════
//  构建循环
// ═══════════════════════════════════════════════════════════════════

Shared<Stmt> TailRecursionOptimizer::buildLoop(
    Shared<FuncDeclStmt> func,
    const TailCallInfo& /*tcInfo*/
) {
    // 构建循环体
    // 原始形式：
    //   函数 f(x, y) {
    //       如果 条件 则 返回 base;
    //       返回 f(x', y');
    //   }
    //
    // 转换为：
    //   函数 f(x, y) {
    //       当 真 时 {
    //           如果 条件 则 返回 base;
    //           x = x';
    //           y = y';
    //       }
    //   }
    
    auto loopBody = std::make_shared<BlockStmt>();
    
    // 收集所有 return 语句
    auto returns = collectReturns(func->body);
    
    for (const auto& ret : returns) {
        auto tc = analyzeTailCall(func, ret->value);
        
        if (tc.isTailCall) {
            // 这是尾递归调用，转换为赋值
            for (size_t i = 0; i < tc.args.size() && i < func->params.size(); i++) {
                // 尾递归转循环：参数重新赋值使用二元表达式 (=)
                auto assignStmt = std::make_shared<ExprStmt>();
                auto binExpr = std::make_shared<BinaryExpr>();
                
                auto target = std::make_shared<IdentifierExpr>();
                target->name = func->params[i].first;
                
                binExpr->left = target;
                binExpr->right = tc.args[i];
                binExpr->op = TokenType::OP_ASSIGN;
                assignStmt->expr = binExpr;
                
                loopBody->statements.push_back(assignStmt);
            }
        } else {
            // 这是基本情况，保留 return
            loopBody->statements.push_back(ret);
        }
    }
    
    // 构建无限循环
    auto whileStmt = std::make_shared<WhileStmt>();
    
    auto trueLit = std::make_shared<LiteralExpr>();
    trueLit->value = true;
    
    whileStmt->condition = trueLit;
    whileStmt->body = loopBody;
    
    return whileStmt;
}

// ═══════════════════════════════════════════════════════════════════
//  优化单个函数
// ═══════════════════════════════════════════════════════════════════

TailRecOptResult TailRecursionOptimizer::optimize(Shared<FuncDeclStmt> func) {
    TailRecOptResult result;
    
    if (!func) {
        result.error = "函数为空";
        return result;
    }
    
    // 检查是否是尾递归
    if (!isTailRecursive(func)) {
        result.error = "不是尾递归函数";
        return result;
    }
    
    // 收集 return 语句
    auto returns = collectReturns(func->body);
    
    // 找到尾递归调用
    TailCallInfo tcInfo;
    for (const auto& ret : returns) {
        tcInfo = analyzeTailCall(func, ret->value);
        if (tcInfo.isTailCall) break;
    }
    
    // 构建循环
    result.loopStmt = buildLoop(func, tcInfo);
    result.success = true;
    result.eliminatedCalls = 1;
    optCount_++;
    
    return result;
}

// ═══════════════════════════════════════════════════════════════════
//  优化整个程序
// ═══════════════════════════════════════════════════════════════════

Shared<Program> TailRecursionOptimizer::optimizeProgram(Shared<Program> program) {
    if (!program) return program;
    
    for (auto& stmt : program->statements) {
        if (auto func = std::dynamic_pointer_cast<FuncDeclStmt>(stmt)) {
            if (isTailRecursive(func)) {
                auto result = optimize(func);
                if (result.success) {
                    // 替换函数体为循环
                    func->body = std::dynamic_pointer_cast<BlockStmt>(result.loopStmt);
                }
            }
        }
    }
    
    return program;
}

// ═══════════════════════════════════════════════════════════════════
//  打印分析
// ═══════════════════════════════════════════════════════════════════

void TailRecursionOptimizer::printAnalysis(Shared<FuncDeclStmt> func) const {
    if (!cplang::verboseEnabled()) return;
        std::cout << "=== 尾递归分析 ===" << std::endl;
        std::cout << "函数名: " << func->name << std::endl;

        auto returns = collectReturns(func->body);
        std::cout << "return 语句数: " << returns.size() << std::endl;

        for (const auto& ret : returns) {
            std::cout << "\nreturn 值: ";
            if (auto call = std::dynamic_pointer_cast<CallExpr>(ret->value)) {
                if (auto id = std::dynamic_pointer_cast<IdentifierExpr>(call->callee)) {
                    std::cout << "调用 " << id->name << std::endl;
                    if (id->name == func->name) {
                        std::cout << "  ✓ 尾递归调用" << std::endl;
                    } else {
                        std::cout << "  ✗ 非递归调用" << std::endl;
                    }
                }
            } else {
                std::cout << "非调用表达式（基本情况）" << std::endl;
            }
        }

        std::cout << "\n优化次数: " << optCount_ << std::endl;
}

} // namespace cplang