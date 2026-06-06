#include "stdlib/stdlib.hpp"

namespace cplang {

// Algorithm extension functions (sample, search, equal, etc.)
// #include'd from stdlib.cpp, already inside namespace cplang

namespace algo_ext {
    Value sample(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].isArray() || !args[1].isInt()) return Value::nil();
        VMArray* arr = args[0].asArray();
        size_t n = (size_t)args[1].asInt();
        if (n > arr->data.size()) n = arr->data.size();
        if (n == 0) return Value::Array(VMArray::create());
        VMArray* result = VMArray::create((UInt32)n);
        std::vector<Value> shuffled = arr->data;
        std::mt19937 rng(std::random_device{}());
        std::shuffle(shuffled.begin(), shuffled.end(), rng);
        for (size_t i = 0; i < n; i++)
            result->data.push_back(shuffled[i]);
        return Value::Array(result);
    }
    Value search(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].isArray() || !args[1].isArray()) return Value::Int(-1);
        VMArray* arr = args[0].asArray();
        VMArray* sub = args[1].asArray();
        auto it = std::search(arr->data.begin(), arr->data.end(), sub->data.begin(), sub->data.end(), ValueEqual{});
        if (it == arr->data.end()) return Value::Int(-1);
        return Value::Int((int)(it - arr->data.begin()));
    }
    Value equal(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].isArray() || !args[1].isArray()) return Value::Bool(false);
        VMArray* a = args[0].asArray();
        VMArray* b = args[1].asArray();
        return Value::Bool(std::equal(a->data.begin(), a->data.end(), b->data.begin(), b->data.end(), ValueEqual{}));
    }
    Value mismatch(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].isArray() || !args[1].isArray()) return Value::Int(-1);
        VMArray* a = args[0].asArray();
        VMArray* b = args[1].asArray();
        auto [it1, it2] = std::mismatch(a->data.begin(), a->data.end(), b->data.begin(), ValueEqual{});
        if (it1 == a->data.end()) return Value::Int(-1);
        return Value::Int((int)(it1 - a->data.begin()));
    }
    Value swapRange(std::vector<Value>& args) {
        if (args.size() < 4 || !args[0].isArray() || !args[1].isInt() || !args[2].isInt() || !args[3].isInt())
            return Value::Bool(false);
        VMArray* arr = args[0].asArray();
        int start1 = (int)args[1].asInt();
        int start2 = (int)args[2].asInt();
        int n = (int)args[3].asInt();
        if (start1 < 0 || start2 < 0 || start1 + n > (int)arr->data.size() || start2 + n > (int)arr->data.size())
            return Value::Bool(false);
        std::swap_ranges(arr->data.begin() + start1, arr->data.begin() + start1 + n, arr->data.begin() + start2);
        return Value::Bool(true);
    }
    Value lexicographicalCompare(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].isArray() || !args[1].isArray()) return Value::Int(0);
        VMArray* a = args[0].asArray();
        VMArray* b = args[1].asArray();
        auto cmp = [](const Value& x, const Value& y) -> bool {
            double vx = x.isInt() ? (double)x.asInt() : x.isFloat() ? x.asFloat() : 0.0;
            double vy = y.isInt() ? (double)y.asInt() : y.isFloat() ? y.asFloat() : 0.0;
            return vx < vy;
        };
        if (std::lexicographical_compare(a->data.begin(), a->data.end(), b->data.begin(), b->data.end(), cmp))
            return Value::Int(-1);
        if (std::lexicographical_compare(b->data.begin(), b->data.end(), a->data.begin(), a->data.end(), cmp))
            return Value::Int(1);
        return Value::Int(0);
    }
}

void StdLib::registerAlgoExt(VM* vm) {
    registerFunction(vm, "sample",                  algo_ext::sample);
    registerFunction(vm, "search",                  algo_ext::search);
    registerFunction(vm, "equal",                   algo_ext::equal);
    registerFunction(vm, "mismatch",                algo_ext::mismatch);
    registerFunction(vm, "swapRange",               algo_ext::swapRange);
    registerFunction(vm, "lexicographicalCompare",  algo_ext::lexicographicalCompare);
    registerAlias(vm, "采样",               "sample");
    registerAlias(vm, "搜索子序列",         "search");
    registerAlias(vm, "数组相等",           "equal");
    registerAlias(vm, "首个不匹配",         "mismatch");
    registerAlias(vm, "交换范围",           "swapRange");
    registerAlias(vm, "字典序比较",         "lexicographicalCompare");
}

} // namespace cplang
