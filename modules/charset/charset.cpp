// @cp/charset — 字符集转换（UTF-8 ↔ GBK / Big5 / Shift-JIS）
#include "stdlib/stdlib_fwd.hpp"
extern "C" __declspec(dllexport) void cplang_module_charset_register(cplang::VM* vm) {
    cplang::StdLib::registerCharset(vm);
}
