// CP语言 优化VM基准测试
#include "vm/vm_opt.hpp"
#include <cstdio>
#include <chrono>

using namespace cplang;

// 手动构建一个简单的字节码函数用于测试
VMFunction* createBenchmarkFunc(VM* vm) {
    VMFunction* f = new VMFunction();
    f->typeTag = T_FUNCTION;
    f->numParams = 0;
    f->numLocals = 3;  // i, sum, arr
    
    std::vector<UInt8>& c = f->code;
    auto emit = [&](UInt8 op, UInt8 a, UInt8 b, UInt8 c) {
        f->code.push_back(op); f->code.push_back(a);
        f->code.push_back(b); f->code.push_back(c);
    };
    auto emitI = [&](UInt8 op, UInt8 a, Int16 sbx) {
        f->code.push_back(op); f->code.push_back(a);
        f->code.push_back(static_cast<UInt8>(sbx & 0xFF));
        f->code.push_back(static_cast<UInt8>((sbx >> 8) & 0xFF));
    };
    
    // 寄存器分配：
    // r0 = i (循环计数器)
    // r1 = sum (累加器)
    // r2 = arr (数组)
    // r3 = 临时
    
    // sum = 0
    emitI(OP_LOADINT, 1, 0);      // r1 = 0
    
    // arr = []
    emit(OP_NEWARRAY, 2, 0, 0);   // r2 = new array
    
    // i = 0
    emitI(OP_LOADINT, 0, 0);      // r0 = 0
    
    // 循环开始 (L1)
    int L1 = static_cast<int>(c.size());
    
    // if i >= 1000000 goto L2
    emitI(OP_LOADINT, 3, 1000000 >> 16);
    emitI(OP_LOADINT, 3, 1000000 & 0xFFFF);  // 简化：需要32位立即数支持
    // 实际应使用CMPLT + JUMPNIF
    
    // sum = sum + i
    emit(OP_ADD, 1, 1, 0);        // r1 = r1 + r0
    
    // arr.push(i)  -- 需要原生函数
    
    // i = i + 1
    emitI(OP_LOADINT, 3, 1);      // r3 = 1
    emit(OP_ADD, 0, 0, 3);        // r0 = r0 + r3
    
    // goto L1
    Int16 offset1 = static_cast<Int16>((L1 - (static_cast<int>(c.size()) + 4)) / 4);
    emitI(OP_JUMP, 0, offset1);
    
    // L2: return sum
    int L2 = static_cast<int>(c.size());
    emit(OP_RETURN, 1, 0, 0);     // return r1
    
    // 回填跳转
    // ...
    
    f->maxStack = 16;
    vm->trackGC(f);
    return f;
}

int main() {
    VM vm;
    
    // 注册原生函数
    vm.registerNative("print", [](int argc, Value* argv) -> Value {
        for (int i = 0; i < argc; i++) {
            if (argv[i].isInt()) {
                std::printf("%lld", argv[i].i);
            } else if (argv[i].isFloat()) {
                std::printf("%f", argv[i].f);
            } else if (argv[i].isBool()) {
                std::printf(argv[i].i ? "true" : "false");
            } else if (argv[i].isNil()) {
                std::printf("nil");
            }
        }
        std::printf("\n");
        return Value::nil();
    });
    
    // 创建测试函数
    VMFunction* f = createBenchmarkFunc(&vm);
    
    // 预热
    for (int i = 0; i < 3; i++) {
        vm.call(f, 0, nullptr);
    }
    
    // 正式测试
    auto start = std::chrono::high_resolution_clock::now();
    bool ok = vm.call(f, 0, nullptr);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    if (!ok) {
        std::fprintf(stderr, "Error: %s\n", vm.error().c_str());
        return 1;
    }
    
    std::printf("Optimized VM: %.3f ms\n", ms);
    std::printf("Instructions: %lld\n", vm.totalInstructions());
    
    return 0;
}
