#include "stdlib/stdlib.hpp"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace cplang {

// ═══════════════════════════════════════════════════════════════
//  CP语言 标准库 — 堆操作 (heap)
// ═══════════════════════════════════════════════════════════════

#include <algorithm>

namespace heap {
    Value makeHeap(std::vector<Value>& args) {
        if (args.empty() || !args[0].isArray()) return Value::nil();
        VMArray* arr = args[0].asArray();
        std::make_heap(arr->data.begin(), arr->data.end(), ValueLess{});
        return args[0];
    }
    Value pushHeap(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].isArray()) return Value::nil();
        VMArray* arr = args[0].asArray();
        arr->data.push_back(args[1]);
        std::push_heap(arr->data.begin(), arr->data.end(), ValueLess{});
        return args[0];
    }
    Value popHeap(std::vector<Value>& args) {
        if (args.empty() || !args[0].isArray()) return Value::nil();
        VMArray* arr = args[0].asArray();
        if (arr->data.empty()) return Value::nil();
        std::pop_heap(arr->data.begin(), arr->data.end(), ValueLess{});
        Value v = arr->data.back();
        arr->data.pop_back();
        return v;
    }
    Value sortHeap(std::vector<Value>& args) {
        if (args.empty() || !args[0].isArray()) return Value::nil();
        VMArray* arr = args[0].asArray();
        std::sort_heap(arr->data.begin(), arr->data.end(), ValueLess{});
        return args[0];
    }
    Value isHeap(std::vector<Value>& args) {
        if (args.empty() || !args[0].isArray()) return Value::Bool(false);
        VMArray* arr = args[0].asArray();
        return Value::Bool(std::is_heap(arr->data.begin(), arr->data.end(), ValueLess{}));
    }
    Value heapTop(std::vector<Value>& args) {
        if (args.empty() || !args[0].isArray()) return Value::nil();
        VMArray* arr = args[0].asArray();
        if (arr->data.empty()) return Value::nil();
        return arr->data.front();
    }
}

void StdLib::registerHeap(VM* vm) {
    registerFunction(vm, "makeHeap", heap::makeHeap);
    registerFunction(vm, "pushHeap", heap::pushHeap);
    registerFunction(vm, "popHeap",  heap::popHeap);
    registerFunction(vm, "sortHeap", heap::sortHeap);
    registerFunction(vm, "isHeap",   heap::isHeap);
    registerFunction(vm, "heapTop",  heap::heapTop);
    registerAlias(vm, "",    "makeHeap");
    registerAlias(vm, "", "pushHeap");
    registerAlias(vm, "", "popHeap");
    registerAlias(vm, "", "sortHeap");
    registerAlias(vm, "","isHeap");
    registerAlias(vm, "",    "heapTop");
}

} // namespace cplang
