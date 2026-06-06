#include "stdlib/stdlib.hpp"

namespace cplang {

// Missing algorithms + charconv float for CP stdlib
// #include'd from stdlib.cpp, already inside namespace cplang

// Helper: compare two Values (for sorting)
namespace {
bool valueLess(const Value& a, const Value& b) {
    if (a.isInt() && b.isInt()) return a.asInt() < b.asInt();
    double af = a.isInt() ? static_cast<double>(a.asInt()) : a.asFloat();
    double bf = b.isInt() ? static_cast<double>(b.asInt()) : b.asFloat();
    return af < bf;
}
bool valueEqual(const Value& a, const Value& b) {
    if (a.isInt() && b.isInt()) return a.asInt() == b.asInt();
    if (a.isFloat() && b.isFloat()) return a.asFloat() == b.asFloat();
    return false;
}
} // anon

namespace algo_missing {

// ── stableSort ──
Value stableSort_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::nil();
    auto arr = args[0].asArray();
    std::stable_sort(arr->data.begin(), arr->data.end(), valueLess);
    return args[0];
}

// ── partition ──
Value partition_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::Int(0);
    auto arr = args[0].asArray();
    Value pred = args[1];
    auto it = std::stable_partition(arr->data.begin(), arr->data.end(), [&](const Value& v) {
        std::vector<Value> cargs = {v};
        Value r = VM::current()->callFunction(pred, cargs);
        return r.isTrue();
    });
    return Value::Int(static_cast<Int64>(std::distance(arr->data.begin(), it)));
}

// ── stablePartition ──
Value stablePartition_(std::vector<Value>& args) {
    // std::stable_partitionと同じ
    return partition_(args);
}

// ── partitionPoint ──
Value partitionPoint_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::Int(0);
    auto arr = args[0].asArray();
    Value pred = args[1];
    auto it = std::partition_point(arr->data.begin(), arr->data.end(), [&](const Value& v) {
        std::vector<Value> cargs = {v};
        Value r = VM::current()->callFunction(pred, cargs);
        return r.isTrue();
    });
    return Value::Int(static_cast<Int64>(std::distance(arr->data.begin(), it)));
}

// ── anyOf ──
Value anyOf_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::Bool(false);
    auto arr = args[0].asArray();
    Value pred = args[1];
    for (auto& v : arr->data) {
        std::vector<Value> cargs = {v};
        Value r = VM::current()->callFunction(pred, cargs);
        if (r.isTrue()) return Value::Bool(true);
    }
    return Value::Bool(false);
}

// ── allOf ──
Value allOf_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::Bool(false);
    auto arr = args[0].asArray();
    Value pred = args[1];
    for (auto& v : arr->data) {
        std::vector<Value> cargs = {v};
        Value r = VM::current()->callFunction(pred, cargs);
        if (!r.isTrue()) return Value::Bool(false);
    }
    return Value::Bool(true);
}

// ── noneOf ──
Value noneOf_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::Bool(false);
    auto arr = args[0].asArray();
    Value pred = args[1];
    for (auto& v : arr->data) {
        std::vector<Value> cargs = {v};
        Value r = VM::current()->callFunction(pred, cargs);
        if (r.isTrue()) return Value::Bool(false);
    }
    return Value::Bool(true);
}

// ── countIf (predicate-based count) ──
Value countIf_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::Int(0);
    auto arr = args[0].asArray();
    Value pred = args[1];
    Int64 count = 0;
    for (auto& v : arr->data) {
        std::vector<Value> cargs = {v};
        Value r = VM::current()->callFunction(pred, cargs);
        if (r.isTrue()) count++;
    }
    return Value::Int(count);
}

// ── arrFilter (predicate-based filter, returns new array) ──
Value arrFilter_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::nil();
    auto arr = args[0].asArray();
    Value pred = args[1];
    VMArray* result = VMArray::create();
    for (auto& v : arr->data) {
        std::vector<Value> cargs = {v};
        Value r = VM::current()->callFunction(pred, cargs);
        if (r.isTrue()) result->data.push_back(v);
    }
    return makeArrayVal(result);
}

