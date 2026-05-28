// stdlib_minimal_stub.cpp - Minimal stdlib for pipeline testing on Windows
// Provides StdLib::registerAll as a no-op (stdlib has WinHTTP/WS2/SQLite/etc deps)

#include "stdlib/stdlib.hpp"
#include "vm/vm.hpp"

namespace cplang {

void StdLib::registerAll(VM* vm) {
    // No-op: skip heavy stdlib registration for pipeline testing
    (void)vm;
}

void StdLib::registerImGui(VM*) {}
void StdLib::registerRaylib(VM*) {}
void StdLib::registerFixMissing(VM*) {}

} // namespace cplang
