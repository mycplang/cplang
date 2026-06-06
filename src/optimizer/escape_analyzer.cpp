// 逃逸分析器实现

#include "optimizer/escape_analyzer.hpp"
#include "core/verbose.hpp"
#include <iostream>
#include <algorithm>

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  构造与初始化
// ═══════════════════════════════════════════════════════════════════

EscapeAnalyzer::EscapeAnalyzer() {}

// ═══════════════════════════════════════════════════════════════════
//  公共接口
// ═══════════════════════════════════════════════════════════════════

ProgramEscapeResult EscapeAnalyzer::analyze(Shared<Program> program) {
    ProgramEscapeResult result;
    result.totalStackSaved = 0;
    result.totalHeapAlloc = 0;
    result.totalStackAlloc = 0;
    
    // 分析每个函数
    for (const auto& stmt : program->statements) {
        if (auto func = std::dynamic_pointer_cast<FuncDeclStmt>(stmt)) {
            auto funcResult = analyzeFunction(func);
            result.functionResults.push_back(funcResult);
            result.totalStackSaved += funcResult.stackSaved;
        }
    }
    
    return result;
}

FuncEscapeResult EscapeAnalyzer::analyzeFunction(Shared<FuncDeclStmt> func) {
    FuncEscapeResult result;
    result.canStackAlloc = true;
    result.stackSaved = 0;
    
    // 初始化
    currentFunc_ = func;
    currentVarEscape_.clear();
    addressTaken_.clear();
    
    // 为每个参数创建逃逸信息
    for (const auto& param : func->params) {
        VarEscape ve;
        ve.varName = param.first;
        ve.level = EscapeLevel::None;
        currentVarEscape_[param.first] = ve;
    }
    
    // 分析函数体
    analyzeStmt(func->body);
    
    // 统计可以栈上分配的数量
    for (const auto& pair : currentVarEscape_) {
        if (pair.second.level == EscapeLevel::None) {
            // 不逃逸，可以在栈上分配
            result.canStackAlloc = true;
            result.stackSaved++;
        } else {
            result.canStackAlloc = false;
        }
        result.varEscape[pair.first] = pair.second;
    }
    
    return result;
}

// ═══════════════════════════════════════════════════════════════════
//  语句分析
// ═══════════════════════════════════════════════════════════════════

void EscapeAnalyzer::analyzeStmt(Shared<Stmt> stmt) {
    if (!stmt) return;
    
    // 块语句
    if (auto block = std::dynamic_pointer_cast<BlockStmt>(stmt)) {
        for (const auto& s : block->statements) {
            analyzeStmt(s);
        }
        return;
    }
    
    // 表达式语句
    if (auto expr = std::dynamic_pointer_cast<ExprStmt>(stmt)) {
        // 检查函数调用
        if (auto call = std::dynamic_pointer_cast<CallExpr>(expr->expr)) {
            analyzeCall(call);
        }
        return;
    }
    
    // If 语句
    if (auto ifStmt = std::dynamic_pointer_cast<IfStmt>(stmt)) {
        analyzeIf(ifStmt);
        return;
    }
    
    // While 语句
    if (auto whileStmt = std::dynamic_pointer_cast<WhileStmt>(stmt)) {
        analyzeLoop(whileStmt->body);
        return;
    }
    
    // For 语句
    if (auto forStmt = std::dynamic_pointer_cast<ForStmt>(stmt)) {
        analyzeStmt(forStmt->init);
        analyzeLoop(forStmt->body);
        return;
    }
    
    // ForEach 语句
    if (auto feStmt = std::dynamic_pointer_cast<ForEachStmt>(stmt)) {
        analyzeForEach(feStmt);
        return;
    }
    
    // Return 语句
    if (auto ret = std::dynamic_pointer_cast<ReturnStmt>(stmt)) {
        analyzeReturn(ret);
        return;
    }
}

void EscapeAnalyzer::analyzeCall(Shared<CallExpr> call) {
    if (!call) return;
    
    // 分析参数
    for (const auto& arg : call->arguments) {
        EscapeLevel esc = analyzeExpr(arg);
        
        // 如果参数逃逸，标记为传递给函数
        if (esc > EscapeLevel::None) {
            auto refs = getVarRefs(arg);
            for (const auto& ref : refs) {
                if (currentVarEscape_.count(ref)) {
                    currentVarEscape_[ref].isPassedToFunc = true;
                    currentVarEscape_[ref].escapeSites.insert("函数参数");
                }
            }
        }
    }
}

void EscapeAnalyzer::analyzeReturn(Shared<ReturnStmt> ret) {
    if (!ret || !ret->value) return;
    
    [[maybe_unused]] EscapeLevel esc = analyzeExpr(ret->value);
    
    // 标记返回值引用的变量为逃逸
    auto refs = getVarRefs(ret->value);
    for (const auto& ref : refs) {
        if (currentVarEscape_.count(ref)) {
            markEscaped(ref, EscapeLevel::Return, "作为返回值");
        }
    }
}

