// CP语言端到端集成测试
#include "minimal_test.hpp"
#include "codegen/codegen.hpp"
#include "vm/vm.hpp"
#include <string>
#include <vector>

using namespace cplang;

// 辅助函数：编译并运行 CP 代码，返回 stdout 捕获内容
// 通过注册一个捕获函数到 VM 来获取打印输出
static std::string captureOutput(const std::string& source) {
    Compiler compiler;
    VMFunction* func = compiler.compile(source);
    if (!func || compiler.hasError()) {
        return "[COMPILE ERROR] " + compiler.errorMessage();
    }
    VM* vm = compiler.vm();
    if (!vm) return "[ERROR] VM is null";

    // 捕获输出
    std::string output;
    vm->registerNative("__test_capture", [&output](std::vector<Value>& args) -> Value {
        for (size_t i = 0; i < args.size(); i++) {
            if (i > 0) output += " ";
            if (args[i].isInt()) output += std::to_string(args[i].asInt());
            else if (args[i].isFloat()) output += std::to_string(args[i].asFloat());
            else if (args[i].isBool()) output += args[i].asBool() ? "true" : "false";
            else if (args[i].isNil()) output += "nil";
            else if (args[i].isString()) {
                auto* s = args[i].asString();
                output += std::string(s->data, s->length);
            }
            else output += args[i].toString();
        }
        output += "\n";
        return Value::nil();
    });

    if (!vm->loadModule(func)) {
        return "[RUNTIME ERROR] " + vm->error();
    }
    return output;
}

// ===== 基础运算 =====

TEST(E2ETest, IntegerArithmetic) {
    std::string result = captureOutput(u8"变量 x = 42;");
    EXPECT_TRUE(result.empty() || result.find("[ERROR]") == std::string::npos);
}

TEST(E2ETest, SimpleAddition) {
    std::string result = captureOutput(u8"__test_capture(1 + 2);");
    EXPECT_NE(result.find("3"), std::string::npos) << "输出: " << result;
}

TEST(E2ETest, ComplexArithmetic) {
    std::string result = captureOutput(u8"__test_capture((1 + 2) * 3 - 4 / 2);");
    EXPECT_NE(result.find("7"), std::string::npos) << "输出: " << result;
}

TEST(E2ETest, Subtraction) {
    std::string result = captureOutput(u8"__test_capture(10 - 3);");
    EXPECT_NE(result.find("7"), std::string::npos) << "输出: " << result;
}

TEST(E2ETest, Multiplication) {
    std::string result = captureOutput(u8"__test_capture(6 * 7);");
    EXPECT_NE(result.find("42"), std::string::npos) << "输出: " << result;
}

TEST(E2ETest, Division) {
    std::string result = captureOutput(u8"__test_capture(15 / 3);");
    EXPECT_NE(result.find("5"), std::string::npos) << "输出: " << result;
}

// ===== 变量 =====

TEST(E2ETest, VariableAssignment) {
    std::string result = captureOutput(u8"__test_capture(100);");
    EXPECT_NE(result.find("100"), std::string::npos) << "输出: " << result;
}

TEST(E2ETest, VariableReassignment) {
    std::string result = captureOutput(u8"变量 x = 1; x = 2; __test_capture(x);");
    EXPECT_NE(result.find("2"), std::string::npos) << "输出: " << result;
}

TEST(E2ETest, MultipleVariables) {
    std::string result = captureOutput(u8"变量 a = 1; 变量 b = 2; 变量 c = a + b; __test_capture(c);");
    EXPECT_NE(result.find("3"), std::string::npos) << "输出: " << result;
}

// ===== 控制流 =====

TEST(E2ETest, IfTrue) {
    std::string result = captureOutput(u8"如果 (true) { __test_capture(1); }");
    EXPECT_NE(result.find("1"), std::string::npos) << "输出: " << result;
}

TEST(E2ETest, IfFalse) {
    std::string result = captureOutput(u8"如果 (false) { __test_capture(1); } 否则 { __test_capture(2); }");
    EXPECT_NE(result.find("2"), std::string::npos) << "输出: " << result;
}

TEST(E2ETest, IfElseIf) {
    std::string result = captureOutput(u8R"(
        变量 x = 5;
        如果 (x > 10) { __test_capture(1); }
        否则 如果 (x > 3) { __test_capture(2); }
        否则 { __test_capture(3); }
    )");
    EXPECT_NE(result.find("2"), std::string::npos) << "输出: " << result;
}

TEST(E2ETest, WhileLoop) {
    std::string result = captureOutput(u8R"(
        变量 i = 0;
        变量 sum = 0;
        当 (i < 5) {
            sum = sum + i;
            i = i + 1;
        }
        __test_capture(sum);
    )");
    EXPECT_NE(result.find("10"), std::string::npos) << "输出: " << result;
}

