#include "minimal_test.hpp"
#include "lexer/token.hpp"
using namespace cplang;
static_assert(sizeof(TokenType) > 0, "TokenType known");
TEST(Minimal, Works) {
    TokenType t = TokenType::KW_VAR;
    (void)t;
}
TEST_MAIN()
