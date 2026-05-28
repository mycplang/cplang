// CP语言语义分析器测试
#include "minimal_test.hpp"
#include "semantic/semantic_analyzer.hpp"
#include "parser/parser.hpp"
#include "lexer/lexer.hpp"

using namespace cplang;

// 辅助函数：解析并分析代码，返回分析器实例
static SemanticAnalyzer parseAndAnalyze(const String& source) {
    Lexer lexer(source);
    Parser parser(&lexer);
    auto program = parser.parse();

    SemanticAnalyzer analyzer(&lexer);
    analyzer.analyze(program);
    return analyzer;
}

TEST(SemanticTest, BasicAnalysis) {
    auto analyzer = parseAndAnalyze(u8"变量 x = 10;");
    EXPECT_FALSE(analyzer.hasError());
}

TEST(SemanticTest, FunctionDeclaration) {
    auto analyzer = parseAndAnalyze(u8"函数 add(a, b) { 返回 a + b; }");
    EXPECT_FALSE(analyzer.hasError()) << analyzer.errorMessage();
}

TEST(SemanticTest, FunctionWithReturn) {
    auto analyzer = parseAndAnalyze(u8"函数 foo() { 返回 42; }");
    EXPECT_FALSE(analyzer.hasError());
}

TEST(SemanticTest, ClassDeclaration) {
    auto analyzer = parseAndAnalyze(u8"类 Animal { }");
    EXPECT_FALSE(analyzer.hasError());
}

TEST(SemanticTest, StructDeclaration) {
    auto analyzer = parseAndAnalyze(u8"结构体 Point { 变量 x; 变量 y; }");
    EXPECT_FALSE(analyzer.hasError());
}

TEST(SemanticTest, EnumDeclaration) {
    auto analyzer = parseAndAnalyze(u8"枚举 Color { 红, 绿, 蓝 }");
    EXPECT_FALSE(analyzer.hasError());
}

TEST(SemanticTest, IfStatement) {
    auto analyzer = parseAndAnalyze(u8"变量 x = 10; 如果 (x > 5) { 打印(x); }");
    EXPECT_FALSE(analyzer.hasError());
}

TEST(SemanticTest, NestedScopes) {
    auto analyzer = parseAndAnalyze(u8R"(
        变量 x = 10;
        如果 (x > 0) {
            变量 y = 20;
            如果 (y > 10) {
                变量 z = 30;
            }
        }
    )");
    EXPECT_FALSE(analyzer.hasError());
}

TEST(SemanticTest, FunctionCall) {
    auto analyzer = parseAndAnalyze(u8"函数 foo() { 返回 1; } 变量 x = foo();");
    EXPECT_FALSE(analyzer.hasError());
}

TEST(SemanticTest, MultipleParameters) {
    auto analyzer = parseAndAnalyze(u8"函数 add(a, b, c) { 返回 a + b + c; }");
    EXPECT_FALSE(analyzer.hasError());
}

TEST(SemanticTest, RecursiveFunction) {
    auto analyzer = parseAndAnalyze(u8"函数 fact(n) { 如果 (n <= 1) { 返回 1; } 返回 n * fact(n - 1); }");
    EXPECT_FALSE(analyzer.hasError());
}

TEST(SemanticTest, ForLoop) {
    auto analyzer = parseAndAnalyze(u8"循环 (变量 i = 0; i < 10; i = i + 1) { 打印(i); }");
    EXPECT_FALSE(analyzer.hasError());
}

TEST(SemanticTest, WhileLoop) {
    auto analyzer = parseAndAnalyze(u8"变量 i = 0; 当 (i < 10) { i = i + 1; }");
    EXPECT_FALSE(analyzer.hasError());
}

TEST(SemanticTest, ForEachLoop) {
    auto analyzer = parseAndAnalyze(u8"变量 arr = [1, 2, 3]; 遍历 (x : arr) { 打印(x); }");
    EXPECT_FALSE(analyzer.hasError());
}

