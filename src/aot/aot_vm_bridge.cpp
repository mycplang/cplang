// AOT VM 桥接 — 在 AOT 可执行文件中嵌入轻量 VM 用于调用 stdlib
//
// 关键发现：VM 的 Value 类使用与 LLVM codegen 完全相同的 NaN-boxing 方案
// (bits 48-63=0xFFFF 标记非浮点值, bit 47=0 指针位, bit 47=1 立即数)。
// 因此整数/bool/nil 可以直接通过 raw() 传递，无需转换。
// 字符串需要转换：LLVM codegen 使用 C 字符串指针 → VM 使用 VMString*。

#include "aot/aot_vm_bridge.hpp"
#include "vm/vm.hpp"
#include "stdlib/stdlib.hpp"
#include "module/module_system.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>

namespace cplang {
    static VM* g_aot_vm = nullptr;

    // AOT 指针 → 转为 VM Value
    // AOT codegen 中的指针可能是：
    //   字符串 → 指向 char[] 的 NaN-boxed 指针
    //   表    → 指向 TableData 的 NaN-boxed 指针
    // 通过检查 TABLE_MAGIC 区分表和字符串
    static Value aot_ptr_to_value(VM* vm, uint64_t v) {
        const void* ptr = reinterpret_cast<const void*>(v & 0x0000FFFFFFFFFFFFULL);
        if (!ptr) return Value::nil();
        // 检查是否是表（TableData 首字段为 TABLE_MAGIC）
        if (*(const uint64_t*)ptr == 0x43504C5441424C45ULL) {  // "CPLTABLE"
            // 表指针：直接返回 NaN-boxed 指针值（VM 可识别）
            return Value(v);
        }
        // 字符串：通过 VM intern 创建 VMString
        const char* s = static_cast<const char*>(ptr);
        uint32_t len = static_cast<uint32_t>(std::strlen(s));
        VMString* str = vm->internString(s, len);
        uint64_t raw = 0xFFFF000000000000ULL | (reinterpret_cast<uint64_t>(str) & 0x0000FFFFFFFFFFFFULL);
        return Value(raw);
    }

    // VM Value 中的字符串 → 转为 AOT 可用的 NaN-boxed 指针
    static uint64_t value_to_aot_str(const Value& val) {
        VMString* s = reinterpret_cast<VMString*>(
            reinterpret_cast<uintptr_t>(val.asString())
        );
        if (!s || !s->data) return 0;
        return 0xFFFF000000000000ULL | (reinterpret_cast<uint64_t>(s->data) & 0x0000FFFFFFFFFFFFULL);
    }
}

using namespace cplang;

void aot_init_runtime(void) {
    if (g_aot_vm) return;
    g_aot_vm = new VM();
    if (!g_aot_vm) { std::fprintf(stderr, "[AOT] 无法创建 VM\n"); std::abort(); }
    StdLib::registerAll(g_aot_vm);
}

uint64_t aot_call_native(const char* name, int32_t argc, uint64_t* args) {
    if (!g_aot_vm) aot_init_runtime();

    // 1. 按名称查找全局 slot
    Int32 slot = g_aot_vm->getGlobalSlot(name);
    if (slot < 0) return 0;

    // 2. 获取全局值 → 验证是原生函数
    Value* gv = g_aot_vm->getGlobalBySlot(static_cast<UInt16>(slot));
    if (!gv || !gv->isPtr()) return 0;
    VMObject* obj = gv->asPtr();
    if (!obj || obj->typeTag != ObjectHeader::TAG_NATIVE) return 0;
    VMNativeFunc* nf = static_cast<VMNativeFunc*>(obj);

    // 3. 构造参数数组
    //    LLVM codegen 用裸 i64 存整数，VM 用 NaN-boxing 标记值
    //    非指针值（bits 48-63 ≠ 0xFFFF）→ 创建整数 Value
    //    指针值（bits 48-63 = 0xFFFF, bit 47 = 0）→ 字符串/表
    std::vector<Value> vm_args(argc);
    for (int32_t i = 0; i < argc; i++) {
        if ((args[i] >> 48) == 0xFFFF && !(args[i] & 0x0000800000000000ULL)) {
            vm_args[i] = aot_ptr_to_value(g_aot_vm, args[i]);
        } else {
            // LLVM 整数 → VM Int32 Value（NaN-boxing Int32 编码）
            vm_args[i] = Value::fromInt32(static_cast<Int32>(static_cast<int64_t>(args[i])));
        }
    }

    // 4. 调用原生函数
    Value result;
    try {
        result = nf->fn(vm_args);
    } catch (...) { return 0; }

    // 5. 结果转换
    uint64_t raw = result.raw();
    // 如果结果是字符串指针，返回 AOT 可用的 NaN-boxed 字符串
    if (result.isString()) {
        return value_to_aot_str(result);
    }
    return raw;
}

int32_t aot_import_module(const char* name) {
    if (!g_aot_vm) aot_init_runtime();
    return importModule(g_aot_vm, std::string(name)) ? 0 : 1;
}

uint64_t aot_get_global(const char* name) {
    if (!g_aot_vm) aot_init_runtime();
    Int32 slot = g_aot_vm->getGlobalSlot(name);
    if (slot < 0) return 0;
    Value* val = g_aot_vm->getGlobalBySlot(static_cast<UInt16>(slot));
    if (!val) return 0;
    return val->raw();
}

// ═══ JIT 运行时函数桩 (AOT 模式下使用) ═══
// 这些函数在 JIT 模式中由 jit_runtime.cpp 提供，但 AOT 链接时
// LLVM codegen 可能生成对它们的引用（来自 standalone 函数映射）。

extern "C" {

uint64_t jit_len(uint64_t raw) {
    using namespace cplang;
    if (!g_aot_vm) aot_init_runtime();
    Value v = aot_ptr_to_value(g_aot_vm, raw);
    if (v.isString()) {
        VMString* s = v.asString();
        if (s) return static_cast<uint64_t>(s->length);
    }
    if (v.isTable()) {
        VMTable* t = v.asTable();
        if (t) return static_cast<uint64_t>(t->size());
    }
    if ((raw >> 48) == 0xFFFF && !(raw & 0x0000800000000000ULL)) {
        const char* str = reinterpret_cast<const char*>(raw & 0x0000FFFFFFFFFFFFULL);
        if (str) return static_cast<uint64_t>(std::strlen(str));
    }
    return 0;
}

uint64_t jit_toString(uint64_t raw) {
    using namespace cplang;
    if (!g_aot_vm) aot_init_runtime();
    if ((raw >> 48) == 0xFFFF && !(raw & 0x0000800000000000ULL)) return raw;
    std::string s = std::to_string(static_cast<int64_t>(raw));
    VMString* str = g_aot_vm->internString(s.c_str(), static_cast<uint32_t>(s.size()));
    return 0xFFFF000000000000ULL | (reinterpret_cast<uint64_t>(str->data) & 0x0000FFFFFFFFFFFFULL);
}

void jit_setVM(void*) { /* AOT: no-op */ }

} // extern "C"

void aot_cleanup_runtime(void) {
    delete g_aot_vm;
    g_aot_vm = nullptr;
}