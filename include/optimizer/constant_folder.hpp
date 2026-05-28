// 常量折叠优化器
// 在编译时计算常量表达式

#pragma once
#include "ast/ast.hpp"
#include "common/types.hpp"
#include <unordered_map>

namespace cplang {

// 常量值（编译时已知）
struct ConstValue {
    enum Type { NIL, BOOL, INT, FLOAT, STRING };
    Type type;
    bool boolVal;
    Int64 intVal;
    Float64 floatVal;
    String stringVal;
    
    static ConstValue nil() { ConstValue v; v.type = NIL; return v; }
    static ConstValue fromBool(bool b) { ConstValue v; v.type = BOOL; v.boolVal = b; return v; }
    static ConstValue fromInt(Int64 i) { ConstValue v; v.type = INT; v.intVal = i; return v; }
    static ConstValue fromFloat(Float64 f) { ConstValue v; v.type = FLOAT; v.floatVal = f; return v; }
    static ConstValue fromString(const String& s) { ConstValue v; v.type = STRING; v.stringVal = s; return v; }
    
    bool isConst() const { return type != NIL; }
    bool isBool() const { return type == BOOL; }
    bool isInt() const { return type == INT; }
    bool isFloat() const { return type == FLOAT; }
    bool isString() const { return type == STRING; }
    bool isNumber() const { return type == INT || type == FLOAT; }
    
    double toNumber() const {
        if (type == INT) return static_cast<double>(intVal);
        if (type == FLOAT) return floatVal;
        return 0.0;
    }
};

// 常量折叠器
class ConstantFolder {
public:
    ConstantFolder() : foldCount_(0) {}
    
    // 折叠表达式（返回新表达式或原表达式）
    Shared<Expr> fold(Shared<Expr> expr);
    
    // 折叠语句
    Shared<Stmt> foldStmt(Shared<Stmt> stmt);
    
    // 统计
    int getFoldCount() const { return foldCount_; }
    
private:
    int foldCount_;
    std::unordered_map<String, ConstValue> constVars_;  // 已知的常量变量
    
public:
    // 尝试求值表达式 (公开给 DeadCodeEliminator)
    ConstValue tryEval(Shared<Expr> expr);
    
    // 创建字面量表达式
    Shared<Expr> makeLiteral(const ConstValue& val);
    
    // 二元运算求值
    ConstValue evalBinary(TokenType op, const ConstValue& left, const ConstValue& right);
    
    // 一元运算求值
    ConstValue evalUnary(TokenType op, const ConstValue& operand);
};

} // namespace cplang
