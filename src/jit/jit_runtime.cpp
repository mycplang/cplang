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

uint64_t jit_strcat(uint64_t a, uint64_t b) {
    using namespace cplang;
    
    Value va(a), vb(b);
    std::string sa = va.toString();
    std::string sb = vb.toString();
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
        Value v(args[i]);
        std::cout << v.toString();
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