void EscapeAnalyzer::analyzeIf(Shared<IfStmt> ifStmt) {
    if (!ifStmt) return;
    
    analyzeStmt(ifStmt->thenBranch);
    if (ifStmt->elseBranch) {
        analyzeStmt(ifStmt->elseBranch);
    }
}

void EscapeAnalyzer::analyzeLoop(Shared<Stmt> loop) {
    if (!loop) return;
    
    // 分析循环体
    analyzeStmt(loop);
    
    // 循环中的变量可能逃逸（循环闭包）
    // 这里简化为：如果变量在循环中被引用，可能逃逸
    // 实际实现需要更复杂的分析
}

void EscapeAnalyzer::analyzeForEach(Shared<ForEachStmt> feStmt) {
    if (!feStmt) return;
    
    // 创建循环变量
    VarEscape ve;
    ve.varName = feStmt->varName;
    ve.level = EscapeLevel::None;
    currentVarEscape_[feStmt->varName] = ve;
    
    // 分析被遍历的表达式
    EscapeLevel esc = analyzeExpr(feStmt->iterable);
    if (esc > EscapeLevel::None) {
        markEscaped(feStmt->varName, esc, "foreach遍历逃逸");
    }
    
    // 分析循环体
    analyzeStmt(feStmt->body);
}

// ═══════════════════════════════════════════════════════════════════
//  表达式分析
// ═══════════════════════════════════════════════════════════════════

EscapeLevel EscapeAnalyzer::analyzeExpr(Shared<Expr> expr) {
    if (!expr) return EscapeLevel::None;
    
    // 字面量不逃逸
    if (std::dynamic_pointer_cast<LiteralExpr>(expr)) {
        return EscapeLevel::None;
    }
    
    // 标识符
    if (auto id = std::dynamic_pointer_cast<IdentifierExpr>(expr)) {
        if (currentVarEscape_.count(id->name)) {
            return currentVarEscape_[id->name].level;
        }
        return EscapeLevel::None;
    }
    
    // 二元表达式
    if (auto binary = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
        EscapeLevel left = analyzeExpr(binary->left);
        EscapeLevel right = analyzeExpr(binary->right);
        return static_cast<EscapeLevel>(std::max(static_cast<int>(left), static_cast<int>(right)));
    }
    
    // 一元表达式
    if (auto unary = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
        return analyzeExpr(unary->operand);
    }
    
    // 函数调用
    if (auto call = std::dynamic_pointer_cast<CallExpr>(expr)) {
        // 函数调用的结果可能逃逸
        analyzeCall(call);
        return EscapeLevel::Return;
    }
    
    // 数组/表字面量
    if (std::dynamic_pointer_cast<ArrayExpr>(expr) ||
        std::dynamic_pointer_cast<StructLiteralExpr>(expr)) {
        // 字面量通常在栈上，除非逃逸
        return EscapeLevel::None;
    }
    
    // 成员访问
    if (auto member = std::dynamic_pointer_cast<MemberExpr>(expr)) {
        return analyzeExpr(member->object);
    }
    
    // 索引访问
    if (auto index = std::dynamic_pointer_cast<IndexExpr>(expr)) {
        EscapeLevel obj = analyzeExpr(index->array);
        EscapeLevel idx = analyzeExpr(index->index);
        return static_cast<EscapeLevel>(std::max(static_cast<int>(obj), static_cast<int>(idx)));
    }
    
    return EscapeLevel::None;
}

// ═══════════════════════════════════════════════════════════════════
//  变量引用提取
// ═══════════════════════════════════════════════════════════════════

std::unordered_set<std::string> EscapeAnalyzer::getVarRefs(Shared<Expr> expr) {
    std::unordered_set<std::string> refs;
    
    if (!expr) return refs;
    
    // 标识符
    if (auto id = std::dynamic_pointer_cast<IdentifierExpr>(expr)) {
        refs.insert(id->name);
        return refs;
    }
    
    // 二元表达式
    if (auto binary = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
        auto leftRefs = getVarRefs(binary->left);
        auto rightRefs = getVarRefs(binary->right);
        refs.insert(leftRefs.begin(), leftRefs.end());
        refs.insert(rightRefs.begin(), rightRefs.end());
        return refs;
    }
    
    // 一元表达式
    if (auto unary = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
        return getVarRefs(unary->operand);
    }
    
    // 数组/表访问
    if (auto index = std::dynamic_pointer_cast<IndexExpr>(expr)) {
        auto objRefs = getVarRefs(index->array);
        auto idxRefs = getVarRefs(index->index);
        refs.insert(objRefs.begin(), objRefs.end());
        refs.insert(idxRefs.begin(), idxRefs.end());
        return refs;
    }
    
    // 成员访问
    if (auto member = std::dynamic_pointer_cast<MemberExpr>(expr)) {
        return getVarRefs(member->object);
    }
    
    // 函数调用参数
    if (auto call = std::dynamic_pointer_cast<CallExpr>(expr)) {
        for (const auto& arg : call->arguments) {
            auto argRefs = getVarRefs(arg);
            refs.insert(argRefs.begin(), argRefs.end());
        }
        return refs;
    }
    
    return refs;
}

