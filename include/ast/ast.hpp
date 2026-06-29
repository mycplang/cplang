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
    // 使用 std::monostate 表示 null 字面量（放在第一位作为默认值）
    Variant<std::monostate, Int64, Float64, String, bool> value;
};

// 标识符表达式
struct IdentifierExpr : Expr {
    String name;
};

// await / 等待 表达式
struct AwaitExpr : Expr {
    Shared<Expr> target;  // 等待的操作
};

// is / 属于 类型守卫表达式：expr is Type
struct IsExpr : Expr {
    Shared<Expr> expr;     // 要检查的表达式
    String checkType;      // 要检查的类型名
};

// this / self 表达式（在类方法中引用当前实例）
struct ThisExpr : Expr {};

// super 调用表达式（调用父类方法）
struct SuperExpr : Expr {
    String method;                          // 要调用的父类方法名
    std::vector<Shared<Expr>> arguments;    // 参数
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
struct SpreadExpr : Expr {
    Shared<Expr> array;  // the array/expression to spread
};

struct MemberExpr : Expr {
    Shared<Expr> object;
    String member;
    bool optional = false;
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

struct BlockStmt;  // forward decl

// Lambda表达式（匿名函数/闭包）
struct LambdaExpr : Expr {
    std::vector<std::pair<String, Optional<String> > > params;   // 参数名, 可选类型注解
    Optional<String> returnType;                               // 返回类型注解
    Shared<BlockStmt> body;                                    // 函数体
    std::vector<String> captures;                              // 捕获的变量（由语义分析器填充）
};

// 语句节点
struct Stmt : ASTNode {};

// 管道表达式：a |> f 等价于 f(a)
struct PipeExpr : Expr {
    Shared<Expr> left;   // 左侧值
    Shared<Expr> right;  // 右侧函数/表达式
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
    bool isStatic = false;    // 静态成员变量
};

// 宏声明
struct MacroDeclStmt : Stmt {
    String name;
    std::vector<String> params;  // 参数名列表
    Optional<String> varParam;   // 可变参数名（...args）
    Shared<Expr> body;           // 宏体（表达式形式，向后兼容）
    Shared<BlockStmt> stmtBody;  // 宏体（语句块形式，P8.1新增）
    bool isStmtMacro = false;    // 是否为语句级宏
};

// 类型别名声明
struct TypeAliasStmt : Stmt {
    String name;          // 别名名称
    String targetType;    // 目标类型字符串
};

// 解构赋值声明: var [a, b, c] = 数组 或 var {x, y} = 表
struct DestructuringDecl : Stmt {
    enum Kind { ARRAY, TABLE };
    Kind kind;
    std::vector<String> names;      // 变量名列表
    std::vector<String> keys;       // 表解构时的键名（数组解构时为空）
    Shared<Expr> init;              // 右侧表达式
    bool isConst = false;
};

// 类型参数（用于泛型）
struct TypeParam {
    String name;
    Optional<String> constraint;  // 可选的约束/trait 名（如 "可比较"）- 单约束简写
    std::vector<String> constraints;  // 多个约束（where子句中使用）
};

// 访问修饰符
enum class AccessModifier { PUBLIC, PRIVATE, PROTECTED };

// 函数声明
struct FuncDeclStmt : Stmt {
    String name;
    std::vector<TypeParam> typeParams;  // 泛型类型参数: <T: 可比较, U>
    std::vector<std::pair<String, Optional<String>>> params;  // name, type
    std::vector<Shared<Expr>> paramDefaults;                // default values (null if none)
    Optional<String> returnType;
    Shared<BlockStmt> body;
    bool isStatic = false;
    bool isVirtual = false;
    bool isAsync = false;    // 异步函数标记
    bool isGenerator = false; // 生成器函数标记（包含yield）
    AccessModifier access = AccessModifier::PUBLIC;
};

// 类声明
struct ClassDeclStmt : Stmt {
    String name;
    std::vector<TypeParam> typeParams;  // 泛型类型参数: <T: 可比较, U>
    Optional<String> baseClass;
    std::vector<String> interfaces;     // 实现的接口列表
    std::vector<Shared<Stmt>> members;  // fields, methods
    AccessModifier currentAccess = AccessModifier::PUBLIC;  // 当前访问级别
};

// new 表达式
struct NewExpr : Expr {
    String className;
    std::vector<String> typeArgs;  // 泛型类实例化类型参数: new 栈<整数>(16)
    std::vector<Shared<Expr>> args;  // constructor args
};

// 接口声明
struct InterfaceDeclStmt : Stmt {
    String name;
    std::vector<TypeParam> typeParams;  // 泛型类型参数
    std::vector<String> baseInterfaces; // 继承的接口列表
    std::vector<Shared<Stmt>> methods;  // only method signatures
};

// 枚举声明
// EnumVariantDef: 枚举中的一个变体（ADT 风格）
struct EnumVariantDef {
    String name;
    std::vector<std::pair<String, String>> fields;  // 字段名 → 类型名
};

struct EnumDeclStmt : Stmt {
    String name;
    std::vector<std::pair<String, Optional<Int64>>> values;  // 简单 C 风格枚举值（向后兼容）
    std::vector<EnumVariantDef> variants;  // ADT 变体（带关联数据）
    bool isADT = false;  // true 表示此枚举有带关联数据的变体
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

// 模式类型
enum class PatternKind {
    PATTERN_VARIANT,   // 枚举变体模式: 情况 变体名(绑定)
    PATTERN_LITERAL,   // 字面量模式: 情况 42 / 情况 "hello"
    PATTERN_WILDCARD, // 通配符模式: 情况 _
    PATTERN_BINDING,   // 变量绑定模式: 情况 x => 绑定到变量
    PATTERN_ARRAY      // A3.1: 数组/元组模式: 情况 [a, b, c]
};

// MatchCase：匹配语句中的一个分支
struct MatchCase {
    PatternKind kind = PatternKind::PATTERN_VARIANT; // 模式类型
    
