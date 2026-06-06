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
                auto* str = static_cast<cplang::VMString*>(obj);
                return std::string(str->data, str->length);
            }
        }
        // 不是 VM 对象 → LLVM 全局 char* 常量
        return std::string(static_cast<const char*>(ptr));
    }
    // 非指针 → 标准 Value 解码
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

uint64_t jit_tick() {
    using namespace cplang;
    static auto start = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
    return Value::Int(ms).raw();
}

uint64_t jit_call_native(const char* name, int32_t argc, uint64_t* args) {
    using namespace cplang;

    VM* vm = VM::current();
    if (!vm) {
        std::cerr << "[JIT] jit_call_native: 无可用 VM 实例\n";
        return 0;
    }

    // 1. 按函数名查找全局 slot
    Int32 slot = vm->getGlobalSlot(name);
    if (slot < 0) {
        std::cerr << "[JIT] jit_call_native: 未找到函数 '" << name << "'\n";
        return 0;
    }

    // 2. 获取全局值 → 验证是原生函数
    Value* gv = vm->getGlobalBySlot(static_cast<UInt16>(slot));
    if (!gv || !gv->isPtr()) return 0;
    VMObject* obj = gv->asPtr();
    if (!obj || obj->typeTag != ObjectHeader::TAG_NATIVE) return 0;
    VMNativeFunc* nf = static_cast<VMNativeFunc*>(obj);

    // 3. 构造参数数组（i64 → VM Value）
    //    LLVM codegen 中的字符串是 NaN-boxed char* 指针，VM 需要 VMString*。
    //    需要像 aot_vm_bridge.cpp 那样检测并转换。
    std::vector<Value> vm_args(argc);
    for (int32_t i = 0; i < argc; i++) {
        uint64_t raw = args[i];
        // NaN-boxed 指针（bits 48-63=0xFFFF, bit47=0）→ char* → intern 为 VMString*
        if ((raw >> 48) == 0xFFFF && !(raw & 0x0000800000000000ULL)) {
            const void* ptr = reinterpret_cast<const void*>(raw & 0x0000FFFFFFFFFFFFULL);
            if (!ptr) {
                vm_args[i] = Value::nil();
            } else if (*(const uint64_t*)ptr == 0x43504C5441424C45ULL) { // "CPLTABLE"
                vm_args[i] = Value(raw); // 表指针直接传
            } else {
                // char* → VMString*
                const char* s = static_cast<const char*>(ptr);
                VMString* str = vm->internString(s, static_cast<uint32_t>(std::strlen(s)));
                vm_args[i] = Value::String(str);
            }
        } else {
            vm_args[i] = Value(raw);
        }
    }

    // 4. 调用原生函数
    Value result;
    try {
        result = nf->fn(vm_args);
    } catch (...) {
        std::cerr << "[JIT] jit_call_native: 函数 '" << name << "' 调用异常\n";
        return 0;
    }

    return result.raw();
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
