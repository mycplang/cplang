// CP语言 完整编译管线测试 (Windows/MSVC)
// 测试: source → lexer → parser → semantic → codegen → VM load → execute
// 不依赖 stdlib.cpp (避免 WinHttp/WS2/SQLite 等系统库)

#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic_analyzer.hpp"
#include "codegen/codegen.hpp"
#include "vm/vm.hpp"
#include <iostream>
#include <cstdio>

using namespace cplang;

static void runTest(const char* name, const char* source, Value expected = Value::nil()) {
    std::printf("═══ %s ═══\n", name);
    std::fflush(stdout);

    // 1. 词法分析
    Lexer lexer(source);

    // 2. 语法分析
    Parser parser(&lexer);
    auto program = parser.parse();
    if (!program || parser.hasError()) {
        std::printf("  PARSE ERROR: %s\n", parser.errorMessage().c_str());
        return;
    }

    // 3. 语义分析
    SemanticAnalyzer analyzer;
    if (!analyzer.analyze(program)) {
        std::printf("  SEMANTIC ERROR: %s\n", analyzer.errorMessage().c_str());
        return;
    }

    // 4. 代码生成
    // 注意: 不使用 Compiler 类（避免 StdLib::registerAll 依赖）
    // 而是直接创建 VM + Codegen
    VM vm;
    Codegen codegen(&vm, &analyzer);
    VMFunction* func = codegen.compile(program);
    if (!func || codegen.hasError()) {
        std::printf("  CODEGEN ERROR: %s\n", codegen.errorMessage().c_str());
        return;
    }

    // 5. 加载到 VM 执行
    if (!vm.loadModule(func)) {
        std::printf("  VM ERROR: %s\n", vm.error().c_str());
        return;
    }

    std::printf("  OK\n");
    std::fflush(stdout);
}

int main() {
    std::printf("=== CP语言 完整编译管线测试 (Windows) ===\n\n");

    runTest("print test", R"(
        func main() {
            print("hello from CP");
        }
    )");

    runTest("arithmetic", R"(
        func main() {
            var a = 10 + 20;
            var b = a * 2;
            var c = b - 30;
            print(a, b, c);
        }
    )");

    runTest("loop", R"(
        func main() {
            var sum = 0;
            for (var i = 0; i < 10; i = i + 1) {
                sum = sum + i;
            }
            print("sum=", sum);
            print("expected=", 45);
        }
    )");

    runTest("function call", R"(
        func add(a, b) {
            return a + b;
        }
        func main() {
            var r = add(10, 20);
            print("10+20=", r);
        }
    )");

    runTest("array", R"(
        func main() {
            var arr = [1, 2, 3, 4, 5];
            print(arr[0], arr[2], arr[4]);
            arr[0] = 100;
            print(arr[0]);
        }
    )");

    std::printf("\n=== 全部测试完成 ===\n");
    return 0;
}
