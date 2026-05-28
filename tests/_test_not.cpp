#include "minimal_test.hpp"
#include "lexer/lexer.hpp"
#include "lexer/token.hpp"
using namespace cplang;

TEST(Minimal, Works) {
    Lexer lexer(u8"! ");
    Token t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::NOT);
}

TEST_MAIN()
