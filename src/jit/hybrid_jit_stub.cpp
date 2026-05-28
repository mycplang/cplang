// HybridJIT stub for non-LLVM builds
#include "jit/hybrid_jit.hpp"

namespace cplang {

// Define stub classes for unique_ptr completeness
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

}