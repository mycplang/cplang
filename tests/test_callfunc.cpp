// 测试 callFunction 的独立程序
#include "codegen/codegen.hpp"
#include "vm/vm.hpp"
#include "stdlib/stdlib.hpp"
#include <iostream>

int main() {
    cplang::Compiler compiler;
    cplang::VM* vm = compiler.vm();
    cplang::StdLib::registerAll(vm);
    
    // 编译一个返回表达式的函数
    std::string wrapped = "函数 __repl_expr__() { 返回 1+2; }";
    auto func = compiler.compile(wrapped);
    if (!func) {
        std::cerr << "编译失败: " << compiler.errorMessage() << std::endl;
        return 1;
    }
    
    // 加载模块
    if (!vm->loadModule(func)) {
        std::cerr << "加载失败: " << vm->error() << std::endl;
        return 1;
    }
    
    // 查找函数并调用
    int slot = vm->getGlobalSlot("__repl_expr__");
    if (slot < 0) {
        std::cerr << "函数未找到!" << std::endl;
        return 1;
    }
    
    cplang::Value* funcValPtr = vm->getGlobalBySlot(static_cast<cplang::UInt16>(slot));
    if (!funcValPtr) {
        std::cerr << "函数值为空!" << std::endl;
        return 1;
    }
    
    std::cout << "函数类型: isClosure=" << funcValPtr->isClosure() 
              << " isCFunction=" << funcValPtr->isCFunction() 
              << " isFunction=" << funcValPtr->isFunction() << std::endl;
    std::cout << "函数值 raw=" << std::hex << funcValPtr->raw() << std::dec << std::endl;
    
    std::vector<cplang::Value> args;
    cplang::Value result = vm->callFunction(*funcValPtr, args);
    
    std::cout << "结果: isNil=" << result.isNil()
              << " isInt=" << result.isInt()
              << " isFloat=" << result.isFloat()
              << " isBool=" << result.isBool()
              << std::endl;
    
    if (result.isInt()) {
        std::cout << "整数值: " << result.asInt() << std::endl;
    }
    if (result.isFloat()) {
        std::cout << "浮点值: " << result.asFloat() << std::endl;
    }
    
    return 0;
}
