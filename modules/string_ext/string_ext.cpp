// @cp/string_ext — 字符串高级（正则/格式化/搜索/大小写/统计/工具）
#include "stdlib/stdlib_fwd.hpp"
extern "C" __declspec(dllexport) void cplang_module_string_ext_register(cplang::VM* vm) {
    cplang::StdLib::registerStringExt(vm);
    cplang::StdLib::registerStringMore(vm);
    cplang::StdLib::registerStringSearch(vm);
    cplang::StdLib::registerStringCase(vm);
    cplang::StdLib::registerStrCi(vm);
    cplang::StdLib::registerRegex(vm);
    cplang::StdLib::registerStatistics(vm);
    cplang::StdLib::registerUtils(vm);
    cplang::StdLib::registerMoreUtils(vm);
}
