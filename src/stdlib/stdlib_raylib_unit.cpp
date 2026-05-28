// CP语言 Raylib 标准库 — 独立编译单元
// 独立编译以避免 raylib.h 与 windows.h 的函数名冲突
// （CloseWindow, ShowCursor, DrawText, DrawTextEx 等）

#include "stdlib/stdlib.hpp"

// raylib.h 必须在此翻译单元中最先被包含，
// 确保没有其他头文件先引入了 windows.h
extern "C" {
    #include "raylib.h"
}

// stdlib_raylib.cpp 期望在 namespace cplang 内
namespace cplang {
#include "stdlib_raylib.cpp"
} // namespace cplang
