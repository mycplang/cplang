#include "stdlib/stdlib.hpp"

namespace cplang {

// HTTP 扩展函数
// #include'd from stdlib.cpp, already inside namespace cplang

// 注：此模块为预留扩展，当前为占位实现
// 基础 HTTP 功能已通过 stdlib_json_http.cpp 中的 registerHTTP() 提供

void StdLib::registerHttp(VM* vm) {
    (void)vm;
    // 预留：可在此注册更多 HTTP 相关功能
}

} // namespace cplang