// ── findIf / findIfNot ──
Value findIf_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::Int(-1);
    auto arr = args[0].asArray();
    Value pred = args[1];
    for (size_t i = 0; i < arr->data.size(); i++) {
        std::vector<Value> cargs = {arr->data[i]};
        Value r = VM::current()->callFunction(pred, cargs);
        if (r.isTrue()) return Value::Int(static_cast<Int64>(i));
    }
    return Value::Int(-1);
}
Value findIfNot_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::Int(-1);
    auto arr = args[0].asArray();
    Value pred = args[1];
    for (size_t i = 0; i < arr->data.size(); i++) {
        std::vector<Value> cargs = {arr->data[i]};
        Value r = VM::current()->callFunction(pred, cargs);
        if (!r.isTrue()) return Value::Int(static_cast<Int64>(i));
    }
    return Value::Int(-1);
}

// ── maxBy / minBy (predicate-based) ──
Value maxBy_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::nil();
    auto arr = args[0].asArray();
    if (arr->data.empty()) return Value::nil();
    Value pred = args[1];
    Value best = arr->data[0];
    Value bestKey = [&]() -> Value {
        std::vector<Value> cargs = {best};
        return VM::current()->callFunction(pred, cargs);
    }();
    for (size_t i = 1; i < arr->data.size(); i++) {
        std::vector<Value> cargs = {arr->data[i]};
        Value key = VM::current()->callFunction(pred, cargs);
        if (valueLess(bestKey, key)) { best = arr->data[i]; bestKey = key; }
    }
    return best;
}
Value minBy_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::nil();
    auto arr = args[0].asArray();
    if (arr->data.empty()) return Value::nil();
    Value pred = args[1];
    Value best = arr->data[0];
    Value bestKey = [&]() -> Value {
        std::vector<Value> cargs = {best};
        return VM::current()->callFunction(pred, cargs);
    }();
    for (size_t i = 1; i < arr->data.size(); i++) {
        std::vector<Value> cargs = {arr->data[i]};
        Value key = VM::current()->callFunction(pred, cargs);
        if (valueLess(key, bestKey)) { best = arr->data[i]; bestKey = key; }
    }
    return best;
}

// ── accumulate ──
Value accumulate_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::nil();
    auto arr = args[0].asArray();
    if (arr->data.empty()) return Value::Int(0);
    Value acc = arr->data[0];
    for (size_t i = 1; i < arr->data.size(); i++) {
        if (acc.isInt() && arr->data[i].isInt())
            acc = Value::Int(acc.asInt() + arr->data[i].asInt());
        else {
            double af = acc.isInt() ? static_cast<double>(acc.asInt()) : acc.asFloat();
            double bf = arr->data[i].isInt() ? static_cast<double>(arr->data[i].asInt()) : arr->data[i].asFloat();
            acc = Value::Float(af + bf);
        }
    }
    return acc;
}

// ── generate ──
Value generate_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[1].isInt()) return Value::nil();
    VMArray* result = VMArray::create();
    Int64 n = args[1].asInt();
    Value gen = args[0];
    for (Int64 i = 0; i < n; i++) {
        std::vector<Value> cargs;
        Value v = VM::current()->callFunction(gen, cargs);
        result->data.push_back(v);
    }
    return makeArrayVal(result);
}

// ── forEach (遍历数组，回调(element, index)) ──
Value forEach_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::nil();
    auto arr = args[0].asArray();
    Value fn = args[1];
    for (Int64 i = 0; i < static_cast<Int64>(arr->data.size()); i++) {
        std::vector<Value> cargs = {arr->data[i], Value::Int(i)};
        VM::current()->callFunction(fn, cargs);
    }
    return Value::nil();
}

// ── transform (数组映射，回调(element, index) → 新元素) ──
Value transform_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::nil();
    auto arr = args[0].asArray();
    Value fn = args[1];
    VMArray* result = VMArray::create();
    for (Int64 i = 0; i < static_cast<Int64>(arr->data.size()); i++) {
        std::vector<Value> cargs = {arr->data[i], Value::Int(i)};
        Value r = VM::current()->callFunction(fn, cargs);
        result->data.push_back(r);
    }
    return makeArrayVal(result);
}

