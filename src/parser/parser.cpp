// CP语言 语法分析器实现
#include "parser/parser.hpp"
#include <iostream>

namespace cplang {

// === 构造函数 ===

Parser::Parser(Lexer* lexer)
    : lexer_(lexer), hasError_(false) {
    // 读取前四个 token 初始化 current_, peek_, peek2_, peek3_
    current_ = lexer_->nextToken();
    peek_ = lexer_->nextToken();
    peek2_ = lexer_->nextToken();
    peek3_ = lexer_->nextToken();
}

Parser::~Parser() = default;

// === 工具方法 ===

void Parser::consume() {
    current_ = peek_;
    peek_ = peek2_;
    peek2_ = peek3_;
    peek3_ = lexer_->nextToken();
}

Token Parser::consumeToken(TokenType expected) {
    if (current_.type == expected) {
        Token t = current_;
        consume();
        return t;
    }
    reportError("期望令牌类型 " + std::to_string(static_cast<int>(expected)) +
                " 但遇到 " + std::to_string(static_cast<int>(current_.type)));
    return Token(TokenType::INVALID, "", current_.line, current_.column);
}

bool Parser::match(TokenType type) {
    return current_.type == type;
}

bool Parser::match(TokenType t1, TokenType t2) {
    return current_.type == t1 && peek_.type == t2;
}

void Parser::expect(TokenType type, const String& msg) {
    if (!match(type)) {
        reportError(msg);
    } else {
        consume();
    }
}

void Parser::reportError(const String& msg) {
    if (!hasError_) {
        hasError_ = true;
        errorMsg_ = "第" + std::to_string(current_.line) +
                   "行第" + std::to_string(current_.column) +
                   "列: " + msg + " (当前: '" + current_.text + "')";
    }
}

void Parser::synchronize() {
    // 跳过到下一个语句边界
    while (!match(TokenType::END_OF_FILE)) {
        if (match(TokenType::SEMICOLON)) {
            consume();
            return;
        }
        consume();
    }
}

Token Parser::peekSecond() const {
    return peek2_;
}

// === 程序级 ===

Shared<Program> Parser::parse() {
    return parseProgram();
}

Shared<Program> Parser::parseProgram() {
    auto program = Shared<Program>(new Program());
    
    // 可选的 package 声明
    if (match(TokenType::K_PACKAGE)) {
        program->package = parsePackage();
    }
    
    // 解析语句
    while (!match(TokenType::END_OF_FILE)) {
        auto stmt = parseStatement();
        if (stmt) {
            program->statements.push_back(stmt);
        } else {
            break;
        }
    }
    
    return program;
}

Shared<Stmt> Parser::parseStatement() {
    // package
    if (match(TokenType::K_PACKAGE)) {
        return parsePackage();
    }
    
    // import
    if (match(TokenType::K_IMPORT)) {
        return parseImport();
    }
    
    // class
    if (match(TokenType::K_CLASS)) {
        return parseClassDecl();
    }
    
    // interface
    if (match(TokenType::K_INTERFACE)) {
        return parseInterfaceDecl();
    }
    
    // enum
    if (match(TokenType::K_ENUM)) {
        return parseEnumDecl();
    }
    
    // struct
    if (match(TokenType::K_STRUCT)) {
        return parseStructDecl();
    }
    
    // func
    if (match(TokenType::K_FUNC)) {
        return parseFunctionDecl();
    }
    
    // const
    if (match(TokenType::K_CONST)) {
        consume();  // consume 'const' keyword
        return parseVariableDecl(true);
    }
    
    // var / 变量
    if (match(TokenType::K_VAR)) {
        consume();  // consume 'var' keyword
        return parseVariableDecl(true);
    }
    
    // let / 设

    // mutable
    if (match(TokenType::K_MUTABLE)) {
        consume();
        return parseVariableDecl(false);
    }
    if (match(TokenType::K_LET)) {
        consume();  // consume '设' keyword
        return parseVariableDecl(false, true);
    }
    
    // if
    if (match(TokenType::K_IF)) {
        return parseIfStatement();
    }
    
    // switch
    if (match(TokenType::K_SWITCH)) {
        return parseSwitchStatement();
    }
    
    // match
    if (match(TokenType::K_MATCH)) {
        return parseMatchStatement();
    }
    
    // for
    if (match(TokenType::K_FOR)) {
        return parseForStatement();
    }
    
    // for-each (遍历/forEach/foreach) — 通过标识符文本判断，避免与 stdlib 函数名冲突
    // 语法1 (旧): 遍历 ( x : arr ) { ... }
    // 语法2 (新): 遍历 x 在 arr { ... }
    // 如果不符合上述模式，则作为普通函数调用（如遍历(arr, fn)）处理
    if (match(TokenType::IDENTIFIER) &&
        (current_.text == "遍历" || current_.text == "forEach" || current_.text == "foreach")) {
        // 检查上下文判断是否为 for-each 语句
        bool isForEach = false;
        if (peek_.type == TokenType::LPAREN) {
            // 旧语法: 遍历 ( x : arr )  — 需要 ( identifier : 模式
            if (peekSecond().type == TokenType::IDENTIFIER &&
                peekThird().type == TokenType::OP_COLON) {
                isForEach = true;
            }
        } else if (peek_.type == TokenType::IDENTIFIER &&
                   peekSecond().type == TokenType::K_IN) {
            // 新语法: 遍历 x 在 arr
            isForEach = true;
        }
        if (isForEach) {
            return parseForEachStatement();
        }
        // 否则是函数调用，fall through 到表达式语句
    }
    // 兼容旧版：如果 lexer 仍产生 K_FOREACH
    if (match(TokenType::K_FOREACH)) {
        return parseForEachStatement();
    }
    
    // while
    if (match(TokenType::K_WHILE)) {
        return parseWhileStatement();
    }
    
    // do-while
    if (match(TokenType::K_DO)) {
        return parseDoWhileStatement();
    }

    // defer
    if (match(TokenType::K_DEFER)) {
        return parseDeferStatement();
    }

    // return
    if (match(TokenType::K_RETURN)) {
        return parseReturnStatement();
    }
    
    // break
    if (match(TokenType::K_BREAK)) {
        return parseBreakStatement();
    }
    
    // continue
    if (match(TokenType::K_CONTINUE)) {
        return parseContinueStatement();
    }
    
    // throw
    if (match(TokenType::K_THROW)) {
        return parseThrowStatement();
    }
    
    // try
    if (match(TokenType::K_TRY)) {
        return parseTryStatement();
    }
    
    // 可信块 (unsafe block)
    if (match(TokenType::K_TRUST)) {
        consume();
        auto stmt = Shared<TrustBlockStmt>(new TrustBlockStmt());
        stmt->body = parseBlock();
        return stmt;
    }
    
    // block
    if (match(TokenType::LBRACE)) {
        return parseBlock();
    }
    
    // 可能是变量声明或表达式
    return parseExpressionStatement();
}

Shared<PackageStmt> Parser::parsePackage() {
    consume();  // consume 'package'
    auto pkg = Shared<PackageStmt>(new PackageStmt());
    
    expect(TokenType::IDENTIFIER, "需要包名");
    pkg->name = current_.text;
    consume();
    
    // 支持点号分隔的包名
    while (match(TokenType::OP_DOT)) {
        consume();
        pkg->name += ".";
        expect(TokenType::IDENTIFIER, "需要标识符");
        pkg->name += current_.text;
        consume();
    }
    
    expect(TokenType::SEMICOLON, "需要 ';'");
    return pkg;
}

Shared<ImportStmt> Parser::parseImport() {
    consume();  // consume 'import'
    auto imp = Shared<ImportStmt>(new ImportStmt());
    
    // 支持点号分隔的模块名
    while (true) {
        // 先保存当前标识符，再调用expect()
        String namePart = current_.text;
        expect(TokenType::IDENTIFIER, "需要模块名");
        imp->moduleName += namePart;
        // expect()已经调用了consume()
        
        if (match(TokenType::OP_DOT)) {
            consume();
            imp->moduleName += ".";
        } else {
            break;
        }
    }
    
    // 可选的别名: import xxx as alias
    if (match(TokenType::IDENTIFIER) && current_.text == "as") {
        consume();
        String aliasName = current_.text;
        expect(TokenType::IDENTIFIER, "需要别名");
        imp->alias = aliasName;
        // expect()已经调用了consume()
    }
    
    expect(TokenType::SEMICOLON, "需要 ';'");
    return imp;
}
    
// === 类型解析 ===

Optional<String> Parser::parseType() {
    if (!match(TokenType::IDENTIFIER)) {
        return std::nullopt;
    }
    
    String type = current_.text;
    consume();
    
    // 处理泛型（支持嵌套，如 列表<对<整数, 字符串>>）
    if (match(TokenType::OP_LT)) {
        consume();
        type += "<";
        type += parseGenericTypeArg();

        while (match(TokenType::COMMA)) {
            consume();
            type += ", ";
            type += parseGenericTypeArg();
        }

        expect(TokenType::OP_GT, "需要 '>'");
        type += ">";
    }
    
    // 处理数组
    while (match(TokenType::LBRACKET)) {
        consume();
        expect(TokenType::RBRACKET, "需要 ']'");
        type += "[]";
    }
    
    return type;
}

// === 便捷函数 ===

Shared<Program> parseString(const String& source) {
    Lexer lexer(source);
    Parser parser(&lexer);
    return parser.parse();
}

Shared<Program> parseFile(const String& /*filename*/) {
    // TODO: 从文件读取
    return nullptr;
}

Shared<Expr> parseExprString(const String& source) {
    Lexer lexer(source);
    Parser parser(&lexer);
    return parser.parseExpression();
}

String Parser::parseGenericTypeArg() {
    // 解析泛型类型参数，支持嵌套泛型如 "对<整数, 字符串>"
    // 返回完整的类型参数字符串
    String typeArg = current_.text;
    expect(TokenType::IDENTIFIER, "需要类型名");

    // 检查后面是否有 < （嵌套泛型）
    if (match(TokenType::OP_LT)) {
        typeArg += "<";
        consume();  // consume '<'
        int depth = 1;

        while (depth > 0 && !match(TokenType::END_OF_FILE)) {
            if (match(TokenType::OP_GT)) {
                depth--;
                if (depth == 0) {
                    typeArg += ">";
                    consume();  // consume '>'
                    break;
                }
                typeArg += ">";
                consume();
            } else if (match(TokenType::OP_LT)) {
                depth++;
                typeArg += "<";
                consume();
            } else if (match(TokenType::COMMA)) {
                typeArg += ", ";
                consume();
            } else if (match(TokenType::IDENTIFIER)) {
                typeArg += current_.text;
                consume();
            } else {
                // 其他 token
                typeArg += current_.text;
                consume();
            }
        }

        if (depth != 0) {
            reportError("泛型类型参数缺少 '>'");
        }
    }

    return typeArg;
}

} // namespace cplang