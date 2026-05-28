// 模块系统测试程序

#include <iostream>
#include <fstream>
#include "module/module_system.hpp"
#include "codegen/codegen.hpp"
#include "vm/vm.hpp"
#include "stdlib/stdlib.hpp"

using namespace cplang;

// 测试模块导入
bool testModuleImport() {
    std::cout << "\n=== 测试模块导入 ===\n";

    // 创建 VM
    VM vm;
    StdLib::registerAll(&vm);

    // 创建模块管理器
    ModuleManager manager(&vm);

    // 添加搜索路径
    manager.loader()->addSearchPath("tests");

    // 尝试导入模块
    std::cout << "导入 math_utils 模块...\n";
    bool result = manager.import("math_utils");

    if (!result) {
        std::cout << "导入失败: " << manager.lastError() << "\n";
        return false;
    }

    std::cout << "导入成功!\n";

    // 检查模块是否已导入
    if (manager.isImported("math_utils")) {
        std::cout << "模块已正确导入\n";
    } else {
        std::cout << "模块导入状态异常\n";
        return false;
    }

    return true;
}

// 测试循环依赖检测
bool testCircularDependency() {
    std::cout << "\n=== 测试循环依赖检测 ===\n";

    // 创建 VM
    VM vm;
    StdLib::registerAll(&vm);

    // 创建模块管理器
    ModuleManager manager(&vm);
    manager.loader()->addSearchPath("tests");

    // 尝试导入有循环依赖的模块
    std::cout << "导入 circular_a 模块（测试循环依赖）...\n";
    bool result = manager.import("circular_a");

    if (result) {
        std::cout << "警告: 循环依赖未检测到\n";
        return false;
    }

    std::cout << "循环依赖检测成功: " << manager.lastError() << "\n";
    return true;
}

// 测试模块缓存
bool testModuleCache() {
    std::cout << "\n=== 测试模块缓存 ===\n";

    // 创建 VM
    VM vm;
    StdLib::registerAll(&vm);

    // 创建模块管理器
    ModuleManager manager(&vm);
    manager.loader()->addSearchPath("tests");

    // 第一次导入
    std::cout << "第一次导入 math_utils...\n";
    bool result1 = manager.import("math_utils");
    if (!result1) {
        std::cout << "第一次导入失败: " << manager.lastError() << "\n";
        return false;
    }

    // 第二次导入（应该使用缓存）
    std::cout << "第二次导入 math_utils（应该使用缓存）...\n";
    bool result2 = manager.import("math_utils");
    if (!result2) {
        std::cout << "第二次导入失败: " << manager.lastError() << "\n";
        return false;
    }

    std::cout << "模块缓存测试通过\n";
    return true;
}

// 测试模块路径解析
bool testModulePathResolution() {
    std::cout << "\n=== 测试模块路径解析 ===\n";

    // 创建 VM
    VM vm;
    StdLib::registerAll(&vm);

    // 创建模块管理器
    ModuleManager manager(&vm);

    // 添加自定义搜索路径
    manager.loader()->addSearchPath("tests");

    // 测试导入
    std::cout << "从 tests/ 目录导入 math_utils...\n";
    bool result = manager.import("math_utils");

    if (!result) {
        std::cout << "路径解析失败: " << manager.lastError() << "\n";
        return false;
    }

    std::cout << "路径解析成功\n";
    return true;
}

int main() {
    std::cout << "╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║        CP语言模块系统测试                          ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n";

    int passed = 0;
    int total = 0;

    // 运行测试
    total++;
    if (testModuleImport()) {
        passed++;
        std::cout << "✅ 模块导入测试通过\n";
    } else {
        std::cout << "❌ 模块导入测试失败\n";
    }

    total++;
    if (testModuleCache()) {
        passed++;
        std::cout << "✅ 模块缓存测试通过\n";
    } else {
        std::cout << "❌ 模块缓存测试失败\n";
    }

    total++;
    if (testModulePathResolution()) {
        passed++;
        std::cout << "✅ 模块路径解析测试通过\n";
    } else {
        std::cout << "❌ 模块路径解析测试失败\n";
    }

    // 循环依赖测试（需要创建循环依赖的测试模块）
    total++;
    if (testCircularDependency()) {
        passed++;
        std::cout << "✅ 循环依赖检测测试通过\n";
    } else {
        std::cout << "❌ 循环依赖检测测试失败\n";
    }

    // 总结
    std::cout << "\n╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║        测试结果总结                                  ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n";
    std::cout << "通过: " << passed << "/" << total << "\n";
    std::cout << "失败: " << (total - passed) << "/" << total << "\n";

    return (passed == total) ? 0 : 1;
}
