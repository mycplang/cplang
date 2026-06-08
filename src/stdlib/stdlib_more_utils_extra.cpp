// 从 stdlib_linux.cpp 移植的工具函数（Windows 编译补充）
#include "stdlib/stdlib.hpp"
#include <cmath>
#include <algorithm>
#include <cctype>
#include <chrono>

namespace cplang {
namespace more_utils {

static std::string mu_getStr(const Value& v) {
    if (v.isString()) return std::string(v.asString()->data, v.asString()->length);
    if (v.isInt()) return std::to_string(v.asInt());
    if (v.isFloat()) return std::to_string(v.asFloat());
    if (v.isBool()) return v.asBool() ? "true" : "false";
    if (v.isNil()) return "nil";
    return "";
}

static int mu_getInt(const Value& v) {
    if (v.isInt()) return (int)v.asInt();
    if (v.isFloat()) return (int)v.asFloat(); return 0;
}

static double mu_getNum(const Value& v) {
    if (v.isInt()) return (double)v.asInt();
    if (v.isFloat()) return v.asFloat(); return 0.0;
}

Value toSnakeCase(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::String(VMString::create(""));
    std::string s = mu_getStr(args[0]); std::string r;
    for (size_t i = 0; i < s.size(); i++) {
        if (isupper(s[i])) { if (i) r += '_'; r += (char)tolower(s[i]); }
        else r += s[i];
    }
    return Value::String(VMString::create(r));
}

Value toCamelCase(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::String(VMString::create(""));
    std::string s = mu_getStr(args[0]); std::string r; bool cap = false;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '_') { cap = true; }
        else { r += cap ? (char)toupper(s[i]) : s[i]; cap = false; }
    }
    return Value::String(VMString::create(r));
}

Value uniq(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::nil();
    auto& data = args[0].asArray()->data;
    VMArray* arr = VMArray::create(data.size());
    for (auto& v : data) { bool f = false; for (auto& e : arr->data) { if (e.equals(v)) { f = true; break; } } if (!f) arr->data.push_back(v); }
    return Value::Array(arr);
}

Value enumerate(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::nil();
    auto& data = args[0].asArray()->data;
    VMArray* arr = VMArray::create(data.size());
    for (size_t i = 0; i < data.size(); i++) {
        VMArray* p = VMArray::create(2); p->data.push_back(Value::Int((Int64)i)); p->data.push_back(data[i]);
        arr->data.push_back(Value::Array(p));
    }
    return Value::Array(arr);
}

Value arrSum(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::Float(0);
    double s = 0; for (auto& v : args[0].asArray()->data) s += mu_getNum(v);
    return Value::Float(s);
}

Value arrAvg(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::Float(0);
    auto& d = args[0].asArray()->data; if (d.empty()) return Value::Float(0);
    double s = 0; for (auto& v : d) s += mu_getNum(v);
    return Value::Float(s / d.size());
}

Value arrTake(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::nil();
    int n = (int)args[1].asFloat(); auto& d = args[0].asArray()->data;
    VMArray* a = VMArray::create(0); int c = n < 0 ? 0 : (n > (int)d.size() ? (int)d.size() : n);
    for (int i = 0; i < c; i++) a->data.push_back(d[i]);
    return Value::Array(a);
}

Value arrDrop(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::nil();
    int n = (int)args[1].asFloat(); auto& d = args[0].asArray()->data;
    if (n >= (int)d.size()) return Value::Array(VMArray::create(0));
    if (n < 0) n = 0;
    VMArray* a = VMArray::create(0);
    for (size_t i = (size_t)n; i < d.size(); i++) a->data.push_back(d[i]);
    return Value::Array(a);
}

Value intPow(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Int(0);
    return Value::Float(std::pow(mu_getNum(args[0]), mu_getNum(args[1])));
}

Value roundTo(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Float(0);
    double m = std::pow(10.0, mu_getInt(args[1]));
    return Value::Float(std::round(mu_getNum(args[0]) * m) / m);
}

Value merge(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    if (args[0].isArray()) {
        VMArray* a = VMArray::create(0);
        for (auto& x : args) { if (x.isArray()) for (auto& v : x.asArray()->data) a->data.push_back(v); else a->data.push_back(x); }
        return Value::Array(a);
    }
    if (args[0].isTable()) {
        VMTable* t = VMTable::create();
        for (auto& x : args) if (x.isTable()) for (auto& p : x.asTable()->data) t->set(p.first, p.second);
        return Value::Table(t);
    }
    return Value::nil();
}

