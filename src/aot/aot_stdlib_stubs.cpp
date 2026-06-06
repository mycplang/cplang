// AOT 存根：替换 raylib 和 ImGui 注册函数
// stdlib.cpp 中的 StdLib::registerAll() 无条件调用 registerRaylib() 和 registerImGui()。
// 在 AOT 编译上下文中，这些函数需要可链接但不需要实际功能。
// 此文件提供空存根，避免链接完整的 raylib/ImGui 依赖。

#include "vm/vm.hpp"
#include "stdlib/stdlib_fwd.hpp"

namespace cplang {

void StdLib::registerRaylib(VM* /*vm*/) {
    // AOT 模式：raylib 图形不可用，静默跳过
}

void StdLib::registerImGui(VM* /*vm*/) {
    // AOT 模式：ImGui 不可用，静默跳过
}

} // namespace cplang
