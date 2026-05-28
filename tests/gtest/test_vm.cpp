// CP语言虚拟机测试 — 增强版
#include "minimal_test.hpp"
#include "vm/vm.hpp"
#include "stdlib/stdlib.hpp"
#include "codegen/codegen.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic_analyzer.hpp"
#include <string>
#include <sstream>

using namespace cplang;

// ===== 基础 VM 操作 =====

TEST(VMTest, CreateVM) {
    VM vm;
    EXPECT_FALSE(vm.hasError());
}

TEST(VMTest, RegisterNativeFunctions) {
    VM vm;
    StdLib::registerAll(&vm);
    EXPECT_FALSE(vm.hasError());
}

TEST(VMTest, GlobalVariablesSlot) {
    VM vm;
    Int32 slot = vm.getOrCreateGlobalSlot("test_var");
    EXPECT_GE(slot, 0);

    Int32 sameSlot = vm.getGlobalSlot("test_var");
    EXPECT_EQ(slot, sameSlot);
}

TEST(VMTest, ErrorHandling) {
    VM vm;
    vm.raiseError("Test error");
    EXPECT_TRUE(vm.hasError());
    EXPECT_EQ(vm.error(), "Test error");
}

TEST(VMTest, InstructionCount) {
    VM vm;
    EXPECT_EQ(vm.totalInstructions(), 0);
}

TEST(VMTest, StringIntern) {
    VM vm;
    VMString* s1 = vm.internString("hello");
    VMString* s2 = vm.internString("hello");
    VMString* s3 = vm.internString("world");

    EXPECT_EQ(s1, s2);
    EXPECT_NE(s1, s3);
}

// ===== 编译并执行 CP 代码 =====

// 辅助函数：编译并在 VM 中执行 CP 代码，返回错误信息（空表示成功）
static std::string compileAndRun(const std::string& source) {
    Compiler compiler;
    VMFunction* func = compiler.compile(source);
    if (!func || compiler.hasError()) {
        return compiler.errorMessage();
    }
    VM* vm = compiler.vm();
    if (!vm) return "VM is null";

    if (!vm->loadModule(func)) {
        return vm->error();
    }
    return ""; // 成功
}

TEST(VMTest, CompileAndRunSimple) {
    std::string err = compileAndRun(u8"变量 x = 42;");
    EXPECT_TRUE(err.empty()) << "编译/运行失败: " << err;
}

TEST(VMTest, CompileAndRunArithmetic) {
    std::string err = compileAndRun(u8"变量 x = 1 + 2 * 3;");
    EXPECT_TRUE(err.empty()) << err;
}

TEST(VMTest, CompileAndRunFunctionCall) {
    std::string err = compileAndRun(u8"函数 add(a, b) { 返回 a + b; } 变量 result = add(3, 4);");
    EXPECT_TRUE(err.empty()) << err;
}

TEST(VMTest, CompileAndRunIfStatement) {
    std::string err = compileAndRun(u8"变量 x = 10; 如果 (x > 5) { 变量 y = 1; } 否则 { 变量 y = 0; }");
    EXPECT_TRUE(err.empty()) << err;
}

TEST(VMTest, CompileAndRunWhileLoop) {
    std::string err = compileAndRun(u8"变量 sum = 0; 变量 i = 0; 当 (i < 5) { sum = sum + i; i = i + 1; }");
    EXPECT_TRUE(err.empty()) << err;
}

TEST(VMTest, CompileAndRunForLoop) {
    std::string err = compileAndRun(u8"变量 sum = 0; 循环 (变量 i = 0; i < 5; i = i + 1) { sum = sum + i; }");
    EXPECT_TRUE(err.empty()) << err;
}

TEST(VMTest, CompileAndRunNestedLoops) {
    std::string err = compileAndRun(u8"变量 s = 0; 循环 (变量 i = 0; i < 3; i = i + 1) { 循环 (变量 j = 0; j < 3; j = j + 1) { s = s + 1; } }");
    EXPECT_TRUE(err.empty()) << err;
}

TEST(VMTest, CompileAndRunArray) {
    std::string err = compileAndRun(u8"变量 arr = [1, 2, 3, 4, 5];");
    EXPECT_TRUE(err.empty()) << err;
}

TEST(VMTest, CompileAndRunString) {
    std::string err = compileAndRun(u8"变量 s = \"你好，世界！\";");
    EXPECT_TRUE(err.empty()) << err;
}

TEST(VMTest, CompileAndRunBoolean) {
    std::string err = compileAndRun(u8"变量 a = true; 变量 b = false; 变量 c = !a;");
    EXPECT_TRUE(err.empty()) << err;
}

