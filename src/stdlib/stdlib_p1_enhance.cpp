#include "stdlib/stdlib.hpp"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace cplang {

// P1 enhancements: charconv float, span subspan/first/last, result monad, tuple swap, binary I/O, call_once
// #include'd from stdlib.cpp, already inside namespace cplang

// ==================== Charconv Float ====================
namespace charconv_float_ns {

Value floatToStr_(std::vector<Value>& args) {
    if (args.empty()) return makeStringVal(VMString::create("0"));
    double val = args[0].isInt() ? static_cast<double>(args[0].asInt()) : args[0].asFloat();
    char buf[64];
    snprintf(buf, sizeof(buf), "%.17g", val);
    return makeStringVal(VMString::create(std::string(buf)));
}
Value strToFloat_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Float(0.0);
    std::string s(args[0].asString()->data, args[0].asString()->length);
    return Value::Float(std::strtod(s.c_str(), nullptr));
}

} // namespace charconv_float_ns

void StdLib::registerCharconvFloat(VM* vm) {
    using namespace charconv_float_ns;
    registerFunction(vm, "floatToStr", floatToStr_);
    registerFunction(vm, "strToFloat", strToFloat_);
    registerAlias(vm, "浮点转字符串", "floatToStr");
    registerAlias(vm, "字符串转浮点", "strToFloat");
}

// ==================== Span Enhance ====================
namespace span_enhance_ns {

Value spanSubspan_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isTable()) return Value::nil();
    VMTable* src = args[0].asTable();
    VMTable* t = VMTable::create();
    t->set(makeStringVal(VMString::create("_src")), src->get(makeStringVal(VMString::create("_src"))));
    Int64 oldStart = src->get(makeStringVal(VMString::create("_start"))).asInt();
    Int64 offset = args[1].asInt();
    Int64 count = -1;
    if (args.size() >= 3) count = args[2].asInt();
    t->set(makeStringVal(VMString::create("_start")), Value::Int(oldStart + offset));
    t->set(makeStringVal(VMString::create("_len")), Value::Int(count));
    return makeTableVal(t);
}
Value spanFirst_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isTable()) return Value::nil();
    std::vector<Value> cargs = {args[0], Value::Int(0), args[1]};
    return spanSubspan_(cargs);
}
Value spanLast_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isTable()) return Value::nil();
    VMTable* src = args[0].asTable();
    Int64 total = src->get(makeStringVal(VMString::create("_len"))).asInt();
    VMTable* srcTable = src->get(makeStringVal(VMString::create("_src"))).asTable();
    if (total < 0) total = static_cast<Int64>(srcTable->data.size())
                        - src->get(makeStringVal(VMString::create("_start"))).asInt();
    Int64 n = args[1].asInt();
    if (n > total) n = total;
    std::vector<Value> cargs = {args[0], Value::Int(total - n), Value::Int(n)};
    return spanSubspan_(cargs);
}

} // namespace span_enhance_ns

void StdLib::registerSpanEnhance(VM* vm) {
    using namespace span_enhance_ns;
    registerFunction(vm, "spanSubspan", spanSubspan_);
    registerFunction(vm, "spanFirst",   spanFirst_);
    registerFunction(vm, "spanLast",    spanLast_);
    registerAlias(vm, "切分子段",       "spanSubspan");
    registerAlias(vm, "切分首段",       "spanFirst");
    registerAlias(vm, "切分末段",       "spanLast");
}

// ==================== Result Monad ====================
namespace result_monad_ns {

// map: if Ok, apply fn to value, wrap in new Ok
Value resMap_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isTable()) return args[0];
    VMTable* t = args[0].asTable();
    Value ok = t->get(makeStringVal(VMString::create("_ok")));
    if (!ok.isTrue()) return args[0]; // return Err as-is
    Value val = t->get(makeStringVal(VMString::create("_val")));
    Value fn = args[1];
    std::vector<Value> cargs = {val};
    Value mapped = VM::current()->callFunction(fn, cargs);
    VMTable* r = VMTable::create();
    r->set(makeStringVal(VMString::create("_ok")), Value::Bool(true));
    r->set(makeStringVal(VMString::create("_val")), mapped);
    return makeTableVal(r);
}
// flatMap: if Ok, apply fn(val) which returns Result
Value resFlatMap_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isTable()) return args[0];
    VMTable* t = args[0].asTable();
    Value ok = t->get(makeStringVal(VMString::create("_ok")));
    if (!ok.isTrue()) return args[0];
    Value val = t->get(makeStringVal(VMString::create("_val")));
    Value fn = args[1];
    std::vector<Value> cargs = {val};
    return VM::current()->callFunction(fn, cargs);
}
// orElse: if Ok return value, else return default
Value resOrElse_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isTable()) return args[1];
    VMTable* t = args[0].asTable();
    Value ok = t->get(makeStringVal(VMString::create("_ok")));
    if (ok.isTrue()) return t->get(makeStringVal(VMString::create("_val")));
    Value fn = args[1];
    std::vector<Value> cargs = {t->get(makeStringVal(VMString::create("_val")))};
    return VM::current()->callFunction(fn, cargs);
}
// resAndThen: alias for flatMap, more readable
Value resAndThen_(std::vector<Value>& args) {
    return resFlatMap_(args);
}

} // namespace result_monad_ns

