// @cp/database — CP语言 数据库模块（SQLite + MySQL + Redis + WebSocket）
// 独立编译为 cplang_database.lib，AOT 按需链接

#include "stdlib/stdlib_fwd.hpp"

extern "C" __declspec(dllexport) void cplang_module_database_register(cplang::VM* vm) {
    cplang::StdLib::registerSqlite(vm);
    cplang::StdLib::registerMysql(vm);
    cplang::StdLib::registerPg(vm);
    cplang::StdLib::registerRedis(vm);
    cplang::StdLib::registerWebSocket(vm);
}
