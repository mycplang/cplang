// stdlib_string_ext — extracted from stdlib_regex_crypto_string.cpp
void StdLib::registerStringExt(VM* vm) {
    registerFunction(vm, "strFormat", str_ext::format);
    registerFunction(vm, "parseInt", str_ext::parseInt);
    registerFunction(vm, "parseFloat", str_ext::parseFloat);
    registerFunction(vm, "toHex", str_ext::toHex);
    registerFunction(vm, "toOct", str_ext::toOct);
    registerFunction(vm, "toBin", str_ext::toBin);

    registerAlias(vm, "格式化", "strFormat");
    registerAlias(vm, "转整数", "parseInt");
    registerAlias(vm, "解析整数", "parseInt");
    registerAlias(vm, "解析浮点", "parseFloat");
    registerAlias(vm, "转十六进制", "toHex");
    registerAlias(vm, "转八进制", "toOct");
    registerAlias(vm, "转二进制", "toBin");
}

namespace str_ext {
Value format(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string fmt(args[0].asString()->data, args[0].asString()->length);
    std::string result;
    size_t argIdx = 1;
    for (size_t i = 0; i < fmt.size(); i++) {
        if (fmt[i] == '{' && i + 1 < fmt.size() && fmt[i + 1] == '}') {
            if (argIdx < args.size()) {
                if (args[argIdx].isInt()) result += std::to_string(args[argIdx].asInt());
                else if (args[argIdx].isFloat()) result += std::to_string(args[argIdx].asFloat());
                else if (args[argIdx].isString()) result.append(args[argIdx].asString()->data, args[argIdx].asString()->length);
                else if (args[argIdx].isBool()) result += args[argIdx].isTrue() ? "true" : "false";
                else result += "nil";
                argIdx++;
            }
            i++;
        } else {
            result += fmt[i];
        }
    }
    return Value::String(VMString::create(result));
}

Value parseInt(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Int(0);
    std::string str(args[0].asString()->data, args[0].asString()->length);
    int base = 10;
    if (args.size() > 1 && args[1].isInt()) base = static_cast<int>(args[1].asInt());
    try {
        size_t pos = 0;
        Int64 val = std::stoll(str, &pos, base);
        return Value::Int(val);
    } catch (...) {
        return Value::Int(0);
    }
}

Value parseFloat(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Float(0.0);
    std::string str(args[0].asString()->data, args[0].asString()->length);
    try {
        size_t pos = 0;
        double val = std::stod(str, &pos);
        return Value::Float(val);
    } catch (...) {
        return Value::Float(0.0);
    }
}

Value toHex(std::vector<Value>& args) {
    if (args.empty()) return Value::String(VMString::create("0"));
    Int64 n = args[0].isInt() ? args[0].asInt() : static_cast<Int64>(args[0].asFloat());
    std::stringstream ss;
    ss << std::hex << n;
    return Value::String(VMString::create(ss.str()));
}

Value toOct(std::vector<Value>& args) {
    if (args.empty()) return Value::String(VMString::create("0"));
    Int64 n = args[0].isInt() ? args[0].asInt() : static_cast<Int64>(args[0].asFloat());
    std::stringstream ss;
    ss << std::oct << n;
    return Value::String(VMString::create(ss.str()));
}

Value toBin(std::vector<Value>& args) {
    if (args.empty()) return Value::String(VMString::create("0"));
    Int64 n = args[0].isInt() ? args[0].asInt() : static_cast<Int64>(args[0].asFloat());
    if (n == 0) return Value::String(VMString::create("0"));
    std::string result;
    UInt64 v = static_cast<UInt64>(n);
    while (v > 0) {
        result = ((v & 1) ? "1" : "0") + result;
        v >>= 1;
    }
    return Value::String(VMString::create(result));
}
} // namespace str_ext

// ═══════════════════════════════════════════════════════════════════
//  加密哈希实现
// ═══════════════════════════════════════════════════════════════════