void StdLib::registerResultMonad(VM* vm) {
    using namespace result_monad_ns;
    registerFunction(vm, "resMap",      resMap_);
    registerFunction(vm, "resFlatMap",  resFlatMap_);
    registerFunction(vm, "resOrElse",   resOrElse_);
    registerFunction(vm, "resAndThen",  resAndThen_);
    registerAlias(vm, "结果映射",       "resMap");
    registerAlias(vm, "结果平展映射",   "resFlatMap");
    registerAlias(vm, "结果否则",       "resOrElse");
    registerAlias(vm, "结果然后",       "resAndThen");
}

// ==================== Tuple Swap ====================
namespace tuple_enhance_ns {

Value tupSwap_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return args[0];
    VMTable* t = args[0].asTable();
    Value lenV = t->get(makeStringVal(VMString::create("_size")));
    Int64 len = lenV.isInt() ? lenV.asInt() : 0;
    if (len < 2) return args[0];
    Value a = t->get(Value::Int(0));
    Value b = t->get(Value::Int(1));
    t->set(Value::Int(0), b);
    t->set(Value::Int(1), a);
    return args[0];
}
// First/last few elements
Value tupFirst_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isTable()) return args[0];
    VMTable* src = args[0].asTable();
    Int64 n = args[1].asInt();
    VMTable* r = VMTable::create();
    r->set(makeStringVal(VMString::create("_size")), Value::Int(n));
    for (Int64 i = 0; i < n; i++) r->set(Value::Int(i), src->get(Value::Int(i)));
    return makeTableVal(r);
}
Value tupLast_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isTable()) return args[0];
    VMTable* src = args[0].asTable();
    Int64 total = src->get(makeStringVal(VMString::create("_size"))).asInt();
    Int64 n = args[1].asInt();
    if (n > total) n = total;
    VMTable* r = VMTable::create();
    r->set(makeStringVal(VMString::create("_size")), Value::Int(n));
    for (Int64 i = 0; i < n; i++) r->set(Value::Int(i), src->get(Value::Int(total - n + i)));
    return makeTableVal(r);
}

} // namespace tuple_enhance_ns

void StdLib::registerTupleEnhance(VM* vm) {
    using namespace tuple_enhance_ns;
    registerFunction(vm, "tupSwap",  tupSwap_);
    registerFunction(vm, "tupFirst", tupFirst_);
    registerFunction(vm, "tupLast",  tupLast_);
    registerAlias(vm, "元组交换",    "tupSwap");
    registerAlias(vm, "元组首段",    "tupFirst");
    registerAlias(vm, "元组末段",    "tupLast");
}

// ==================== Binary I/O ====================
namespace binary_io_ns {

Value readBinary_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string path(args[0].asString()->data, args[0].asString()->length);
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f.is_open()) return Value::nil();
    size_t sz = static_cast<size_t>(f.tellg());
    f.seekg(0);
    VMArray* arr = VMArray::create();
    arr->data.reserve(sz);
    // Read as array of ints (0-255)
    for (size_t i = 0; i < sz; i++) {
        unsigned char ch;
        f.read(reinterpret_cast<char*>(&ch), 1);
        arr->data.push_back(Value::Int(ch));
    }
    return makeArrayVal(arr);
}
Value writeBinary_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isArray()) return Value::Bool(false);
    std::string path(args[0].asString()->data, args[0].asString()->length);
    auto arr = args[1].asArray();
    std::ofstream f(path, std::ios::binary);
    if (!f.is_open()) return Value::Bool(false);
    for (auto& v : arr->data) {
        unsigned char ch = static_cast<unsigned char>(v.isInt() ? v.asInt() : 0);
        f.write(reinterpret_cast<const char*>(&ch), 1);
    }
    return Value::Bool(true);
}

} // namespace binary_io_ns

void StdLib::registerBinaryIO(VM* vm) {
    using namespace binary_io_ns;
    registerFunction(vm, "readBinary",  readBinary_);
    registerFunction(vm, "writeBinary", writeBinary_);
    registerAlias(vm, "二进制读",       "readBinary");
    registerAlias(vm, "二进制写",       "writeBinary");
}

// ==================== call_once ====================
namespace call_once_ns {

Value callOnce_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::nil();
    // Use a VMTable as the flag: check _done key
    if (!args[0].isTable()) return Value::nil();
    VMTable* flag = args[0].asTable();
    Value done = flag->get(makeStringVal(VMString::create("_done")));
    if (done.isTrue()) return Value::nil();
    Value fn = args[1];
    std::vector<Value> cargs;
    // Pass remaining args to fn
    for (size_t i = 2; i < args.size(); i++) cargs.push_back(args[i]);
    Value result = VM::current()->callFunction(fn, cargs);
    flag->set(makeStringVal(VMString::create("_done")), Value::Bool(true));
    flag->set(makeStringVal(VMString::create("_result")), result);
    return result;
}
Value onceFlag_(std::vector<Value>&) {
    VMTable* t = VMTable::create();
    t->set(makeStringVal(VMString::create("_done")), Value::Bool(false));
    return makeTableVal(t);
}
Value onceDone_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::Bool(false);
    return args[0].asTable()->get(makeStringVal(VMString::create("_done")));
}

} // namespace call_once_ns

void StdLib::registerCallOnce(VM* vm) {
    using namespace call_once_ns;
    registerFunction(vm, "onceFlag", onceFlag_);
    registerFunction(vm, "callOnce", callOnce_);
    registerFunction(vm, "onceDone", onceDone_);
    registerAlias(vm, "一次性标志",   "onceFlag");
    registerAlias(vm, "调用一次",     "callOnce");
    registerAlias(vm, "已调用",       "onceDone");
}

} // namespace cplang
