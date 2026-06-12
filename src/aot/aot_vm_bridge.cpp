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
#include "debug/debugger.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <vector>
#include <string>

// raylib Color 结构体前向声明（避免包含完整 raylib.h 头文件）
#ifdef __cplusplus
extern "C" {
#endif
typedef struct AOTColor { unsigned char r, g, b, a; } AOTColor;
int  InitWindow(int w, int h, const char* title);
int  WindowShouldClose(void);
void BeginDrawing(void);
void EndDrawing(void);
void SetTargetFPS(int fps);
int  IsKeyPressed(int key);
void DrawRectangle(int x, int y, int w, int h, AOTColor color);
void ClearBackground(AOTColor color);
void DrawText(const char* text, int x, int y, int fontSize, AOTColor color);
void SetTraceLogLevel(int logType);
#define LOG_ERROR   4
void jit_cleanup(void);
#ifdef __cplusplus
}
#endif

namespace cplang {
    static VM* g_aot_vm = nullptr;

    // AOT 指针 → 转为 VM Value
    // AOT codegen 中的指针可能是：
    //   字符串 → 指向 char[] 的 NaN-boxed 指针
    //   表    → 指向 TableData 的 NaN-boxed 指针
    // 通过检查 TABLE_MAGIC 区分表和字符串
        // TableData 结构（与 jit_runtime_standalone.cpp 保持一致）
    static constexpr uint64_t kTableMagic = 0x43504C5441424C45ULL; // "CPLTABLE"
    static constexpr uint64_t kTableEmpty = 0x8000000000000001ULL;
    struct TableEntry { uint64_t key; uint64_t value; };
    struct TableData { uint64_t magic; TableEntry* entries; int32_t count; int32_t capacity; };

    static Value aot_table_to_vmtable(VM* vm, const TableData* td);

    // 将 AOT 侧的 uint64_t 值转为 VM Value
    static Value toVMValue(VM* vm, uint64_t raw) {
        if ((raw >> 48) == 0xFFFF && !(raw & 0x0000800000000000ULL)) {
            const void* p = reinterpret_cast<const void*>(raw & 0x0000FFFFFFFFFFFFULL);
            if (!p) return Value::nil();

            // 通过 ObjectHeader.typeTag 判断指针指向的是哪种对象
            // VMObject: typeTag 在 0..30 范围内 (TAG_STRING=0, TAG_ARRAY=1, ...)
            // AOT TableData: 首 8 字节 = kTableMagic (0x43504C5441424C45)
            // C 字符串: 前几个字节是可打印 ASCII
            const uint8_t* bytes = static_cast<const uint8_t*>(p);
            uint8_t typeTag = bytes[1];  // ObjectHeader.typeTag 偏移量

            if (typeTag <= 30) {
                // 已经是有效的 VM 对象指针，直接透传
                return Value(raw);
            }
            if (*(const uint64_t*)p == kTableMagic) {
                // AOT 侧分配的 TableData → 转换为 VMTable
                return aot_table_to_vmtable(vm, static_cast<const TableData*>(p));
            }
            // C 字符串 → 通过 VM intern 创建 VMString
            const char* s = static_cast<const char*>(p);
            VMString* vs = vm->internString(s, (uint32_t)std::strlen(s));
            return Value::Ptr(reinterpret_cast<VMObject*>(vs));
        }
        // 整数 / 非指针值：从 int32 内联读取
        return Value::fromInt32((int32_t)(int64_t)raw);
    }

    // 将 TableData 转换为 VMTable
    static Value aot_table_to_vmtable(VM* vm, const TableData* td) {
        VMTable* tbl = VMTable::create();
        vm->trackGC(reinterpret_cast<VMObject*>(tbl));  // GC 跟踪
        for (int32_t i = 0; i < td->capacity; i++) {
            if (td->entries[i].key == kTableEmpty) continue;
            Value keyVal = toVMValue(vm, td->entries[i].key);
            Value valVal = toVMValue(vm, td->entries[i].value);
            tbl->set(keyVal, valVal);
        }
        return Value::Ptr(static_cast<VMObject*>(tbl));
    }

