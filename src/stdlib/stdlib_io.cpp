#include "stdlib/stdlib.hpp"
#include "platform/platform.hpp"
#include <sys/stat.h>
#include <cmath>
#include <algorithm>
#include <cctype>
#include <chrono>

namespace cplang {

// IO functions (print, println, input, readFile, writeFile, exists)
// #include'd from stdlib.cpp, already inside namespace cplang

void StdLib::registerIO(VM* vm) {
    registerFunction(vm, "print", io::print);
    registerFunction(vm, "println", io::println);
    registerFunction(vm, "input", io::input);
    registerFunction(vm, "readFile", io::readFile);
    registerFunction(vm, "writeFile", io::writeFile);
    registerFunction(vm, "exists", io::exists);
    
    // 中文别名
    registerAlias(vm, "打印", "println");
    registerAlias(vm, "输入", "input");

    // 类型转换与反射函数（原在 stdlib_linux.cpp 中缺失）
    registerFunction(vm, "str", io::strFunc);
    registerFunction(vm, "toString", io::strFunc);
    registerAlias(vm, "串", "str");
    registerAlias(vm, "字符串", "str");
    
    registerFunction(vm, "type", io::typeFunc);
    registerAlias(vm, "类型", "type");
    
    registerFunction(vm, "len", io::lenFunc);
    registerAlias(vm, "长度", "len");
    
    registerFunction(vm, "abs", io::absFunc);
    
    // 基础数组/表操作函数
    registerFunction(vm, "arrNew", io::arrNewFunc);
    registerFunction(vm, "arrPush", io::arrPushFunc);
    registerAlias(vm, "追加", "arrPush");
    registerFunction(vm, "arrLen", io::arrLenFunc);
    registerFunction(vm, "arrGet", io::arrGetFunc);
    registerFunction(vm, "strConcat", io::strConcatFunc);
    registerAlias(vm, "字符串拼接", "strConcat");
    
    registerFunction(vm, "table", io::tableFunc);
    registerFunction(vm, "tableSet", io::tableSetFunc);
    registerFunction(vm, "tableGet", io::tableGetFunc);
    registerFunction(vm, "tableHas", io::tableHasFunc);
    registerFunction(vm, "tableKeys", io::tableKeysFunc);
    registerFunction(vm, "tableRemove", io::tableRemoveFunc);
    registerAlias(vm, "表创建", "table");
    registerAlias(vm, "表设", "tableSet");
    registerAlias(vm, "表取", "tableGet");
    registerAlias(vm, "表含", "tableHas");
    registerAlias(vm, "表键", "tableKeys");

    // 文件相关补充
    registerFunction(vm, "fileExists", io::fileExistsFunc);
    registerFunction(vm, "isDir", io::isDirFunc);
    registerFunction(vm, "fileSizeBytes", io::fileSizeBytesFunc);
    registerFunction(vm, "fileModified", io::fileModifiedFunc);
    registerAlias(vm, "文件存在", "fileExists");
    registerAlias(vm, "是目录", "isDir");
    registerFunction(vm, "rleCompress", io::rleCompressFunc);
    registerFunction(vm, "rleDecompress", io::rleDecompressFunc);
    
    // 文件监视（Linux 兼容桩）
    registerFunction(vm, "fileWatchCreate", io::fileWatchCreateFunc);
    registerFunction(vm, "fileWatchPoll", io::fileWatchPollFunc);
    registerFunction(vm, "fileWatchClose", io::fileWatchCloseFunc);
}

Value io::print(std::vector<Value>& args) {
    for (size_t i = 0; i < args.size(); i++) {
        if (i > 0) std::cout << " ";
        
        if (args[i].isString()) {
            std::cout.write(args[i].asString()->data, args[i].asString()->length);
        } else if (args[i].isInt()) {
            std::cout << args[i].asInt();
        } else if (args[i].isFloat()) {
            std::cout << args[i].asFloat();
        } else if (args[i].isBool()) {
            std::cout << (args[i].asInt() ? "true" : "false");
        } else if (args[i].isNil()) {
            std::cout << "nil";
        } else if (args[i].isArray()) {
            std::cout << "[array]";
        } else {
            std::cout << "[object]";
        }
    }
    return Value::nil();
}

Value io::println(std::vector<Value>& args) {
    io::print(args);
    std::cout << std::endl;
    return Value::nil();
}

Value io::input(std::vector<Value>& args) {
    if (!args.empty()) {
        io::print(args);
    }
    
    std::string line;
    std::getline(std::cin, line);
    return Value::String(VMString::create(line));
}

Value io::readFile(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    
    std::string filename(args[0].asString()->data, args[0].asString()->length);
    std::ifstream file(filename);
    
    if (!file.is_open()) return Value::nil();
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    return Value::String(VMString::create(buffer.str()));
}

Value io::writeFile(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isString()) {
        return Value::Bool(false);
    }
    
