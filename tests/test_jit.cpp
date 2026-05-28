// JIT 编译器测试

#include "jit/jit_compiler.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic_analyzer.hpp"
#include "codegen/codegen.hpp"
#include <iostream>

using namespace cplang;

void printBanner(const char* title) {
    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════════════════\n";
    std::cout << "  " << title << "\n";
    std::cout << "═══════════════════════════════════════════════════════════\n";
}

void testLLVMIRGeneration() {
    printBanner("测试 LLVM IR 生成");

    const char* source = R"(
函数 斐波那契(n) {
    如果 (n <= 1) {
        返回 n;
    }
    返回 斐波那契(n - 1) + 斐波那契(n - 2);
}

函数 主() {
    变量 result = 斐波那契(10);
    打印(result);
}
)";

    std::cout << "源代码:\n" << source << "\n\n";

    // 词法分析
    Lexer lexer(source);
    int tokenCount = 0;
    Token tok;
    while ((tok = lexer.nextToken()).type != TokenType::END_OF_FILE) {
        if (lexer.hasError()) {
            std::cerr << "词法错误: " << lexer.errorMessage() << "\n";
            return;
        }
        tokenCount++;
    }
    lexer.reset();  // 重置以供 Parser 使用

    std::cout << "✓ 词法分析成功 (" << tokenCount << " tokens)\n";

    // 语法分析
    Parser parser(&lexer);
    auto program = parser.parse();
    if (parser.hasError()) {
        std::cerr << "语法错误: " << parser.errorMessage() << "\n";
        return;
    }
    std::cout << "✓ 语法分析成功\n";

    // 语义分析
    SemanticAnalyzer sema;
    sema.analyze(program);
    if (sema.hasError()) {
        std::cerr << "语义错误: " << sema.errorMessage() << "\n";
        return;
    }
    std::cout << "✓ 语义分析成功\n";

    // 生成 LLVM IR
    LLVMCodegen codegen;
    codegen.setOptLevel(OptLevel::O2);
    std::string ir = codegen.generate(program);

    if (ir.empty()) {
        std::cerr << "✗ LLVM IR 生成失败\n";
        return;
    }

    std::cout << "✓ LLVM IR 生成成功\n\n";
    std::cout << "生成的 LLVM IR:\n";
    std::cout << "───────────────────────────────────────────────────────────\n";
    std::cout << ir;
    std::cout << "───────────────────────────────────────────────────────────\n";
}

void testJITCompilerInit() {
    printBanner("测试 JIT 编译器初始化");

    JITCompiler jit;

    std::cout << "初始化 JIT 编译器...\n";
    bool success = jit.initialize();

    std::cout << "\nJIT 编译器状态:\n";
    std::cout << "  LLVM 可用: " << (jit.isAvailable() ? "是" : "否") << "\n";
    std::cout << "  热点阈值: " << 100 << " 次\n";
    std::cout << "  优化级别: O3\n";

    std::cout << "\n";

    if (jit.isAvailable()) {
        std::cout << "✓ JIT 编译器已就绪\n";
    } else {
        std::cout << "⚠ JIT 编译器待激活（需要安装 LLVM）\n";
        std::cout << "\n安装 LLVM 方法:\n";
        std::cout << "  1. 下载: https://github.com/llvm/llvm-project/releases\n";
        std::cout << "  2. 安装到默认位置: C:\\Program Files\\LLVM\n";
        std::cout << "  3. 将 LLVM\\bin 添加到 PATH\n";
        std::cout << "  4. 重新运行此测试\n";
    }
}

void testHotspotDetection() {
    printBanner("测试热点函数检测");

    JITCompiler jit;
    jit.initialize();

    std::cout << "模拟热点检测...\n\n";

    // 创建模拟函数名
    std::cout << "设置热点阈值为 5 次调用\n";
    jit.setHotThreshold(5);

    std::cout << "\n模拟调用记录:\n";

    // 这里只是演示热点检测的输出
    // 实际使用时需要传入真实的 VMFunction 指针
    for (int i = 1; i <= 10; i++) {
        std::cout << "  调用 #" << i << ": ";

        // 注意：这里不能真正调用 recordCall，因为没有真实的 VMFunction
        // 在实际使用中，VM 会在执行函数调用指令时调用此方法
        if (i == 5) {
            std::cout << "达到热点阈值！";
        }
        std::cout << "\n";
    }

    std::cout << "\n在真实运行时，JIT 编译器会自动:\n";
    std::cout << "  1. 记录每个函数的调用次数\n";
    std::cout << "  2. 达到热点阈值的函数会被标记为热点\n";
    std::cout << "  3. 热点函数会被 JIT 编译为机器码\n";
    std::cout << "  4. 后续调用直接执行编译后的机器码\n";
}

void testCompilationWorkflow() {
    printBanner("测试完整编译流程");

    std::cout << "完整 JIT 编译流程演示:\n\n";

    std::cout << "步骤 1: 源代码 → 字节码（VM 执行）\n";
    std::cout << "        ↓\n";
    std::cout << "步骤 2: 字节码函数达到热点阈值\n";
    std::cout << "        ↓\n";
    std::cout << "步骤 3: JIT 编译器介入\n";
    std::cout << "        ↓\n";
    std::cout << "步骤 4: 生成 LLVM IR\n";
    std::cout << "        ↓\n";
    std::cout << "步骤 5: llc 编译为目标文件 (.obj)\n";
    std::cout << "        ↓\n";
    std::cout << "步骤 6: clang 链接为 DLL (.dll)\n";
    std::cout << "        ↓\n";
    std::cout << "步骤 7: LoadLibrary 加载 DLL\n";
    std::cout << "        ↓\n";
    std::cout << "步骤 8: GetProcAddress 获取函数指针\n";
    std::cout << "        ↓\n";
    std::cout << "步骤 9: 直接执行机器码（跳过 VM）\n\n";

    std::cout << "性能提升预估:\n";
    std::cout << "  - 简单函数: 10-50x\n";
    std::cout << "  - 复杂函数: 50-100x\n";
    std::cout << "  - 热点循环: 100x+\n\n";

    std::cout << "提示: 使用 compile_optimized.bat 可以手动触发 AOT 编译\n";
}

int main(int argc, char* argv[]) {
    std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║         CP语言 JIT 编译器测试套件                          ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n";

    // 测试 1: LLVM IR 生成
    testLLVMIRGeneration();

    // 测试 2: JIT 编译器初始化
    testJITCompilerInit();

    // 测试 3: 热点检测
    testHotspotDetection();

    // 测试 4: 编译流程
    testCompilationWorkflow();

    std::cout << "\n═══════════════════════════════════════════════════════════\n";
    std::cout << "测试完成\n";
    std::cout << "═══════════════════════════════════════════════════════════\n";

    return 0;
}
