// CP语言 语法分析器实现 - 表达式解析
#include "parser/parser.hpp"

namespace cplang {

// === 表达式 (递归下降) ===

Shared<Expr> Parser::parseExpression() {
    return parseAssignment();
}

Shared<Expr> Parser::parseAssignment() {
    auto left = parseTernary();
    
    if (match(TokenType::OP_ASSIGN) ||
        match(TokenType::OP_PLUS_ASSIGN) ||
        match(TokenType::OP_MINUS_ASSIGN) ||
        match(TokenType::OP_MUL_ASSIGN) ||
        match(TokenType::OP_DIV_ASSIGN) ||
        match(TokenType::OP_MOD_ASSIGN)) {
        
        TokenType op = current_.type;
        consume();
        auto right = parseAssignment();
        
        auto bin = Shared<BinaryExpr>(new BinaryExpr());
        bin->left = left;
        bin->op = op;
        bin->right = right;
        return bin;
    }
    
    return left;
}

Shared<Expr> Parser::parseTernary() {
    auto cond = parsePipe();
    
    if (match(TokenType::OP_QUESTION)) {
        consume();
        auto thenExpr = parseExpression();
        expect(TokenType::OP_COLON, "Expected ':' in ternary");
        auto elseExpr = parseTernary();
        
        auto ternary = Shared<BinaryExpr>(new BinaryExpr());
        ternary->left = thenExpr;
        ternary->op = TokenType::OP_QUESTION;
        ternary->right = elseExpr;
        
        // 包装成条件表达式
        auto result = Shared<BinaryExpr>(new BinaryExpr());
        result->left = cond;
        result->op = TokenType::OP_QUESTION;
        result->right = ternary;
        return result;
    }
    
    return cond;
}

Shared<Expr> Parser::parsePipe() {
    auto left = parseOr();

    while (match(TokenType::OP_PIPE)) {
        consume();  // consume |>
        auto right = parseOr();
        auto pipe = Shared<PipeExpr>(new PipeExpr());
        pipe->left = left;
        pipe->right = right;
        pipe->token = left->token;
        left = pipe;
    }

    return left;
}

Shared<Expr> Parser::parseOr() {
    auto left = parseAnd();
    
    while (match(TokenType::OP_OR) || match(TokenType::K_OR)) {
        TokenType op = current_.type;
        consume();
        // 将关键词转换为运算符 token
        if (op == TokenType::K_OR) op = TokenType::OP_OR;
        auto right = parseAnd();
        
        auto bin = Shared<BinaryExpr>(new BinaryExpr());
        bin->left = left;
        bin->op = op;
        bin->right = right;
        left = bin;
    }
    
    return left;
}

Shared<Expr> Parser::parseAnd() {
    auto left = parseBitOr();
    
    while (match(TokenType::OP_AND) || match(TokenType::K_AND)) {
        TokenType op = current_.type;
        consume();
        // 将关键词转换为运算符 token
        if (op == TokenType::K_AND) op = TokenType::OP_AND;
        auto right = parseBitOr();
        
        auto bin = Shared<BinaryExpr>(new BinaryExpr());
        bin->left = left;
        bin->op = op;
        bin->right = right;
        left = bin;
    }
    
    return left;
}

Shared<Expr> Parser::parseBitOr() {
    auto left = parseBitXor();
    
    while (match(TokenType::OP_BIT_OR)) {
        TokenType op = current_.type;
        consume();
        auto right = parseBitXor();
        
        auto bin = Shared<BinaryExpr>(new BinaryExpr());
        bin->left = left;
        bin->op = op;
        bin->right = right;
        left = bin;
    }
    
    return left;
}

Shared<Expr> Parser::parseBitXor() {
    auto left = parseBitAnd();
    
    while (match(TokenType::OP_BIT_XOR)) {
        TokenType op = current_.type;
        consume();
        auto right = parseBitAnd();
        
        auto bin = Shared<BinaryExpr>(new BinaryExpr());
        bin->left = left;
        bin->op = op;
        bin->right = right;
        left = bin;
    }
    
    return left;
}

Shared<Expr> Parser::parseBitAnd() {
    auto left = parseEquality();
    
    while (match(TokenType::OP_BIT_AND)) {
        TokenType op = current_.type;
        consume();
        auto right = parseEquality();
        
        auto bin = Shared<BinaryExpr>(new BinaryExpr());
        bin->left = left;
        bin->op = op;
        bin->right = right;
        left = bin;
    }
    
    return left;
}

Shared<Expr> Parser::parseEquality() {
    auto left = parseComparison();
    
    while (match(TokenType::OP_EQ) || match(TokenType::OP_NE) ||
           match(TokenType::K_EQ) || match(TokenType::K_NE)) {
        TokenType op = current_.type;
        consume();
        // 将关键词转换为运算符 token
        if (op == TokenType::K_EQ) op = TokenType::OP_EQ;
        if (op == TokenType::K_NE) op = TokenType::OP_NE;
        auto right = parseComparison();
        
        auto bin = Shared<BinaryExpr>(new BinaryExpr());
        bin->left = left;
        bin->op = op;
        bin->right = right;
        left = bin;
    }
    
    return left;
}

Shared<Expr> Parser::parseComparison() {
    auto left = parseShift();
    
    while (match(TokenType::OP_LT) || match(TokenType::OP_GT) ||
           match(TokenType::OP_LE) || match(TokenType::OP_GE) ||
           match(TokenType::K_LT) || match(TokenType::K_GT) ||
           match(TokenType::K_LE) || match(TokenType::K_GE)) {
        TokenType op = current_.type;
        consume();
        // 将关键词转换为运算符 token
        if (op == TokenType::K_LT) op = TokenType::OP_LT;
        if (op == TokenType::K_GT) op = TokenType::OP_GT;
        if (op == TokenType::K_LE) op = TokenType::OP_LE;
        if (op == TokenType::K_GE) op = TokenType::OP_GE;
        auto right = parseShift();
        
        auto bin = Shared<BinaryExpr>(new BinaryExpr());
        bin->left = left;
        bin->op = op;
        bin->right = right;
        left = bin;
    }
    
    return left;
}

Shared<Expr> Parser::parseShift() {
    auto left = parseAdditive();

    while (match(TokenType::OP_LSHIFT) ||
           (match(TokenType::OP_GT) && peek_.type == TokenType::OP_GT)) {
        TokenType op;
        if (match(TokenType::OP_LSHIFT)) {
            op = TokenType::OP_LSHIFT;
        } else {
            op = TokenType::OP_RSHIFT;
            consume();  // consume first >
        }
        consume();  // consume LSHIFT or second >
        auto right = parseAdditive();

        auto bin = Shared<BinaryExpr>(new BinaryExpr());
        bin->left = left;
        bin->op = op;
        bin->right = right;
        left = bin;
    }
    
    return left;
}

Shared<Expr> Parser::parseAdditive() {
    auto left = parseMultiplicative();
    
    while (match(TokenType::OP_PLUS) || match(TokenType::OP_MINUS)) {
        TokenType op = current_.type;
        consume();
        auto right = parseMultiplicative();
        
        auto bin = Shared<BinaryExpr>(new BinaryExpr());
        bin->left = left;
        bin->op = op;
        bin->right = right;
        left = bin;
    }
    
    return left;
}

Shared<Expr> Parser::parseMultiplicative() {
    auto left = parseUnary();
    
    while (match(TokenType::OP_MUL) || match(TokenType::OP_DIV) || match(TokenType::OP_MOD)) {
        TokenType op = current_.type;
        consume();
        auto right = parseUnary();
        
        auto bin = Shared<BinaryExpr>(new BinaryExpr());
        bin->left = left;
        bin->op = op;
        bin->right = right;
        left = bin;
    }
    
    return left;
}

Shared<Expr> Parser::parseUnary() {
    if (match(TokenType::OP_MINUS) || match(TokenType::OP_NOT) ||
        match(TokenType::OP_INC) || match(TokenType::OP_DEC) ||
        match(TokenType::K_NOT)) {

        TokenType op = current_.type;
        consume();
        // 将关键词转换为运算符 token
        if (op == TokenType::K_NOT) op = TokenType::OP_NOT;
        auto operand = parseUnary();

        auto unary = Shared<UnaryExpr>(new UnaryExpr());
        unary->op = op;
        unary->operand = operand;
        return unary;
    }

    return parsePostfix();
}

Shared<Expr> Parser::parsePostfix() {
    auto expr = parsePrimary();

    while (true) {
        if (match(TokenType::OP_DOT)) {
            consume();
            String memberName = current_.text;  // save BEFORE expect consumes
            expect(TokenType::IDENTIFIER, "Expected member name after '.'");

            auto member = Shared<MemberExpr>(new MemberExpr());
            member->object = expr;
            member->member = memberName;
            expr = member;
        }
        else if (match(TokenType::OP_LT)) {
            // 泛型函数调用: 函数名<类型1, 类型2>(参数...)
            // 使用 lookahead 消除与 < 比较运算符的歧义
            bool isGenericCall = false;
            if (std::dynamic_pointer_cast<IdentifierExpr>(expr)) {
                // 检查 peek_ 是否为类型名（在 CP 语言中，类型名都是 IDENTIFIER 或内置类型名）
                if (peek_.type == TokenType::IDENTIFIER) {
                    if (peek2_.type == TokenType::COMMA) {
                        // 模式: < 类型 , → 泛型多参数，肯定是泛型调用
                        isGenericCall = true;
                    } else if (peek2_.type == TokenType::OP_GT) {
                        // 模式: < 类型 > → 检查后面是否是 ( 或 { 来判定
                        if (peek3_.type == TokenType::LPAREN || peek3_.type == TokenType::LBRACE) {
                            isGenericCall = true;
                        }
                        // 否则可能是 a < b > c 这样的比较链
                    } else if (peek2_.type == TokenType::OP_LT) {
                        // 模式: < 类型 < → 嵌套泛型，肯定是泛型调用
                        isGenericCall = true;
                    }
                }
            }

            if (isGenericCall) {
                consume();  // consume '<'
                auto call = Shared<CallExpr>(new CallExpr());
                call->callee = expr;
                call->token = expr->token;

                // 解析泛型类型参数列表（支持嵌套，如 排序<对<整数, 字符串>>）
                while (!match(TokenType::OP_GT) && !match(TokenType::END_OF_FILE)) {
                    String typeName = parseGenericTypeArg();
                    call->typeArgs.push_back(typeName);

                    if (match(TokenType::COMMA)) {
                        consume();
                    } else {
                        break;
                    }
                }
                expect(TokenType::OP_GT, "Expected '>' after generic type arguments");

                // 检查后面是 ( 函数调用 还是 { 结构体字面量
                if (match(TokenType::LPAREN)) {
                    consume();  // consume '('
                    if (!match(TokenType::RPAREN)) {
                        while (true) {
                            call->arguments.push_back(parseExpression());
                            if (match(TokenType::COMMA)) {
                                consume();
                            } else {
                                break;
                            }
                        }
                    }
                    expect(TokenType::RPAREN, "Expected ')' after arguments");
                    expr = call;
                } else if (match(TokenType::LBRACE)) {
                    // 泛型结构体字面量: TypeName<类型>{field: value, ...}
                    consume();  // consume '{'
                    auto structLit = Shared<StructLiteralExpr>(new StructLiteralExpr());

                    // 从标识符和类型参数构造完整类型名
                    if (auto id = std::dynamic_pointer_cast<IdentifierExpr>(expr)) {
                        String genericName = id->name + "<";
                        for (size_t i = 0; i < call->typeArgs.size(); i++) {
                            if (i > 0) genericName += ", ";
                            genericName += call->typeArgs[i];
                        }
                        genericName += ">";
                        structLit->structName = genericName;
                        structLit->token = id->token;  // 设置 token 以便错误报告定位
                    }

                    if (!match(TokenType::RBRACE)) {
                        while (true) {
                            if (!match(TokenType::IDENTIFIER)) {
                                reportError("结构体字面量中缺少字段名");
                                break;
                            }
                            String fieldName = current_.text;
                            consume();

                            expect(TokenType::OP_COLON, "Expected ':' after field name");
                            Shared<Expr> value = parseExpression();
                            structLit->fields.push_back({fieldName, value});

                            if (match(TokenType::COMMA)) {
                                consume();
                            } else {
                                break;
                            }
                        }
                    }
                    expect(TokenType::RBRACE, "Expected '}' after struct literal");
                    expr = structLit;
                } else {
                    reportError("Expected '(' or '{' after generic type arguments");
                    expr = call;
                }
            } else {
                // 不是泛型调用，退出后交由 parseComparison 处理 < 运算符
                break;
            }
        }
        else if (match(TokenType::LPAREN)) {
            // 函数调用
            consume();
            auto call = Shared<CallExpr>(new CallExpr());
            call->callee = expr;
            call->token = expr->token;  // 继承被调用者的行号

            if (!match(TokenType::RPAREN)) {
                while (true) {
                    call->arguments.push_back(parseExpression());
                    if (match(TokenType::COMMA)) {
                        consume();
                    } else {
                        break;
                    }
                }
            }

            expect(TokenType::RPAREN, "Expected ')' after arguments");
            expr = call;
        }
        else if (match(TokenType::LBRACKET)) {
            // 数组访问
            consume();
            auto idx = Shared<IndexExpr>(new IndexExpr());
            idx->array = expr;
            idx->index = parseExpression();
            expect(TokenType::RBRACKET, "Expected ']' after index");
            expr = idx;
        }
        else if (match(TokenType::OP_INC) || match(TokenType::OP_DEC)) {
            // 后置 ++ --
            TokenType op = current_.type;
            consume();

            auto unary = Shared<UnaryExpr>(new UnaryExpr());
            unary->op = op;
            unary->operand = expr;
            unary->isPostfix = true;
            expr = unary;
        }
        else {
            break;
        }
    }

    return expr;
}

Shared<Expr> Parser::parsePrimary() {
    // Lambda表达式: |x, y| { ... } 或 |x, y| expr
    if (match(TokenType::OP_BIT_OR)) {
        return parseLambda();
    }
    
    // 括号 (可能是 lambda: (x, y) => { ... } 或普通括号表达式)
    if (match(TokenType::LPAREN)) {
        // 用 lookahead 判断是否是 lambda: (标识符 ...) =>
        // 模式: ( ) => 或 ( 标识符  : 或 , 或 ) 然后 =>
        bool isLambda = false;
        if (peek_.type == TokenType::RPAREN) {
            // () => 模式
            if (peek2_.type == TokenType::OP_FAT_ARROW) {
                isLambda = true;
            }
        } else if (peek_.type == TokenType::IDENTIFIER) {
            // (标识符 ...) => 模式
            // 需要扫描到匹配的 )，然后检查后面是否是 =>
            // 简化：检查 peek_ 是否为标识符，且后面有 : 或 , 或 )
            if (peek2_.type == TokenType::OP_COLON || peek2_.type == TokenType::COMMA) {
                // 可能为 lambda，扫描到 ) 后检查 =>
                int parenDepth = 1;
                size_t lookaheadIdx = 1; // 已跳过 '(' 和 peek_[0]
                // 我们需要扫描 tokens 来找到匹配的 )
                // 由于没有 Token 流缓存，使用简化的启发式判断：
                // 如果 peek_ 是标识符且 peek2_ 是 : 或 , 或 )，则认为是 lambda 参数列表的开始
                isLambda = true;
            }
        }
        
        if (isLambda) {
            return parseLambda();
        }
        
        consume();
        auto expr = parseExpression();
        expect(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }
    
    // new 表达式
    // await / 等待 — 等待异步操作
    if (match(TokenType::K_AWAIT)) {
        consume();
        auto awaitExpr = Shared<AwaitExpr>(new AwaitExpr());
        awaitExpr->target = parsePrimary();
        return awaitExpr;
    }

    // this / 这个 — 引用当前实例
    if (match(TokenType::K_THIS)) {
        consume();
        return Shared<ThisExpr>(new ThisExpr());
    }

    // super / 继承 — 调用父类方法
    if (match(TokenType::K_SUPER)) {
        consume();
        auto superExpr = Shared<SuperExpr>(new SuperExpr());
        if (match(TokenType::OP_DOT)) {
            consume();
            superExpr->method = current_.text;
            expect(TokenType::IDENTIFIER, "需要父类方法名");
        }
        expect(TokenType::LPAREN, "super 调用需要 '('");
        if (!match(TokenType::RPAREN)) {
            while (true) {
                superExpr->arguments.push_back(parseExpression());
                if (match(TokenType::COMMA)) consume();
                else break;
            }
        }
        expect(TokenType::RPAREN, "需要 ')'");
        return superExpr;
    }

    if (match(TokenType::K_NEW)) {
        consume();  // consume 'new'
        auto newExpr = Shared<NewExpr>(new NewExpr());
        newExpr->className = current_.text;
        expect(TokenType::IDENTIFIER, "Expected class name after 'new'");
        expect(TokenType::LPAREN, "Expected '(' after class name");
        
        if (!match(TokenType::RPAREN)) {
            while (true) {
                newExpr->args.push_back(parseExpression());
                if (match(TokenType::COMMA)) {
                    consume();
                } else {
                    break;
                }
            }
        }
        expect(TokenType::RPAREN, "Expected ')' after constructor args");
        return newExpr;
    }
    
    // true / false
    if (match(TokenType::K_TRUE)) {
        consume();
        auto lit = Shared<LiteralExpr>(new LiteralExpr());
        lit->value = true;
        return lit;
    }
    
    if (match(TokenType::K_FALSE)) {
        consume();
        auto lit = Shared<LiteralExpr>(new LiteralExpr());
        lit->value = false;
        return lit;
    }
    
    // null
    if (match(TokenType::K_NULL)) {
        consume();
        auto lit = Shared<LiteralExpr>(new LiteralExpr());
        lit->value = String("null");
        return lit;
    }
    
    // 数组字面量 [1, 2, 3]
    if (match(TokenType::LBRACKET)) {
        consume();
        auto arr = Shared<ArrayExpr>(new ArrayExpr());
        if (!match(TokenType::RBRACKET)) {
            while (true) {
                arr->elements.push_back(parseExpression());
                if (match(TokenType::COMMA)) {
                    consume();
                } else {
                    break;
                }
            }
        }
        expect(TokenType::RBRACKET, "Expected ']' after array elements");
        return arr;
    }
    
    // 匿名表/对象字面量 {} 或 {key: value, ...}
    if (match(TokenType::LBRACE)) {
        consume();  // consume '{'
        auto tableLit = Shared<StructLiteralExpr>(new StructLiteralExpr());
        tableLit->structName = "";  // 空名字 = 匿名表
        
        if (!match(TokenType::RBRACE)) {
            while (true) {
                // 字段名
                String fieldName = current_.text;
                if (!match(TokenType::IDENTIFIER)) {
                    reportError("表字面量中缺少字段名");
                    break;
                }
                consume();
                
                // 冒号
                expect(TokenType::OP_COLON, "Expected ':' after field name");
                
                // 字段值
                Shared<Expr> value = parseExpression();
                tableLit->fields.push_back({fieldName, value});
                
                if (match(TokenType::COMMA)) {
                    consume();
                } else {
                    break;
                }
            }
        }
        
        expect(TokenType::RBRACE, "Expected '}' after table literal");
        return tableLit;
    }
    
    // 数字
    if (match(TokenType::INTEGER)) {
        auto lit = Shared<LiteralExpr>(new LiteralExpr());
        if (auto val = std::get_if<Int64>(&current_.value)) {
            lit->value = *val;
        } else {
            lit->value = Int64(0);
        }
        consume();
        return lit;
    }
    
    if (match(TokenType::FLOAT)) {
        auto lit = Shared<LiteralExpr>(new LiteralExpr());
        if (auto val = std::get_if<Float64>(&current_.value)) {
            lit->value = *val;
        } else {
            lit->value = Float64(0.0);
        }
        consume();
        return lit;
    }
    
    // 字符串（支持 ${} 插值）
    if (match(TokenType::STRING)) {
        String strVal;
        if (auto val = std::get_if<String>(&current_.value)) {
            strVal = *val;
        }
        consume();

        // 检测是否包含 ${} 插值
        if (strVal.find("${") == String::npos) {
            // 普通字符串
            auto lit = Shared<LiteralExpr>(new LiteralExpr());
            lit->value = strVal;
            return lit;
        }

        // 插值字符串：拆分为交替字符串部分和表达式部分，用 + 连接
        std::vector<Shared<Expr>> parts;
        size_t pos = 0;
        while (pos < strVal.size()) {
            size_t dollarBrace = strVal.find("${", pos);
            if (dollarBrace == String::npos) {
                // 剩余部分为普通字符串
                if (pos < strVal.size()) {
                    auto lit = Shared<LiteralExpr>(new LiteralExpr());
                    lit->value = strVal.substr(pos);
                    parts.push_back(lit);
                }
                break;
            }
            // ${ 前面的字符串部分
            if (dollarBrace > pos) {
                auto lit = Shared<LiteralExpr>(new LiteralExpr());
                lit->value = strVal.substr(pos, dollarBrace - pos);
                parts.push_back(lit);
            }
            // 查找匹配的 }
            size_t closeBrace = strVal.find('}', dollarBrace + 2);
            if (closeBrace == String::npos) {
                // 未闭合的 ${ — 按普通字符串处理
                auto lit = Shared<LiteralExpr>(new LiteralExpr());
                lit->value = strVal.substr(pos);
                parts.push_back(lit);
                break;
            }
            // 提取表达式文本并解析
            String exprText = strVal.substr(dollarBrace + 2, closeBrace - dollarBrace - 2);
            if (!exprText.empty()) {
                Shared<Expr> expr = parseExprString(exprText);
                if (expr) parts.push_back(expr);
            }
            pos = closeBrace + 1;
        }

        if (parts.empty()) {
            auto lit = Shared<LiteralExpr>(new LiteralExpr());
            lit->value = String("");
            return lit;
        }
        if (parts.size() == 1) return parts[0];

        // 用 + 连接各部分
        Shared<Expr> result = parts[0];
        for (size_t i = 1; i < parts.size(); i++) {
            auto bin = Shared<BinaryExpr>(new BinaryExpr());
            bin->left = result;
            bin->op = TokenType::OP_PLUS;
            bin->right = parts[i];
            result = bin;
        }
        return result;
    }
    
    // 标识符或结构体字面量
    if (match(TokenType::IDENTIFIER)) {
        Token idToken = current_;  // 保存标识符 token（含行号列号）
        String name = current_.text;
        consume();

        // 泛型结构体字面量由 parsePostfix 处理（此处不处理 < 运算符）

        // 检查是否是结构体字面量: TypeName{field: value, ...}
        // 但需要排除匹配语句: 匹配 x { ... } — { 后跟情况/其他 关键字
        if (match(TokenType::LBRACE) &&
            peek_.type != TokenType::K_CASE &&
            peek_.type != TokenType::K_DEFAULT) {
            consume();  // consume '{'
            auto structLit = Shared<StructLiteralExpr>(new StructLiteralExpr());
            structLit->structName = name;
            structLit->token = idToken;  // 设置 token 以便错误报告定位

            // 解析字段列表
            if (!match(TokenType::RBRACE)) {
                while (true) {
                    // 字段名
                    if (!match(TokenType::IDENTIFIER)) {
                        reportError("结构体字面量中缺少字段名");
                        break;
                    }
                    String fieldName = current_.text;
                    consume();

                    // 冒号
                    expect(TokenType::OP_COLON, "Expected ':' after field name");

                    // 字段值
                    Shared<Expr> value = parseExpression();
                    structLit->fields.push_back({fieldName, value});

                    if (match(TokenType::COMMA)) {
                        consume();
                    } else {
                        break;
                    }
                }
            }

            expect(TokenType::RBRACE, "Expected '}' after struct literal");
            return structLit;
        }

        // 普通标识符
        auto id = Shared<IdentifierExpr>(new IdentifierExpr());
        id->name = name;
        id->token = idToken;  // 使用保存的 token（含正确的行号列号）
        return id;
    }
    
    // 错误
    reportError("未预期的符号: " + current_.text);
    consume();
    return Shared<IdentifierExpr>(new IdentifierExpr());
}

Shared<Expr> Parser::parseLambda() {
    // 语法: |参数1, 参数2: 类型| { 语句; ... }  或  |参数1, 参数2| 表达式
    // 语法: (参数1, 参数2: 类型) => { 语句; ... }  或  (参数1, 参数2) => 表达式
    // 语法: () => { 语句; ... }  或  () => 表达式
    
    bool isArrowSyntax = match(TokenType::LPAREN);  // ( ) => { } 语法
    
    if (isArrowSyntax) {
        consume();  // consume '('
    } else {
        // | 语法
        consume();  // consume '|'
    }
    
    auto lambda = Shared<LambdaExpr>(new LambdaExpr());
    lambda->token = current_;  // 记录位置信息
    
    // 解析参数列表
    if (!isArrowSyntax || !match(TokenType::RPAREN)) {
        while (true) {
            if (isArrowSyntax && match(TokenType::RPAREN)) break;
            if (!isArrowSyntax && match(TokenType::OP_BIT_OR)) break;
            if (match(TokenType::END_OF_FILE)) {
                reportError("Lambda表达式参数列表未关闭");
                return lambda;
            }
            
            // 参数名
            String paramName = current_.text;
            expect(TokenType::IDENTIFIER, "Lambda参数需要标识符名称");
            
            // 可选类型注解
            Optional<String> paramType;
            if (match(TokenType::OP_COLON)) {
                consume();  // consume ':'
                paramType = parseType();
            }
            
            lambda->params.push_back({paramName, paramType});
            
            // 逗号继续
            if (match(TokenType::COMMA)) {
                consume();
                continue;
            } else {
                break;
            }
        }
    }
    
    // 关闭分隔符
    if (isArrowSyntax) {
        expect(TokenType::RPAREN, "Expected ')' after lambda parameters");
        expect(TokenType::OP_FAT_ARROW, "Expected '=>' after lambda parameters");
    } else {
        expect(TokenType::OP_BIT_OR, "Expected '|' after lambda parameters");
    }
    
    // 解析函数体
    if (match(TokenType::LBRACE)) {
        // 块体: { 语句; ... }
        lambda->body = parseBlock();
    } else {
        // 表达式体: expr (自动包装为 return)
        auto bodyBlock = Shared<BlockStmt>(new BlockStmt());
        auto retStmt = Shared<ReturnStmt>(new ReturnStmt());
        retStmt->value = parseExpression();
        bodyBlock->statements.push_back(retStmt);
        lambda->body = bodyBlock;
    }
    
    return lambda;
}

} // namespace cplang