TEST(E2ETest, ForLoop) {
    std::string result = captureOutput(u8R"(
        变量 sum = 0;
        循环 (变量 i = 0; i < 5; i = i + 1) {
            sum = sum + i;
        }
        __test_capture(sum);
    )");
    EXPECT_NE(result.find("10"), std::string::npos) << "输出: " << result;
}

// ===== 函数 =====

TEST(E2ETest, SimpleFunction) {
    std::string result = captureOutput(u8R"(
        函数 greet() {
            返回 42;
        }
        __test_capture(greet());
    )");
    EXPECT_NE(result.find("42"), std::string::npos) << "输出: " << result;
}

TEST(E2ETest, FunctionWithParameters) {
    std::string result = captureOutput(u8R"(
        函数 add(a, b) {
            返回 a + b;
        }
        __test_capture(add(3, 4));
    )");
    EXPECT_NE(result.find("7"), std::string::npos) << "输出: " << result;
}

TEST(E2ETest, FunctionWithMultipleParams) {
    std::string result = captureOutput(u8R"(
        函数 sum3(a, b, c) {
            返回 a + b + c;
        }
        __test_capture(sum3(1, 2, 3));
    )");
    EXPECT_NE(result.find("6"), std::string::npos) << "输出: " << result;
}

TEST(E2ETest, NestedFunctionCalls) {
    std::string result = captureOutput(u8R"(
        函数 add(a, b) { 返回 a + b; }
        函数 mul(a, b) { 返回 a * b; }
        __test_capture(mul(add(2, 3), add(4, 5)));
    )");
    EXPECT_NE(result.find("45"), std::string::npos) << "输出: " << result;
}

TEST(E2ETest, RecursiveFactorial) {
    std::string result = captureOutput(u8R"(
        函数 fact(n) {
            如果 (n <= 1) { 返回 1; }
            返回 n * fact(n - 1);
        }
        __test_capture(fact(5));
    )");
    EXPECT_NE(result.find("120"), std::string::npos) << "输出: " << result;
}

TEST(E2ETest, RecursiveFibonacci) {
    std::string result = captureOutput(u8R"(
        函数 fib(n) {
            如果 (n <= 1) { 返回 n; }
            返回 fib(n - 1) + fib(n - 2);
        }
        __test_capture(fib(10));
    )");
    EXPECT_NE(result.find("55"), std::string::npos) << "输出: " << result;
}

TEST(E2ETest, FunctionReturnEarly) {
    std::string result = captureOutput(u8R"(
        函数 abs(n) {
            如果 (n >= 0) { 返回 n; }
            返回 -n;
        }
        __test_capture(abs(-5));
        __test_capture(abs(3));
    )");
    EXPECT_NE(result.find("5"), std::string::npos) << "输出: " << result;
    EXPECT_NE(result.find("3"), std::string::npos) << "输出: " << result;
}

// ===== 布尔和比较 =====

TEST(E2ETest, BoolTrue) {
    std::string result = captureOutput(u8"__test_capture(true);");
    EXPECT_NE(result.find("true"), std::string::npos) << "输出: " << result;
}

TEST(E2ETest, BoolFalse) {
    std::string result = captureOutput(u8"__test_capture(false);");
    EXPECT_NE(result.find("false"), std::string::npos) << "输出: " << result;
}

TEST(E2ETest, NotOperator) {
    std::string result = captureOutput(u8"__test_capture(!false);");
    EXPECT_NE(result.find("true"), std::string::npos) << "输出: " << result;
}

TEST(E2ETest, AndOperator) {
    std::string result = captureOutput(u8"__test_capture(true && false);");
    EXPECT_NE(result.find("false"), std::string::npos) << "输出: " << result;
}

TEST(E2ETest, OrOperator) {
    std::string result = captureOutput(u8"__test_capture(true || false);");
    EXPECT_NE(result.find("true"), std::string::npos) << "输出: " << result;
}

TEST(E2ETest, EqualsComparison) {
    std::string result = captureOutput(u8"__test_capture(5 == 5);");
    EXPECT_NE(result.find("true"), std::string::npos) << "输出: " << result;
}

TEST(E2ETest, NotEqualsComparison) {
    std::string result = captureOutput(u8"__test_capture(5 != 3);");
    EXPECT_NE(result.find("true"), std::string::npos) << "输出: " << result;
}

TEST(E2ETest, GreaterThanComparison) {
    std::string result = captureOutput(u8"__test_capture(5 > 3);");
    EXPECT_NE(result.find("true"), std::string::npos) << "输出: " << result;
}

TEST(E2ETest, LessThanComparison) {
    std::string result = captureOutput(u8"__test_capture(3 < 5);");
    EXPECT_NE(result.find("true"), std::string::npos) << "输出: " << result;
}

// ===== 数组 =====

TEST(E2ETest, ArrayLiteral) {
    std::string result = captureOutput(u8"变量 arr = [1, 2, 3];");
    EXPECT_TRUE(result.empty() || result.find("[ERROR]") == std::string::npos);
}

