#include "stdlib/stdlib.hpp"

namespace cplang {

// Table functions (create, len, keys, values, has, get, set, delete)
// #include'd from stdlib.cpp, already inside namespace cplang

void StdLib::registerTable(VM* vm) {
    registerFunction(vm, "table", table::create);
    registerFunction(vm, "tblen", table::len);
    registerFunction(vm, "keys", table::keys);
    registerFunction(vm, "values", table::values);
    registerFunction(vm, "has", table::has);
    registerFunction(vm, "tableGet", table::get);
    registerFunction(vm, "tableSet", table::set);
    registerFunction(vm, "tableDel", table::delete_);
    registerAlias(vm, "表", "table");
    registerAlias(vm, "表长", "tblen");
    registerAlias(vm, "表创建", "table");
    registerAlias(vm, "表键", "keys");
    registerAlias(vm, "表值", "values");
    registerFunction(vm, "tableEntries", table::entries);
    registerAlias(vm, "表条目", "tableEntries");
    registerFunction(vm, "tableClear", table::clear);
    registerAlias(vm, "表清空", "tableClear");
    registerFunction(vm, "tableMerge", table::merge);
    registerAlias(vm, "表合并", "tableMerge");
    registerFunction(vm, "tableClone", table::clone);
    registerAlias(vm, "表克隆", "tableClone");
    registerFunction(vm, "tableToArray", table::toArray);
    registerAlias(vm, "表转数组", "tableToArray");
    registerFunction(vm, "tableFromArray", table::fromArray);
    registerAlias(vm, "数组转表", "tableFromArray");
    registerAlias(vm, "表有", "has");
    registerAlias(vm, "表取", "tableGet");
    registerAlias(vm, "表设", "tableSet");
    registerAlias(vm, "表删", "tableDel");
}

Value table::len(std::vector<Value>& args) {
    if (args.empty()) return Value::Int(0);
    if (args[0].isTable()) {
        VMTable* t = args[0].asTable();
        return Value::Int(static_cast<Int64>(t->data.size()));
    }
    if (args[0].isArray()) {
        VMArray* a = args[0].asArray();
        return Value::Int(static_cast<Int64>(a->data.size()));
    }
    return Value::Int(0);
}

Value table::keys(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::Array(VMArray::create());
    VMTable* t = args[0].asTable();
    VMArray* arr = VMArray::create();
    for (auto& kv : t->data) arr->data.push_back(kv.first);
    return Value::Array(arr);
}

Value table::values(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::Array(VMArray::create());
    VMTable* t = args[0].asTable();
    VMArray* arr = VMArray::create();
    for (auto& kv : t->data) arr->data.push_back(kv.second);
    return Value::Array(arr);
}

Value table::has(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isTable()) return Value::Bool(false);
    VMTable* t = args[0].asTable();
    return Value::Bool(t->has(args[1]));
}

Value table::get(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isTable()) return Value::nil();
    VMTable* t = args[0].asTable();
    return t->get(args[1]);
}

Value table::set(std::vector<Value>& args) {
    if (args.size() < 3 || !args[0].isTable()) return Value::nil();
    VMTable* t = args[0].asTable();
    t->set(args[1], args[2]);
    return args[2];
}

Value table::delete_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isTable()) return Value::Bool(false);
    VMTable* t = args[0].asTable();
    return Value::Bool(t->remove(args[1]));
}

Value table::create(std::vector<Value>& args) {
    (void)args;
    return Value::Table(VMTable::create());
}

// entries(t) — 返回键值对数组 [[k1,v1], [k2,v2], ...]
Value table::entries(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::Array(VMArray::create());
    VMTable* t = args[0].asTable();
    VMArray* arr = VMArray::create();
    for (auto& kv : t->data) {
        VMArray* pair = VMArray::create();
        pair->data.push_back(kv.first);
        pair->data.push_back(kv.second);
        arr->data.push_back(Value::Array(pair));
    }
    return Value::Array(arr);
}

// clear(t) — 清空表
Value table::clear(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::nil();
    VMTable* t = args[0].asTable();
    t->data.clear();
    return args[0];
}

// merge(a, b) — 合并两个表（b 的键覆盖 a 的同名键）
Value table::merge(std::vector<Value>& args) {
    if (args.size() < 2) return args.empty() ? Value::nil() : args[0];
    VMTable* result = VMTable::create();
    if (args[0].isTable()) {
        VMTable* a = args[0].asTable();
        for (auto& kv : a->data) result->set(kv.first, kv.second);
    }
    if (args[1].isTable()) {
        VMTable* b = args[1].asTable();
        for (auto& kv : b->data) result->set(kv.first, kv.second);
    }
    return Value::Table(result);
}

// clone(t) — 深拷贝表
Value table::clone(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::nil();
    VMTable* src = args[0].asTable();
    VMTable* dst = VMTable::create();
    for (auto& kv : src->data) dst->set(kv.first, kv.second);
    return Value::Table(dst);
}

// toArray(t) — 表转为数组 [v1, v2, ...]
Value table::toArray(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::Array(VMArray::create());
    VMTable* t = args[0].asTable();
    VMArray* arr = VMArray::create();
    for (auto& kv : t->data) arr->data.push_back(kv.second);
    return Value::Array(arr);
}

// fromArray(arr) — 数组转为表 {0: v0, 1: v1, ...}
Value table::fromArray(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::nil();
    VMArray* arr = args[0].asArray();
    VMTable* t = VMTable::create();
    for (size_t i = 0; i < arr->data.size(); i++) {
        t->set(Value::Int(static_cast<Int64>(i)), arr->data[i]);
    }
    return Value::Table(t);
}

} // namespace cplang
