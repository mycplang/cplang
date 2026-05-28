#pragma once
#include "ast/ast.hpp"
#include <vector>
#include <unordered_set>

namespace cplang {

// 循环展开结果
struct LoopUnrollResult {
    Shared<Stmt> unrolledLoop;      // 展开后的语句
    int unrollFactor;               // 展开因子
    int iterationsUnrolled;         // 展开的迭代次数
    bool success;
    String error;
};

// 循环信息
struct LoopInfo {
    String loopVar;                 // 循环变量
    Shared<Expr> start;             // 起始值
    Shared<Expr> end;               // 结束值
    Shared<Expr> step;              // 步长
    Shared<Stmt> body;              // 循环体
    bool isCountable;               // 是否是可计数循环
    int tripCount;                  // 循环次数（如果可确定）
};

// 循环展开优化器
// 将循环展开为重复的循环体，减少循环开销
class LoopUnroller {
public:
    LoopUnroller(int maxUnrollFactor = 8, int maxBodySize = 100)
        : maxUnrollFactor_(maxUnrollFactor)
        , maxBodySize_(maxBodySize)
        , unrollCount_(0) {}
    
    // 优化语句
    Shared<Stmt> unroll(Shared<Stmt> stmt);
    
    // 优化整个程序
    Shared<Program> unrollProgram(Shared<Program> program);
    
    // 获取展开次数
    int getUnrollCount() const { return unrollCount_; }
    
    // 设置最大展开因子
    void setMaxUnrollFactor(int factor) { maxUnrollFactor_ = factor; }
    
    // 打印分析
    void printAnalysis(Shared<Stmt> loop) const;
    
private:
    int maxUnrollFactor_;           // 最大展开因子
    int maxBodySize_;               // 最大循环体大小
    int unrollCount_;               // 展开次数
    
    // 分析循环
    LoopInfo analyzeLoop(Shared<Stmt> stmt) const;
    
    // 估算语句大小
    int estimateSize(Shared<Stmt> stmt) const;
    int estimateExprSize(Shared<Expr> expr) const;
    
    // 检查是否应该展开
    bool shouldUnroll(const LoopInfo& info) const;
    
    // 计算展开因子
    int computeUnrollFactor(const LoopInfo& info) const;
    
    // 展开循环
    LoopUnrollResult unrollFor(Shared<ForStmt> forStmt, int factor);
    LoopUnrollResult unrollWhile(Shared<WhileStmt> whileStmt, int factor);
    
    // 复制语句并替换变量
    Shared<Stmt> cloneStmt(Shared<Stmt> stmt, const String& var, int offset);
    Shared<Expr> cloneExpr(Shared<Expr> expr, const String& var, int offset);
    
    // 构建展开后的循环体
    Shared<BlockStmt> buildUnrolledBody(
        Shared<Stmt> body,
        const String& loopVar,
        int factor
    );
    
    // 判断是否是简单循环体
    bool isSimpleBody(Shared<Stmt> body);
    
    // 提取循环变量引用
    std::unordered_set<String> extractVarRefs(Shared<Stmt> stmt);
};

} // namespace cplang
