#include "stdlib/stdlib.hpp"

namespace cplang {

// Array functions (len, push, pop, insert, remove, slice, reverse, sort, indexOf)
// #include'd from stdlib.cpp, already inside namespace cplang

void StdLib::registerArray(VM* vm) {
    registerFunction(vm, "arrlen", array::len);
    registerAlias(vm, "数组长", "arrlen");
    registerFunction(vm, "push", array::push);
    registerAlias(vm, "追加", "push");
    registerFunction(vm, "pop", array::pop);
    registerAlias(vm, "弹出", "pop");
    registerFunction(vm, "insert", array::insert);
    registerAlias(vm, "插入", "insert");
    registerFunction(vm, "remove", array::remove);
    registerAlias(vm, "移除", "remove");
    registerFunction(vm, "slice", array::slice);
    registerFunction(vm, "reverse", array::reverse);
    registerFunction(vm, "sort", array::sort);
    registerFunction(vm, "indexOf", array::indexOf);
    
    // 中文别名
    registerAlias(vm, "追加", "push");
    registerAlias(vm, "弹出", "pop");
    registerAlias(vm, "插入", "insert");
    registerAlias(vm, "删除", "remove");
    registerAlias(vm, "切片", "slice");
}

Value array::len(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::Int(0);
    return Value::Int(args[0].asArray()->length());
}

Value array::push(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::nil();
    
    auto arr = args[0].asArray();
    for (size_t i = 1; i < args.size(); i++) {
        arr->data.push_back(args[i]);
    }
    return Value::Int(arr->length());
}

Value array::pop(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::nil();
    
    auto arr = args[0].asArray();
    if (arr->data.empty()) return Value::nil();
    
    Value val = arr->data.back();
    arr->data.pop_back();
    return val;
}

Value array::insert(std::vector<Value>& args) {
    if (args.size() < 3 || !args[0].isArray()) return Value::nil();
    
    auto arr = args[0].asArray();
    Int64 index = args[1].asInt();
    
    if (index < 0) index = arr->length() + index + 1;
    if (index < 0) index = 0;
    if (index > arr->length()) index = arr->length();
    
    arr->data.insert(arr->data.begin() + index, args[2]);
    return Value::Int(arr->length());
}

Value array::remove(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::nil();
    
    auto arr = args[0].asArray();
    Int64 index = args[1].asInt();
    
    if (index < 0) index = arr->length() + index;
    if (index < 0 || index >= arr->length()) return Value::nil();
    
    Value val = arr->data[index];
    arr->data.erase(arr->data.begin() + index);
    return val;
}

Value array::slice(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::Array(VMArray::create());
    
    auto arr = args[0].asArray();
    Int64 start = (args.size() > 1) ? args[1].asInt() : 0;
    Int64 end = (args.size() > 2) ? args[2].asInt() : arr->length();
    
    if (start < 0) start = arr->length() + start;
    if (end < 0) end = arr->length() + end;
    if (start < 0) start = 0;
    if (end > arr->length()) end = arr->length();
    
    auto result = VMArray::create();
    for (Int64 i = start; i < end; i++) {
        result->data.push_back(arr->data[i]);
    }
    
    return Value::Array(result);
}

Value array::reverse(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::nil();
    
    auto arr = args[0].asArray();
    std::reverse(arr->data.begin(), arr->data.end());
    return args[0];
}

Value array::sort(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::nil();
    
    auto arr = args[0].asArray();
    std::sort(arr->data.begin(), arr->data.end(), [](const Value& a, const Value& b) {
        if (a.isInt() && b.isInt()) return a.asInt() < b.asInt();
        if (a.isFloat() && b.isFloat()) return a.asFloat() < b.asFloat();
        if (a.isInt() && b.isFloat()) return static_cast<double>(a.asInt()) < b.asFloat();
        if (a.isFloat() && b.isInt()) return a.asFloat() < static_cast<double>(b.asInt());
        return false;
    });
    
    return args[0];
}

Value array::indexOf(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::Int(-1);
    
    auto arr = args[0].asArray();
    const Value& target = args[1];
    
    for (size_t i = 0; i < arr->data.size(); i++) {
        if (arr->data[i].equals(target)) {
            return Value::Int(static_cast<Int64>(i));
        }
    }
    
    return Value::Int(-1);
}

} // namespace cplang
