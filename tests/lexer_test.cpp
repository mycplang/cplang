#include "lexer/lexer.hpp"
#include <iostream>
#include <iomanip>

using namespace cplang;

int main(int argc, char** argv) {
    String src = (argc > 1) ? String(argv[1]) : u8"如果(1)";
    Lexer lex(src);
    int count = 0;
    while (true) {
        Token t = lex.nextToken();
        std::cout << "Token[" << count++ << "]: type=" << static_cast<int>(t.type)
                  << " text_bytes=";
        for (unsigned char c : t.text) {
            std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)c;
        }
        std::cout << " len=" << std::dec << t.text.size()
                  << " line=" << t.line << "\n";
        // Verify against keyword table
        auto kw = KeywordTable::instance().find(t.text);
        if (kw.has_value()) {
            std::cout << "  -> keyword lookup = " << static_cast<int>(kw.value()) << "\n";
        }
        if (t.type == TokenType::END_OF_FILE) break;
    }
    return 0;
}
