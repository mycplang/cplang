#include "stdlib/stdlib.hpp"
#include <regex>

namespace cplang {

// stdlib_regex — extracted from stdlib_regex_crypto_string.cpp
// Regex, String Ext, Crypto, Encoding, String More functions
// #include'd from stdlib.cpp, already inside namespace cplang

void StdLib::registerRegex(VM* vm) {
    registerFunction(vm, "regexMatch", regex_::match);
    registerFunction(vm, "regexSearch", regex_::search);
    registerFunction(vm, "regexReplace", regex_::replace);
    registerFunction(vm, "regexSplit", regex_::split);
    registerFunction(vm, "regexFindAll", regex_::findAll);

    registerAlias(vm, "正则匹配", "regexMatch");
    registerAlias(vm, "正则搜索", "regexSearch");
    registerAlias(vm, "正则替换", "regexReplace");
    registerAlias(vm, "正则分割", "regexSplit");
    registerAlias(vm, "正则全找", "regexFindAll");
}

namespace regex_ {
Value match(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return Value::Bool(false);
    try {
        std::string str(args[0].asString()->data, args[0].asString()->length);
        std::string pat(args[1].asString()->data, args[1].asString()->length);
        std::regex re(pat);
        return Value::Bool(std::regex_match(str, re));
    } catch (...) {
        return Value::Bool(false);
    }
}

Value search(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return Value::Bool(false);
    try {
        std::string str(args[0].asString()->data, args[0].asString()->length);
        std::string pat(args[1].asString()->data, args[1].asString()->length);
        std::regex re(pat);
        return Value::Bool(std::regex_search(str, re));
    } catch (...) {
        return Value::Bool(false);
    }
}

Value replace(std::vector<Value>& args) {
    if (args.size() < 3 || !args[0].isString() || !args[1].isString() || !args[2].isString()) return args.empty() ? Value::nil() : args[0];
    try {
        std::string str(args[0].asString()->data, args[0].asString()->length);
        std::string pat(args[1].asString()->data, args[1].asString()->length);
        std::string repl(args[2].asString()->data, args[2].asString()->length);
        std::regex re(pat);
        std::string result = std::regex_replace(str, re, repl);
        return Value::String(VMString::create(result));
    } catch (...) {
        return args[0];
    }
}

Value split(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isString()) {
        VMArray* arr = VMArray::create();
        if (args.size() >= 1 && args[0].isString()) {
            arr->data.push_back(args[0]);
        }
        return Value::Array(arr);
    }
    try {
        std::string str(args[0].asString()->data, args[0].asString()->length);
        std::string pat(args[1].asString()->data, args[1].asString()->length);
        std::regex re(pat);
        std::sregex_token_iterator it(str.begin(), str.end(), re, -1);
        std::sregex_token_iterator end;
        VMArray* arr = VMArray::create();
        while (it != end) {
            arr->data.push_back(Value::String(VMString::create(*it)));
            ++it;
        }
        return Value::Array(arr);
    } catch (...) {
        VMArray* arr = VMArray::create();
        arr->data.push_back(args[0]);
        return Value::Array(arr);
    }
}

Value findAll(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isString()) {
        return Value::Array(VMArray::create());
    }
    try {
        std::string str(args[0].asString()->data, args[0].asString()->length);
        std::string pat(args[1].asString()->data, args[1].asString()->length);
        std::regex re(pat);
        std::sregex_iterator it(str.begin(), str.end(), re);
        std::sregex_iterator end;
        VMArray* arr = VMArray::create();
        while (it != end) {
            arr->data.push_back(Value::String(VMString::create(it->str())));
            ++it;
        }
        return Value::Array(arr);
    } catch (...) {
        return Value::Array(VMArray::create());
    }
}
} // namespace regex_

// ═══════════════════════════════════════════════════════════════════
//  字符串扩展实现
// ═══════════════════════════════════════════════════════════════════

} // namespace cplang
