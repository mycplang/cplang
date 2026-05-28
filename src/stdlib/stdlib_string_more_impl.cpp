// stdlib_string_more_impl — extracted from stdlib_regex_crypto_string.cpp
void StdLib::registerStringMore(VM* vm) {
    registerFunction(vm, "strTrim", str_more::strTrim);
    registerFunction(vm, "strTrimLeft", str_more::strTrimLeft);
    registerFunction(vm, "strTrimRight", str_more::strTrimRight);
    registerFunction(vm, "strPadLeft", str_more::strPadLeft);
    registerFunction(vm, "strPadRight", str_more::strPadRight);
    registerFunction(vm, "strSplit", str_more::strSplit);
    registerFunction(vm, "strJoin", str_more::strJoin);
    registerFunction(vm, "strStartsWith", str_more::strStartsWith);
    registerFunction(vm, "strEndsWith", str_more::strEndsWith);
    registerFunction(vm, "strContains", str_more::strContains);
    registerFunction(vm, "strReverse", str_more::strReverse);
    registerFunction(vm, "strReplace", str_more::strReplace);
    registerFunction(vm, "strCount", str_more::strCount);
    registerFunction(vm, "strIndexOf", str_more::strIndexOf);
    registerFunction(vm, "strLastIndexOf", str_more::strLastIndexOf);
    registerFunction(vm, "strSlice", str_more::strSlice);
    registerFunction(vm, "strEscape", str_more::strEscape);
    registerFunction(vm, "strUnescape", str_more::strUnescape);
    registerFunction(vm, "strPadCenter", str_more::strPadCenter);

    registerAlias(vm, "去空白", "strTrim");
    registerAlias(vm, "去左空白", "strTrimLeft");
    registerAlias(vm, "去右空白", "strTrimRight");
    registerAlias(vm, "左填充", "strPadLeft");
    registerAlias(vm, "右填充", "strPadRight");
    registerAlias(vm, "拆分", "strSplit");
    registerAlias(vm, "连接", "strJoin");
    registerAlias(vm, "以开头", "strStartsWith");
    registerAlias(vm, "以结尾", "strEndsWith");
    registerAlias(vm, "包含", "strContains");
    registerAlias(vm, "反转", "strReverse");
    registerAlias(vm, "替换", "strReplace");
    registerAlias(vm, "计数", "strCount");
    registerAlias(vm, "查找", "strIndexOf");
    registerAlias(vm, "倒查找", "strLastIndexOf");
    registerAlias(vm, "截取", "strSlice");
    registerAlias(vm, "转义", "strEscape");
    registerAlias(vm, "反转义", "strUnescape");
    registerAlias(vm, "居中填充", "strPadCenter");
}

namespace str_more {

static std::string getStr(const Value& v) {
    if (!v.isString()) return "";
    return std::string(v.asString()->data, v.asString()->length);
}

Value strTrim(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string s = getStr(args[0]);
    size_t start = 0, end = s.size();
    while (start < end && std::isspace(static_cast<unsigned char>(s[start]))) start++;
    while (end > start && std::isspace(static_cast<unsigned char>(s[end-1]))) end--;
    return Value::String(VMString::create(s.substr(start, end - start)));
}

Value strTrimLeft(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string s = getStr(args[0]);
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) start++;
    return Value::String(VMString::create(s.substr(start)));
}

Value strTrimRight(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string s = getStr(args[0]);
    size_t end = s.size();
    while (end > 0 && std::isspace(static_cast<unsigned char>(s[end-1]))) end--;
    return Value::String(VMString::create(s.substr(0, end)));
}

Value strPadLeft(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isNumber()) return Value::nil();
    std::string s = getStr(args[0]);
    size_t len = static_cast<size_t>(args[1].asFloat());
    char pad = (args.size() >= 3 && args[2].isString() && args[2].asString()->length > 0) ? args[2].asString()->data[0] : ' ';
    if (s.size() >= len) return args[0];
    return Value::String(VMString::create(std::string(len - s.size(), pad) + s));
}

Value strPadRight(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isNumber()) return Value::nil();
    std::string s = getStr(args[0]);
    size_t len = static_cast<size_t>(args[1].asFloat());
    char pad = (args.size() >= 3 && args[2].isString() && args[2].asString()->length > 0) ? args[2].asString()->data[0] : ' ';
    if (s.size() >= len) return args[0];
    return Value::String(VMString::create(s + std::string(len - s.size(), pad)));
}

Value strSplit(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return Value::nil();
    std::string s = getStr(args[0]);
    std::string delim = getStr(args[1]);
    VMArray* result = VMArray::create(0);
    if (delim.empty()) {
        result->data.push_back(args[0]);
        return Value::Array(result);
    }
    size_t start = 0, pos = 0;
    while ((pos = s.find(delim, start)) != std::string::npos) {
        result->data.push_back(Value::String(VMString::create(s.substr(start, pos - start))));
        start = pos + delim.size();
    }
    result->data.push_back(Value::String(VMString::create(s.substr(start))));
    return Value::Array(result);
}

