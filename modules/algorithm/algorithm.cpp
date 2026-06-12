// @cp/algorithm — 算法（排序/查找/分区/随机/位运算）
#include "stdlib/stdlib_fwd.hpp"
extern "C" __declspec(dllexport) void cplang_module_algorithm_register(cplang::VM* vm) {
    cplang::StdLib::registerAlgorithms(vm);
    cplang::StdLib::registerAlgoExt(vm);
    cplang::StdLib::registerAlgoMissing(vm);
    cplang::StdLib::registerBitwise(vm);
    cplang::StdLib::registerRandom(vm);
}