// ═══════════════════════════════════════════════════════════════════
//  逃逸标记与传播
// ═══════════════════════════════════════════════════════════════════

void EscapeAnalyzer::markEscaped(const std::string& varName, 
                                   EscapeLevel level, 
                                   const std::string& reason) {
    if (!currentVarEscape_.count(varName)) {
        VarEscape ve;
        ve.varName = varName;
        currentVarEscape_[varName] = ve;
    }
    
    auto& ve = currentVarEscape_[varName];
    
    // 更新逃逸级别
    if (static_cast<int>(level) > static_cast<int>(ve.level)) {
        ve.level = level;
    }
    
    // 记录逃逸原因
    if (!reason.empty()) {
        ve.escapeSites.insert(reason);
    }
}

void EscapeAnalyzer::propagateEscape(const std::string& from, const std::string& to) {
    if (!currentVarEscape_.count(from)) return;
    if (!currentVarEscape_.count(to)) return;
    
    auto& fromVar = currentVarEscape_[from];
    auto& toVar = currentVarEscape_[to];
    
    // 如果 from 逃逸，to 也逃逸
    if (fromVar.level > EscapeLevel::None) {
        if (static_cast<int>(fromVar.level) > static_cast<int>(toVar.level)) {
            toVar.level = fromVar.level;
        }
        toVar.escapeSites.insert("传播自 " + from);
    }
}

bool EscapeAnalyzer::isAddressExpr(Shared<Expr> expr) {
    // 某些操作隐含取地址
    if (!expr) return false;
    
    // 数组/表字面量
    if (std::dynamic_pointer_cast<ArrayExpr>(expr) ||
        std::dynamic_pointer_cast<StructLiteralExpr>(expr)) {
        return true;
    }
    
    return false;
}

// ═══════════════════════════════════════════════════════════════════
//  查询接口
// ═══════════════════════════════════════════════════════════════════

bool EscapeAnalyzer::isEscaped(const std::string& varName) {
    if (!currentVarEscape_.count(varName)) return false;
    return currentVarEscape_[varName].level > EscapeLevel::None;
}

bool EscapeAnalyzer::canStackAlloc(const std::string& varName) {
    if (!currentVarEscape_.count(varName)) return true;
    return currentVarEscape_[varName].level == EscapeLevel::None;
}

const VarEscape* EscapeAnalyzer::getEscapeInfo(const std::string& /*funcName*/, 
                                                const std::string& varName) {
    // 简化版本，只返回当前函数的逃逸信息
    if (currentVarEscape_.count(varName)) {
        return &currentVarEscape_[varName];
    }
    return nullptr;
}

// ═══════════════════════════════════════════════════════════════════
//  打印结果
// ═══════════════════════════════════════════════════════════════════

void EscapeAnalyzer::printResult(const ProgramEscapeResult& result) const {
    if (!cplang::verboseEnabled()) return;
        std::cout << "\n╔══════════════════════════════════════════════════════╗\n";
        std::cout << "║        逃逸分析结果                                 ║\n";
        std::cout << "╚══════════════════════════════════════════════════════╝\n\n";

        std::cout << "总统计:\n";
        std::cout << "  节省的堆分配: " << result.totalStackSaved << " 个\n";
        std::cout << "  堆分配数: " << result.totalHeapAlloc << " 个\n";
        std::cout << "  栈分配数: " << result.totalStackAlloc << " 个\n\n";

        std::cout << "各函数分析:\n";
        std::cout << "──────────────────────────────────────────────────\n";

        for (const auto& funcResult : result.functionResults) {
            std::cout << "\n函数:\n";

            for (const auto& pair : funcResult.varEscape) {
                const std::string& name = pair.first;
                const VarEscape& ve = pair.second;

                std::cout << "  " << name << ":\n";
                std::cout << "    逃逸级别: ";
                switch (ve.level) {
                    case EscapeLevel::None: std::cout << "无 (栈上分配)"; break;
                    case EscapeLevel::Arg: std::cout << "参数逃逸"; break;
                    case EscapeLevel::Return: std::cout << "返回值"; break;
                    case EscapeLevel::Global: std::cout << "全局"; break;
                    case EscapeLevel::Heap: std::cout << "堆"; break;
                }
                std::cout << "\n";

                if (!ve.escapeSites.empty()) {
                    std::cout << "    逃逸位置:\n";
                    for (const auto& site : ve.escapeSites) {
                        std::cout << "      - " << site << "\n";
                    }
                }
            }

            std::cout << "    可栈上分配: " << (funcResult.canStackAlloc ? "是" : "否") << "\n";
            std::cout << "    节省分配: " << funcResult.stackSaved << "\n";
        }
}

} // namespace cplang