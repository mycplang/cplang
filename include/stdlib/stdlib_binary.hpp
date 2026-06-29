#pragma once
#include "vm/value.hpp"
#include <vector>

namespace cplang {
namespace binary {

// ========== 创建 ==========
Value create(std::vector<Value>& args);       // 创建字节数组(大小)
Value fromString(std::vector<Value>& args);   // 字节数组自字符串(字符串)
Value fromArray(std::vector<Value>& args);    // 字节数组自数组(数组)
Value fromHex(std::vector<Value>& args);      // 字节数组自十六进制(十六进制字符串)

// ========== 属性 ==========
Value len(std::vector<Value>& args);          // 字节数组长度(buf)
Value cap(std::vector<Value>& args);          // 字节数组容量(buf)

// ========== 读写 ==========
Value get(std::vector<Value>& args);          // 字节数组获取(buf, 索引)
Value set(std::vector<Value>& args);          // 字节数组设置(buf, 索引, 值)
Value fill(std::vector<Value>& args);         // 字节数组填充(buf, 值, [偏移], [长度])
Value copy(std::vector<Value>& args);         // 字节数组拷贝(buf, [偏移], [长度])

// ========== 视图 ==========
Value slice(std::vector<Value>& args);        // 字节数组切片(buf, 偏移, 长度)

// ========== 转换 ==========
Value toString(std::vector<Value>& args);     // 字节数组转字符串(buf)
Value toArray(std::vector<Value>& args);      // 字节数组转数组(buf)
Value toHex(std::vector<Value>& args);        // 字节数组转十六进制(buf, [分隔符])

// ========== 写入 ==========
Value write(std::vector<Value>& args);        // 字节数组写入(buf, 偏移, 字符串/字节数组)
Value append(std::vector<Value>& args);       // 字节数组追加(buf, src)

// ========== 比较 ==========
Value compare(std::vector<Value>& args);      // 字节数组比较(buf1, buf2)

// ========== 调整 ==========
Value resize(std::vector<Value>& args);       // 字节数组设置长度(buf, 新长度)

} // namespace binary
} // namespace cplang
