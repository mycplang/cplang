// 最简VM测试
#include "vm/vm.hpp"
#include <iostream>
using namespace cplang;

VMFunction* makeMain() {
    auto* func = new VMFunction();
    func->maxStack = 32;
    func->constants.push_back(Value::Int(10));
    func->constants.push_back(Value::Int(20));
    func->code = {
        OP_LOADCONST, 0, 0, 0,  // r0 = 10
        OP_LOADCONST, 1, 0, 0,  // r1 = 10
        OP_LOADCONST, 2, 1, 0,  // r2 = 20
        OP_ADD, 3, 0, 2,        // r3 = r0 + r2 = 30
        OP_RETURN, 3, 0, 0,     // return r3
    };
    return func;
}

int main() {
    std::cerr << "1. creating VM..." << std::endl;
    VM vm;
    std::cerr << "2. VM created, registering..." << std::endl;
    vm.registerNative("print", [](std::vector<Value>& args) -> Value {
        for (auto& v : args) std::cout << v.toString() << " ";
        std::cout << std::endl;
        return Value::nil();
    });
    std::cerr << "3. registered, creating func..." << std::endl;
    VMFunction* func = makeMain();
    std::cerr << "4. func created, loading..." << std::endl;
    bool ok = vm.loadModule(func);
    std::cerr << "5. loadModule=" << ok << " instr=" << vm.totalInstructions() << std::endl;
    return 0;
}
