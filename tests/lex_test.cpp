#include "lexer/lexer.hpp"
#include <iostream>

int main() {
    cplang::Lexer lexer("x=42;");
    lexer.nextToken(); // initialize curr_
    while (true) {
        auto tok = lexer.curr_;
        std::cout << "type=" << static_cast<int>(tok.type) 
                  << " text=" << tok.text 
                  << " line=" << tok.line << " col=" << tok.col << std::endl;
        if (tok.type == 0) break; // EOF
        lexer.nextToken();
    }
    return 0;
}
