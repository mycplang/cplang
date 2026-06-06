// CP语言 语法分析器实现 - 声明解析
#include "parser/parser.hpp"

namespace cplang {

// === 声明 ===

Shared<Stmt> Parser::parseDeclaration() {
    // const
    if (match(TokenType::K_CONST)) {
        return parseVariableDecl(true);
    }
    
    // 其他声明...
    return nullptr;
}

Shared<VarDeclStmt> Parser::parseVariableDecl(bool isConst, bool isLet) {
    // keyword token already consumed by parseStatement's match()
    auto decl = Shared<VarDeclStmt>(new VarDeclStmt());
    decl->isConst = isConst;

    // 变量名
    Token varToken = current_;  // 保存完整 token（含行号列号）
    String varName = current_.text;
    expect(TokenType::IDENTIFIER, "Expected variable name");
    decl->name = varName;
    decl->token = varToken;  // 设置 token 以便错误报告定位
    
    // 可选的类型注解
    if (match(TokenType::OP_COLON)) {
        consume();
        decl->type = parseType();
    }
    
    // 可选的初始化
    if (isLet) {
        // 设 x 为 10 语法
        if (match(TokenType::K_DO)) {  // "为" (注意：K_DO 映射到 "为")
            consume();
            decl->init = parseExpression();
        }
    } else {
        // 变量 x = 10 语法
        if (match(TokenType::OP_ASSIGN)) {
            consume();
            decl->init = parseExpression();
        }
    }
    
    expect(TokenType::SEMICOLON, "Expected ';' after variable declaration");
    return decl;
}

Shared<FuncDeclStmt> Parser::parseFunctionDecl() {
    consume();  // consume 'func'
    auto func = Shared<FuncDeclStmt>(new FuncDeclStmt());

    // function name
    Token funcToken = current_;  // save token with line/col BEFORE expect
    String funcName = current_.text;
    expect(TokenType::IDENTIFIER, "Expected function name");
    func->token = funcToken;
    func->name = funcName;

    // 泛型类型参数: 函数名<T: 约束, U, ...>
    if (match(TokenType::OP_LT)) {
        consume();  // consume '<'
        while (!match(TokenType::OP_GT) && !match(TokenType::END_OF_FILE)) {
            TypeParam tp;
            tp.name = current_.text;
            expect(TokenType::IDENTIFIER, "Expected type parameter name");
            // 可选约束: T: 约束名
            if (match(TokenType::OP_COLON)) {
                consume();
                tp.constraint = current_.text;
                expect(TokenType::IDENTIFIER, "Expected constraint/trait name after ':'");
            }
            func->typeParams.push_back(tp);
            if (match(TokenType::COMMA)) {
                consume();
            } else {
                break;
            }
        }
        expect(TokenType::OP_GT, "Expected '>' after generic type parameters");
    }

    // 参数列表
    expect(TokenType::LPAREN, "Expected '(' after function name");
    
    // 解析参数
    if (!match(TokenType::RPAREN)) {
        while (true) {
            String paramName = current_.text;  // save BEFORE expect
            expect(TokenType::IDENTIFIER, "Expected parameter name");
            
            Optional<String> paramType;
            if (match(TokenType::OP_COLON)) {
                consume();
                paramType = parseType();
            }
            
            func->params.push_back({paramName, paramType});
            
            if (match(TokenType::COMMA)) {
                consume();
            } else {
                break;
            }
        }
    }
    
    expect(TokenType::RPAREN, "Expected ')' after parameters");
    
    // 可选返回类型: 支持 -> 和 : 两种语法
    if (match(TokenType::OP_ARROW)) {
        consume();
        func->returnType = parseType();
    } else if (match(TokenType::OP_COLON)) {
        consume();
        func->returnType = parseType();
    }
    
    // 函数体
    func->body = parseBlock();
    
    return func;
}

Shared<ClassDeclStmt> Parser::parseClassDecl() {
    consume();  // consume 'class'
    auto cls = Shared<ClassDeclStmt>(new ClassDeclStmt());
    
    // 类名
    cls->name = current_.text;
    expect(TokenType::IDENTIFIER, "Expected class name");
    
    // 可选的基类
    if (match(TokenType::K_EXTENDS) || match(TokenType::K_SUPER)) {
        consume();
        expect(TokenType::IDENTIFIER, "Expected base class name");
        cls->baseClass = current_.text;
        consume();
    }
    
    // 类体
    expect(TokenType::LBRACE, "Expected '{' after class declaration");

    while (!match(TokenType::RBRACE) && !match(TokenType::END_OF_FILE)) {
        // 访问修饰符：public / private / protected
        if (match(TokenType::K_PUBLIC)) {
            consume();
            cls->currentAccess = AccessModifier::PUBLIC;
            continue;
        }
        if (match(TokenType::K_PRIVATE)) {
            consume();
            cls->currentAccess = AccessModifier::PRIVATE;
            continue;
        }
        if (match(TokenType::K_PROTECTED)) {
            consume();
            cls->currentAccess = AccessModifier::PROTECTED;
            continue;
        }

        auto member = parseStatement();
        if (member) {
            // 将当前访问级别应用到函数声明
            if (auto func = std::dynamic_pointer_cast<FuncDeclStmt>(member)) {
                func->access = cls->currentAccess;
            }
            cls->members.push_back(member);
        }
    }
    
    if (match(TokenType::RBRACE)) {
        consume();
    }
    
    return cls;
}

Shared<InterfaceDeclStmt> Parser::parseInterfaceDecl() {
    consume();  // consume 'interface'
    auto iface = Shared<InterfaceDeclStmt>(new InterfaceDeclStmt());
    
    expect(TokenType::IDENTIFIER, "Expected interface name");
    iface->name = current_.text;
    consume();
    
    expect(TokenType::LBRACE, "Expected '{' after interface declaration");
    
    while (!match(TokenType::RBRACE) && !match(TokenType::END_OF_FILE)) {
        auto method = parseFunctionDecl();
        if (method) {
            iface->methods.push_back(method);
        }
    }
    
    if (match(TokenType::RBRACE)) {
        consume();
    }
    
    return iface;
}

Shared<EnumDeclStmt> Parser::parseEnumDecl() {
    consume();  // consume 'enum'
    auto enm = Shared<EnumDeclStmt>(new EnumDeclStmt());
    
    enm->name = current_.text;
    expect(TokenType::IDENTIFIER, "Expected enum name");
    // expect already consumed the name token
    
    expect(TokenType::LBRACE, "Expected '{' after enum declaration");
    
    // 检查第一个变体是否带括号 → ADT 风格枚举
    bool firstIsADT = false;
    if (match(TokenType::IDENTIFIER) && peek_.type == TokenType::LPAREN) {
        firstIsADT = true;
    }
    
    if (firstIsADT) {
        // ADT 风格枚举: 枚举 结果 { 成功(字符串, 整数) | 失败(字符串) }
        enm->isADT = true;
        while (!match(TokenType::RBRACE) && !match(TokenType::END_OF_FILE)) {
            EnumVariantDef variant;
            variant.name = current_.text;
            expect(TokenType::IDENTIFIER, "Expected enum variant name");
            
            // 解析关联数据字段
            if (match(TokenType::LPAREN)) {
                consume();  // consume '('
                while (!match(TokenType::RPAREN) && !match(TokenType::END_OF_FILE)) {
                    String fieldName = current_.text;
                    expect(TokenType::IDENTIFIER, "Expected field name in variant");
                    
                    String fieldType = "auto";  // 默认类型
                    if (match(TokenType::OP_COLON)) {
                        consume();
                        auto typeOpt = parseType();
                        if (typeOpt.has_value()) {
                            fieldType = typeOpt.value();
                        }
                    }
                    variant.fields.push_back({fieldName, fieldType});
                    
                    if (match(TokenType::COMMA)) {
                        consume();
                    } else {
                        break;
                    }
                }
                expect(TokenType::RPAREN, "Expected ')' after variant fields");
            }
            
            enm->variants.push_back(variant);
            
            // 使用 '|' 作为 ADT 变体分隔符
            if (match(TokenType::OP_BIT_OR)) {
                consume();
            } else {
                break;
            }
        }
    } else {
        // 简单 C 风格枚举（向后兼容）
        Int64 value = 0;
        while (!match(TokenType::RBRACE) && !match(TokenType::END_OF_FILE)) {
            String name = current_.text;
            expect(TokenType::IDENTIFIER, "Expected enum value name");
            
            Optional<Int64> initValue;
            if (match(TokenType::OP_ASSIGN)) {
                consume();
                auto expr = parseExpression();
                // 从字面量提取值（LiteralExpr::value 是 variant）
                if (auto lit = std::dynamic_pointer_cast<LiteralExpr>(expr)) {
                    if (auto v = std::get_if<Int64>(&lit->value)) {
                        initValue = *v;
                        value = *v + 1;
                    }
                }
            } else {
                initValue = value++;
            }
            
            enm->values.push_back({name, initValue});
            
            if (match(TokenType::COMMA)) {
                consume();
            } else {
                break;
            }
        }
    }
    
    expect(TokenType::RBRACE, "Expected '}' after enum declaration");
    expect(TokenType::SEMICOLON, "Expected ';' after enum");
    
    return enm;
}

Shared<StructDeclStmt> Parser::parseStructDecl() {
    consume();  // consume 'struct'
    auto st = Shared<StructDeclStmt>(new StructDeclStmt());

    String structName = current_.text;  // save BEFORE expect consumes
    expect(TokenType::IDENTIFIER, "Expected struct name");
    st->name = structName;

    // 泛型类型参数: 结构体名<T: 约束, U, ...>
    if (match(TokenType::OP_LT)) {
        consume();  // consume '<'
        while (!match(TokenType::OP_GT) && !match(TokenType::END_OF_FILE)) {
            TypeParam tp;
            tp.name = current_.text;
            expect(TokenType::IDENTIFIER, "Expected type parameter name");
            // 可选约束: T: 约束名
            if (match(TokenType::OP_COLON)) {
                consume();
                tp.constraint = current_.text;
                expect(TokenType::IDENTIFIER, "Expected constraint/trait name after ':'");
            }
            st->typeParams.push_back(tp);
            if (match(TokenType::COMMA)) {
                consume();
            } else {
                break;
            }
        }
        expect(TokenType::OP_GT, "Expected '>' after generic type parameters");
    }

    expect(TokenType::LBRACE, "Expected '{' after struct declaration");
    
    while (!match(TokenType::RBRACE) && !match(TokenType::END_OF_FILE)) {
        auto decl = Shared<VarDeclStmt>(new VarDeclStmt());
        decl->isConst = false;
        
        // 支持三种格式：
        // 1. "type memberName;" - 显式类型
        // 2. "memberName;" - 无类型（待推导）
        // 3. "memberName = value;" - 类型推导（从初始值推导）
        if (match(TokenType::IDENTIFIER)) {
            if (peekIsIdentifier()) {
                // "type memberName;" 格式 - 显式类型
                decl->type = current_.text;
                consume();
                if (match(TokenType::IDENTIFIER)) {
                    decl->name = current_.text;
                    consume();
                } else {
                    reportError("类型后缺少成员名");
                    break;
                }
            } else {
                // "memberName;" 或 "memberName = value;" 格式
                decl->name = current_.text;
                consume();
                
                // 检查是否有初始值（类型推导）
                if (match(TokenType::OP_ASSIGN)) {
                    consume();  // consume '='
                    decl->init = parseExpression();  // 解析初始值，用于类型推导
                }
            }
        } else {
            reportError("结构体中缺少成员声明");
            break;
        }
        
        // 可选数组标记 [...]
        if (match(TokenType::LBRACKET)) {
            consume();
            expect(TokenType::RBRACKET, "Expected ']' after array marker");
            if (decl->type.has_value()) {
                decl->type = decl->type.value() + "[]";
            }
        }
        
        expect(TokenType::SEMICOLON, "Expected ';' after struct member");
        st->members.push_back(decl);
    }
    
    if (match(TokenType::RBRACE)) {
        consume();
    }
    
    // 结构体声明末尾分号可选（兼容 C 和 Go 风格）
    if (match(TokenType::SEMICOLON)) {
        consume();
    }
    
    return st;
}

} // namespace cplang
