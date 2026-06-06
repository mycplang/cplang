#include "stdlib/stdlib.hpp"

namespace cplang {

// Reflection utilities (keys, values, hasKey, getField, setField)
// #include'd from stdlib.cpp, already inside namespace cplang

namespace reflect {
    Value keys(std::vector<Value>& args) {
        if (args.empty() || !args[0].isTable())
            return Value::Array(VMArray::create());
        VMTable* t = (VMTable*)args[0].obj;
        VMArray* result = VMArray::create((UInt32)t->size());
        for (auto& [k, v] : t->data) result->data.push_back(k);
        return Value::Array(result);
    }
    Value values(std::vector<Value>& args) {
        if (args.empty() || !args[0].isTable())
            return Value::Array(VMArray::create());
        VMTable* t = (VMTable*)args[0].obj;
        VMArray* result = VMArray::create((UInt32)t->size());
        for (auto& [k, v] : t->data) result->data.push_back(v);
        return Value::Array(result);
    }
    Value hasKey(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].isTable())
            return Value::Bool(false);
        VMTable* t = (VMTable*)args[0].obj;
        return Value::Bool(t->has(args[1]));
    }
    Value getField(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].isTable())
            return Value::nil();
        VMTable* t = (VMTable*)args[0].obj;
        return t->get(args[1]);
    }
    Value setField(std::vector<Value>& args) {
        if (args.size() < 3 || !args[0].isTable())
            return Value::nil();
        VMTable* t = (VMTable*)args[0].obj;
        t->set(args[1], args[2]);
        return args[0];
    }
}

void StdLib::registerReflection(VM* vm) {
    registerFunction(vm, "keys",     reflect::keys);
    registerFunction(vm, "values",   reflect::values);
    registerFunction(vm, "hasKey",   reflect::hasKey);
    registerFunction(vm, "getField", reflect::getField);
    registerFunction(vm, "setField", reflect::setField);
    registerAlias(vm, "所有键",     "keys");
    registerAlias(vm, "所有值",     "values");
    registerAlias(vm, "有键",       "hasKey");
    registerAlias(vm, "取字段",     "getField");
    registerAlias(vm, "设字段",     "setField");
}

} // namespace cplang