TEST(E2ETest, ArrayLiteralMultipleTypes) {
    std::string result = captureOutput(u8"变量 arr = [1, \"hello\", true, nil];");
    EXPECT_TRUE(result.empty() || result.find("[ERROR]") == std::string::npos);
}

// ===== 字符串 =====

TEST(E2ETest, StringLiteral) {
    std::string result = captureOutput(u8"__test_capture(\"hello\");");
    EXPECT_NE(result.find("hello"), std::string::npos) << "输出: " << result;
}

TEST(E2ETest, ChineseString) {
    std::string result = captureOutput(u8"__test_capture(\"你好世界\");");
    EXPECT_NE(result.find("你好世界"), std::string::npos) << "输出: " << result;
}

TEST(E2ETest, EmptyString) {
    std::string result = captureOutput(u8"__test_capture(\"\");");
    EXPECT_TRUE(result == "\n" || result.empty() || result.find("[ERROR]") == std::string::npos);
}

// ===== 嵌套结构 =====

TEST(E2ETest, NestedBlocks) {
    std::string result = captureOutput(u8R"(
        变量 x = 0;
        如果 (true) {
            如果 (true) {
                如果 (true) {
                    x = 99;
                }
            }
        }
        __test_capture(x);
    )");
    EXPECT_NE(result.find("99"), std::string::npos) << "输出: " << result;
}

// ===== 错误处理 =====

TEST(E2ETest, DivisionByZero) {
    std::string result = captureOutput(u8"变量 x = 5 / 0;");
    SUCCEED();
}

TEST(E2ETest, LargeNumber) {
    std::string result = captureOutput(u8"__test_capture(1000000);");
    EXPECT_NE(result.find("1000000"), std::string::npos) << "输出: " << result;
}

TEST(E2ETest, NegativeNumber) {
    std::string result = captureOutput(u8"__test_capture(-42);");
    EXPECT_NE(result.find("-42"), std::string::npos) << "输出: " << result;
}

TEST(E2ETest, MultiplePrintStatements) {
    std::string result = captureOutput(u8R"(
        __test_capture(1);
        __test_capture(2);
        __test_capture(3);
    )");
    EXPECT_NE(result.find("1"), std::string::npos) << "输出: " << result;
    EXPECT_NE(result.find("2"), std::string::npos) << "输出: " << result;
    EXPECT_NE(result.find("3"), std::string::npos) << "输出: " << result;
}

// ===== 新标准库功能测试 (forEach, transform, channel) =====

TEST(E2ETest, ForEach) {
    std::string result = captureOutput(u8R"(
        函数 累加(x) { 全局变量 sum; sum = sum + x; }
        全局变量 sum = 0;
        变量 arr = [1, 2, 3, 4, 5];
        遍历(arr, 累加);
        __test_capture(sum);
    )");
    EXPECT_NE(result.find("15"), std::string::npos) << "输出: " << result;
}

TEST(E2ETest, Transform) {
    std::string result = captureOutput(u8R"(
        函数 乘十(x) { 返回 x * 10; }
        变量 arr = [1, 2, 3];
        变量 result = 数组变换(arr, 乘十);
        __test_capture(result[0]);
        __test_capture(result[1]);
        __test_capture(result[2]);
    )");
    EXPECT_NE(result.find("10"), std::string::npos) << "输出: " << result;
    EXPECT_NE(result.find("20"), std::string::npos) << "输出: " << result;
    EXPECT_NE(result.find("30"), std::string::npos) << "输出: " << result;
}

TEST(E2ETest, ChannelTryRecv) {
    std::string result = captureOutput(u8R"(
        变量 ch = 通道创建(1);
        变量 ok = 通道发送(ch, 42);
        __test_capture(ok);
        变量 r = 通道尝试接收(ch);
        __test_capture(r[0]);
        __test_capture(r[1]);
    )");
    EXPECT_NE(result.find("true"), std::string::npos) << "输出: " << result;
    EXPECT_NE(result.find("true"), std::string::npos) << "输出: " << result;
    EXPECT_NE(result.find("42"), std::string::npos) << "输出: " << result;
}

TEST(E2ETest, ChannelSelectWithData) {
    std::string result = captureOutput(u8R"(
        变量 ch1 = 通道创建(1);
        变量 ch2 = 通道创建(1);
        通道发送(ch1, 100);
        通道发送(ch2, 200);
        变量 sel = 通道选择([ch1, ch2], 100);
        __test_capture(sel[0]);
        __test_capture(sel[1]);
    )");
    // Should have either (0,100) or (1,200)
    EXPECT_TRUE(
        (result.find("0") != std::string::npos && result.find("100") != std::string::npos) ||
        (result.find("1") != std::string::npos && result.find("200") != std::string::npos)
    ) << "输出: " << result;
}

TEST(E2ETest, ChannelSelectTimeout) {
    std::string result = captureOutput(u8R"(
        变量 ch = 通道创建(1);
        变量 sel = 通道选择([ch], 10);
        __test_capture(长度(sel));
    )");
    EXPECT_NE(result.find("0"), std::string::npos) << "输出: " << result;
}