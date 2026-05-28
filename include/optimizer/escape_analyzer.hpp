#pragma once
#include "ast/ast.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>

namespace cplang {

// 逃逸级别
enum class EscapeLevel {
    None = 0,       // 不逃逸（栈上分配）
    Arg = 1,        // 作为参数传递
    Return = 2,     // 作为返回值
    Global = 3,     // 全局变量
    Heap = 4        // 逃逸到堆
};

// 逃逸信息
struct EscapeInfo {
    EscapeLevel level;              // 逃逸级别
    std::string reason;             // 逃逸原因
    std::vector<std::string> paths; // 逃逸路径
    
    EscapeInfo() : level(EscapeLevel::None), reason("") {}
    EscapeInfo(EscapeLevel lv, const std::string& r) 
        : level(lv), reason(r) {}
};

// 变量逃逸状态
struct VarEscape {
    std::string varName;            // 变量名
    EscapeLevel level;              // 逃逸级别
    bool isAddressTaken;            // 是否取地址
    bool isPassedToFunc;            // 是否传递给函数
    bool isReturned;                // 是否被返回
    bool isGlobal;                  // 是否是全局变量
    bool isCaptured;                // 是否被闭包捕获
    
    // 可能的逃逸位置
    std::unordered_set<std::string> escapeSites;
    
    VarEscape() : level(EscapeLevel::None), isAddressTaken(false),
                  isPassedToFunc(false), isReturned(false),
                  isGlobal(false), isCaptured(false) {}
};

// 函数逃逸分析结果
struct FuncEscapeResult {
    bool canStackAlloc;             // 是否可以栈上分配
    std::unordered_map<std::string, VarEscape> varEscape;
    int stackSaved;                 // 节省的堆分配数
};

// 程序逃逸分析结果
struct ProgramEscapeResult {
    std::vector<FuncEscapeResult> functionResults;
    int totalStackSaved;            // 总共节省的堆分配
    int totalHeapAlloc;              // 总共的堆分配
    int totalStackAlloc;            // 总共的栈分配
};

// 逃逸分析器
class EscapeAnalyzer {
public:
    EscapeAnalyzer();
    
    // 分析整个程序
    ProgramEscapeResult analyze(Shared<Program> program);
    
    // 分析单个函数
    FuncEscapeResult analyzeFunction(Shared<FuncDeclStmt> func);
    
    // 分析表达式中的逃逸
    EscapeLevel analyzeExpr(Shared<Expr> expr);
    
    // 检查变量是否逃逸
    bool isEscaped(const std::string& varName);
    
    // 检查是否可以在栈上分配
    bool canStackAlloc(const std::string& varName);
    
    // 获取逃逸信息
    const VarEscape* getEscapeInfo(const std::string& funcName, const std::string& varName);
    
    // 打印分析结果
    void printResult(const ProgramEscapeResult& result) const;
    
private:
    // 当前分析上下文
    Shared<FuncDeclStmt> currentFunc_;
    std::unordered_map<std::string, VarEscape> currentVarEscape_;
    std::unordered_set<std::string> addressTaken_;
    
    // 分析语句
    void analyzeStmt(Shared<Stmt> stmt);
    
    // 分析函数调用
    void analyzeCall(Shared<CallExpr> call);
    
    // 分析 return 语句
    void analyzeReturn(Shared<ReturnStmt> ret);
    
    // 分析 if 语句
    void analyzeIf(Shared<IfStmt> ifStmt);
    
    // 分析循环语句
    void analyzeLoop(Shared<Stmt> loop);
    
    // 分析 for-each
    void analyzeForEach(Shared<ForEachStmt> feStmt);
    
    // 取地址分析
    void analyzeAddressOf(Shared<Expr> expr);
    
    // 检查表达式是否引用变量
    std::unordered_set<std::string> getVarRefs(Shared<Expr> expr);
    
    // 标记变量逃逸
    void markEscaped(const std::string& varName, EscapeLevel level, const std::string& reason);
    
    // 检查是否是地址表达式
    bool isAddressExpr(Shared<Expr> expr);
    
    // 传播逃逸信息
    void propagateEscape(const std::string& from, const std::string& to);
};

} // namespace cplang
