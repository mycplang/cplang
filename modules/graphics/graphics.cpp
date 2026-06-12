// @cp/graphics — CP语言 图形模块（Raylib 2D/3D + ImGui）
// 独立编译为 cplang_graphics.lib，AOT 按需链接

#include "stdlib/stdlib_fwd.hpp"

// 注册所有图形相关原生函数到 VM
extern "C" __declspec(dllexport) void cplang_module_graphics_register(cplang::VM* vm) {
    cplang::StdLib::registerRaylib(vm);
    cplang::StdLib::registerImGui(vm);
}
