// CP语言 主入口 - 测试程序
#include "common/types.hpp"
#include "lexer/token.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "ast/ast.hpp"
#include <iostream>
#include <sstream>

using namespace cplang;

String tokenTypeToString(TokenType t) {
    switch (t) {
        case TokenType::END_OF_FILE: return "EOF";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::INTEGER: return "INTEGER";
        case TokenType::FLOAT: return "FLOAT";
        case TokenType::STRING: return "STRING";
        case TokenType::K_TRUE: return "TRUE";
        case TokenType::K_FALSE: return "FALSE";
        case TokenType::K_NULL: return "NULL";
        case TokenType::K_PACKAGE: return "PACKAGE";
        case TokenType::K_IMPORT: return "IMPORT";
        case TokenType::K_CLASS: return "CLASS";
        case TokenType::K_INTERFACE: return "INTERFACE";
        case TokenType::K_ENUM: return "ENUM";
        case TokenType::K_STRUCT: return "STRUCT";
        case TokenType::K_FUNC: return "FUNC";
        case TokenType::K_RETURN: return "RETURN";
        case TokenType::K_IF: return "IF";
        case TokenType::K_ELSE: return "ELSE";
        case TokenType::K_SWITCH: return "SWITCH";
        case TokenType::K_CASE: return "CASE";
        case TokenType::K_DEFAULT: return "DEFAULT";
        case TokenType::K_FOR: return "FOR";
        case TokenType::K_WHILE: return "WHILE";
        case TokenType::K_DO: return "DO";
        case TokenType::K_BREAK: return "BREAK";
        case TokenType::K_CONTINUE: return "CONTINUE";
        case TokenType::K_PUBLIC: return "PUBLIC";
        case TokenType::K_PRIVATE: return "PRIVATE";
        case TokenType::K_PROTECTED: return "PROTECTED";
        case TokenType::K_NEW: return "NEW";
        case TokenType::K_DELETE: return "DELETE";
        case TokenType::K_THIS: return "THIS";
        case TokenType::K_TRY: return "TRY";
        case TokenType::K_CATCH: return "CATCH";
        case TokenType::K_THROW: return "THROW";
        case TokenType::OP_PLUS: return "+";
        case TokenType::OP_MINUS: return "-";
        case TokenType::OP_MUL: return "*";
        case TokenType::OP_DIV: return "/";
        case TokenType::OP_ASSIGN: return "=";
        case TokenType::LPAREN: return "(";
        case TokenType::RPAREN: return ")";
        case TokenType::LBRACE: return "{";
        case TokenType::RBRACE: return "}";
        case TokenType::COMMA: return ",";
        case TokenType::SEMICOLON: return ";";
        default: return "TOKEN(" + std::to_string(static_cast<int>(t)) + ")";
    }
}

void testLexer(const String& source) {
    std::cout << "===== Lexer Test =====" << std::endl;
    
    Lexer lexer(source);
    Token token;
    int count = 0;
    
    while (true) {
        token = lexer.nextToken();
        
        if (token.type == TokenType::END_OF_FILE) {
            break;
        }
        
        std::cout << "[" << tokenTypeToString(token.type) << "] ";
        
        count++;
        if (count > 50) {
            std::cout << "Too many tokens..." << std::endl;
            break;
        }
        
        if (lexer.hasError()) {
            std::cout << "Error: " << lexer.errorMessage() << std::endl;
            break;
        }
    }
    
    if (!lexer.hasError()) {
        std::cout << "Lexer OK" << std::endl;
    }
}

void testParser(const String& source) {
    std::cout << "\n===== Parser Test =====" << std::endl;
    
    Lexer lexer(source);
    Parser parser(&lexer);
    auto program = parser.parse();
    
    if (parser.hasError()) {
        std::cout << "Parser Error: " << parser.errorMessage() << std::endl;
        return;
    }
    
    // 统计解析结果
    std::cout << "Package: ";
    if (program->package.has_value()) {
        std::cout << (*program->package)->name;
    } else {
        std::cout << "(none)";
    }
    std::cout << std::endl;
    
    std::cout << "Statements: " << program->statements.size() << std::endl;
    
    for (size_t i = 0; i < program->statements.size() && i < 5; i++) {
        auto& stmt = program->statements[i];
        std::cout << "  [" << i << "] ";
        
        // 简单判断语句类型
        if (dynamic_cast<ImportStmt*>(stmt.get())) {
            std::cout << "Import";
        } else if (dynamic_cast<FuncDeclStmt*>(stmt.get())) {
            auto f = static_cast<FuncDeclStmt*>(stmt.get());
            std::cout << "Func: " << f->name;
        } else if (dynamic_cast<ClassDeclStmt*>(stmt.get())) {
            auto c = static_cast<ClassDeclStmt*>(stmt.get());
            std::cout << "Class: " << c->name;
        } else if (dynamic_cast<VarDeclStmt*>(stmt.get())) {
            auto v = static_cast<VarDeclStmt*>(stmt.get());
            std::cout << "Var: " << v->name;
        } else if (dynamic_cast<BlockStmt*>(stmt.get())) {
            std::cout << "Block";
        } else {
            std::cout << "Other";
        }
        std::cout << std::endl;
    }
    
    std::cout << "Parser OK" << std::endl;
}

int main() {
    // Test 1: 简单函数
    String test1 = R"(
func add(int a, int b) -> int {
    return a + b;
}
)";
    testLexer(test1);
    testParser(test1);
    
    std::cout << "\n" << std::endl;
    
    // Test 2: 类
    String test2 = R"(
class Player {
    private:
        string name;
        int hp = 100;
        
    public:
        func attack(int damage) -> bool {
            if (damage > 10) {
                return true;
            }
            return false;
        }
}
)";
    testLexer(test2);
    testParser(test2);
    
    std::cout << "\n" << std::endl;
    
    // Test 3: 完整程序
    String test3 = R"(
package game.adventure;

import std.io;
import std.math;

func main() {
    int num = 42;
    float pi = 3.14159;
    string msg = "Hello, CPLang!";
    bool flag = true;
    
    for (int i = 0; i < 10; i++) {
        print(i);
    }
}
)";
    testLexer(test3);
    testParser(test3);
    
    return 0;
}