// AOTCompiler stub for non-LLVM builds
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

}