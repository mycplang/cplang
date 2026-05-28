// CP语言 语法分析器实现 - 语句解析
#include "parser/parser.hpp"

namespace cplang {

// === 语句 ===

Shared<BlockStmt> Parser::parseBlock() {
    consume();  // consume '{'
    auto block = Shared<BlockStmt>(new BlockStmt()); block->token = current_;
    
    while (!match(TokenType::RBRACE) && !match(TokenType::END_OF_FILE)) {
        auto stmt = parseStatement();
        if (stmt) {
            block->statements.push_back(stmt);
        }
    }
    
    if (match(TokenType::RBRACE)) {
        consume();
    }
    
    return block;
}

Shared<Stmt> Parser::parseIfStatement() {
    consume();  // consume 'if'
    auto ifStmt = Shared<IfStmt>(new IfStmt()); ifStmt->token = current_;
    
    // 双语语法: 如果 x > 0 则 { ... }
    // 兼容旧语法: 如果 (x > 0) { ... }
    ifStmt->condition = parseExpression();
    
    // 如果条件后紧跟 "则"，消费它
    if (match(TokenType::K_THEN)) {
        consume();
    }
    
    ifStmt->thenBranch = parseStatement();
    
    if (match(TokenType::K_ELSE)) {
        consume();
        ifStmt->elseBranch = parseStatement();
    }
    
    return ifStmt;
}

Shared<Stmt> Parser::parseSwitchStatement() {
    consume();  // consume 'switch'
    auto sw = Shared<SwitchStmt>(new SwitchStmt()); sw->token = current_;
    
    expect(TokenType::LPAREN, "Expected '(' after 'switch'");
    sw->expr = parseExpression();
    expect(TokenType::RPAREN, "Expected ')' after expression");
    
    expect(TokenType::LBRACE, "Expected '{' after 'switch'");
    
    while (!match(TokenType::RBRACE) && !match(TokenType::END_OF_FILE)) {
        if (match(TokenType::K_CASE)) {
            consume();
            auto caseExpr = parseExpression();
            expect(TokenType::OP_COLON, "Expected ':' after case expression");
            if (!match(TokenType::LBRACE)) {
                reportError("case后缺少 {");
                return sw;
            }
            auto caseBody = parseBlock();
            sw->cases.push_back({caseExpr, caseBody});
        } else if (match(TokenType::K_DEFAULT)) {
            consume();
            expect(TokenType::OP_COLON, "Expected ':' after 'default'");
            if (!match(TokenType::LBRACE)) {
                reportError("default后缺少 {");
                return sw;
            }
            sw->defaultCase = parseBlock();
        } else {
            consume();
        }
    }
    
    if (match(TokenType::RBRACE)) {
        consume();
    }
    
    return sw;
}

Shared<Stmt> Parser::parseForStatement() {
    consume();  // consume 'for'
    
    expect(TokenType::LPAREN, "Expected '(' after 'for'");
    
    // 检测 for-each 旧语法: 循环 (x : arr) { ... }
    if (match(TokenType::IDENTIFIER) && peek_.type == TokenType::OP_COLON) {
        auto forEachStmt = Shared<ForEachStmt>(new ForEachStmt());
        forEachStmt->varName = current_.text;
        consume();  // consume variable name
        consume();  // consume ':'
        forEachStmt->iterable = parseExpression();
        expect(TokenType::RPAREN, "Expected ')' after for-each iterable");
        forEachStmt->body = parseStatement();
        return forEachStmt;
    }
    
    // 经典 for 循环 (init; condition; update)
    auto forStmt = Shared<ForStmt>(new ForStmt()); forStmt->token = current_;
    
    // 初始化
    if (!match(TokenType::SEMICOLON)) {
        forStmt->init = parseStatement();
    } else {
        consume();
    }
    
    // 条件
    if (!match(TokenType::SEMICOLON)) {
        forStmt->condition = parseExpression();
    }
    expect(TokenType::SEMICOLON, "Expected ';' after for condition");
    
    // 更新
    if (!match(TokenType::RPAREN)) {
        forStmt->update = parseExpression();
    }
    expect(TokenType::RPAREN, "Expected ')' after for clauses");
    
    // 循环体
    forStmt->body = parseStatement();
    
    return forStmt;
}

Shared<Stmt> Parser::parseForEachStatement() {
    consume();  // consume '遍历'
    auto forEachStmt = Shared<ForEachStmt>(new ForEachStmt());
    
    // 双语新语法: 遍历 x 在 arr { ... }
    // 兼容旧语法: 遍历 (x : arr) { ... }
    
    // 检查旧语法 (LPAREN first): 遍历 (x : arr)
    if (match(TokenType::LPAREN)) {
        consume();  // consume '('
        forEachStmt->varName = current_.text;
        expect(TokenType::IDENTIFIER, "Expected variable name in for-each");
        expect(TokenType::OP_COLON, "Expected ':' after variable name");
        forEachStmt->iterable = parseExpression();
        expect(TokenType::RPAREN, "Expected ')' after iterable");
    }
    // 新语法: 遍历 x 在 arr { ... }
    else if (match(TokenType::IDENTIFIER)) {
        forEachStmt->varName = current_.text;
        consume();
        
        // 检查 "在"
        if (match(TokenType::K_IN)) {
            consume();  // consume '在'
            
            // 特殊处理: 标识符后紧跟{ → 循环体开始，不是结构体字面量
            // 例: 遍历 n 在 arr { ... }
            if (match(TokenType::IDENTIFIER) && peek_.type == TokenType::LBRACE) {
                auto idExpr = Shared<IdentifierExpr>(new IdentifierExpr());
                idExpr->name = current_.text;
                consume();  // consume identifier
                forEachStmt->iterable = idExpr;
            } else {
                forEachStmt->iterable = parseExpression();
            }
        } else {
            reportError("Expected '在' after variable name in for-each");
            return forEachStmt;
        }
    } else {
        reportError("for-each中缺少变量名或(");
        return forEachStmt;
    }
    
    // 循环体
    forEachStmt->body = parseStatement();
    
    return forEachStmt;
}

Shared<Stmt> Parser::parseWhileStatement() {
    consume();  // consume 'while'
    auto whileStmt = Shared<WhileStmt>(new WhileStmt()); whileStmt->token = current_;
    
    // 双语语法: 当 i < n 时 { ... }
    // 兼容旧语法: 当 (i < n) { ... }
    whileStmt->condition = parseExpression();
    
    // 如果条件后紧跟 "时"，消费它
    if (match(TokenType::K_WHILE_MARK)) {  // "时"
        consume();
    }
    
    whileStmt->body = parseStatement();
    
    return whileStmt;
}

Shared<Stmt> Parser::parseDoWhileStatement() {
    consume();  // consume 'do'
    auto doStmt = Shared<DoWhileStmt>(new DoWhileStmt()); doStmt->token = current_;
    
    doStmt->body = parseStatement();
    
    expect(TokenType::K_WHILE, "Expected 'while' after do body");
    expect(TokenType::LPAREN, "Expected '(' after 'while'");
    doStmt->condition = parseExpression();
    expect(TokenType::RPAREN, "Expected ')' after condition");
    expect(TokenType::SEMICOLON, "Expected ';' after do-while");
    
    return doStmt;
}

Shared<ReturnStmt> Parser::parseReturnStatement() {
    consume();  // consume 'return'
    auto ret = Shared<ReturnStmt>(new ReturnStmt()); ret->token = current_;
    
    if (!match(TokenType::SEMICOLON)) {
        ret->value = parseExpression();
    }
    
    expect(TokenType::SEMICOLON, "Expected ';' after return");
    return ret;
}

Shared<BreakStmt> Parser::parseBreakStatement() {
    Token tok = current_;  // save break token with line/col
    consume();
    expect(TokenType::SEMICOLON, "Expected ';' after break");
    auto stmt = Shared<BreakStmt>(new BreakStmt());
    stmt->token = tok;
    return stmt;
}

Shared<ContinueStmt> Parser::parseContinueStatement() {
    Token tok = current_;  // save continue token with line/col
    consume();
    expect(TokenType::SEMICOLON, "Expected ';' after continue");
    auto stmt = Shared<ContinueStmt>(new ContinueStmt());
    stmt->token = tok;
    return stmt;
}

Shared<ThrowStmt> Parser::parseThrowStatement() {
    consume();  // consume 'throw'
    auto thr = Shared<ThrowStmt>(new ThrowStmt());
    
    thr->exception = parseExpression();
    expect(TokenType::SEMICOLON, "Expected ';' after throw");
    
    return thr;
}

Shared<TryStmt> Parser::parseTryStatement() {
    consume();  // consume 'try'
    auto tr = Shared<TryStmt>(new TryStmt());
    
    tr->tryBlock = parseBlock();
    
    // catch 块
    while (match(TokenType::K_CATCH)) {
        consume();
        expect(TokenType::LPAREN, "Expected '(' after 'catch'");
        String exName;
        if (match(TokenType::IDENTIFIER)) {
            exName = current_.text;
            consume();
        }
        expect(TokenType::RPAREN, "Expected ')' after exception name");
        auto handler = parseBlock();
        tr->catchBlocks.push_back({exName, handler});
    }
    
    // finally 块
    if (match(TokenType::K_FINALLY)) {
        consume();
        tr->finallyBlock = parseBlock();
    }

    return tr;
}

// ═══════════════════════════════════════════════════════════════
//  defer 语句 — 推迟执行，离开作用域时逆序执行
//  语法: 推迟 { 语句... }
//         推迟 语句;
// ═══════════════════════════════════════════════════════════════
Shared<Stmt> Parser::parseDeferStatement() {
    consume();  // consume 'defer'
    auto deferStmt = Shared<DeferStmt>(new DeferStmt());

    // 推迟可以带块 { ... } 或单条语句
    if (match(TokenType::LBRACE)) {
        deferStmt->body = parseBlock();
    } else {
        deferStmt->body = parseStatement();
    }

    return deferStmt;
}

Shared<Stmt> Parser::parseExpressionStatement() {
    if (match(TokenType::IDENTIFIER) && peek_.type == TokenType::OP_ASSIGN) {
        Token nameToken = current_;
        String name = current_.text;
        consume(); consume();
        auto decl = Shared<VarDeclStmt>(new VarDeclStmt());
        decl->token = nameToken; decl->name = name;
        decl->isConst = false; decl->isImplicit = true;
        if (!match(TokenType::SEMICOLON)) decl->init = parseExpression();
        if (match(TokenType::SEMICOLON)) consume();
        return decl;
    }
    auto expr = parseExpression();
    if (match(TokenType::SEMICOLON)) consume();
    auto stmt = Shared<ExprStmt>(new ExprStmt()); stmt->expr = expr;
    stmt->token = expr->token;  // 继承表达式的行号
    return stmt;
}

} // namespace cplang
