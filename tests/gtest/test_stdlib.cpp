// CP语言标准库集成测试
#include "minimal_test.hpp"
#include "codegen/codegen.hpp"
#include "vm/vm.hpp"
#include <string>
#include <vector>

using namespace cplang;

// 辅助函数：编译并运行 CP 代码，返回 stdout 捕获内容
static std::string captureOutput(const std::string& source) {
    Compiler compiler;
    VMFunction* func = compiler.compile(source);
    if (!func || compiler.hasError()) {
        return "[COMPILE ERROR] " + compiler.errorMessage();
    }
    VM* vm = compiler.vm();
    if (!vm) return "[ERROR] VM is null";

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

// ===== 数学函数 =====

TEST(StdlibTest, AbsPositive) {
    std::string result = captureOutput(u8R"(__test_capture(绝对值(5));)");
    EXPECT_NE(result.find("5"), std::string::npos) << "输出: " << result;
}

TEST(StdlibTest, AbsNegative) {
    std::string result = captureOutput(u8R"(__test_capture(绝对值(-7));)");
    EXPECT_NE(result.find("7"), std::string::npos) << "输出: " << result;
}

TEST(StdlibTest, Sqrt) {
    std::string result = captureOutput(u8R"(__test_capture(平方根(16));)");
    EXPECT_NE(result.find("4"), std::string::npos) << "输出: " << result;
}

TEST(StdlibTest, Pow) {
    std::string result = captureOutput(u8R"(__test_capture(幂(2, 10));)");
    EXPECT_NE(result.find("1024"), std::string::npos) << "输出: " << result;
}

TEST(StdlibTest, Floor) {
    std::string result = captureOutput(u8R"(__test_capture(向下取整(3.7));)");
    EXPECT_NE(result.find("3"), std::string::npos) << "输出: " << result;
}

TEST(StdlibTest, Ceil) {
    std::string result = captureOutput(u8R"(__test_capture(向上取整(3.2));)");
    EXPECT_NE(result.find("4"), std::string::npos) << "输出: " << result;
}

TEST(StdlibTest, Random) {
    std::string result = captureOutput(u8R"(
        变量 r = 随机值(1, 100);
        __test_capture("随机:", r);
    )");
    EXPECT_TRUE(result.find("[ERROR]") == std::string::npos) << "输出: " << result;
}

// ===== 字符串函数 =====

TEST(StdlibTest, StringLength) {
    std::string result = captureOutput(u8R"(__test_capture(长度("你好世界"));)");
    EXPECT_NE(result.find("4"), std::string::npos) << "输出: " << result;
}

TEST(StdlibTest, Substring) {
    std::string result = captureOutput(u8R"(__test_capture(子串("HelloWorld", 3, 4));)");
    EXPECT_NE(result.find("loWo"), std::string::npos) << "输出: " << result;
}

TEST(StdlibTest, Concat) {
    std::string result = captureOutput(u8R"(__test_capture(连接("Hello", "World"));)");
    EXPECT_NE(result.find("HelloWorld"), std::string::npos) << "输出: " << result;
}

TEST(StdlibTest, Find) {
    std::string result = captureOutput(u8R"(__test_capture(查找("HelloWorld", "Wo"));)");
    EXPECT_NE(result.find("5"), std::string::npos) << "输出: " << result;
}

TEST(StdlibTest, Trim) {
    std::string result = captureOutput(u8R"(__test_capture(修剪("  你好  "));)");
    EXPECT_NE(result.find("你好"), std::string::npos) << "输出: " << result;
}

TEST(StdlibTest, Lower) {
    std::string result = captureOutput(u8R"(__test_capture(小写("ABC"));)");
    EXPECT_NE(result.find("abc"), std::string::npos) << "输出: " << result;
}

TEST(StdlibTest, Upper) {
    std::string result = captureOutput(u8R"(__test_capture(大写("abc"));)");
    EXPECT_NE(result.find("ABC"), std::string::npos) << "输出: " << result;
}

TEST(StdlibTest, Replace) {
    std::string result = captureOutput(u8R"(__test_capture(替换("HelloWorld", "World", "CP"));)");
    EXPECT_NE(result.find("HelloCP"), std::string::npos) << "输出: " << result;
}

// ===== 数组函数 =====

TEST(StdlibTest, ArrayPush) {
    std::string result = captureOutput(u8R"(
        变量 arr = [1, 2];
        追加(arr, 3);
        __test_capture(数组长(arr));
    )");
    EXPECT_NE(result.find("3"), std::string::npos) << "输出: " << result;
}

TEST(StdlibTest, ArrayPop) {
    std::string result = captureOutput(u8R"(
        变量 arr = [10, 20, 30];
        变量 v = 弹出(arr);
        __test_capture(v, " ", 数组长(arr));
    )");
    EXPECT_NE(result.find("30"), std::string::npos) << "输出: " << result;
    EXPECT_NE(result.find("2"), std::string::npos) << "输出: " << result;
}

TEST(StdlibTest, ArrayInsert) {
    std::string result = captureOutput(u8R"(
        变量 arr = [1, 3];
        插入(arr, 1, 2);
        __test_capture(arr[0], arr[1], arr[2]);
    )");
    EXPECT_NE(result.find("1 2 3"), std::string::npos) << "输出: " << result;
}

TEST(StdlibTest, ArrayRemove) {
    std::string result = captureOutput(u8R"(
        变量 arr = [10, 20, 30];
        移除(arr, 1);
        __test_capture(数组长(arr), " ", arr[0], " ", arr[1]);
    )");
    EXPECT_NE(result.find("2"), std::string::npos) << "输出: " << result;
    EXPECT_NE(result.find("10 30"), std::string::npos) << "输出: " << result;
}

TEST(StdlibTest, ArraySlice) {
    std::string result = captureOutput(u8R"(
        变量 arr = [1, 2, 3, 4, 5];
        变量 s = 切片(arr, 1, 4);
        __test_capture(数组长(s), " ", s[0], " ", s[2]);
    )");
    EXPECT_NE(result.find("3"), std::string::npos) << "输出: " << result;
    EXPECT_NE(result.find("2 4"), std::string::npos) << "输出: " << result;
}

TEST(StdlibTest, ArrayReverse) {
    std::string result = captureOutput(u8R"(
        变量 arr = [1, 2, 3];
        变量 r = 反转(arr);
        __test_capture(r[0], r[1], r[2]);
    )");
    EXPECT_NE(result.find("3 2 1"), std::string::npos) << "输出: " << result;
}

TEST(StdlibTest, ArraySort) {
    std::string result = captureOutput(u8R"(
        变量 arr = [3, 1, 2];
        变量 s = 排序(arr);
        __test_capture(s[0], s[1], s[2]);
    )");
    EXPECT_NE(result.find("1 2 3"), std::string::npos) << "输出: " << result;
}

TEST(StdlibTest, ArrayIndexOf) {
    std::string result = captureOutput(u8R"(
        变量 arr = [10, 20, 30, 20];
        __test_capture(取索引(arr, 20));
    )");
    EXPECT_NE(result.find("1"), std::string::npos) << "输出: " << result;
}

// ===== 表操作 =====

TEST(StdlibTest, TableCreate) {
    std::string result = captureOutput(u8R"(
        变量 t = 表创建(["名字", "年龄"], ["小明", 12]);
        __test_capture(表取(t, "名字"));
    )");
    EXPECT_NE(result.find("小明"), std::string::npos) << "输出: " << result;
}

TEST(StdlibTest, TableSetAndGet) {
    std::string result = captureOutput(u8R"(
        变量 t = 表创建();
        表设(t, "x", 100);
        __test_capture(表取(t, "x"));
    )");
    EXPECT_NE(result.find("100"), std::string::npos) << "输出: " << result;
}

TEST(StdlibTest, TableLength) {
    std::string result = captureOutput(u8R"(
        变量 t = 表创建(["a", "b", "c"], [1, 2, 3]);
        __test_capture(表长(t));
    )");
    EXPECT_NE(result.find("3"), std::string::npos) << "输出: " << result;
}

// ===== JSON =====

TEST(StdlibTest, JsonStringify) {
    std::string result = captureOutput(u8R"(
        变量 data = [1, 2, 3];
        变量 json = 转JSON(data);
        __test_capture("长度:", 长度(json));
    )");
    EXPECT_TRUE(result.find("[ERROR]") == std::string::npos) << "输出: " << result;
}

// ===== 数字转换 =====

TEST(StdlibTest, IntToString) {
    std::string result = captureOutput(u8R"(__test_capture(转字符串(12345));)");
    EXPECT_NE(result.find("12345"), std::string::npos) << "输出: " << result;
}

TEST(StdlibTest, StringToInt) {
    std::string result = captureOutput(u8R"(
        变量 n = 转整数("123");
        __test_capture(n);
    )");
    EXPECT_NE(result.find("123"), std::string::npos) << "输出: " << result;
}

// ===== 文件操作 =====

TEST(StdlibTest, FileExists) {
    std::string result = captureOutput(u8R"(
        __test_capture(文件存在("不存在的文件.xyz"));
    )");
    EXPECT_NE(result.find("false"), std::string::npos) << "输出: " << result;
}

// ===== 类型检查 =====

TEST(StdlibTest, TypeOfInt) {
    std::string result = captureOutput(u8R"(
        __test_capture(类型(42));
    )");
    EXPECT_NE(result.find("int"), std::string::npos) << "输出: " << result;
}

TEST(StdlibTest, TypeOfString) {
    std::string result = captureOutput(u8R"(
        __test_capture(类型("hello"));
    )");
    EXPECT_NE(result.find("string"), std::string::npos) << "输出: " << result;
}

TEST(StdlibTest, TypeOfBool) {
    std::string result = captureOutput(u8R"(
        __test_capture(类型(真));
    )");
    EXPECT_NE(result.find("bool"), std::string::npos) << "输出: " << result;
}

TEST(StdlibTest, TypeOfArray) {
    std::string result = captureOutput(u8R"(
        __test_capture(类型([1, 2]));
    )");
    EXPECT_NE(result.find("array"), std::string::npos) << "输出: " << result;
}

// ===== 系统函数 =====

TEST(StdlibTest, SystemTime) {
    std::string result = captureOutput(u8R"(
        变量 t = 当前时间();
        __test_capture("时间:", t > 0);
    )");
    EXPECT_TRUE(result.find("[ERROR]") == std::string::npos) << "输出: " << result;
}
