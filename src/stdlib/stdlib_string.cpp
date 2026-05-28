// String functions (len, substr, concat, find, replace, split, trim, lower, upper)
// #include'd from stdlib.cpp, already inside namespace cplang

void StdLib::registerString(VM* vm) {
    registerFunction(vm, "strlen", string::len);
    registerFunction(vm, "substr", string::substr);
    registerFunction(vm, "concat", string::concat);
    registerFunction(vm, "find", string::find);
    registerFunction(vm, "replace", string::replace);
    registerFunction(vm, "split", string::split);
    registerFunction(vm, "trim", string::trim);
    registerFunction(vm, "lower", string::lower);
    registerFunction(vm, "upper", string::upper);
    
    // 中文别名
    registerAlias(vm, "长度", "strlen");
    registerAlias(vm, "子串", "substr");
    registerAlias(vm, "连接", "concat");
    registerAlias(vm, "查找", "find");
    registerAlias(vm, "小写", "lower");
    registerAlias(vm, "大写", "upper");
    registerAlias(vm, "替换", "replace");
    registerAlias(vm, "分割", "split");
    registerAlias(vm, "修剪", "trim");
}

Value string::len(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Int(0);
    return Value::Int(args[0].asString()->length);
}

Value string::substr(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString()) return Value::nil();
    
    const char* str = args[0].asString()->data;
    size_t len = args[0].asString()->length;
    Int64 start = args[1].asInt();
    Int64 count = (args.size() > 2) ? args[2].asInt() : len;
    
    if (start < 0) start = len + start;
    if (start < 0) start = 0;
    if (start > static_cast<Int64>(len)) start = len;
    if (count < 0) count = 0;
    if (start + count > static_cast<Int64>(len)) count = len - start;
    
    return Value::String(VMString::create(str + start, static_cast<UInt32>(count)));
}

Value string::concat(std::vector<Value>& args) {
    std::string result;
    for (const auto& arg : args) {
        if (arg.isString()) {
            result.append(arg.asString()->data, arg.asString()->length);
        } else if (arg.isInt()) {
            result += std::to_string(arg.asInt());
        } else if (arg.isFloat()) {
            result += std::to_string(arg.asFloat());
        }
    }
    return Value::String(VMString::create(result));
}

Value string::find(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return Value::Int(-1);
    
    const char* str = args[0].asString()->data;
    const char* substr = args[1].asString()->data;
    
    const char* pos = std::strstr(str, substr);
    if (pos) {
        return Value::Int(static_cast<Int64>(pos - str));
    }
    return Value::Int(-1);
}

Value string::replace(std::vector<Value>& args) {
    if (args.size() < 3 || !args[0].isString() || !args[1].isString() || !args[2].isString()) {
        return args.empty() ? Value::nil() : args[0];
    }
    
    std::string str(args[0].asString()->data, args[0].asString()->length);
    std::string from(args[1].asString()->data, args[1].asString()->length);
    std::string to(args[2].asString()->data, args[2].asString()->length);
    
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos) {
        str.replace(pos, from.length(), to);
        pos += to.length();
    }
    
    return Value::String(VMString::create(str));
}

Value string::split(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isString()) {
        return Value::Array(VMArray::create());
    }
    
    std::string str(args[0].asString()->data, args[0].asString()->length);
    std::string delim(args[1].asString()->data, args[1].asString()->length);
    
    auto arr = VMArray::create();
    size_t pos = 0;
    while (true) {
        size_t found = str.find(delim, pos);
        if (found == std::string::npos) {
            arr->data.push_back(Value::String(VMString::create(str.substr(pos))));
            break;
        }
        arr->data.push_back(Value::String(VMString::create(str.substr(pos, found - pos))));
        pos = found + delim.length();
    }
    
    return Value::Array(arr);
}

Value string::trim(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    
    std::string str(args[0].asString()->data, args[0].asString()->length);
    
    // 去前导空白
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return Value::String(VMString::create(""));
    
    // 去尾随空白
    size_t end = str.find_last_not_of(" \t\n\r");
    
    return Value::String(VMString::create(str.substr(start, end - start + 1)));
}

Value string::lower(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    
    std::string str(args[0].asString()->data, args[0].asString()->length);
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    
    return Value::String(VMString::create(str));
}

Value string::upper(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    
    std::string str(args[0].asString()->data, args[0].asString()->length);
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    
    return Value::String(VMString::create(str));
}
