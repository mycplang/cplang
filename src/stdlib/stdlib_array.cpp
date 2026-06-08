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
    registerFunction(vm, "shift", array::shift);
    registerAlias(vm, "头出", "shift");
    registerFunction(vm, "unshift", array::unshift);
    registerAlias(vm, "头插", "unshift");
    registerFunction(vm, "insert", array::insert);
    registerAlias(vm, "插入", "insert");
    registerFunction(vm, "remove", array::remove);
    registerAlias(vm, "删除", "remove");
    registerAlias(vm, "移除", "remove");
    registerFunction(vm, "slice", array::slice);
    registerAlias(vm, "切片", "slice");
    registerFunction(vm, "splice", array::splice);
    registerAlias(vm, "数组拼接", "splice");
    registerFunction(vm, "reverse", array::reverse);
    registerAlias(vm, "反转", "reverse");
    registerFunction(vm, "sort", array::sort);
    registerAlias(vm, "排序", "sort");
    registerFunction(vm, "indexOf", array::indexOf);
    registerAlias(vm, "取索引", "indexOf");
    registerFunction(vm, "lastIndexOf", array::lastIndexOf);
    registerAlias(vm, "取最后索引", "lastIndexOf");
    registerFunction(vm, "map", array::map);
    registerAlias(vm, "映射", "map");
    registerFunction(vm, "filter", array::filter);
    registerAlias(vm, "过滤", "filter");
    registerFunction(vm, "reduce", array::reduce);
    registerAlias(vm, "累积", "reduce");
    registerFunction(vm, "includes", array::includes);
    registerAlias(vm, "包含元素", "includes");
    registerFunction(vm, "find", array::find);
    registerAlias(vm, "查找元素", "find");
    registerFunction(vm, "findIndex", array::findIndex);
    registerAlias(vm, "查找索引", "findIndex");
    registerFunction(vm, "fill", array::fill);
    registerAlias(vm, "填充数组", "fill");
    registerFunction(vm, "copy", array::copy);
    registerAlias(vm, "拷贝数组", "copy");
    registerFunction(vm, "flatten", array::flatten);
    registerAlias(vm, "展平", "flatten");
    registerFunction(vm, "unique", array::unique);
    registerAlias(vm, "去重", "unique");
    registerFunction(vm, "zip", array::zip);
    registerAlias(vm, "打包", "zip");
    registerFunction(vm, "unzip", array::unzip);
    registerAlias(vm, "解包", "unzip");
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

// shift(arr) — 移除并返回数组第一个元素
Value array::shift(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::nil();
    auto arr = args[0].asArray();
    if (arr->data.empty()) return Value::nil();
    Value first = arr->data[0];
    arr->data.erase(arr->data.begin());
    return first;
}

// unshift(arr, val) — 在数组开头插入元素
Value array::unshift(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::Int(0);
    auto arr = args[0].asArray();
    arr->data.insert(arr->data.begin(), args[1]);
    return Value::Int(static_cast<Int64>(arr->data.size()));
}

// map(arr, fn) — 对每个元素应用函数，返回新数组
Value array::map(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::nil();
    auto arr = args[0].asArray();
    auto result = VMArray::create();
    for (size_t i = 0; i < arr->data.size(); i++) {
        std::vector<Value> callArgs = {arr->data[i]};
        Value mapped = VM::current()->callFunction(args[1], callArgs);
        result->set(static_cast<Int64>(i), mapped);
    }
    return makeArrayVal(result);
}

// filter(arr, fn) — 保留满足条件的元素
Value array::filter(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::nil();
    auto arr = args[0].asArray();
    auto result = VMArray::create();
    Int64 idx = 0;
    for (size_t i = 0; i < arr->data.size(); i++) {
        std::vector<Value> callArgs = {arr->data[i]};
        Value keep = VM::current()->callFunction(args[1], callArgs);
        if (keep.isTrue()) {
            result->set(idx++, arr->data[i]);
        }
    }
    return makeArrayVal(result);
}

// reduce(arr, fn, init) — 从左到右累积
Value array::reduce(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::nil();
    auto arr = args[0].asArray();
    if (arr->data.empty()) return args.size() > 2 ? args[2] : Value::nil();
    size_t start = 0;
    Value acc;
    if (args.size() > 2) {
        acc = args[2];
    } else {
        acc = arr->data[0];
        start = 1;
    }
    for (size_t i = start; i < arr->data.size(); i++) {
        std::vector<Value> callArgs = {acc, arr->data[i]};
        acc = VM::current()->callFunction(args[1], callArgs);
    }
    return acc;
}

// includes(arr, val) — 检查数组是否包含值
Value array::includes(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::Bool(false);
    auto arr = args[0].asArray();
    for (size_t i = 0; i < arr->data.size(); i++) {
        if (arr->data[i].equals(args[1])) return Value::Bool(true);
    }
    return Value::Bool(false);
}

// lastIndexOf(arr, val) — 查找值最后出现的索引
Value array::lastIndexOf(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::Int(-1);
    auto arr = args[0].asArray();
    for (int i = static_cast<int>(arr->data.size()) - 1; i >= 0; i--) {
        if (arr->data[i].equals(args[1])) return Value::Int(static_cast<Int64>(i));
    }
    return Value::Int(-1);
}

// copy(arr) — 浅拷贝数组
Value array::copy(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::nil();
    auto arr = args[0].asArray();
    auto result = VMArray::create();
    for (size_t i = 0; i < arr->data.size(); i++) {
        result->set(static_cast<Int64>(i), arr->data[i]);
    }
    return makeArrayVal(result);
}

