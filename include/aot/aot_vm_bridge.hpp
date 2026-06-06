// AOT VM 桥接 — 在 AOT 可执行文件中嵌入轻量 VM 用于调用 stdlib 原生函数
#pragma once
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

// NaN-boxing 常量（与 AOT/LLVM codegen 保持一致）
#define AOT_NAN_TAG     0xFFFF000000000000ULL
#define AOT_PTR_MASK    0x0000FFFFFFFFFFFFULL

// 初始化 AOT 运行时（创建 VM、注册全部 stdlib）
// 在 main() 入口处调用，全局仅初始化一次
void aot_init_runtime(void);

// 通过桥接调用 stdlib 原生函数
// name:   函数名（UTF-8 C 字符串）
// argc:   参数数量
// args:   NaN-boxed 参数数组（i64）
// 返回:   NaN-boxed 结果值（i64）
uint64_t aot_call_native(const char* name, int32_t argc, uint64_t* args);

// 在运行时导入模块（编译 + 执行模块代码）
// name:   模块名（如 "math"，不含 .cp 后缀）
// 返回:   0=成功，非0=失败
int32_t aot_import_module(const char* name);

// 判断一个 NaN-boxed 值是否为指针类型
static inline int aot_is_ptr(uint64_t v) {
    return (v >> 48) == 0xFFFF && (v & 0x0000800000000000ULL) == 0;
}

// 从 NaN-boxed 值中提取指针
static inline const void* aot_get_ptr(uint64_t v) {
    return (const void*)(uintptr_t)(v & AOT_PTR_MASK);
}

// 将指针编码为 NaN-boxed 值
static inline uint64_t aot_make_ptr(const void* ptr) {
    return AOT_NAN_TAG | ((uint64_t)(uintptr_t)ptr & AOT_PTR_MASK);
}

// 获取全局变量值（如 raylib 常量 `键_左`、`乳白` 等）
// name:   变量名（UTF-8 C 字符串）
// 返回:   NaN-boxed 变量值（i64），未找到返回 0
uint64_t aot_get_global(const char* name);

// 清理 AOT 运行时
void aot_cleanup_runtime(void);

#ifdef __cplusplus
}
#endif
