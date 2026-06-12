// @cp/concurrent — 并发（线程/互斥/条件/信号量/原子/Barrier/Channel/Future/RWLock/TLS）
#include "stdlib/stdlib_fwd.hpp"
extern "C" __declspec(dllexport) void cplang_module_concurrent_register(cplang::VM* vm) {
    cplang::StdLib::registerThreading(vm);
}
