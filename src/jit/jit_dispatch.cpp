// JIT 分派逻辑 — 从 VM 字节码循环中提取，保持 VM 纯净
// 由 vm_exec.cpp 的 CALL 指令处理器调用
// 通过 JITRegistry 访问 JIT 元数据，不直接接触 VMFunction 内部字段

#include "vm/vm.hpp"
#include "jit/hybrid_jit.hpp"
#include "jit/jit_runtime.hpp"
#include <cstdint>

namespace cplang {

// 尝试通过 JIT 执行函数调用
// 返回 true 表示 JIT 已处理（设置了 result），false 表示需要 VM 字节码解释
bool jitTryCallDispatch(VM* vm, VMFunction* func, int argc, Value* args, Value& result) {
    // 设置 currentVM 和全局 VM 指针以便 JIT 运行时回调使用
    VM* savedVM = VM::current();
    VM::setCurrent(vm);
    jit_setVM(vm);
    HybridJIT* jit = vm->getJIT();
    if (!jit) { VM::setCurrent(savedVM); return false; }
    JITRegistry& reg = jit->getJITRegistry();
    JITInfo& info = reg.get(func);

    // 渐进类型：全 typed 指令或显式类型标注 → 直接 JIT 编译（跳过字节码）
    if ((func->isTyped || func->hasExplicitTypes) && !info.compiled) {
        void* entry = jit->compileHotFunction(func);
        if (entry) {
            info.entry = entry;
            info.compiled = true;
        }
    }

    // 快速路径：已编译 → 直接原生调用
    if (info.compiled && info.entry) {
        auto arg = [&](int i) -> int64_t {
            if (i < argc) return (int64_t)args[i].asInt();
            return 0;
        };
        using Fn0  = int64_t(*)(void);
        using Fn1  = int64_t(*)(int64_t);
        using Fn2  = int64_t(*)(int64_t,int64_t);
        using Fn3  = int64_t(*)(int64_t,int64_t,int64_t);
        using Fn4  = int64_t(*)(int64_t,int64_t,int64_t,int64_t);
        using Fn5  = int64_t(*)(int64_t,int64_t,int64_t,int64_t,int64_t);
        using Fn6  = int64_t(*)(int64_t,int64_t,int64_t,int64_t,int64_t,int64_t);
        using Fn7  = int64_t(*)(int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t);
        using Fn8  = int64_t(*)(int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t);
        using Fn9  = int64_t(*)(int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t);
        using Fn10 = int64_t(*)(int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t);
        using Fn11 = int64_t(*)(int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t);
        using Fn12 = int64_t(*)(int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t);

        int64_t r = 0;
        switch (argc) {
            case  0: r = ((Fn0) info.entry)(); break;
            case  1: r = ((Fn1) info.entry)(arg(0)); break;
            case  2: r = ((Fn2) info.entry)(arg(0),arg(1)); break;
            case  3: r = ((Fn3) info.entry)(arg(0),arg(1),arg(2)); break;
            case  4: r = ((Fn4) info.entry)(arg(0),arg(1),arg(2),arg(3)); break;
            case  5: r = ((Fn5) info.entry)(arg(0),arg(1),arg(2),arg(3),arg(4)); break;
            case  6: r = ((Fn6) info.entry)(arg(0),arg(1),arg(2),arg(3),arg(4),arg(5)); break;
            case  7: r = ((Fn7) info.entry)(arg(0),arg(1),arg(2),arg(3),arg(4),arg(5),arg(6)); break;
            case  8: r = ((Fn8) info.entry)(arg(0),arg(1),arg(2),arg(3),arg(4),arg(5),arg(6),arg(7)); break;
            case  9: r = ((Fn9) info.entry)(arg(0),arg(1),arg(2),arg(3),arg(4),arg(5),arg(6),arg(7),arg(8)); break;
            case 10: r = ((Fn10)info.entry)(arg(0),arg(1),arg(2),arg(3),arg(4),arg(5),arg(6),arg(7),arg(8),arg(9)); break;
            case 11: r = ((Fn11)info.entry)(arg(0),arg(1),arg(2),arg(3),arg(4),arg(5),arg(6),arg(7),arg(8),arg(9),arg(10)); break;
            case 12: r = ((Fn12)info.entry)(arg(0),arg(1),arg(2),arg(3),arg(4),arg(5),arg(6),arg(7),arg(8),arg(9),arg(10),arg(11)); break;
            default: r = 0; break;
        }
        result = Value::Int(r);
        VM::setCurrent(savedVM);
		return true;
    }

    // 未编译：记录调用，达到阈值后触发编译
    jit->recordCall(func);
    if (jit->shouldCompile(func)) {
        void* entry2 = jit->compileHotFunction(func);
        if (entry2) {
            info.entry = entry2;
            info.compiled = true;
            // 编译成功：直接用 JIT 入口执行
            auto jitArg = [&](int i) -> int64_t {
                if (i < argc) return (int64_t)args[i].asInt();
                return 0;
            };
            int64_t r = 0;
            switch (argc) {
                case 0: r = ((int64_t(*)(void))entry2)(); break;
                case 1: r = ((int64_t(*)(int64_t))entry2)(jitArg(0)); break;
                case 2: r = ((int64_t(*)(int64_t,int64_t))entry2)(jitArg(0),jitArg(1)); break;
                default: r = 0; break;
            }
            result = Value::Int(r);
            VM::setCurrent(savedVM);
			return true;
        }
    }

    VM::setCurrent(savedVM);
    return false;
}

} // namespace cplang