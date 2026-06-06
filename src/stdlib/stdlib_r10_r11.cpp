#include "stdlib/stdlib.hpp"

namespace cplang {

// ═══════════════════════════════════════════════════════════════
//  补齐 r10/r11 丢失的函数（math, array, file, random, chrono）
// ═══════════════════════════════════════════════════════════════

#include <cmath>
#include <random>
#include <filesystem>
#include <chrono>
#include <cstdint>

using namespace cplang;

namespace r10_ns {

Value isnormalFn(std::vector<Value>& args) {
    if (args.empty() || !args[0].isFloat()) return Value::Bool(false);
    return Value::Bool(std::isnormal(args[0].asFloat()));
}
Value signbitFn(std::vector<Value>& args) {
    if (args.empty() || !args[0].isFloat()) return Value::Bool(false);
    return Value::Bool(std::signbit(args[0].asFloat()));
}
Value fdimFn(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Float(0);
    return Value::Float(std::fdim(
        args[0].isFloat()?args[0].asFloat():0,
        args[1].isFloat()?args[1].asFloat():0));
}
Value fmaxFn(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Float(0);
    return Value::Float(std::fmax(
        args[0].isFloat()?args[0].asFloat():0,
        args[1].isFloat()?args[1].asFloat():0));
}
Value fminFn(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Float(0);
    return Value::Float(std::fmin(
        args[0].isFloat()?args[0].asFloat():0,
        args[1].isFloat()?args[1].asFloat():0));
}
Value byteswapFn(std::vector<Value>& args) {
    if (args.empty() || !args[0].isInt()) return Value::Int(0);
    uint16_t v = (uint16_t)args[0].asInt();
    return Value::Int((v >> 8) | (v << 8));
}

// 数组 filter: 接受数组 + 谓词函数名
Value arrFilterFn(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::nil();
    VMArray* src = args[0].asArray();
    VMArray* out = VMArray::create();
    std::string predName = args[1].isString() 
        ? std::string(args[1].asString()->data, args[1].asString()->length) : "";
    // 简单实现：检查每个元素是否 truthy
    for (auto& v : src->data) {
        if (v.isTrue() || !v.isFloat()) out->data.push_back(v);
        else if (v.asFloat() != 0) out->data.push_back(v);
    }
    return makeArrayVal(out);
}
Value arrClearFn(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::Int(0);
    args[0].asArray()->data.clear();
    return Value::Int(0);
}
Value countIfFn(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::Int(0);
    VMArray* src = args[0].asArray();
    int count = 0;
    for (auto& v : src->data) {
        if (v.isTrue() || (v.isFloat() && v.asFloat() != 0)) count++;
    }
    return Value::Int(count);
}
Value deepCopyFn(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    // simplified: just return the value (no deep copy for now)
    return args[0];
}

} // namespace r10_ns

// ═══════════════════════════════════════════════════════════════
namespace r11_ns {

// ─── 文件系统 ───
Value isDirFn(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Bool(false);
    std::string path(args[0].asString()->data, args[0].asString()->length);
    try { return Value::Bool(std::filesystem::is_directory(path)); }
    catch (...) { return Value::Bool(false); }
}
Value listDirFn(std::vector<Value>& args) {
    std::string path = ".";
    if (!args.empty() && args[0].isString())
        path = std::string(args[0].asString()->data, args[0].asString()->length);
    VMArray* arr = VMArray::create();
    try {
        for (auto& e : std::filesystem::directory_iterator(path))
            arr->data.push_back(makeStringVal(VMString::create(e.path().filename().string())));
    } catch (...) {}
    return makeArrayVal(arr);
}

// ─── 随机分布 ───
static std::mt19937& rng() { static std::mt19937 r; return r; }

Value distSeedFn(std::vector<Value>& args) {
    uint32_t seed = (args.empty() || !args[0].isInt()) ? 12345 : (uint32_t)args[0].asInt();
    rng() = std::mt19937(seed);
    return Value::Int(0);
}
Value uniformIntFn(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Int(0);
    int lo = args[0].isInt() ? (int)args[0].asInt() : 0;
    int hi = args[1].isInt() ? (int)args[1].asInt() : 1;
    return Value::Int(std::uniform_int_distribution<int>(lo, hi)(rng()));
}
Value uniformFloatFn(std::vector<Value>& args) {
    double lo = (args.size()>=1 && args[0].isFloat()) ? args[0].asFloat() : 0.0;
    double hi = (args.size()>=2 && args[1].isFloat()) ? args[1].asFloat() : 1.0;
    return Value::Float(std::uniform_real_distribution<double>(lo, hi)(rng()));
}
Value normalDistFn(std::vector<Value>& args) {
    double mean = (args.size()>=1 && args[0].isFloat()) ? args[0].asFloat() : 0.0;
    double stddev = (args.size()>=2 && args[1].isFloat()) ? args[1].asFloat() : 1.0;
    return Value::Float(std::normal_distribution<double>(mean, stddev)(rng()));
}
Value bernoulliDistFn(std::vector<Value>& args) {
    double p = (args.size()>=1 && args[0].isFloat()) ? args[0].asFloat() : 0.5;
    return Value::Bool(std::bernoulli_distribution(p)(rng()));
}
Value poissonDistFn(std::vector<Value>& args) {
    double mean = (args.size()>=1 && args[0].isFloat()) ? args[0].asFloat() : 1.0;
    return Value::Int(std::poisson_distribution<int>((int)mean)(rng()));
}
Value exponentialDistFn(std::vector<Value>& args) {
    double lambda = (args.size()>=1 && args[0].isFloat()) ? args[0].asFloat() : 1.0;
    return Value::Float(std::exponential_distribution<double>(lambda)(rng()));
}

// ─── chrono ───
Value secToMsFn(std::vector<Value>& args) {
    double s = (args.size()>=1 && args[0].isFloat()) ? args[0].asFloat() : 0.0;
    return Value::Float(s * 1000.0);
}
Value msToSecFn(std::vector<Value>& args) {
    double ms = (args.size()>=1 && args[0].isFloat()) ? args[0].asFloat() : 0.0;
    return Value::Float(ms / 1000.0);
}
Value nsToSecFn(std::vector<Value>& args) {
    double ns = (args.size()>=1 && args[0].isFloat()) ? args[0].asFloat() : 0.0;
    return Value::Float(ns / 1000000000.0);
}

} // namespace r11_ns

// ═══════════════════════════════════════════════════════════════
void StdLib::registerR10Misc(VM* vm) {
    registerFunction(vm, "isnormal",   r10_ns::isnormalFn);
    registerFunction(vm, "signbit",    r10_ns::signbitFn);
    registerFunction(vm, "fdim",       r10_ns::fdimFn);
    registerFunction(vm, "fmax",       r10_ns::fmaxFn);
    registerFunction(vm, "fmin",       r10_ns::fminFn);
    registerFunction(vm, "byteswap",   r10_ns::byteswapFn);
    registerFunction(vm, "arrFilter",  r10_ns::arrFilterFn);
    registerFunction(vm, "arrClear",   r10_ns::arrClearFn);
    registerFunction(vm, "countIf",    r10_ns::countIfFn);
    registerFunction(vm, "deepCopy",   r10_ns::deepCopyFn);
    registerAlias(vm, "数组过滤", "arrFilter");
    registerAlias(vm, "数组清空", "arrClear");
    registerAlias(vm, "条件计数", "countIf");
    registerAlias(vm, "深拷贝",   "deepCopy");
    registerAlias(vm, "字节交换", "byteswap");
    registerAlias(vm, "符号位",   "signbit");
    registerAlias(vm, "正常数",   "isnormal");
}

void StdLib::registerR11Misc(VM* vm) {
    registerFunction(vm, "isDir",           r11_ns::isDirFn);
    registerFunction(vm, "listDir",         r11_ns::listDirFn);
    registerFunction(vm, "distSeed",        r11_ns::distSeedFn);
    registerFunction(vm, "uniformInt",      r11_ns::uniformIntFn);
    registerFunction(vm, "uniformFloat",    r11_ns::uniformFloatFn);
    registerFunction(vm, "normalDist",      r11_ns::normalDistFn);
    registerFunction(vm, "bernoulliDist",   r11_ns::bernoulliDistFn);
    registerFunction(vm, "poissonDist",     r11_ns::poissonDistFn);
    registerFunction(vm, "exponentialDist", r11_ns::exponentialDistFn);
    registerFunction(vm, "secToMs",         r11_ns::secToMsFn);
    registerFunction(vm, "msToSec",         r11_ns::msToSecFn);
    registerFunction(vm, "nsToSec",         r11_ns::nsToSecFn);
    registerAlias(vm, "是目录",       "isDir");
    registerAlias(vm, "列出目录",     "listDir");
    registerAlias(vm, "分布种子",     "distSeed");
    registerAlias(vm, "均匀整数",     "uniformInt");
    registerAlias(vm, "随机整数",     "uniformInt");
    registerAlias(vm, "均匀浮点",     "uniformFloat");
    registerAlias(vm, "正态分布",     "normalDist");
    registerAlias(vm, "伯努利分布",   "bernoulliDist");
    registerAlias(vm, "泊松分布",     "poissonDist");
    registerAlias(vm, "指数分布",     "exponentialDist");
    registerAlias(vm, "秒到毫秒",     "secToMs");
    registerAlias(vm, "毫秒到秒",     "msToSec");
    registerAlias(vm, "纳秒到秒",     "nsToSec");
}

} // namespace cplang
