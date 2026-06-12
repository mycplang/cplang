// @cp/net — CP语言 网络模块（HTTP + JSON）
// 独立编译为 cplang_net.lib，AOT 按需链接
// 注：registerNetwork 保留在 core 中（与 registerTypes 同文件）

#include "stdlib/stdlib_fwd.hpp"

extern "C" __declspec(dllexport) void cplang_module_network_register(cplang::VM* vm) {
    cplang::StdLib::registerJSON(vm);
    cplang::StdLib::registerHTTP(vm);
    cplang::StdLib::registerHttp(vm);
}
