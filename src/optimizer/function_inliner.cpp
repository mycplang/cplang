// 函数内联优化器实现

#include "optimizer/function_inliner.hpp"
#include "core/verbose.hpp"
#include <iostream>
#include <algorithm>

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  AST 大小计算
// ═══════════════════════════════════════════════════════════════════

int FunctionInliner::calcASTSize(Shared<Stmt> stmt) {
    if (!stmt) return 0;
    
    int size = 1;  // 当前节点
    
    if (auto block = std::dynamic_pointer_cast<BlockStmt>(stmt)) {
        for (const auto& s : block->statements) {
            size += calcASTSize(s);
        }
    } else if (auto var = std::dynamic_pointer_cast<VarDeclStmt>(stmt)) {
        if (var->init) size += calcASTSize(var->init);
    } else if (auto exprStmt = std::dynamic_pointer_cast<ExprStmt>(stmt)) {
        size += calcASTSize(exprStmt->expr);
    } else if (auto ifStmt = std::dynamic_pointer_cast<IfStmt>(stmt)) {
        size += calcASTSize(ifStmt->condition);
        size += calcASTSize(ifStmt->thenBranch);
        size += calcASTSize(ifStmt->elseBranch);
    } else if (auto whileStmt = std::dynamic_pointer_cast<WhileStmt>(stmt)) {
        size += calcASTSize(whileStmt->condition);
        size += calcASTSize(whileStmt->body);
    } else if (auto forStmt = std::dynamic_pointer_cast<ForStmt>(stmt)) {
        size += calcASTSize(forStmt->init);
        size += calcASTSize(forStmt->condition);
        size += calcASTSize(forStmt->update);
        size += calcASTSize(forStmt->body);
    } else if (auto func = std::dynamic_pointer_cast<FuncDeclStmt>(stmt)) {
        size += (int)func->params.size();  // 参数
        size += calcASTSize(func->body);
    } else if (auto ret = std::dynamic_pointer_cast<ReturnStmt>(stmt)) {
        size += calcASTSize(ret->value);
    }
    
    return size;
}

int FunctionInliner::calcASTSize(Shared<Expr> expr) {
    if (!expr) return 0;
    
    int size = 1;
    
    if (auto binary = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
        size += calcASTSize(binary->left);
        size += calcASTSize(binary->right);
    } else if (auto unary = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
        size += calcASTSize(unary->operand);
    } else if (auto call = std::dynamic_pointer_cast<CallExpr>(expr)) {
        size += calcASTSize(call->callee);
        for (const auto& arg : call->arguments) {
            size += calcASTSize(arg);
        }
    } else if (auto member = std::dynamic_pointer_cast<MemberExpr>(expr)) {
        size += calcASTSize(member->object);
    } else if (auto subscript = std::dynamic_pointer_cast<IndexExpr>(expr)) {
        size += calcASTSize(subscript->array);
        size += calcASTSize(subscript->index);
    }
    
    return size;
}

// ═══════════════════════════════════════════════════════════════════
//  函数分析
// ═══════════════════════════════════════════════════════════════════

void FunctionInliner::analyze(Shared<Program> program) {
    functions_.clear();
    
    // 收集所有函数定义
    for (const auto& stmt : program->statements) {
        if (auto func = std::dynamic_pointer_cast<FuncDeclStmt>(stmt)) {
            FuncInfo info;
            info.name = func->name;
            info.decl = func;
            info.astSize = calcASTSize(func->body);
            info.canInline = true;
            
            // 检查是否递归
            // TODO: 实现递归检测
            
            functions_[func->name] = info;
        }
    }
    
    // 统计调用次数
    for (const auto& stmt : program->statements) {
        collectCalls(stmt);
    }
}

void FunctionInliner::collectCalls(Shared<Stmt> stmt) {
    if (!stmt) return;
    
    if (auto block = std::dynamic_pointer_cast<BlockStmt>(stmt)) {
        for (const auto& s : block->statements) {
            collectCalls(s);
        }
    } else if (auto var = std::dynamic_pointer_cast<VarDeclStmt>(stmt)) {
        collectCalls(var->init);
    } else if (auto exprStmt = std::dynamic_pointer_cast<ExprStmt>(stmt)) {
        collectCalls(exprStmt->expr);
    } else if (auto ifStmt = std::dynamic_pointer_cast<IfStmt>(stmt)) {
        collectCalls(ifStmt->condition);
        collectCalls(ifStmt->thenBranch);
        collectCalls(ifStmt->elseBranch);
    } else if (auto whileStmt = std::dynamic_pointer_cast<WhileStmt>(stmt)) {
        collectCalls(whileStmt->condition);
        collectCalls(whileStmt->body);
    } else if (auto func = std::dynamic_pointer_cast<FuncDeclStmt>(stmt)) {
        collectCalls(func->body);
    } else if (auto ret = std::dynamic_pointer_cast<ReturnStmt>(stmt)) {
        collectCalls(ret->value);
    }
}

void FunctionInliner::collectCalls(Shared<Expr> expr) {
    if (!expr) return;
    
    if (auto call = std::dynamic_pointer_cast<CallExpr>(expr)) {
        if (auto id = std::dynamic_pointer_cast<IdentifierExpr>(call->callee)) {
            auto it = functions_.find(id->name);
            if (it != functions_.end()) {
                it->second.callCount++;
            }
        }
        for (const auto& arg : call->arguments) {
            collectCalls(arg);
        }
    } else if (auto binary = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
        collectCalls(binary->left);
        collectCalls(binary->right);
    } else if (auto unary = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
        collectCalls(unary->operand);
    }
}

