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
    
    // 声明
    Shared<Stmt> parseDeclaration();
    Shared<FuncDeclStmt> parseFunctionDecl();
    Shared<ClassDeclStmt> parseClassDecl();
    Shared<InterfaceDeclStmt> parseInterfaceDecl();
    Shared<EnumDeclStmt> parseEnumDecl();
    Shared<Expr> parsePipe();
    Shared<StructDeclStmt> parseStructDecl();
    Shared<VarDeclStmt> parseVariableDecl(bool isConst, bool isLet = false);
    
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
    Shared<TryStmt> parseTryStatement();
    
    // 表达式 (从低到高优先级)
    Shared<Expr> parseAssignment();
    Shared<Expr> parseTernary();        // ?:
    Shared<Expr> parseOr();            // ||
    Shared<Expr> parseAnd();            // &&
    Shared<Expr> parseBitOr();         // |
    Shared<Expr> parseBitXor();        // ^
    Shared<Expr> parseBitAnd();         // &
    Shared<Expr> parseEquality();       // == !=
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

    // 解析泛型类型参数（支持嵌套，如 "对<整数, 字符串>"）
    String parseGenericTypeArg();
};

// 便捷函数
Shared<Program> parseString(const String& source);
Shared<Program> parseFile(const String& filename);
Shared<Expr> parseExprString(const String& source);

} // namespace cplang