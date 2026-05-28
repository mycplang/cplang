#include <cstdio>
#include "minimal_test.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
using namespace cplang;

TEST(Minimal, LexerWorks) {
    Lexer lexer(u8"变量 x = 42;");
    Token t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::K_VAR);
}

TEST_MAIN()
