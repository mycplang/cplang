// JIT Stub for runtime_test
// Provides no-op implementations of HybridJIT methods
// so the pure-VM interpreter can be tested without LLVM dependency.

#include "jit/hybrid_jit.hpp"

namespace cplang {

HybridJIT::HybridJIT() {}
HybridJIT::~HybridJIT() {}
void HybridJIT::initialize() {}
void HybridJIT::compileAll(std::shared_ptr<Program>) {}
void HybridJIT::storeProgram(std::shared_ptr<Program>) {}
void HybridJIT::setHotThreshold(int) {}
void HybridJIT::dumpStats() const {}
bool HybridJIT::compile(std::shared_ptr<Program>, const std::string&) { return false; }

void HybridJIT::recordCall(VMFunction*) {
    // No-op: runtime tests don't need JIT
}

bool HybridJIT::shouldCompile(VMFunction*) const {
    return false; // Never compile via JIT in test mode
}

void* HybridJIT::compileHotFunction(VMFunction*) {
    return nullptr; // No JIT compilation in test mode
}

} // namespace cplang
