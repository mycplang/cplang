#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer/lexer.hpp"
#include "lexer/token.hpp"
#include "parser/parser.hpp"

using namespace cplang;

const char* tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::END_OF_FILE: return "EOF";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::INTEGER: return "INTEGER";
        case TokenType::FLOAT: return "FLOAT";
        case TokenType::STRING: return "STRING";
        case TokenType::BOOLEAN: return "BOOLEAN";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::K_IMPORT: return "K_IMPORT";
        case TokenType::K_VAR: return "K_VAR";
        case TokenType::K_FUNC: return "K_FUNC";
        default: return "OTHER";
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file>" << std::endl;
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Cannot open file: " << argv[1] << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    std::cout << "=== Parsing ===" << std::endl;

    Lexer lexer(source);
    Parser parser(&lexer);
    
    auto program = parser.parse();
    
    if (program) {
        std::cout << "Parse successful!" << std::endl;
        std::cout << "Statements: " << program->statements.size() << std::endl;
    } else {
        std::cout << "Parse failed!" << std::endl;
    }

    return 0;
}
