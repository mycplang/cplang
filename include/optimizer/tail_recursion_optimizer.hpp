// 尾递归优化器
// 将尾递归函数转换为循环，避免栈溢出

#pragma once
#include "ast/ast.hpp"
#include <unordered_map>
#include <unordered_set>

namespace cplang {

// 尾调用信息
struct TailCallInfo {
    bool isTailCall = false;        // 是否是尾调用
    String funcName;                // 被调用的函数名
    std::vector<Shared<Expr>> args; // 参数表达式
    
    // 参数是否是递归的（包含对原函数参数的引用）
    std::vector<bool> argIsRecursive;
};

// 尾递归优化结果
struct TailRecOptResult {
    bool success = false;
    Shared<Stmt> loopStmt;          // 生成的循环语句
    String error;
    
    // 统计
    int eliminatedCalls = 0;
};

// 尾递归优化器
class TailRecursionOptimizer {
public:
    TailRecursionOptimizer() : optCount_(0) {}
    
    // 分析函数是否是尾递归
    TailCallInfo analyzeTailCall(
        Shared<FuncDeclStmt> func,
        Shared<Expr> expr
    );
    
    // 检查整个函数是否是尾递归
    bool isTailRecursive(Shared<FuncDeclStmt> func);
    
    // 优化尾递归函数
    TailRecOptResult optimize(Shared<FuncDeclStmt> func);
    
    // 优化程序中的所有尾递归函数
    Shared<Program> optimizeProgram(Shared<Program> program);
    
    // 统计
    int getOptCount() const { return optCount_; }
    
    // 打印分析结果
    void printAnalysis(Shared<FuncDeclStmt> func) const;
    
private:
    int optCount_;
    
    // 检查表达式是否是函数调用
    Shared<CallExpr> getCallExpr(Shared<Expr> expr);
    
    // 检查表达式是否引用了参数
    bool referencesParam(
        Shared<Expr> expr,
        const String& paramName,
        const std::unordered_set<String>& otherParams
    );
    
    // 构建循环语句
    Shared<Stmt> buildLoop(
        Shared<FuncDeclStmt> func,
        const TailCallInfo& tcInfo
    );
    
    // 收集函数中的所有 return 语句
    std::vector<Shared<ReturnStmt>> collectReturns(Shared<Stmt> stmt) const;
};

} // namespace cplang
