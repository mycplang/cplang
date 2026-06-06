// all_stubs.cpp - Stub implementations for LLVM/JIT/AOT symbols missing without LLVM
// Include LLVM stubs FIRST to make llvm::Module and other types complete for unique_ptr
#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Function.h"
#include "llvm/BasicBlock.h"
#include "llvm/Value.h"
#include "llvm/Type.h"

#include "jit/jit_compiler.hpp"
#include "codegen/aot_compiler.hpp"
#include "codegen/llvm_codegen.hpp"
#include "stdlib/stdlib.hpp"
#include <string>

// Provide stub implementations for LLVMCodegen (the real llvm_codegen.cpp is #ifdef CPLANG_HAS_LLVM)
// and for StdLib::registerRaylib (stdlib_raylib_unit.cpp requires raylib.h)
namespace cplang {

// LLVMCodegen stubs
LLVMCodegen::LLVMCodegen() {}
LLVMCodegen::~LLVMCodegen() {}
std::string LLVMCodegen::generateIRString(Shared<Program>, OptLevel) { return ""; }
llvm::Module* LLVMCodegen::generate(Shared<Program>, OptLevel) { return nullptr; }
llvm::Module* LLVMCodegen::generateSingleFunction(Shared<Program>, const std::string&, OptLevel) { return nullptr; }
std::string LLVMCodegen::generateSingleFunctionIRString(Shared<Program>, const std::string&, OptLevel) { return ""; }
std::string LLVMCodegen::sanitizeName(const std::string& s) { return s; }

// StdLib stub for registerRaylib
void StdLib::registerRaylib(VM*) {}

} // namespace cplang
