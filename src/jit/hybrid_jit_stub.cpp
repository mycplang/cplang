// CP Language — HybridJIT stub for non-LLVM builds
//
// Stub architecture:
//   When LLVM is not available (CPLANG_HAS_LLVM=0), the real HybridJIT
//   implementation in hybrid_jit.cpp is compiled with #ifdef guards that
//   exclude LLVM-dependent code. This file provides dummy implementations
//   for builds that link without the full jit_runtime/llvm_codegen modules.
//
// While the CMake build handles this via conditional source lists, the
// batch build scripts (build_msvc.bat, build_clang.bat) use this stub
// as a simpler fallback.

#include "jit/hybrid_jit.hpp"

namespace cplang {

class HybridJIT::ExternalJIT {};
class OrcJIT {};

HybridJIT::HybridJIT() {}
HybridJIT::~HybridJIT() {}
bool HybridJIT::initialize() { return false; }
bool HybridJIT::shouldCompile(VMFunction*) const { return false; }
void HybridJIT::recordCall(VMFunction*) {}
void* HybridJIT::compileHotFunction(VMFunction*) { return nullptr; }
void* HybridJIT::compile(std::shared_ptr<Program>, const std::string&) { return nullptr; }
void HybridJIT::compileAll(std::shared_ptr<Program>) {}
void HybridJIT::storeProgram(std::shared_ptr<Program>) {}
void HybridJIT::setHotThreshold(int) {}
void HybridJIT::dumpStats() const {}

} // namespace cplang
