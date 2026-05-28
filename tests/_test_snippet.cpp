#include "minimal_test.hpp"
#include "lexer/token.hpp"
#include "lexer/lexer.hpp"
using namespace cplang;

TEST(Minimal, Works) {
    Lexer lexer(u8"变量 x = 10;");
    Token t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::KW_VAR);
}