// ── reduce (折叠归约，无初始值) ──
Value reduce_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::nil();
    auto arr = args[0].asArray();
    if (arr->data.empty()) return Value::nil();
    Value fn = args[1];
    Value acc = arr->data[0];
    for (size_t i = 1; i < arr->data.size(); i++) {
        std::vector<Value> cargs = {acc, arr->data[i]};
        acc = VM::current()->callFunction(fn, cargs);
    }
    return acc;
}

// ── generateN (生成序列，指定个数) ──
Value generateN_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[1].isInt()) return Value::nil();
    Int64 n = args[1].asInt();
    if (n < 0) return Value::nil();
    Value gen = args[0];
    VMArray* result = VMArray::create();
    for (Int64 i = 0; i < n; i++) {
        std::vector<Value> cargs;
        Value v = VM::current()->callFunction(gen, cargs);
        result->data.push_back(v);
    }
    return makeArrayVal(result);
}

// ── iota (用递增值填充数组) ──
Value iota_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray() || !args[1].isInt()) return Value::nil();
    auto arr = args[0].asArray();
    Int64 start = args[1].asInt();
    for (size_t i = 0; i < arr->data.size(); i++)
        arr->data[i] = Value::Int(start + static_cast<Int64>(i));
    return args[0];
}

// ── fill (用固定值填充数组) ──
Value fill_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::nil();
    auto arr = args[0].asArray();
    for (auto& v : arr->data)
        v = args[1];
    return args[0];
}

} // namespace algo_missing

void StdLib::registerAlgoMissing(VM* vm) {
    using namespace algo_missing;
    registerFunction(vm, "stableSort",      stableSort_);
    registerFunction(vm, "partition",       partition_);
    registerFunction(vm, "stablePartition", stablePartition_);
    registerFunction(vm, "partitionPoint",  partitionPoint_);
    registerFunction(vm, "anyOf",           anyOf_);
    registerFunction(vm, "allOf",           allOf_);
    registerFunction(vm, "noneOf",          noneOf_);
    registerFunction(vm, "countIf",         countIf_);
    registerFunction(vm, "arrFilter",       arrFilter_);
    registerFunction(vm, "findIf",          findIf_);
    registerFunction(vm, "findIfNot",       findIfNot_);
    registerFunction(vm, "maxBy",           maxBy_);
    registerFunction(vm, "minBy",           minBy_);
    registerFunction(vm, "accumulate",      accumulate_);
    registerFunction(vm, "generate",        generate_);
    registerFunction(vm, "forEach",         forEach_);
    registerFunction(vm, "transform",       transform_);
    registerFunction(vm, "reduce",          reduce_);
    registerFunction(vm, "generateN",       generateN_);
    registerFunction(vm, "iota",            iota_);
    registerFunction(vm, "fill",            fill_);
    registerAlias(vm, "稳定排序",           "stableSort");
    registerAlias(vm, "分区",               "partition");
    registerAlias(vm, "稳定分割",           "stablePartition");
    registerAlias(vm, "分割点",             "partitionPoint");
    registerAlias(vm, "任一满足",           "anyOf");
    registerAlias(vm, "全部满足",           "allOf");
    registerAlias(vm, "无一满足",           "noneOf");
    registerAlias(vm, "条件计数",           "countIf");
    registerAlias(vm, "数组筛选",           "arrFilter");
    registerAlias(vm, "条件查找",           "findIf");
    registerAlias(vm, "条件查找非",         "findIfNot");
    registerAlias(vm, "最大按",             "maxBy");
    registerAlias(vm, "最小按",             "minBy");
    registerAlias(vm, "累计",               "accumulate");
    registerAlias(vm, "生成",               "generate");
    registerAlias(vm, "遍历",               "forEach");
    registerAlias(vm, "数组变换",           "transform");
    registerAlias(vm, "归约",               "reduce");
    registerAlias(vm, "生成N",              "generateN");
    registerAlias(vm, "填充递增值",         "iota");
    registerAlias(vm, "填充",               "fill");
}

} // namespace cplang
