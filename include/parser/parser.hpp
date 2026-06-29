// CP语言 语法分析器
#pragma once

#include "lexer/lexer.hpp"
#include "ast/ast.hpp"
#include <memory>

namespace cplang {

// 语法分析器
class Parser {
public:
    explicit Parser(Lexer* lexer);
    ~Parser();
    
    // 解析入口
    Shared<Program> parse();
    
    // 错误处理
    bool hasError() const { return hasError_; }
    const String& errorMessage() const { return errorMsg_; }

public:
    // 表达式解析（可从子解析器调用，用于 ${} 插值）
    Shared<Expr> parseExpression();

private:
    Lexer* lexer_;
    Token current_;
    Token peek_;
    Token peek2_;
    Token peek3_;  // 三层 lookahead，用于泛型调用 vs 比较的歧义消除
    bool hasError_;
    String errorMsg_;
    int gensymCounter_ = 0;  // 卫生宏唯一名称计数器
    
    // 宏表
    struct MacroDef {
        String name;
        std::vector<String> params;
        Optional<String> varParam;   // 可变参数名（...args）
        Shared<Expr> body;           // 表达式级宏体
        Shared<BlockStmt> stmtBody;  // 语句级宏体（P8.1）
        bool isStmtMacro = false;    // 是否为语句级宏
    };
    std::unordered_map<String, MacroDef> macros_;
    
    // 工具方法
    void consume();
    Token consumeToken(TokenType expected);
    bool match(TokenType type);
    bool match(TokenType t1, TokenType t2);
    void expect(TokenType type, const String& msg);
    void reportError(const String& msg);
    void synchronize();
    Token peekSecond() const;
    Token peekThird() const { return peek3_; }
    bool peekIsIdentifier() const { return peek_.type == TokenType::IDENTIFIER; }
    
    // === 语法分析 ===
    
    // 程序级
    Shared<Program> parseProgram();
    Shared<Stmt> parseStatement();
    Shared<Stmt> parseExpressionStatement();
    
    // 包和导入
    Shared<PackageStmt> parsePackage();
    Shared<ImportStmt> parseImport();
    Shared<Stmt> parseExport();
    Shared<Stmt> parseDecorator();  // 装饰器
    
    // 声明
    Shared<Stmt> parseDeclaration();
    Shared<FuncDeclStmt> parseFunctionDecl();
    Shared<Stmt> parseMacroDecl();
    Shared<Stmt> parseTypeAlias();
    Shared<ClassDeclStmt> parseClassDecl();
    Shared<InterfaceDeclStmt> parseInterfaceDecl();
    Shared<EnumDeclStmt> parseEnumDecl();
    Shared<Expr> parsePipe();
    Shared<StructDeclStmt> parseStructDecl();
    Shared<VarDeclStmt> parseVariableDecl(bool isConst, bool isLet = false);
    Shared<Stmt> parseDestructuringDecl(bool isConst, bool isLet, bool isAssign);  // 解构赋值
    
    // where 子句解析
    void parseWhereClause(std::vector<TypeParam>& typeParams);
    
    // 宏展开辅助
    Shared<Stmt> expandStmtMacro(const String& macroName, Token token);  // 展开语句级宏调用
    
    // 宏参数替换（表达式级）
    static Shared<Expr> substituteMacroParams(Shared<Expr> expr,
        const std::unordered_map<String, Shared<Expr>>& bindings);
    // 宏变量重命名（卫生宏用）
    static Shared<Expr> renameVarsInExpr(Shared<Expr> expr,
        const std::unordered_map<String, String>& varMap);
    // 递归展开表达式中的宏调用（带深度限制）
    Shared<Expr> expandMacrosInExpr(Shared<Expr> expr, int maxDepth);
    
    // 条件编译
    Shared<Stmt> parseConditionalCompilation();  // 解析条件编译
    bool evalCondExpr();  // 计算条件编译表达式
    
    // 语句
    Shared<BlockStmt> parseBlock();
    Shared<Stmt> parseIfStatement();
    Shared<Stmt> parseSwitchStatement();
    Shared<Stmt> parseMatchStatement();
    Shared<Stmt> parseForStatement();
    Shared<Stmt> parseForEachStatement();
    Shared<Stmt> parseWhileStatement();
    Shared<Stmt> parseDoWhileStatement();
    Shared<Stmt> parseDeferStatement();
    Shared<ReturnStmt> parseReturnStatement();
    Shared<BreakStmt> parseBreakStatement();
    Shared<ContinueStmt> parseContinueStatement();
    Shared<ThrowStmt> parseThrowStatement();
    Shared<YieldStmt> parseYieldStatement();
    Shared<GoStmt> parseGoStatement();
    Shared<TryStmt> parseTryStatement();
    Shared<WithStmt> parseWithStatement();
    
    // 表达式 (从低到高优先级)
    Shared<Expr> parseAssignment();
    Shared<Expr> parseTernary();        // ?:
    Shared<Expr> parseNullCoalesce();  // ??
    Shared<Expr> parseOr();            // ||
    Shared<Expr> parseAnd();            // &&
    Shared<Expr> parseBitOr();         // |
    Shared<Expr> parseBitXor();        // ^
    Shared<Expr> parseBitAnd();         // &
    Shared<Expr> parseEquality();       // == !=
    Shared<Expr> parseTypeTest();       // is / 属于
    Shared<Expr> parseComparison();     // < > <= >=
    Shared<Expr> parseShift();          // << >>
    Shared<Expr> parseAdditive();      // + -
    Shared<Expr> parseMultiplicative(); // * / %
    Shared<Expr> parseUnary();          // - ! ~ ++ --
    Shared<Expr> parsePostfix();        // . [] () ++ --
    Shared<Expr> parsePrimary();        // 字面量、标识符、括号
    Shared<Expr> parseLambda();         // Lambda表达式 (|x, y| { ... } 或 (x, y) => { ... })
    
    // 类型
    Optional<String> parseType();
    Optional<String> parseTypeExpr();  // 增强类型表达式（联合、可选）
    Optional<String> parseTypeArray(); // 数组 + 可选类型

    // 解析泛型类型参数（支持嵌套，如 "对<整数, 字符串>"）
    String parseGenericTypeArg();
};

// 便捷函数
Shared<Program> parseString(const String& source);
Shared<Program> parseFile(const String& filename);
Shared<Expr> parseExprString(const String& source);

} // namespace cplang