// CP语言语法分析器测试
#include "minimal_test.hpp"
#include "parser/parser.hpp"
#include "ast/ast.hpp"

using namespace cplang;

TEST(ParserTest, BasicVariableDeclaration) {
    auto program = parseString(u8"变量 x = 10;");
    ASSERT_NE(program, nullptr);
    EXPECT_EQ(program->statements.size(), 1);
}

TEST(ParserTest, FunctionDeclaration) {
    auto program = parseString(u8"函数 add(a, b) { 返回 a + b; }");
    ASSERT_NE(program, nullptr);
    EXPECT_EQ(program->statements.size(), 1);
}

TEST(ParserTest, IfStatement) {
    auto program = parseString(u8"如果 (x > 0) { 打印(x); }");
    ASSERT_NE(program, nullptr);
    EXPECT_EQ(program->statements.size(), 1);
}

TEST(ParserTest, WhileLoop) {
    auto program = parseString(u8"当 (i < 10) { i = i + 1; }");
    ASSERT_NE(program, nullptr);
    EXPECT_EQ(program->statements.size(), 1);
}

TEST(ParserTest, ForLoop) {
    auto program = parseString(u8"循环 (变量 i = 0; i < 10; i = i + 1) { 打印(i); }");
    ASSERT_NE(program, nullptr);
    EXPECT_EQ(program->statements.size(), 1);
}

TEST(ParserTest, ForEachLoop) {
    auto program = parseString(u8"遍历 (x : arr) { 打印(x); }");
    ASSERT_NE(program, nullptr);
    EXPECT_EQ(program->statements.size(), 1);
}

TEST(ParserTest, ArrayLiteral) {
    auto program = parseString(u8"变量 arr = [1, 2, 3, 4, 5];");
    ASSERT_NE(program, nullptr);
    EXPECT_EQ(program->statements.size(), 1);
}

TEST(ParserTest, MultipleStatements) {
    auto program = parseString(u8"变量 x = 10; 变量 y = 20; 变量 sum = x + y; 打印(sum);");
    ASSERT_NE(program, nullptr);
    EXPECT_EQ(program->statements.size(), 4);
}

TEST(ParserTest, StructDeclaration) {
    auto program = parseString(u8"结构体 Person { name; age; }");
    ASSERT_NE(program, nullptr);
    EXPECT_EQ(program->statements.size(), 1);
}

TEST(ParserTest, ImportStatement) {
    auto program = parseString(u8"导入 math;");
    ASSERT_NE(program, nullptr);
    EXPECT_EQ(program->statements.size(), 1);
}