// ═══════════════════════════════════════════════════════════════════
//  内联决策
// ═══════════════════════════════════════════════════════════════════

bool FunctionInliner::shouldInline(const FuncInfo& func) {
    if (!func.canInline) return false;
    if (func.isRecursive && !config_.inlineRecursive) return false;
    if (currentDepth_ >= config_.maxCallDepth) return false;
    if (inlined_.count(func.name)) return false;  // 避免重复内联
    
    // 检查大小
    if (config_.forceInline) return true;
    if (func.astSize > config_.maxASTSize) return false;
    
    return true;
}

// ═══════════════════════════════════════════════════════════════════
//  表达式克隆与替换
// ═══════════════════════════════════════════════════════════════════

Shared<Expr> FunctionInliner::cloneAndSubstitute(
    Shared<Expr> expr,
    const std::unordered_map<String, Shared<Expr>>& subst
) {
    if (!expr) return nullptr;
    
    // 标识符 -> 替换为实参
    if (auto id = std::dynamic_pointer_cast<IdentifierExpr>(expr)) {
        auto it = subst.find(id->name);
        if (it != subst.end()) {
            return it->second;  // 替换
        }
        return expr;  // 不替换
    }
    
    // 字面量 -> 直接返回
    if (std::dynamic_pointer_cast<LiteralExpr>(expr)) {
        return expr;
    }
    
    // 二元表达式
    if (auto binary = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
        auto result = std::make_shared<BinaryExpr>();
        result->op = binary->op;
        result->left = cloneAndSubstitute(binary->left, subst);
        result->right = cloneAndSubstitute(binary->right, subst);
        return result;
    }
    
    // 一元表达式
    if (auto unary = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
        auto result = std::make_shared<UnaryExpr>();
        result->op = unary->op;
        result->operand = cloneAndSubstitute(unary->operand, subst);
        return result;
    }
    
    // 调用表达式
    if (auto call = std::dynamic_pointer_cast<CallExpr>(expr)) {
        auto result = std::make_shared<CallExpr>();
        result->callee = cloneAndSubstitute(call->callee, subst);
        for (const auto& arg : call->arguments) {
            result->arguments.push_back(cloneAndSubstitute(arg, subst));
        }
        return result;
    }
    
    return expr;
}

// ═══════════════════════════════════════════════════════════════════
//  内联调用
// ═══════════════════════════════════════════════════════════════════

Shared<Expr> FunctionInliner::inlineCall(Shared<CallExpr> call, const FuncInfo& func) {
    if (!shouldInline(func)) {
        return call;  // 不内联
    }
    
    currentDepth_++;
    inlineCount_++;
    inlined_.insert(func.name);
    
    // 构建参数替换表
    std::unordered_map<String, Shared<Expr>> subst;
    for (size_t i = 0; i < func.decl->params.size() && i < call->arguments.size(); i++) {
        subst[func.decl->params[i].first] = call->arguments[i];
    }
    
    // 查找函数体的 return 语句
    Shared<Expr> result = nullptr;
    
    if (auto block = std::dynamic_pointer_cast<BlockStmt>(func.decl->body)) {
        for (const auto& stmt : block->statements) {
            if (auto ret = std::dynamic_pointer_cast<ReturnStmt>(stmt)) {
                // 克隆并替换 return 表达式
                result = cloneAndSubstitute(ret->value, subst);
                break;
            }
        }
    }
    
    currentDepth_--;
    
    return result ? result : call;
}

// ═══════════════════════════════════════════════════════════════════
//  内联整个程序
// ═══════════════════════════════════════════════════════════════════

Shared<Program> FunctionInliner::inlineFunctions(Shared<Program> program) {
    if (!program) return program;
    
    analyze(program);
    
    // TODO: 实现完整的内联遍历
    
    return program;
}

// ═══════════════════════════════════════════════════════════════════
//  打印分析结果
// ═══════════════════════════════════════════════════════════════════

void FunctionInliner::printAnalysis() const {
    if (!cplang::verboseEnabled()) return;
        std::cout << "=== 函数内联分析 ===" << std::endl;

        std::vector<FuncInfo> sortedFuncs;
        for (const auto& [name, info] : functions_) {
            sortedFuncs.push_back(info);
        }

        // 按收益排序
        std::sort(sortedFuncs.begin(), sortedFuncs.end(),
            [](const FuncInfo& a, const FuncInfo& b) {
                return a.estimateBenefit() > b.estimateBenefit();
            });

        std::cout << "\n函数名           大小    调用次数  收益    内联?\n";
        std::cout << "─────────────────────────────────────────────────\n";

        for (const auto& func : sortedFuncs) {
            bool willInline = func.astSize <= config_.maxASTSize && !func.isRecursive;
            std::cout << func.name
                      << "\t\t" << func.astSize
                      << "\t" << func.callCount
                      << "\t  " << func.estimateBenefit()
                      << "\t" << (willInline ? "✓" : "✗")
                      << std::endl;
        }

        std::cout << "\n内联次数: " << inlineCount_ << std::endl;
}

} // namespace cplang