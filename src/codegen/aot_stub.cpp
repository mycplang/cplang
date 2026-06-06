// CP Language — AOTCompiler stub for non-LLVM builds
//
// Stub architecture:
//   Provides AOTCompiler symbols when LLVM is not available.
//   The real implementation in src/codegen/aot_compiler.cpp requires
//   LLVM libraries (libLLVM*.a / LLVM*.lib). This stub allows the
//   project to link without LLVM, returning a clear error message
//   if AOT compilation is attempted.
//
// Used by: build_msvc.bat, build_clang.bat (non-LLVM builds)
// Not needed for: CMake builds (handled via CPLANG_USE_LLVM option)

#include "codegen/aot_compiler.hpp"

namespace cplang {

AOTCompiler::AOTCompiler() {}
AOTCompiler::~AOTCompiler() {}

AOTResult AOTCompiler::compileFile(const String&, const AOTConfig&) {
    return AOTResult{false, "AOT compilation requires LLVM (set HAS_LLVM=1)", "", 0};
}

AOTResult AOTCompiler::compileSource(const String&, const AOTConfig&) {
    return AOTResult{false, "AOT compilation requires LLVM (set HAS_LLVM=1)", "", 0};
}

AOTResult AOTCompiler::compileAST(Shared<Program>, const AOTConfig&) {
    return AOTResult{false, "AOT compilation requires LLVM (set HAS_LLVM=1)", "", 0};
}

} // namespace cplang
