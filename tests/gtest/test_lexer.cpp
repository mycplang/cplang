// CP语言词法分析器测试
#include "minimal_test.hpp"
#include "lexer/lexer.hpp"
#include "lexer/token.hpp"

using namespace cplang;

TEST(LexerTest, BasicTokens) {
    Lexer lexer(u8"变量 x = 10;");

    Token t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::K_VAR);

    t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::IDENTIFIER);
    EXPECT_EQ(t.text, u8"x");

    t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::OP_ASSIGN);

    t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::INTEGER);
    EXPECT_EQ(t.text, u8"10");

    t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::SEMICOLON);
}

TEST(LexerTest, ChineseKeywords) {
    Lexer lexer(u8"函数 返回 如果 否则 当 循环 遍历 打印");

    Token t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::K_FUNC);

    t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::K_RETURN);

    t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::K_IF);

    t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::K_ELSE);

    t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::K_WHILE);

    t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::K_FOR);

    t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::IDENTIFIER); // "遍历" is an identifier (not a keyword) to avoid conflict with stdlib forEach function

    t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::IDENTIFIER); // "打印" is an identifier, not a keyword
}

TEST(LexerTest, StringLiterals) {
    Lexer lexer(u8"\"Hello World\" \"中文\"");
    
    Token t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::STRING);
    EXPECT_EQ(t.text, u8"Hello World");
    
    t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::STRING);
    EXPECT_EQ(t.text, u8"中文");
}

TEST(LexerTest, NumberLiterals) {
    Lexer lexer(u8"123 45.67");
    
    Token t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::INTEGER);
    EXPECT_EQ(t.text, u8"123");
    
    t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::FLOAT);
    EXPECT_EQ(t.text, u8"45.67");
}

TEST(LexerTest, Operators) {
    Lexer lexer(u8"+ - * / % == != < > <= >= && || !");

    Token t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::OP_PLUS);

    t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::OP_MINUS);

    t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::OP_MUL);

    t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::OP_DIV);

    t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::OP_MOD);

    t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::OP_EQ);

    t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::OP_NE);

    t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::OP_LT);

    t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::OP_GT);

    t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::OP_LE);

    t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::OP_GE);

    t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::OP_AND);

    t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::OP_OR);

    t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::OP_NOT);
}

TEST(LexerTest, BracketsAndBraces) {
    Lexer lexer(u8"( ) [ ] { }");

    Token t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::LPAREN);

    t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::RPAREN);

    t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::LBRACKET);

    t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::RBRACKET);

    t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::LBRACE);

    t = lexer.nextToken();
    EXPECT_EQ(t.type, TokenType::RBRACE);
}