    std::string filename(args[0].asString()->data, args[0].asString()->length);
    std::ofstream file(filename);
    
    if (!file.is_open()) return Value::Bool(false);
    
    file.write(args[1].asString()->data, args[1].asString()->length);
    file.close();
    
    return Value::Bool(true);
}

Value io::exists(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Bool(false);
    
    std::string filename(args[0].asString()->data, args[0].asString()->length);
    return Value::Bool(std::ifstream(filename).good());
}

// ═══════════════════════════════════════════════════════════════════
//  新增辅助函数（移植自 stdlib_linux.cpp）
// ═══════════════════════════════════════════════════════════════════

static std::string io_getStr(const Value& v) {
    if (v.isString()) return std::string(v.asString()->data, v.asString()->length);
    if (v.isInt()) return std::to_string(v.asInt());
    if (v.isFloat()) return std::to_string(v.asFloat());
    if (v.isBool()) return v.asBool() ? "true" : "false";
    if (v.isNil()) return "nil";
    if (v.isArray()) return "[array]";
    if (v.isTable()) return "[table]";
    return "[object]";
}

Value io::strFunc(std::vector<Value>& args) {
    if (args.empty()) return Value::String(VMString::create("nil"));
    return Value::String(VMString::create(io_getStr(args[0])));
}

Value io::typeFunc(std::vector<Value>& args) {
    if (args.empty()) return Value::String(VMString::create("nil"));
    auto& v = args[0];
    if (v.isNil())    return Value::String(VMString::create("nil"));
    if (v.isBool())   return Value::String(VMString::create("bool"));
    if (v.isInt())    return Value::String(VMString::create("int"));
    if (v.isFloat())  return Value::String(VMString::create("float"));
    if (v.isString()) return Value::String(VMString::create("string"));
    if (v.isArray())  return Value::String(VMString::create("array"));
    if (v.isTable())  return Value::String(VMString::create("table"));
    if (v.isFunction() || v.isCFunction()) return Value::String(VMString::create("function"));
    return Value::String(VMString::create("object"));
}

Value io::lenFunc(std::vector<Value>& args) {
    if (args.empty()) return Value::Int(0);
    auto& v = args[0];
    if (v.isString()) return Value::Int((Int64)v.asString()->length);
    if (v.isArray())  return Value::Int((Int64)v.asArray()->data.size());
    if (v.isTable())  return Value::Int((Int64)v.asTable()->size());
    if (v.isInt()) {
        Int64 n = v.asInt();
        if (n < 0) n = -n;
        int c = 0; do { c++; n /= 10; } while (n);
        return Value::Int(c);
    }
    return Value::Int(0);
}

Value io::absFunc(std::vector<Value>& args) {
    if (args.empty()) return Value::Int(0);
    auto& v = args[0];
    if (v.isInt()) return Value::Int(v.asInt() < 0 ? -v.asInt() : v.asInt());
    if (v.isFloat()) return Value::Float(v.asFloat() < 0 ? -v.asFloat() : v.asFloat());
    return Value::Int(0);
}

Value io::arrNewFunc(std::vector<Value>& args) {
    VMArray* arr = VMArray::create(0);
    for (auto& a : args) arr->data.push_back(a);
    return Value::Array(arr);
}