    static Value aot_ptr_to_value(VM* vm, uint64_t v) {
        const void* ptr = reinterpret_cast<const void*>(v & 0x0000FFFFFFFFFFFFULL);
        if (!ptr) return Value::nil();

        // 通过 ObjectHeader.typeTag 判断指针类型
        const uint8_t* bytes = static_cast<const uint8_t*>(ptr);
        uint8_t typeTag = bytes[1];  // ObjectHeader.typeTag 偏移量

        if (typeTag <= 30) {
            // 已经是有效的 VM 对象指针（VMArray/VMString/VMTable 等），直接透传
            return Value(v);
        }
        // 检查是否是 AOT TableData（首 8 字节为 TABLE_MAGIC）
        if (*(const uint64_t*)ptr == 0x43504C5441424C45ULL) {  // "CPLTABLE"
            return aot_table_to_vmtable(vm, static_cast<const TableData*>(ptr));
        }
        // C 字符串：通过 VM intern 创建 VMString
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

// ═══ AOT 调试日志 ═══
static void aot_log(const char* msg) {
    FILE* f = fopen("aot_debug.log", "a");
    if (f) { fprintf(f, "[AOT] %s\n", msg); fflush(f); fclose(f); }
}
static void aot_logf(const char* fmt, ...) {
    char buf[4096];
    va_list ap; va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    aot_log(buf);
}

static int64_t g_frame_count = 0;

void aot_init_runtime(void) {
    if (g_aot_vm) { aot_log("init: already initialized"); return; }
    aot_log("init: creating VM...");
    g_aot_vm = new VM();
    if (!g_aot_vm) {
        aot_log("init: FATAL - failed to create VM");
        std::fprintf(stderr, "[AOT] 无法创建 VM\n");
        std::abort();
    }
    aot_log("init: VM created, registering core stdlib...");
    StdLib::registerCore(g_aot_vm);
    aot_log("init: core stdlib registration complete, registering modules...");
    aot_register_modules(g_aot_vm);

#ifndef CPLANG_AOT_NO_RAYLIB
    // raylib 窗口函数（仅在图形 AOT 中注册）
    StdLib::registerFunction(g_aot_vm, "initWindow", [](std::vector<Value>& args) -> Value {
        int w = args.size()>=1 && args[0].isInt() ? (int)args[0].asInt() : 800;
        int h = args.size()>=2 && args[1].isInt() ? (int)args[1].asInt() : 600;
        SetTraceLogLevel(LOG_ERROR);  // 静默 raylib 的 INFO/WARNING
        InitWindow(w, h, "CP + raylib");
        return Value::Int(0);
    });
    StdLib::registerFunction(g_aot_vm, "windowShouldClose", [](std::vector<Value>&) -> Value {
        return Value::Bool(WindowShouldClose() != 0);
    });
    StdLib::registerFunction(g_aot_vm, "beginDrawing", [](std::vector<Value>&) -> Value {
        BeginDrawing(); return Value::Int(0);
    });
    StdLib::registerFunction(g_aot_vm, "endDrawing", [](std::vector<Value>&) -> Value {
        EndDrawing(); return Value::Int(0);
    });
    StdLib::registerFunction(g_aot_vm, "setTargetFPS", [](std::vector<Value>& args) -> Value {
        int fps = args.size()>=1 && args[0].isInt() ? (int)args[0].asInt() : 60;
        SetTargetFPS(fps); return Value::Int(0);
    });
    StdLib::registerFunction(g_aot_vm, "keyPressed", [](std::vector<Value>& args) -> Value {
        int key = args.size()>=1 && args[0].isInt() ? (int)args[0].asInt() : 0;
        return Value::Bool(IsKeyPressed(key) != 0);
    });

    StdLib::registerFunction(g_aot_vm, "closeWindow", [](std::vector<Value>&) -> Value {
        CloseWindow(); return Value::Int(0);
    });#endif

    aot_log("init: all registration complete");
}

uint64_t aot_call_native(const char* name, int32_t argc, uint64_t* args) {
    aot_logf("call_native: %s argc=%d", name, argc);
    if (!g_aot_vm) { aot_log("call_native: VM null, reinitializing..."); aot_init_runtime(); }


// ═══ 直接 raylib 调用绕过（仅在链接 raylib 时启用，避免非图形 AOT 链接 raylib）═══
    // 对于非图形 AOT 编译，走标准 stdlib 路径
#ifndef CPLANG_AOT_NO_RAYLIB
    {
        bool isRect = (strcmp(name, "drawRectangle") == 0) || (strcmp(name, "\xe7\xbb\x98\xe5\x88\xb6\xe7\x9f\xa9\xe5\xbd\xa2") == 0);
        bool isBg   = (strcmp(name, "clearBackground") == 0) || (strcmp(name, "\xe6\xb8\x85\xe7\xa9\xba\xe8\x83\x8c\xe6\x99\xaf") == 0);
        bool isText = (strcmp(name, "drawText") == 0) || (strcmp(name, "\xe7\xbb\x98\xe5\x88\xb6\xe6\x96\x87\xe6\x9c\xac") == 0);
        if (isRect || isBg || isText) {
            int colorArgIdx = isBg ? 0 : 4;
            if (argc > colorArgIdx && (args[colorArgIdx] >> 48) == 0xFFFF && !(args[colorArgIdx] & 0x0000800000000000ULL)) {
                const void* colPtr = reinterpret_cast<const void*>(args[colorArgIdx] & 0x0000FFFFFFFFFFFFULL);
                if (colPtr && *(const uint64_t*)colPtr == kTableMagic) {
                    auto* td = static_cast<const TableData*>(colPtr);
                    int r = -1, g = -1, b = -1, a = -1;
                    for (int32_t ei = 0; ei < td->capacity; ei++) {
                        if (td->entries[ei].key == kTableEmpty) continue;
                        uint64_t ek = td->entries[ei].key;
                        if ((ek >> 48) == 0xFFFF && !(ek & 0x0000800000000000ULL)) {
                            const char* fname = reinterpret_cast<const char*>(ek & 0x0000FFFFFFFFFFFFULL);
                            if (fname) {
                                int64_t fv = static_cast<int64_t>(td->entries[ei].value);
                                if      (fname[0] == 'r' && fname[1] == '\0') r = (int)fv;
                                else if (fname[0] == 'g' && fname[1] == '\0') g = (int)fv;
                                else if (fname[0] == 'b' && fname[1] == '\0') b = (int)fv;
                                else if (fname[0] == 'a' && fname[1] == '\0') a = (int)fv;
                            }
                        }
                    }
                    if (r >= 0 && g >= 0 && b >= 0 && a < 0) a = 255;
                    if (r >= 0 && g >= 0 && b >= 0) {
                        AOTColor col = {(unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)(a >= 0 ? a : 255)};
                        if (isRect) {
                            int x = (int)(int32_t)(int64_t)args[0];
                            int y = (int)(int32_t)(int64_t)args[1];
                            int w = (int)(int32_t)(int64_t)args[2];
                            int h = (int)(int32_t)(int64_t)args[3];
                            DrawRectangle(x, y, w, h, col);
                        } else if (isBg) {
                            ClearBackground(col);
                        } else if (isText) {
                            const char* text = "";
                            if ((args[0] >> 48) == 0xFFFF && !(args[0] & 0x0000800000000000ULL))
                                text = reinterpret_cast<const char*>(args[0] & 0x0000FFFFFFFFFFFFULL);
                            int x = (int)(int32_t)(int64_t)args[1];
                            int y = (int)(int32_t)(int64_t)args[2];
                            int fontSize = (int)(int32_t)(int64_t)args[3];
                            DrawText(text ? text : "", x, y, fontSize, col);
                        }
                        // 释放内联表字面量分配的 tracked_malloc 内存（防止循环中泄漏）
                        jit_table_free(args[colorArgIdx]);
                        return 0xFFFFD00000000000ULL;
                    }
                }
            }
        }
    }
#endif // CPLANG_AOT_NO_RAYLIB
    // 1. 按名称查找全局 slot
    Int32 slot = g_aot_vm->getGlobalSlot(name);
    if (slot < 0) { aot_logf("call_native: slot not found for '%s'", name); return 0; }

    // 2. 获取全局值 → 验证是原生函数
    Value* gv = g_aot_vm->getGlobalBySlot(static_cast<UInt16>(slot));
    if (!gv || !gv->isPtr()) return 0;
    VMObject* obj = gv->asPtr();
    if (!obj || obj->typeTag != ObjectHeader::TAG_NATIVE) return 0;
    VMNativeFunc* nf = static_cast<VMNativeFunc*>(obj);

    

    // 3. 构造参数数组
    std::vector<Value> vm_args(argc);
    for (int32_t i = 0; i < argc; i++) {
        if ((args[i] >> 48) == 0xFFFF && !(args[i] & 0x0000800000000000ULL)) {
            vm_args[i] = aot_ptr_to_value(g_aot_vm, args[i]);
        } else {
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
    if (strcmp(name, "windowShouldClose") == 0 || strcmp(name, "keyPressed") == 0) {
        aot_logf("retval: %s = %llu  frame:%lld", name, raw, g_frame_count);
    }
    if (strcmp(name, "endDrawing") == 0) {
        g_frame_count++;
        aot_logf("frame: %lld", g_frame_count);
        // 注意：此处不调用 jit_cleanup()，因为它会释放格子表等持久数据
    }
    // NaN-boxing 的 bool 值（0xFFFF800000000001/2）对 LLVM 是非零=真，必须归一化为 0/1
    // 注意：此处不依赖 Value::isBool()，直接检查 raw 避免跨编译单元的方法调用问题
    if (raw == 0xFFFF800000000001ULL || raw == 0xFFFF800000000002ULL) {
        raw = (raw == 0xFFFF800000000002ULL) ? 1 : 0;
        aot_logf("converted bool %s -> %llu", name, raw);
    }
    // 如果结果是字符串指针，返回 AOT 可用的 NaN-boxed 字符串
    if (result.isString()) {
        return value_to_aot_str(result);
    }
    return raw;
}

// 默认弱实现：由 AOT 编译器生成的 _aot_bootstrap.obj 覆盖
void aot_register_modules(void* vm) {
    // 空实现——AOT 编译器会生成覆盖版本
    (void)vm;
}

int32_t aot_import_module(const char* name) {
    // AOT 模式下模块由 bootstrap 在初始化时注册，运行时 import 为空操作
    // 避免调用 registerModules() 拉入 stdlib.obj（含 registerAll → 全量依赖）
    (void)name;
    return 0;
}

uint64_t aot_get_global(const char* name) {
    if (!g_aot_vm) aot_init_runtime();
    Int32 slot = g_aot_vm->getGlobalSlot(name);
    if (slot < 0) return 0;
    Value* val = g_aot_vm->getGlobalBySlot(static_cast<UInt16>(slot));
    if (!val) return 0;
    return val->raw();
}

// ═══ VMArray 桥接适配（供 jit_runtime_standalone 调用）════
// jit_runtime_standalone 的 len/push/arrlen 等函数只识别 TableData
// （首 8 字节 = TABLE_MAGIC），但 LLVM codegen 通过 VM 桥接创建的
// 数组是 VMArray 对象（ObjectHeader.typeTag = TAG_ARRAY = 1）。
// 以下函数提供 VMArray 操作的 C 接口，供 standalone 函数路由使用。

static bool aot_is_vm_array(uint64_t raw) {
    if ((raw >> 48) != 0xFFFF || (raw & 0x0000800000000000ULL)) return false;
    const void* p = reinterpret_cast<const void*>(raw & 0x0000FFFFFFFFFFFFULL);
    if (!p) return false;
    const uint8_t* bytes = static_cast<const uint8_t*>(p);
    return bytes[1] == 1;  // ObjectHeader.typeTag offset 1, TAG_ARRAY = 1
}

extern "C" {

uint64_t aot_vm_array_len(uint64_t raw) {
    if (!g_aot_vm) aot_init_runtime();
    Value v(raw);
    if (v.isArray()) {
        VMArray* arr = v.asArray();
        if (arr) return static_cast<uint64_t>(arr->length());
    }
    return 0;
}

uint64_t aot_vm_array_push(uint64_t arr_raw, uint64_t val_raw) {
    if (!g_aot_vm) aot_init_runtime();
    Value arr(arr_raw);
    Value val = toVMValue(g_aot_vm, val_raw);
    if (arr.isArray()) {
        VMArray* a = arr.asArray();
        if (a) {
            a->data.push_back(val);
            g_aot_vm->trackGC(reinterpret_cast<VMObject*>(a));  // GC 跟踪
            return arr_raw;
        }
    }
    return arr_raw;
}

uint64_t aot_vm_array_get(uint64_t arr_raw, uint64_t idx_raw) {
    if (!g_aot_vm) aot_init_runtime();
    Value arr(arr_raw);
    if (arr.isArray()) {
        VMArray* a = arr.asArray();
        int64_t idx = static_cast<int64_t>(idx_raw);
        if (a && idx >= 0 && idx < a->length()) {
            return a->data[static_cast<size_t>(idx)].raw();
        }
    }
    return 0xFFFF800000000000ULL;  // nil
}

uint64_t aot_vm_array_pop(uint64_t arr_raw) {
    if (!g_aot_vm) aot_init_runtime();
    Value arr(arr_raw);
    if (arr.isArray()) {
        VMArray* a = arr.asArray();
        if (a && !a->data.empty()) {
            Value v = a->data.back();
            a->data.pop_back();
            return v.raw();
        }
    }
    return 0xFFFF800000000000ULL;  // nil
}

uint64_t aot_vm_array_insert(uint64_t arr_raw, uint64_t idx_raw, uint64_t val_raw) {
    if (!g_aot_vm) aot_init_runtime();
    Value arr(arr_raw);
    Value val = toVMValue(g_aot_vm, val_raw);
    if (arr.isArray()) {
        VMArray* a = arr.asArray();
        int64_t idx = static_cast<int64_t>(idx_raw);
        if (a && idx >= 0 && idx <= a->length()) {
            a->data.insert(a->data.begin() + static_cast<size_t>(idx), val);
            g_aot_vm->trackGC(reinterpret_cast<VMObject*>(a));
            return arr_raw;
        }
    }
    return arr_raw;
}

}  // extern "C"

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
    if (v.isArray()) {
        VMArray* a = v.asArray();
        if (a) return static_cast<uint64_t>(a->length());
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
    uint64_t s_val = raw;
    if ((raw >> 48) == 0xFFFF && (raw & 0x0000C00000000000ULL) == 0x0000C00000000000ULL) {
        s_val = static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(raw & 0xFFFFFFFF)));
    }
    // 静态循环缓冲区（零内存分配，无 GC 风险）
    static char buf[16][24];
    static int slot = 0;
    slot = (slot + 1) % 16;
    std::snprintf(buf[slot], 24, "%lld", static_cast<long long>(static_cast<int64_t>(s_val)));
    buf[slot][23] = '\0';
    return 0xFFFF000000000000ULL | (reinterpret_cast<uint64_t>(buf[slot]) & 0x0000FFFFFFFFFFFFULL);
}

void jit_setVM(void*) { /* AOT: no-op */ }

void jit_printv(void) { /* AOT: no-op, print handled by stdlib */ }

void jit_cleanup(void) { /* AOT: no-op */ }

// JIT 分派桩（AOT 不需要 JIT）
bool jitTryCallDispatch(VM*, VMFunction*, int, Value*, Value&) { return false; }

} // extern "C"

void aot_cleanup_runtime(void) {
    delete g_aot_vm;
    g_aot_vm = nullptr;
}