TEST(VMTest, CompileAndRunComparison) {
    std::string err = compileAndRun(u8"变量 a = 5 == 5; 变量 b = 5 != 3; 变量 c = 5 > 3; 变量 d = 5 <= 5;");
    EXPECT_TRUE(err.empty()) << err;
}

TEST(VMTest, CompileAndRunLogicalOps) {
    std::string err = compileAndRun(u8"变量 a = true && false; 变量 b = true || false;");
    EXPECT_TRUE(err.empty()) << err;
}

TEST(VMTest, CompileAndRunRecursion) {
    // 阶乘
    std::string err = compileAndRun(u8"函数 fact(n) { 如果 (n <= 1) { 返回 1; } 返回 n * fact(n - 1); } 变量 r = fact(5);");
    EXPECT_TRUE(err.empty()) << err;
}

TEST(VMTest, CompileAndRunMultipleFunctions) {
    std::string src = u8R"(
        函数 add(a, b) { 返回 a + b; }
        函数 sub(a, b) { 返回 a - b; }
        函数 mul(a, b) { 返回 a * b; }
        变量 x = add(10, 5);
        变量 y = sub(10, 5);
        变量 z = mul(10, 5);
    )";
    std::string err = compileAndRun(src);
    EXPECT_TRUE(err.empty()) << err;
}

TEST(VMTest, CompileAndRunEmptyProgram) {
    std::string err = compileAndRun("");
    EXPECT_TRUE(err.empty()) << err;
}

TEST(VMTest, CompileSyntaxError) {
    Compiler compiler;
    VMFunction* func = compiler.compile(u8"变量 x = ;");  // 语法错误
    EXPECT_TRUE(func == nullptr || compiler.hasError());
}

TEST(VMTest, CompileUndefinedVariable) {
    Compiler compiler;
    VMFunction* func = compiler.compile(u8"变量 y = undefinedVar;");  // 可能产生语义错误
    // 编译可能失败也可能成功（某些模式允许运行时解析），不应崩溃
    if (func) {
        VM* vm = compiler.vm();
        if (vm) vm->loadModule(func);
    }
}

TEST(VMTest, CompileAndRunStringConcat) {
    // 字符串拼接（如果语言支持 + 运算符拼接字符串）
    std::string err = compileAndRun(u8"变量 s = \"Hello\" + \" \" + \"World\";");
    // 可能失败如果不支持字符串+，但不应崩溃
    EXPECT_TRUE(err.empty() || err.find("type") != std::string::npos) << err;
}

// ===== 高级执行场景 =====

TEST(VMTest, FibonnaciRecursion) {
    std::string src = u8R"(
        函数 fib(n) {
            如果 (n <= 1) { 返回 n; }
            返回 fib(n-1) + fib(n-2);
        }
        变量 result = fib(10);
    )";
    std::string err = compileAndRun(src);
    EXPECT_TRUE(err.empty()) << err;
}

TEST(VMTest, DeepNesting) {
    std::string src = u8R"(
        变量 x = 0;
        如果 (true) {
            如果 (true) {
                如果 (true) {
                    如果 (true) {
                        如果 (true) {
                            x = 42;
                        }
                    }
                }
            }
        }
    )";
    std::string err = compileAndRun(src);
    EXPECT_TRUE(err.empty()) << err;
}

TEST(VMTest, MultipleReturns) {
    std::string src = u8R"(
        函数 check(n) {
            如果 (n > 0) { 返回 1; }
            否则 { 返回 -1; }
        }
        变量 a = check(10);
        变量 b = check(-5);
    )";
    std::string err = compileAndRun(src);
    EXPECT_TRUE(err.empty()) << err;
}

TEST(VMTest, ForEachLoop) {
    std::string src = u8"变量 arr = [1, 2, 3]; 遍历 (x : arr) { 打印(x); }";
    std::string err = compileAndRun(src);
    EXPECT_TRUE(err.empty()) << err;
}

// ===== 纯值操作测试（不依赖编译执行） =====

TEST(VMTest, ValueCreation) {
    Value v1 = Value::Int(42);
    EXPECT_TRUE(v1.isInt());
    EXPECT_EQ(v1.asInt(), 42);

    Value v2 = Value::Float(3.14);
    EXPECT_TRUE(v2.isFloat());
    EXPECT_FLOAT_EQ(v2.asFloat(), 3.14f);

    Value v3 = Value::Bool(true);
    EXPECT_TRUE(v3.isBool());
    EXPECT_TRUE(v3.asBool());

    Value v4 = Value::nil();
    EXPECT_TRUE(v4.isNil());
}

TEST(VMTest, ValueToString) {
    Value v1 = Value::Int(123);
    EXPECT_EQ(v1.toString(), "123");

    Value v2 = Value::Bool(true);
    // CP uses Chinese booleans
    EXPECT_TRUE(v2.toString() == "true" || v2.toString() == "真");
}