Value io::arrPushFunc(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::nil();
    args[0].asArray()->data.push_back(args[1]);
    return args[0];
}

Value io::arrLenFunc(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::Int(0);
    return Value::Int((Int64)args[0].asArray()->data.size());
}

Value io::arrGetFunc(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray() || !args[1].isNumber()) return Value::nil();
    int idx = (int)args[1].asFloat();
    auto& data = args[0].asArray()->data;
    if (idx < 0 || idx >= (int)data.size()) return Value::nil();
    return data[idx];
}

Value io::strConcatFunc(std::vector<Value>& args) {
    std::string r;
    for (auto& a : args) r += io_getStr(a);
    return Value::String(VMString::create(r));
}

Value io::tableFunc(std::vector<Value>& args) {
    VMTable* t = VMTable::create();
    return Value::Table(t);
}

Value io::tableSetFunc(std::vector<Value>& args) {
    if (args.size() < 3 || !args[0].isTable()) return Value::nil();
    args[0].asTable()->set(args[1], args[2]);
    return args[0];
}

Value io::tableGetFunc(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isTable()) return Value::nil();
    return args[0].asTable()->get(args[1]);
}

Value io::tableHasFunc(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isTable()) return Value::Bool(false);
    return Value::Bool(args[0].asTable()->has(args[1]));
}

Value io::tableKeysFunc(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::nil();
    VMArray* arr = VMArray::create(0);
    for (auto& p : args[0].asTable()->data) arr->data.push_back(p.first);
    return Value::Array(arr);
}

Value io::tableRemoveFunc(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isTable()) return Value::nil();
    auto& data = args[0].asTable()->data;
    for (size_t i = 0; i < data.size(); i++) {
        if (data[i].first.equals(args[1])) {
            data.erase(data.begin() + i);
            break;
        }
    }
    return args[0];
}

Value io::fileExistsFunc(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Bool(false);
    std::string path(args[0].asString()->data, args[0].asString()->length);
    return Value::Bool(std::ifstream(path).good());
}

Value io::isDirFunc(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Bool(false);
    std::string path(args[0].asString()->data, args[0].asString()->length);
    return Value::Bool(platform::file_is_dir(path.c_str()));
}

Value io::fileSizeBytesFunc(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Int(0);
    std::string path(args[0].asString()->data, args[0].asString()->length);
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return Value::Int(0);
    return Value::Int((Int64)st.st_size);
}

Value io::fileModifiedFunc(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Int(0);
    std::string path(args[0].asString()->data, args[0].asString()->length);
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return Value::Int(0);
    return Value::Int((Int64)st.st_mtime);
}

Value io::rleCompressFunc(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::String(VMString::create(""));
    std::string s(args[0].asString()->data, args[0].asString()->length);
    if (s.empty()) return Value::String(VMString::create(""));
    std::string r;
    char cur = s[0]; int cnt = 1;
    for (size_t i = 1; i < s.size(); i++) {
        if (s[i] == cur) { cnt++; }
        else { r += cur; r += (char)('0' + cnt); cur = s[i]; cnt = 1; }
    }
    r += cur; r += (char)('0' + cnt);
    return Value::String(VMString::create(r));
}

Value io::rleDecompressFunc(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::String(VMString::create(""));
    std::string s(args[0].asString()->data, args[0].asString()->length);
    std::string r;
    for (size_t i = 0; i + 1 < s.size(); i += 2) {
        char c = s[i]; int n = s[i+1] - '0';
        for (int j = 0; j < n; j++) r += c;
    }
    return Value::String(VMString::create(r));
}

Value io::fileWatchCreateFunc(std::vector<Value>& args) {
    return Value::Int(0);
}

Value io::fileWatchPollFunc(std::vector<Value>& args) {
    return Value::Array(VMArray::create(0));
}

Value io::fileWatchCloseFunc(std::vector<Value>& args) {
    return Value::nil();
}

} // namespace cplang