// 函数内联优化器
// 将小函数直接展开到调用点

#pragma once
#include "ast/ast.hpp"
#include <unordered_map>
#include <unordered_set>

namespace cplang {

// 函数信息（用于内联决策）
struct FuncInfo {
    String name;
    Shared<FuncDeclStmt> decl;
    int astSize = 0;        // AST 节点数
    int callCount = 0;      // 调用次数
    bool isRecursive = false;
    bool canInline = true;
    
    // 内联收益估算
    int estimateBenefit() const {
        if (!canInline || isRecursive) return 0;
        // 调用次数越多，函数越小，收益越大
        return callCount * (100 - astSize);
    }
};

// 内联配置
struct InlineConfig {
    int maxASTSize = 50;         // 最大 AST 节点数
    int maxCallDepth = 3;        // 最大内联深度
    bool inlineRecursive = false; // 是否内联递归函数
    bool forceInline = false;     // 强制内联（忽略大小限制）
};

// 函数内联器
class FunctionInliner {
public:
    FunctionInliner(const InlineConfig& config = InlineConfig())
        : config_(config), inlineCount_(0) {}
    
    // 分析程序中的函数
    void analyze(Shared<Program> program);
    
    // 内联优化
    Shared<Program> inlineFunctions(Shared<Program> program);
    
    // 内联单个调用
    Shared<Expr> inlineCall(Shared<CallExpr> call, const FuncInfo& func);
    
    // 统计
    int getInlineCount() const { return inlineCount_; }
    
    // 打印分析结果
    void printAnalysis() const;
    
private:
    InlineConfig config_;
    int inlineCount_;
    int currentDepth_ = 0;
    
    std::unordered_map<String, FuncInfo> functions_;
    std::unordered_set<String> inlined_;  // 已内联的函数
    
    // 计算 AST 大小
    int calcASTSize(Shared<Stmt> stmt);
    int calcASTSize(Shared<Expr> expr);
    
    // 检查是否应该内联
    bool shouldInline(const FuncInfo& func);
    
    // 克隆表达式（替换参数）
    Shared<Expr> cloneAndSubstitute(
        Shared<Expr> expr,
        const std::unordered_map<String, Shared<Expr>>& subst
    );
    
    // 克隆语句
    Shared<Stmt> cloneStmt(Shared<Stmt> stmt);
    
    // 收集函数调用
    void collectCalls(Shared<Stmt> stmt);
    void collectCalls(Shared<Expr> expr);
};

} // namespace cplang