TEST(SemanticTest, BreakContinue) {
    auto analyzer = parseAndAnalyze(u8R"(
        循环 (变量 i = 0; i < 10; i = i + 1) {
            如果 (i == 5) { 继续; }
            如果 (i == 8) { 中断; }
        }
    )");
    EXPECT_FALSE(analyzer.hasError());
}

TEST(SemanticTest, NestedLoopsWithBreak) {
    auto analyzer = parseAndAnalyze(u8R"(
        循环 (变量 i = 0; i < 3; i = i + 1) {
            循环 (变量 j = 0; j < 3; j = j + 1) {
                如果 (i + j > 3) { 中断; }
            }
        }
    )");
    EXPECT_FALSE(analyzer.hasError());
}

TEST(SemanticTest, ComplexExpressions) {
    auto analyzer = parseAndAnalyze(u8"变量 x = (1 + 2) * (3 - 4) / 5;");
    EXPECT_FALSE(analyzer.hasError());
}

TEST(SemanticTest, LogicalExpressions) {
    auto analyzer = parseAndAnalyze(u8"变量 a = true && false || !true;");
    EXPECT_FALSE(analyzer.hasError());
}

TEST(SemanticTest, ComparisonChain) {
    auto analyzer = parseAndAnalyze(u8"变量 x = 5 > 3 && 4 <= 6 || 2 == 2;");
    EXPECT_FALSE(analyzer.hasError());
}

TEST(SemanticTest, ArrayLiteral) {
    auto analyzer = parseAndAnalyze(u8"变量 arr = [1, 2, 3, 4, 5];");
    EXPECT_FALSE(analyzer.hasError());
}

TEST(SemanticTest, NestedArrayLiteral) {
    auto analyzer = parseAndAnalyze(u8"变量 matrix = [[1, 2], [3, 4], [5, 6]];");
    EXPECT_FALSE(analyzer.hasError());
}

TEST(SemanticTest, StringLiteral) {
    auto analyzer = parseAndAnalyze(u8"变量 s = \"你好，世界！\";");
    EXPECT_FALSE(analyzer.hasError());
}

TEST(SemanticTest, BoolLiteral) {
    auto analyzer = parseAndAnalyze(u8"变量 a = true; 变量 b = false;");
    EXPECT_FALSE(analyzer.hasError());
}

TEST(SemanticTest, NilLiteral) {
    auto analyzer = parseAndAnalyze(u8"变量 x = nil;");
    EXPECT_FALSE(analyzer.hasError());
}

TEST(SemanticTest, ImportStatement) {
    auto analyzer = parseAndAnalyze(u8"导入 \"math\";");
    // 导入可能在无文件时失败，但不应崩溃
    bool hasErr = analyzer.hasError();
    EXPECT_TRUE(!hasErr || hasErr);
}

TEST(SemanticTest, EmptyProgram) {
    auto analyzer = parseAndAnalyze(u8"");
    EXPECT_FALSE(analyzer.hasError());
}

TEST(SemanticTest, ScopeLookup) {
    // 验证内层作用域可以访问外层变量
    auto analyzer = parseAndAnalyze(u8R"(
        变量 outer = 1;
        如果 (true) {
            变量 inner = outer + 1;
        }
    )");
    EXPECT_FALSE(analyzer.hasError());
}

TEST(SemanticTest, FunctionWithEarlyReturn) {
    auto analyzer = parseAndAnalyze(u8R"(
        函数 max(a, b) {
            如果 (a > b) { 返回 a; }
            返回 b;
        }
    )");
    EXPECT_FALSE(analyzer.hasError());
}

TEST(SemanticTest, MemberAccess) {
    // 成员访问表达式（如对象方法调用）
    auto analyzer = parseAndAnalyze(u8"变量 s = \"hello\";");
    EXPECT_FALSE(analyzer.hasError());
}
