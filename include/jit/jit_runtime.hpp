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

// 字符串长度: 返回 NaN-boxed 字符串的长度
uint64_t jit_len(uint64_t raw);

// 值转字符串: 将整数值转为 NaN-boxed VMString*
uint64_t jit_toString(uint64_t raw);

// 通过函数值调用（间接调用桥）：fnValue 是 NaN-boxed 函数/闭包/原生函数值
uint64_t jit_call_value(uint64_t fnValue, int32_t argc, uint64_t* args);

// 获取函数值：从全局作用域按名查找函数，返回 NaN-boxed 函数值
uint64_t jit_get_function_value(const char* name);

// 通用 native 函数调用桥接：按名称在 VM 中查找并调用标准库函数
// name:  函数名 (UTF-8, 如 "长度")
// argc:  参数数量
// args:  NaN-boxed 参数数组
// 返回:  NaN-boxed 结果值
uint64_t jit_call_native(const char* name, int32_t argc, uint64_t* args);

// 设置 JIT 运行时使用的 VM 实例（在 JIT 入口调用前设置）
void jit_setVM(void* vm);

// 释放 JIT 运行时临时分配的对象
void jit_cleanup();

} // extern "C"
