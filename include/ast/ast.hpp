// CP语言 AST 节点定义
#pragma once

#include "common/types.hpp"
#include "lexer/token.hpp"
#include <vector>

namespace cplang {

// AST 节点基类
struct ASTNode {
    virtual ~ASTNode() = default;
    Token token;
};

// 表达式节点
struct Expr : ASTNode {};

// 字面量表达式
struct LiteralExpr : Expr {
    Variant<Int64, Float64, String, bool> value;
};

// 标识符表达式
struct IdentifierExpr : Expr {
    String name;
};

// 二元表达式
struct BinaryExpr : Expr {
    Shared<Expr> left;
    TokenType op;
    Shared<Expr> right;
};

// 一元表达式
struct UnaryExpr : Expr {
    TokenType op;
    Shared<Expr> operand;
    bool isPostfix = false;  // true for x++, x-- (postfix)
};

// 调用表达式
struct CallExpr : Expr {
    Shared<Expr> callee;
    std::vector<Shared<Expr>> arguments;
    std::vector<String> typeArgs;  // 泛型调用类型参数: 排序<整数>(1, 2)
};

// 成员访问表达式
struct MemberExpr : Expr {
    Shared<Expr> object;
    String member;
};

// 数组访问表达式
struct IndexExpr : Expr {
    Shared<Expr> array;
    Shared<Expr> index;
};

// 数组字面量表达式
struct ArrayExpr : Expr {
    std::vector<Shared<Expr>> elements;
};

// 结构体字面量表达式
struct StructLiteralExpr : Expr {
    String structName;  // 结构体类型名
    std::vector<std::pair<String, Shared<Expr>>> fields;  // 字段名和值
};

// 语句节点
struct Stmt : ASTNode {};

// 借用表达式（Rust式所有权）
struct BorrowExpr : Expr {
    Shared<Expr> target;
    bool isMutable = false;  // &可写 vs &
};

// 移动表达式（移动所有权）
struct MoveExpr : Expr {
    Shared<Expr> target;  // 要移动所有权的目标
};

// 删除/释放表达式
struct DropExpr : Expr {
    Shared<Expr> target;  // 要释放的目标
};

struct BlockStmt;  // forward decl

// 可信块（unsafe 块）
struct TrustBlockStmt : Stmt {
    Shared<BlockStmt> body;
};

// 表达式语句
struct ExprStmt : Stmt {
    Shared<Expr> expr;
};

// 空语句
struct EmptyStmt : Stmt {};

// 块语句
struct BlockStmt : Stmt {
    std::vector<Shared<Stmt>> statements;
};

// 变量声明
struct VarDeclStmt : Stmt {
    String name;
    Optional<String> type;
    Shared<Expr> init;
    bool isConst;
    bool isImplicit = false;  // x=10 without 变量
};

// 类型参数（用于泛型）
struct TypeParam {
    String name;
    Optional<String> constraint;  // 可选的约束/trait 名（如 "可比较"）
};

// 函数声明
struct FuncDeclStmt : Stmt {
    String name;
    std::vector<TypeParam> typeParams;  // 泛型类型参数: <T: 可比较, U>
    std::vector<std::pair<String, Optional<String>>> params;  // name, type
    Optional<String> returnType;
    Shared<BlockStmt> body;
    bool isStatic;
    bool isVirtual;
};

// 类声明
struct ClassDeclStmt : Stmt {
    String name;
    Optional<String> baseClass;
    std::vector<Shared<Stmt>> members;  // fields, methods
};

// new 表达式
struct NewExpr : Expr {
    String className;
    std::vector<Shared<Expr>> args;  // constructor args
};

// 接口声明
struct InterfaceDeclStmt : Stmt {
    String name;
    std::vector<Shared<Stmt>> methods;  // only method signatures
};

// 枚举声明
struct EnumDeclStmt : Stmt {
    String name;
    std::vector<std::pair<String, Optional<Int64>>> values;
};

// 结构体声明
struct StructDeclStmt : Stmt {
    String name;
    std::vector<TypeParam> typeParams;  // 泛型类型参数: <T: 可比较, U>
    std::vector<Shared<Stmt>> members;
};

// if 语句
struct IfStmt : Stmt {
    Shared<Expr> condition;
    Shared<Stmt> thenBranch;
    Shared<Stmt> elseBranch;
};

// switch 语句
struct SwitchStmt : Stmt {
    Shared<Expr> expr;
    std::vector<std::pair<Optional<Shared<Expr>>, Shared<Stmt>>> cases;  // expr, stmt
    Shared<Stmt> defaultCase;
};

// for 语句
struct ForStmt : Stmt {
    Shared<Stmt> init;       // VarDecl or ExprStmt
    Shared<Expr> condition;
    Shared<Expr> update;
    Shared<Stmt> body;
};

// for-each 语句: 循环 (变量 : 数组) { ... }
struct ForEachStmt : Stmt {
    String varName;          // 循环变量名
    Shared<Expr> iterable;   // 可迭代对象（数组）
    Shared<Stmt> body;
};

// while 语句
struct WhileStmt : Stmt {
    Shared<Expr> condition;
    Shared<Stmt> body;
};

// do-while 语句
struct DoWhileStmt : Stmt {
    Shared<Stmt> body;
    Shared<Expr> condition;
};

// break 语句
struct BreakStmt : Stmt {};

// continue 语句
struct ContinueStmt : Stmt {};

// defer 语句（推迟执行，离开作用域时逆序执行）
struct DeferStmt : Stmt {
    Shared<Stmt> body;  // 推迟执行的语句
};

// return 语句
struct ReturnStmt : Stmt {
    Shared<Expr> value;
};

// throw 语句
struct ThrowStmt : Stmt {
    Shared<Expr> exception;
};

// try-catch 语句
struct TryStmt : Stmt {
    Shared<Stmt> tryBlock;
    std::vector<std::pair<String, Shared<Stmt>>> catchBlocks;  // exception name, handler
    Shared<Stmt> finallyBlock;
};

// import 语句
struct ImportStmt : Stmt {
    String moduleName;      // 模块名（不含.cp后缀）
    Optional<String> alias; // 可选别名
};

// package 语句
struct PackageStmt : Stmt {
    String name;
};

// 程序根节点
struct Program : ASTNode {
    Optional<Shared<PackageStmt>> package;
    std::vector<Shared<Stmt>> statements;
};

} // namespace cplang