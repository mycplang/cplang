// @cp/ffi — CP语言 外部函数接口模块（DLL 动态调用）
// 独立编译为 cplang_ffi.lib，AOT 按需链接

#include "stdlib/stdlib_fwd.hpp"

extern "C" __declspec(dllexport) void cplang_module_ffi_register(cplang::VM* vm) {
    cplang::StdLib::registerFFI(vm);
}
