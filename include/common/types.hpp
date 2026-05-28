// CP语言 公共头文件
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <optional>
#include <cstdint>

namespace cplang {

// 基本类型别名
using Int8 = int8_t;
using Int16 = int16_t;
using Int32 = int32_t;
using Int64 = int64_t;
using UInt8 = uint8_t;
using UInt16 = uint16_t;
using UInt32 = uint32_t;
using UInt64 = uint64_t;
using Float32 = float;
using Float64 = double;

// 字符类型
using Char = char;

// 字符串类型
using String = std::string;

// 可选类型
template<typename T>
using Optional = std::optional<T>;

// 智能指针
template<typename T>
using Unique = std::unique_ptr<T>;

template<typename T>
using Shared = std::shared_ptr<T>;

template<typename T>
using Weak = std::weak_ptr<T>;

// 变体
template<typename... Types>
using Variant = std::variant<Types...>;

    // ═══════════════════════════════════════════════════════
    // 优化级别
    // ═══════════════════════════════════════════════════════
    enum class OptLevel : int {
        None = 0,  // 无优化
        O1   = 1,  // 基本优化
        O2   = 2,  // 中等优化（推荐）
        O3   = 3   // 激进优化
    };

} // namespace cplang