Value getOrDefault(std::vector<Value>& args) {
    if (args.size() < 2) return Value::nil();
    if (args[0].isTable()) { if (args[0].asTable()->has(args[1])) return args[0].asTable()->get(args[1]); return args.size() >= 3 ? args[2] : Value::nil(); }
    if (args[0].isArray()) {
        int idx = mu_getInt(args[1]); auto& d = args[0].asArray()->data;
        if (idx >= 0 && idx < (int)d.size()) return d[idx];
    }
    return args.size() >= 3 ? args[2] : Value::nil();
}

Value swap(std::vector<Value>& args) {
    if (args.size() < 3 || !args[0].isArray()) return Value::nil();
    auto& d = args[0].asArray()->data; int i = mu_getInt(args[1]), j = mu_getInt(args[2]);
    if (i >= 0 && i < (int)d.size() && j >= 0 && j < (int)d.size()) std::swap(d[i], d[j]);
    return args[0];
}

Value contains(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Bool(false);
    if (args[0].isArray()) for (auto& v : args[0].asArray()->data) { if (v.equals(args[1])) return Value::Bool(true); }
    if (args[0].isString()) return Value::Bool(mu_getStr(args[0]).find(mu_getStr(args[1])) != std::string::npos);
    return Value::Bool(false);
}

Value intersection(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray() || !args[1].isArray()) return Value::nil();
    auto& a = args[0].asArray()->data; auto& b = args[1].asArray()->data;
    VMArray* r = VMArray::create(0);
    for (auto& va : a) for (auto& vb : b) { if (va.equals(vb)) { bool f = false; for (auto& e : r->data) { if (e.equals(va)) { f = true; break; } } if (!f) r->data.push_back(va); break; } }
    return Value::Array(r);
}

Value difference(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray() || !args[1].isArray()) return Value::nil();
    auto& a = args[0].asArray()->data; auto& b = args[1].asArray()->data;
    VMArray* r = VMArray::create(0);
    for (auto& va : a) { bool f = false; for (auto& vb : b) { if (va.equals(vb)) { f = true; break; } } if (!f) r->data.push_back(va); }
    return Value::Array(r);
}

Value strCount(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return Value::Int(0);
    std::string s = mu_getStr(args[0]), sub = mu_getStr(args[1]);
    if (sub.empty()) return Value::Int(0); int c = 0; size_t p = 0;
    while ((p = s.find(sub, p)) != std::string::npos) { c++; p += sub.size(); }
    return Value::Int(c);
}

Value strCompareIC(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return Value::Int(0);
    std::string a = mu_getStr(args[0]), b = mu_getStr(args[1]);
    std::transform(a.begin(), a.end(), a.begin(), ::tolower);
    std::transform(b.begin(), b.end(), b.begin(), ::tolower);
    int c = a.compare(b); return Value::Int(c < 0 ? -1 : c > 0 ? 1 : 0);
}

Value strIsBlank(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Bool(true);
    for (char c : mu_getStr(args[0])) { if (!isspace((unsigned char)c)) return Value::Bool(false); }
    return Value::Bool(true);
}

Value timestamp(std::vector<Value>& args) {
    using namespace std::chrono;
    return Value::Int((Int64)duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

Value clock(std::vector<Value>& args) {
    using namespace std::chrono;
    return Value::Int((Int64)duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

Value accumulate(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::Float(0);
    double s = mu_getNum(args[1]); for (auto& v : args[0].asArray()->data) s += mu_getNum(v);
    return Value::Float(s);
}

Value product(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::Float(0);
    double r = 1; for (auto& v : args[0].asArray()->data) r *= mu_getNum(v);
    return Value::Float(r);
}

static bool isTruthy(const Value& v) {
    if (v.isBool()) return v.asBool();
    if (v.isInt()) return v.asInt() != 0;
    if (v.isFloat()) return v.asFloat() != 0;
    if (v.isString()) return v.asString()->length > 0;
    if (v.isNil()) return false;
    return true;
}

Value anyOf(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::Bool(false);
    for (auto& v : args[0].asArray()->data) { if (isTruthy(v)) return Value::Bool(true); }
    return Value::Bool(false);
}

Value allOf(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::Bool(true);
    for (auto& v : args[0].asArray()->data) { if (!isTruthy(v)) return Value::Bool(false); }
    return Value::Bool(true);
}

Value noneOf(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::Bool(true);
    for (auto& v : args[0].asArray()->data) { if (isTruthy(v)) return Value::Bool(false); }
    return Value::Bool(true);
}

Value erfFunc(std::vector<Value>& args) {
    if (args.empty()) return Value::Float(0);
    return Value::Float(std::erf(mu_getNum(args[0])));
}

Value tgammaFunc(std::vector<Value>& args) {
    if (args.empty()) return Value::Float(0);
    return Value::Float(std::tgamma(mu_getNum(args[0])));
}

} // namespace more_utils
} // namespace cplang
