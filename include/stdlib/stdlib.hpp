// CP语言 标准库汇总头文件
// ⚡ 此文件由拆分工具自动生成，仅包含各子模块的 include
//    所有实际声明已移至子目录下的 stdlib_*.hpp 文件中
#pragma once

// 防止 Windows.h min/max 宏污染
#ifndef NOMINMAX
#define NOMINMAX
#endif
#undef min
#undef max

// 标准库头文件（各 split 文件独立编译时需要）

// 标准库头文件（各 split 文件独立编译时需要）
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
#include <thread>
#include <regex>
#include <filesystem>
#include <random>



// StdLib 注册类声明
#include "stdlib/stdlib_fwd.hpp"

// 各模块函数声明
#include "stdlib/stdlib_math.hpp"
#include "stdlib/stdlib_string.hpp"
#include "stdlib/stdlib_array.hpp"
#include "stdlib/stdlib_table.hpp"
#include "stdlib/stdlib_io.hpp"
#include "stdlib/stdlib_file.hpp"
#include "stdlib/stdlib_time_system.hpp"
#include "stdlib/stdlib_types_net.hpp"
#include "stdlib/stdlib_containers.hpp"
#include "stdlib/stdlib_algo_bitwise.hpp"
#include "stdlib/stdlib_regex_encoding_crypto.hpp"
#include "stdlib/stdlib_variant_utils.hpp"

// 所有实际声明已移至子目录下的 stdlib_*.hpp 文件中