// 辅助: Value 转字符串
static std::string valueToString(const Value& v) {
    if (v.isInt()) return std::to_string(v.asInt());
    if (v.isFloat()) return std::to_string(v.asFloat());
    if (v.isBool()) return v.i ? "true" : "false";
    if (v.isString()) return std::string(v.asString()->data, v.asString()->length);
    if (v.isNil()) return "nil";
    return "[object]";
}

Value strJoin(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray() || !args[1].isString()) return Value::nil();
    VMArray* arr = args[0].asArray();
    std::string delim = getStr(args[1]);
    std::string result;
    for (size_t i = 0; i < arr->data.size(); i++) {
        if (i > 0) result += delim;
        result += valueToString(arr->data[i]);
    }
    return Value::String(VMString::create(result));
}

Value strStartsWith(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return Value::Bool(false);
    std::string s = getStr(args[0]), prefix = getStr(args[1]);
    return Value::Bool(s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix);
}

Value strEndsWith(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return Value::Bool(false);
    std::string s = getStr(args[0]), suffix = getStr(args[1]);
    return Value::Bool(s.size() >= suffix.size() && s.substr(s.size() - suffix.size()) == suffix);
}

Value strContains(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return Value::Bool(false);
    std::string s = getStr(args[0]), sub = getStr(args[1]);
    return Value::Bool(s.find(sub) != std::string::npos);
}

Value strReverse(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string s = getStr(args[0]);
    std::reverse(s.begin(), s.end());
    return Value::String(VMString::create(s));
}

Value strReplace(std::vector<Value>& args) {
    if (args.size() < 3 || !args[0].isString() || !args[1].isString() || !args[2].isString()) return Value::nil();
    std::string s = getStr(args[0]), from = getStr(args[1]), to = getStr(args[2]);
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return Value::String(VMString::create(s));
}

Value strCount(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return Value::Float(0);
    std::string s = getStr(args[0]), sub = getStr(args[1]);
    if (sub.empty()) return Value::Float(0);
    size_t count = 0, pos = 0;
    while ((pos = s.find(sub, pos)) != std::string::npos) { count++; pos += sub.size(); }
    return Value::Float(static_cast<double>(count));
}

Value strIndexOf(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return Value::Float(-1);
    std::string s = getStr(args[0]), sub = getStr(args[1]);
    size_t pos = s.find(sub);
    return Value::Float(pos == std::string::npos ? -1.0 : static_cast<double>(pos));
}

Value strLastIndexOf(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return Value::Float(-1);
    std::string s = getStr(args[0]), sub = getStr(args[1]);
    size_t pos = s.rfind(sub);
    return Value::Float(pos == std::string::npos ? -1.0 : static_cast<double>(pos));
}

Value strSlice(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isNumber()) return Value::nil();
    std::string s = getStr(args[0]);
    int start = static_cast<int>(args[1].asFloat());
    int end = (args.size() >= 3 && args[2].isNumber()) ? static_cast<int>(args[2].asFloat()) : static_cast<int>(s.size());
    if (start < 0) start = static_cast<int>(s.size()) + start;
    if (end < 0) end = static_cast<int>(s.size()) + end;
    if (start < 0) start = 0;
    if (end > static_cast<int>(s.size())) end = static_cast<int>(s.size());
    if (start >= end) return Value::String(VMString::create(""));
    return Value::String(VMString::create(s.substr(start, end - start)));
}

Value strEscape(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string s = getStr(args[0]);
    std::string out;
    for (char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"':  out += "\\\""; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c; break;
        }
    }
    return Value::String(VMString::create(out));
}

Value strUnescape(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string s = getStr(args[0]);
    std::string out;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            switch (s[i + 1]) {
                case 'n':  out += '\n'; i++; break;
                case 'r':  out += '\r'; i++; break;
                case 't':  out += '\t'; i++; break;
                case '\\': out += '\\'; i++; break;
                case '"':  out += '"';  i++; break;
                default:   out += s[i]; break;
            }
        } else {
            out += s[i];
        }
    }
    return Value::String(VMString::create(out));
}

Value strPadCenter(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isNumber()) return Value::nil();
    std::string s = getStr(args[0]);
    int width = static_cast<int>(args[1].asFloat());
    if (width <= static_cast<int>(s.size())) return Value::String(VMString::create(s));
    char padChar = (args.size() >= 3 && args[2].isString()) ? getStr(args[2])[0] : ' ';
    int leftPad = (width - static_cast<int>(s.size())) / 2;
    int rightPad = width - static_cast<int>(s.size()) - leftPad;
    return Value::String(VMString::create(std::string(leftPad, padChar) + s + std::string(rightPad, padChar)));
}
} // namespace str_more

// ═══════════════════════════════════════════════════════════════════
//  数组增强实现
// ═══════════════════════════════════════════════════════════════════