// unzip(arr) — 解压数组对 [ [a1,b1], [a2,b2] ] → [ [a1,a2], [b1,b2] ]
Value array::unzip(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::nil();
    auto arr = args[0].asArray();
    if (arr->data.empty()) {
        auto empty = VMArray::create();
        return makeArrayVal(empty);
    }
    size_t pairCount = arr->data[0].isArray() ? arr->data[0].asArray()->data.size() : 0;
    if (pairCount == 0) return makeArrayVal(VMArray::create());

    auto result = VMArray::create();
    for (size_t j = 0; j < pairCount; j++) {
        auto col = VMArray::create();
        for (size_t i = 0; i < arr->data.size(); i++) {
            if (arr->data[i].isArray()) {
                auto row = arr->data[i].asArray();
                if (j < row->data.size()) col->set(static_cast<Int64>(i), row->data[j]);
            }
        }
        result->set(static_cast<Int64>(j), makeArrayVal(col));
    }
    return makeArrayVal(result);
}

// splice(arr, start, deleteCount, ...items) — 删除/替换/插入元素
Value array::splice(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::nil();
    auto arr = args[0].asArray();
    Int64 start = args.size() > 1 ? args[1].asInt() : 0;
    Int64 deleteCount = args.size() > 2 ? args[2].asInt() : static_cast<Int64>(arr->data.size());
    if (start < 0) start = std::max(Int64(0), static_cast<Int64>(arr->data.size()) + start);
    if (start > static_cast<Int64>(arr->data.size())) start = static_cast<Int64>(arr->data.size());
    if (deleteCount < 0) deleteCount = 0;

    auto removed = VMArray::create();
    size_t end = std::min(static_cast<size_t>(start + deleteCount), arr->data.size());
    for (size_t i = static_cast<size_t>(start); i < end; i++) {
        removed->set(static_cast<Int64>(i - start), arr->data[i]);
    }
    arr->data.erase(arr->data.begin() + static_cast<size_t>(start), arr->data.begin() + end);

    // 插入新元素
    for (size_t i = 3; i < args.size(); i++) {
        arr->data.insert(arr->data.begin() + static_cast<size_t>(start) + (i - 3), args[i]);
    }
    return makeArrayVal(removed);
}

// find(arr, val_or_fn) — 查找第一个满足条件的元素
Value array::find(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::nil();
    auto arr = args[0].asArray();
    if (args[1].isFunction() || args[1].isCFunction()) {
        for (size_t i = 0; i < arr->data.size(); i++) {
            std::vector<Value> callArgs = {arr->data[i]};
            Value found = VM::current()->callFunction(args[1], callArgs);
            if (found.isTrue()) return arr->data[i];
        }
    } else {
        for (size_t i = 0; i < arr->data.size(); i++) {
            if (arr->data[i].equals(args[1])) return arr->data[i];
        }
    }
    return Value::nil();
}

// findIndex(arr, val_or_fn) — 查找第一个满足条件的索引
Value array::findIndex(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::Int(-1);
    auto arr = args[0].asArray();
    if (args[1].isFunction() || args[1].isCFunction()) {
        for (size_t i = 0; i < arr->data.size(); i++) {
            std::vector<Value> callArgs = {arr->data[i]};
            if (VM::current()->callFunction(args[1], callArgs).isTrue())
                return Value::Int(static_cast<Int64>(i));
        }
    } else {
        for (size_t i = 0; i < arr->data.size(); i++) {
            if (arr->data[i].equals(args[1])) return Value::Int(static_cast<Int64>(i));
        }
    }
    return Value::Int(-1);
}

// fill(arr, val, start, end) — 填充数组
Value array::fill(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return args[0];
    auto arr = args[0].asArray();
    Int64 start = args.size() > 2 ? args[2].asInt() : 0;
    Int64 end = args.size() > 3 ? args[3].asInt() : static_cast<Int64>(arr->data.size());
    if (start < 0) start = 0;
    if (end > static_cast<Int64>(arr->data.size())) end = static_cast<Int64>(arr->data.size());
    for (Int64 i = start; i < end; i++) {
        arr->set(i, args[1]);
    }
    return args[0];
}

// flatten(arr, depth) — 展平嵌套数组
Value array::flatten(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::nil();
    auto arr = args[0].asArray();
    int depth = args.size() > 1 ? static_cast<int>(args[1].asInt()) : 1;
    auto result = VMArray::create();
    std::function<void(VMArray*, int)> flattenImpl = [&](VMArray* src, int d) {
        for (size_t i = 0; i < src->data.size(); i++) {
            if (src->data[i].isArray() && d > 0) {
                flattenImpl(src->data[i].asArray(), d - 1);
            } else {
                result->set(static_cast<Int64>(result->data.size()), src->data[i]);
            }
        }
    };
    flattenImpl(arr, depth);
    return makeArrayVal(result);
}

// unique(arr) — 数组去重
Value array::unique(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::nil();
    auto arr = args[0].asArray();
    auto result = VMArray::create();
    Int64 idx = 0;
    for (size_t i = 0; i < arr->data.size(); i++) {
        bool dup = false;
        for (size_t j = 0; j < i; j++) {
            if (arr->data[i].equals(arr->data[j])) { dup = true; break; }
        }
        if (!dup) result->set(idx++, arr->data[i]);
    }
    return makeArrayVal(result);
}

// zip(a, b) — 将两个数组打包为对数组
Value array::zip(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray() || !args[1].isArray()) return Value::nil();
    auto a = args[0].asArray(), b = args[1].asArray();
    auto result = VMArray::create();
    size_t n = std::min(a->data.size(), b->data.size());
    for (size_t i = 0; i < n; i++) {
        auto pair = VMArray::create();
        pair->set(0, a->data[i]);
        pair->set(1, b->data[i]);
        result->set(static_cast<Int64>(i), makeArrayVal(pair));
    }
    return makeArrayVal(result);
}

} // namespace cplang