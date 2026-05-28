// JIT 运行时辅助函数 - 供 ORC JIT 编译的代码调用
// 
// 这些函数使用 extern "C" 链接以被 LLJIT 符号解析

#pragma once

#include <cstdint>

// C 链接函数（供 LLVM IR 直接调用）
extern "C" {

// 字符串拼接: 接受两个 NaN-boxed CP Value，返回拼接结果的 NaN-boxed Value
uint64_t jit_strcat(uint64_t a, uint64_t b);

// 打印: count = 参数数量, args = NaN-boxed Value 数组
void jit_printv(int32_t count, uint64_t* args);

// Table（字典）操作: key 是 NaN-boxed 字符串指针
uint64_t jit_table_create(void);
uint64_t jit_table_get(uint64_t table, uint64_t key);
uint64_t jit_table_set(uint64_t table, uint64_t key, uint64_t value);

// Tick: 返回从首次调用起经过的毫秒数（与 stdlib time::tick 逻辑一致）
uint64_t jit_tick(void);

// 释放 JIT 运行时临时分配的对象
void jit_cleanup();

} // extern "C"
