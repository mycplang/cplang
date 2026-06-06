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
    registerAlias(vm, "表有", "has");
    registerAlias(vm, "表取", "tableGet");
    registerAlias(vm, "表设", "tableSet");
    registerAlias(vm, "表删", "tableDel");
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

} // namespace cplang
