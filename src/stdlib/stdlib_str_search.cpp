// String search utility functions
// #include'd from stdlib.cpp, already inside namespace cplang

namespace str_search {
    Value findFirstOf(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return Value::Int(-1);
        std::string s(args[0].asString()->data, args[0].asString()->length);
        std::string chars(args[1].asString()->data, args[1].asString()->length);
        size_t pos = args.size() >= 3 ? (size_t)args[2].asInt() : 0;
        size_t found = s.find_first_of(chars, pos);
        return found == std::string::npos ? Value::Int(-1) : Value::Int((int)found);
    }
    Value findLastOf(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return Value::Int(-1);
        std::string s(args[0].asString()->data, args[0].asString()->length);
        std::string chars(args[1].asString()->data, args[1].asString()->length);
        size_t pos = args.size() >= 3 ? (size_t)args[2].asInt() : std::string::npos;
        size_t found = s.find_last_of(chars, pos);
        return found == std::string::npos ? Value::Int(-1) : Value::Int((int)found);
    }
    Value findFirstNotOf(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return Value::Int(-1);
        std::string s(args[0].asString()->data, args[0].asString()->length);
        std::string chars(args[1].asString()->data, args[1].asString()->length);
        size_t pos = args.size() >= 3 ? (size_t)args[2].asInt() : 0;
        size_t found = s.find_first_not_of(chars, pos);
        return found == std::string::npos ? Value::Int(-1) : Value::Int((int)found);
    }
    Value findLastNotOf(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return Value::Int(-1);
        std::string s(args[0].asString()->data, args[0].asString()->length);
        std::string chars(args[1].asString()->data, args[1].asString()->length);
        size_t pos = args.size() >= 3 ? (size_t)args[2].asInt() : std::string::npos;
        size_t found = s.find_last_not_of(chars, pos);
        return found == std::string::npos ? Value::Int(-1) : Value::Int((int)found);
    }
    Value strCompare(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return Value::Int(0);
        std::string a(args[0].asString()->data, args[0].asString()->length);
        std::string b(args[1].asString()->data, args[1].asString()->length);
        int cmp = a.compare(b);
        return Value::Int(cmp < 0 ? -1 : (cmp > 0 ? 1 : 0));
    }
}

void StdLib::registerStringSearch(VM* vm) {
    registerFunction(vm, "findFirstOf",    str_search::findFirstOf);
    registerFunction(vm, "findLastOf",     str_search::findLastOf);
    registerFunction(vm, "findFirstNotOf", str_search::findFirstNotOf);
    registerFunction(vm, "findLastNotOf",  str_search::findLastNotOf);
    registerFunction(vm, "strCompare",     str_search::strCompare);
    registerAlias(vm, "首字查找",      "findFirstOf");
    registerAlias(vm, "末字查找",      "findLastOf");
    registerAlias(vm, "首非字查找",    "findFirstNotOf");
    registerAlias(vm, "末非字查找",    "findLastNotOf");
    registerAlias(vm, "比较字符串",    "strCompare");
}
