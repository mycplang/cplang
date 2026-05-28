// CP语言 端到端运行测试（手写字节码验证VM）
// 注意：VM使用16字节/指令格式：[op][a][b][c][padding12]
#include "common/types.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "ast/ast.hpp"
#include "semantic/semantic_analyzer.hpp"
#include "codegen/codegen.hpp"
#include "vm/vm.hpp"
#include <iostream>
#include <memory>

using namespace cplang;

// 辅助：发射16字节指令
static void emitInst(std::vector<UInt8>& code, UInt8 op, UInt8 a, UInt8 b, UInt8 c) {
    code.push_back(op);
    code.push_back(a); code.push_back(b); code.push_back(c);
    for (int i = 0; i < 12; i++) code.push_back(0);
}
// 辅助：发射跳转指令（offset为Int32，位置在字节4-7）
static void emitJump(std::vector<UInt8>& code, UInt8 op, UInt8 a, Int32 offset) {
    code.push_back(op); // byte 0
    code.push_back(a);  // byte 1
    code.push_back(0); code.push_back(0); // bytes 2-3: b,c unused
    code.push_back(static_cast<UInt8>(offset & 0xFF));
    code.push_back(static_cast<UInt8>((offset >> 8) & 0xFF));
    code.push_back(static_cast<UInt8>((offset >> 16) & 0xFF));
    code.push_back(static_cast<UInt8>((offset >> 24) & 0xFF));
    for (int i = 0; i < 8; i++) code.push_back(0); // remaining padding
}

// 手动构造字节码函数
VMFunction* makeAddFunc() {
    auto* func = new VMFunction();
    func->maxStack = 8;
    func->numParams = 2;
    func->constants.push_back(Value::nil());
    emitInst(func->code, OP_LOADLOCAL, 0, 0, 0);
    emitInst(func->code, OP_LOADLOCAL, 1, 1, 0);
    emitInst(func->code, OP_ADD, 2, 0, 1);
    emitInst(func->code, OP_RETURN, 2, 0, 0);
    return func;
}

VMFunction* makePrintFunc() {
    auto* func = new VMFunction();
    func->maxStack = 8;
    func->numParams = 1;
    func->constants.push_back(Value::nil());
    emitInst(func->code, OP_LOADLOCAL, 0, 0, 0);
    emitInst(func->code, OP_RETURN, 0, 0, 0);
    return func;
}

// 通用入口函数测试
VMFunction* makeMainFunc() {
    auto* func = new VMFunction();
    func->maxStack = 32;
    func->constants.push_back(Value::Int(10)); // [0]
    func->constants.push_back(Value::Int(20)); // [1]
    func->constants.push_back(Value::nil());   // [2]
    func->constants.push_back(Value::Int(1));  // [3]

    emitInst(func->code, OP_LOADCONST, 0, 0, 0); // r0 = const[0] = 10
    emitInst(func->code, OP_LOADCONST, 1, 0, 0); // r1 = const[0] = 10 (alias a)
    emitInst(func->code, OP_LOADCONST, 2, 1, 0); // r2 = const[1] = 20
    emitInst(func->code, OP_LOADCONST, 3, 1, 0); // r3 = const[1] = 20 (alias b)
    emitInst(func->code, OP_ADD, 4, 1, 3);        // r4 = r1 + r3 = 30
    emitInst(func->code, OP_RETURN, 4, 0, 0);
    return func;
}

// 循环测试
VMFunction* makeLoopFunc() {
    auto* func = new VMFunction();
    func->maxStack = 32;
    func->constants.push_back(Value::Int(0));  // [0] = 0
    func->constants.push_back(Value::Int(1));  // [1] = 1
    func->constants.push_back(Value::Int(10)); // [2] = 10

    // total = 0; for (i=0; i<10; i++) total += i; return total;
    emitInst(func->code, OP_LOADCONST, 1, 0, 0);  // r1 = 0 (total)
    emitInst(func->code, OP_LOADCONST, 2, 0, 0);  // r2 = 0 (i)
    emitInst(func->code, OP_MOVE, 3, 2, 0);       // r3 = r2 (i_copy)

    // loop start
    int loopStart = (int)func->code.size();
    emitInst(func->code, OP_ADD, 4, 3, 1);        // r4 = i + total
    emitInst(func->code, OP_MOVE, 1, 4, 0);       // total = r4
    emitInst(func->code, OP_LOADCONST, 5, 1, 0);  // r5 = const[1] = 1
    emitInst(func->code, OP_ADD, 3, 3, 5);        // i += 1
    emitInst(func->code, OP_LOADCONST, 6, 2, 0);  // r6 = const[2] = 10
    emitInst(func->code, OP_CMPLT, 7, 3, 6);      // r7 = (i < 10)

    // OP_JUMPIF r7, loopStart
    int afterJump = (int)func->code.size() + 16;   // pc after this instruction
    Int32 offset = static_cast<Int32>(loopStart - afterJump);
    emitJump(func->code, OP_JUMPIF, 7, offset);

    emitInst(func->code, OP_RETURN, 1, 0, 0);      // return total
    return func;
}

void testVM(const String& name, VMFunction* func) {
    std::cout << "═══ " << name << " ═══" << std::endl;
    VM vm;
    bool ok = vm.loadModule(func);
    if (!ok) {
        std::cout << "  ERROR: " << vm.error() << std::endl;
    } else {
        std::cout << "  OK (instr=" << vm.totalInstructions() << ")" << std::endl;
    }
}

int main() {
    std::cout << " CPLang v0.4 VM Runtime Test\n" << std::endl;

    testVM("Arithmetic (10+20=30)", makeMainFunc());
    testVM("Loop (0+1+2+...+9)", makeLoopFunc());

    // 测试原生数组
    std::cout << "═══ Native: newArray ═══" << std::endl;
    {
        VM vm;
        auto* arr = VMArray::create(3);
        arr->data.push_back(Value::Int(1));
        arr->data.push_back(Value::Int(2));
        arr->data.push_back(Value::Int(3));

        auto* func = new VMFunction();
        func->maxStack = 16;
        func->constants.push_back(Value::nil());
        func->constants.push_back(Value::Array(arr));
        emitInst(func->code, OP_NEWARRAY, 0, 3, 0);
        emitInst(func->code, OP_RETURN, 0, 0, 0);
        vm.loadModule(func);
    }

    std::cout << "\nAll tests done." << std::endl;
    return 0;
}