    // 变体模式
    String variantName;                    // 要匹配的变体名
    std::vector<String> bindings;           // 绑定到变体字段的变量名
    
    // 字面量模式
    Shared<LiteralExpr> literalValue;     // 字面量值
    
    // 绑定模式
    String bindingName;                  // 绑定变量名
    
    // A3.1: 数组/元组模式
    std::vector<String> arrayBindings;     // 数组元素绑定变量名
    
    // A2.1: 守卫条件（可选）
    Shared<Expr> guard;                    // 守卫条件表达式: 当 cond =>
    
    Shared<Stmt> body;                      // 分支体
};

// MatchStmt：模式匹配语句
struct MatchStmt : Stmt {
    Shared<Expr> expr;                      // 要匹配的表达式
    std::vector<MatchCase> cases;            // 匹配分支
    Shared<Stmt> defaultCase;               // 可选的默认分支
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

// yield 语句（生成器产出值）
struct YieldStmt : Stmt {
    Shared<Expr> value;  // 产出的值（可选，yield; 时为null）
};

// go / 协程 语句：启动并发执行
struct GoStmt : Stmt {
    Shared<Expr> expr;   // 要并发执行的表达式（通常是函数调用）
};

// throw 语句
struct ThrowStmt : Stmt {
    Shared<Expr> exception;
};

// Catch块：包含异常类型、变量名和处理代码
struct CatchBlock {
    String exceptionType;  // B1: 异常类型名（空字符串表示捕获所有）
    String varName;        // 异常变量名
    Shared<Stmt> body;     // 处理代码
};

// try-catch 语句
struct TryStmt : Stmt {
    Shared<Stmt> tryBlock;
    std::vector<CatchBlock> catchBlocks;  // B1: 类型化的catch块
    Shared<Stmt> finallyBlock;
};

// with / 使用 语句（上下文管理器）
// 使用 表达式 as 变量名: 块
// 等价于: var 变量名 = 表达式; try { 块 } finally { 变量名.close(); }
struct WithStmt : Stmt {
    Shared<Expr> expr;       // 资源表达式
    String varName;          // 资源变量名（as 后面的变量）
    Shared<Stmt> body;       // with 体
};

// import 语句
struct ImportStmt : Stmt {
    String moduleName;      // 模块名（不含.cp后缀）
    Optional<String> alias; // 可选别名
    bool isNamedImport = false;  // 是否是命名导入（from...import）
    // 命名导入列表：{原名, 可选别名}
    std::vector<std::pair<String, Optional<String>>> importedNames;
    bool isNamespaceImport = false;  // 是否是命名空间导入（import * as alias）
};

// 装饰器定义
struct Decorator {
    String name;                    // 装饰器名称
    std::vector<Shared<Expr>> args; // 装饰器参数（可选）
};

// 装饰器语句 - 包装被装饰的声明（类似 ExportStmt）
struct DecoratorStmt : Stmt {
    std::vector<Decorator> decorators;  // 装饰器列表（支持多个堆叠）
    Shared<Stmt> declaration;           // 被装饰的声明（函数、类等）
};

// export 语句
struct ExportStmt : Stmt {
    enum class Kind {
        DECLARATION,  // export function/class/var ...
        NAMED_EXPORTS // export { a, b as c }
    };
    Kind kind;
    Shared<Stmt> declaration;  // 当 kind == DECLARATION 时
    // 命名导出列表：{原名, 可选别名}
    std::vector<std::pair<String, Optional<String>>> namedExports;
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