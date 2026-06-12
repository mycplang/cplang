// @cp/container — 高级容器（栈/队列/链表/集合/映射/堆/位集/复数/对）
#include "stdlib/stdlib_fwd.hpp"
extern "C" __declspec(dllexport) void cplang_module_container_register(cplang::VM* vm) {
    cplang::StdLib::registerSet(vm);
    cplang::StdLib::registerStack(vm);
    cplang::StdLib::registerQueue(vm);
    cplang::StdLib::registerDeque(vm);
    cplang::StdLib::registerPriorityQueue(vm);
    cplang::StdLib::registerLinkedList(vm);
    cplang::StdLib::registerSLinkedList(vm);
    cplang::StdLib::registerMultiSet(vm);
    cplang::StdLib::registerMultiMap(vm);
    cplang::StdLib::registerUnorderedSet(vm);
    cplang::StdLib::registerUnorderedMultiSet(vm);
    cplang::StdLib::registerUnorderedMap(vm);
    cplang::StdLib::registerUnorderedMultiMap(vm);
    cplang::StdLib::registerBitset(vm);
    cplang::StdLib::registerHeap(vm);
    cplang::StdLib::registerComplex(vm);
    cplang::StdLib::registerPair(vm);
}
