// JIT 运行时辅助函数实现
//
// 这些函数被 ORC JIT 编译后的机器码直接调用。
// 使用 extern "C" 链接以确保符号名与 LLVM IR 中的声明一致。
//
// 注意: 此文件编译为 cplang.exe 的一部分，可安全依赖 cplang 运行时头文件。
// AOT 链接使用 jit_runtime_standalone.cpp 的独立编译版本。

#include "jit/jit_runtime.hpp"
#include "vm/value.hpp"
#include "vm/vm.hpp"
#include <iostream>
#include <vector>
#include <mutex>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <atomic>

namespace {

// 临时对象跟踪：JIT 辅助函数分配的堆对象，
// 在程序结束时由 jit_cleanup() 释放
struct JitTempObjects {
    std::vector<cplang::VMObject*> objects;
    std::mutex mutex;
};
JitTempObjects& tempObjects() {
    static JitTempObjects instance;
    return instance;
}

} // anonymous namespace

extern "C" {

// 全局 VM 指针（用于 JIT 运行时函数访问 VM，避免 thread_local 在 JIT 调用链中的问题）
static std::atomic<cplang::VM*> g_jitVM{nullptr};
void jit_setVM(void* vm) { 
    g_jitVM.store(static_cast<cplang::VM*>(vm), std::memory_order_release);
}

// 从 NaN-boxed raw 值提取字符串内容
static std::string extractString(uint64_t raw) {
    // 检查是否是 NaN-boxed 指针
    if ((raw >> 48) == 0xFFFF && !(raw & 0x0000800000000000ULL)) {
        const void* ptr = reinterpret_cast<const void*>(raw & 0x0000FFFFFFFFFFFFULL);
        if (!ptr) return "nil";
        // 通过 vtable 指针区分 VM 对象和 LLVM char*:
        // VM 对象的前 8 字节是 vtable 指针（通常在代码段，0x00007FF...开头）
        // LLVM char* 的前 8 字节是字符串内容（ASCII/UTF-8，不会是代码段地址）
        uint64_t first8 = *(const uint64_t*)ptr;
        bool looksLikeVTable = (first8 >> 40) == 0x7F; // 代码段通常在 0x7FF... 范围
        if (looksLikeVTable) {
            auto* obj = reinterpret_cast<const cplang::VMObject*>(ptr);
            if (obj->typeTag == cplang::ObjectHeader::TAG_STRING) {
                auto* str = (cplang::VMString*)(obj);
                return std::string(str->data, str->length);
            }
        }
        // 不是 VM 对象 → LLVM 全局 char* 常量
        return std::string(static_cast<const char*>(ptr));
    }
    // 非指针 → 可能是 NaN-boxed 值或 JIT codegen 产出的 raw integer
    // NaN-boxed 的 typed immediate 在此处正确解码
    // 但对于 raw integer（JIT codegen 不装箱），按 IEEE 754 解析会得到 denormalized double
    // 检查 exponent 位：所有 NaN-boxed 值的高 16 位都是 0xFFFF
    // 普通 double 的 exponent 位（52-62）不会是 0（除非是 0.0 或 denormalized）
    // raw integer 的 exponent 位 = 0（因为整数的高 12 位都是 0）
    uint64_t exp = (raw >> 52) & 0x7FF;
    if (exp == 0 && raw != 0 && raw != 0x8000000000000000ULL) {
        // exponent=0 且非零 → denormalized double，几乎是 raw integer
        return std::to_string(static_cast<int64_t>(raw));
    }
    return cplang::Value(raw).toString();
}

uint64_t jit_strcat(uint64_t a, uint64_t b) {
    using namespace cplang;
    
    std::string sa = extractString(a);
    std::string sb = extractString(b);
    std::string result = sa + sb;
    
    auto* str = VMString::create(result);
    
    auto& to = tempObjects();
    {
        std::lock_guard<std::mutex> lock(to.mutex);
        to.objects.push_back(reinterpret_cast<cplang::VMObject*>(str));
    }
    
    return Value::Ptr(str).raw();
}

void jit_printv(int32_t count, uint64_t* args) {
    using namespace cplang;
    
    for (int32_t i = 0; i < count; i++) {
        if (i > 0) std::cout << " ";
        std::cout << extractString(args[i]);
    }
    std::cout << std::endl;
    std::cout.flush();
}

uint64_t jit_len(uint64_t raw) {
    // 支持字符串、数组、表的长度计算
    using namespace cplang;
    // 检查是否是 NaN-boxed 的 raw integer（JIT codegen 不装箱）
    uint64_t exp = (raw >> 52) & 0x7FF;
    if (exp == 0 && raw != 0 && raw != 0x8000000000000000ULL) {
        // 不是 VM 对象，返回 raw integer 自身
        return raw;
    }
    if ((raw >> 48) == 0xFFFF && !(raw & 0x0000800000000000ULL)) {
        const void* ptr = reinterpret_cast<const void*>(raw & 0x0000FFFFFFFFFFFFULL);
        if (!ptr) return 0;
        uint64_t first8 = *(const uint64_t*)ptr;
        if ((first8 >> 40) == 0x7F) { // vtable → VM 对象
            auto* obj = reinterpret_cast<const VMObject*>(ptr);
            if (obj->typeTag == ObjectHeader::TAG_STRING) {
                return static_cast<uint64_t>(((VMString*)obj)->length);
            }
            if (obj->typeTag == ObjectHeader::TAG_TABLE) {
                return static_cast<uint64_t>(((VMTable*)obj)->size());
            }
        }
    }
    return static_cast<uint64_t>(extractString(raw).length());
}

uint64_t jit_toString(uint64_t raw) {
    using namespace cplang;
    // raw integer from JIT codegen
    uint64_t exp = (raw >> 52) & 0x7FF;
    if (exp == 0 && raw != 0 && raw != 0x8000000000000000ULL) {
        std::string s = std::to_string(static_cast<int64_t>(raw));
        auto* str = VMString::create(s);
        auto& to = tempObjects();
        { std::lock_guard<std::mutex> lock(to.mutex);
          to.objects.push_back(reinterpret_cast<VMObject*>(str)); }
        return Value::Ptr(str).raw();
    }
    if ((raw >> 48) == 0xFFFF && !(raw & 0x0000800000000000ULL)) return raw;
    std::string s = std::to_string(static_cast<int64_t>(raw));
    auto* str = VMString::create(s);
    auto& to = tempObjects();
    { std::lock_guard<std::mutex> lock(to.mutex);
      to.objects.push_back(reinterpret_cast<VMObject*>(str)); }
    return Value::Ptr(str).raw();
}

uint64_t jit_tick() {
    using namespace cplang;
    static auto start = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
    return Value::Int(ms).raw();
}

uint64_t jit_call_native(const char* name, int32_t argc, uint64_t* args) {
    using namespace cplang;
    if (!name || !args) return 0;
    
    // 使用全局 VM 指针（避免 thread_local 在 JIT 调用链中的问题）
    VM* vm = g_jitVM.load(std::memory_order_acquire);
    if (!vm) return 0;
    
    Int32 slot = vm->getGlobalSlot(name);
    if (slot < 0) return 0;
    
    Value* gv = vm->getGlobalBySlot(static_cast<UInt16>(slot));
    if (!gv || !gv->isPtr()) return 0;
    VMObject* obj = gv->asPtr();
    if (!obj || obj->typeTag != ObjectHeader::TAG_NATIVE) return 0;
    
    // 尝试构建参数并调用原生函数
    VMNativeFunc* nf = (VMNativeFunc*)obj;
    if (!nf) return 0;
    
    try {
        if (argc == 0) {
            std::vector<Value> vargs;
            Value result = nf->fn(vargs);
            return result.raw();
        } else {
            std::vector<Value> vargs;
            vargs.reserve(argc);
            for (int32_t i = 0; i < argc; i++) {
                uint64_t raw = args[i];
                uint64_t exp = (raw >> 52) & 0x7FF;
                if (exp == 0 && raw != 0 && raw != 0x8000000000000000ULL) {
                    vargs.push_back(Value::Int(static_cast<int64_t>(raw)));
                } else {
                    vargs.push_back(Value(raw));
                }
            }
            Value result = nf->fn(vargs);
            return result.raw();
        }
    } catch (...) {
        return 0;
    }
}

// ─── 获取函数值（返回 i64 NaN-boxed Value）───
//  JIT codegen 在 loadVar 中找不到变量时调用此函数查函数值
uint64_t jit_get_function_value(const char* name) {
    using namespace cplang;
    VM* vm = g_jitVM.load(std::memory_order_acquire);
    if (!vm || !name) return 0;
    Int32 slot = vm->getGlobalSlot(name);
    if (slot < 0) return 0;
    Value* gv = vm->getGlobalBySlot(static_cast<UInt16>(slot));
    if (!gv || gv->isNil()) return 0;
    return gv->raw();
}

// ─── 通过函数值调用（间接调用桥）───
//  JIT codegen 在遇到 fn() 形式（fn 不是函数名而是变量/参数）时调用
uint64_t jit_call_value(uint64_t fnValue, int32_t argc, uint64_t* args) {
    using namespace cplang;
    VM* vm = g_jitVM.load(std::memory_order_acquire);
    if (!vm) return 0;
    Value func(fnValue);
    std::vector<Value> cpArgs;
    cpArgs.reserve(argc);
    for (int32_t i = 0; i < argc; i++) {
        uint64_t raw = args[i];
        // JIT codegen 产生 raw integer，需要装箱为 NaN-boxed Value
        uint64_t exp = (raw >> 52) & 0x7FF;
        if (exp == 0 && raw != 0 && raw != 0x8000000000000000ULL) {
            // raw integer → 装箱为 Int32
            int64_t val = static_cast<int64_t>(raw);
            cpArgs.push_back(Value::Int(val));
        } else {
            cpArgs.push_back(Value(raw));
        }
    }
    if (func.isFunction() || func.isClosure()) {
        return vm->callFunction(func, cpArgs).raw();
    }
    // 原生函数
    if (func.isPtr() && func.asPtr()) {
        VMObject* obj = func.asPtr();
        if (obj->typeTag == ObjectHeader::TAG_NATIVE) {
            try {
                VMNativeFunc* nf = (VMNativeFunc*)obj;
                return nf->fn(cpArgs).raw();
            } catch (...) { return 0; }
        }
    }
    return 0;
}

void jit_cleanup() {
    using namespace cplang;
    
    auto& to = tempObjects();
    std::lock_guard<std::mutex> lock(to.mutex);
    for (auto* obj : to.objects) {
        if (obj) {
            delete obj;
        }
    }
    to.objects.clear();
}

uint64_t jit_table_create() {
    using namespace cplang;
    auto* tbl = VMTable::create();
    
    auto& to = tempObjects();
    {
        std::lock_guard<std::mutex> lock(to.mutex);
        to.objects.push_back(reinterpret_cast<cplang::VMObject*>(tbl));
    }
    
    return Value::Table(tbl).raw();
}

uint64_t jit_table_get(uint64_t tableVal, uint64_t key) {
    using namespace cplang;
    Value tblVal(tableVal);
    Value keyVal(key);
    if (!tblVal.isUserData() && !tblVal.isTable()) return 0;
    auto* tbl = tblVal.asTable();
    if (!tbl) return 0;
    Value result = tbl->get(keyVal);
    return result.raw();
}

uint64_t jit_table_set(uint64_t tableVal, uint64_t key, uint64_t value) {
    using namespace cplang;
    Value tblVal(tableVal);
    Value keyVal(key);
    Value valVal(value);
    if (!tblVal.isUserData() && !tblVal.isTable()) return 0;
    auto* tbl = tblVal.asTable();
    if (!tbl) return 0;
    tbl->set(keyVal, valVal);
    return value;
}

} // extern "C"
