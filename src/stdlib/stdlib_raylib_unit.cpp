// CP Language Standard Library — Raylib unit (isolated compilation)
// 独立编译以避免 raylib.h 与 windows.h 的宏冲突

#include "stdlib/stdlib.hpp"

// stdlib_raylib.cpp 已提取为独立翻译单元，由 CMake 统一编译链接
// 此文件保留用于隔离 raylib.h 与 windows.h 的包含顺序冲突

namespace cplang {
// Raylib 实现在 stdlib_raylib.cpp 中（已单独编译）
} // namespace cplang
