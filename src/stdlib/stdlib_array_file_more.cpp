#include "stdlib/stdlib.hpp"

namespace cplang {

// Array and File extension functions
// #include'd from stdlib.cpp, already inside namespace cplang

void StdLib::registerArrayMore(VM* vm) {
    registerFunction(vm, "arrReverse", arr_more::arrReverse);
    registerFunction(vm, "arrRotate", arr_more::arrRotate);
    registerFunction(vm, "arrFill", arr_more::arrFill);
    registerFunction(vm, "arrSlice", arr_more::arrSlice);
    registerFunction(vm, "arrSplice", arr_more::arrSplice);
    registerFunction(vm, "arrFind", arr_more::arrFind);
    registerFunction(vm, "arrFindIndex", arr_more::arrFindIndex);
    registerFunction(vm, "arrUnique", arr_more::arrUnique);
    registerFunction(vm, "arrFlatten", arr_more::arrFlatten);
    registerFunction(vm, "arrZip", arr_more::arrZip);
    registerFunction(vm, "arrChunk", arr_more::arrChunk);

    registerAlias(vm, "数组反转", "arrReverse");
    registerAlias(vm, "数组旋转", "arrRotate");
    registerAlias(vm, "数组填充", "arrFill");
    registerAlias(vm, "数组切片", "arrSlice");
    registerAlias(vm, "数组拼接", "arrSplice");
    registerAlias(vm, "数组查找", "arrFind");
    registerAlias(vm, "数组查找索引", "arrFindIndex");
    registerAlias(vm, "数组去重", "arrUnique");
    registerAlias(vm, "数组扁平化", "arrFlatten");
    registerAlias(vm, "数组拉链", "arrZip");
    registerAlias(vm, "数组分块", "arrChunk");
}

namespace arr_more {

Value arrReverse(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::nil();
    VMArray* arr = args[0].asArray();
    std::reverse(arr->data.begin(), arr->data.end());
    return args[0];
}

Value arrRotate(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray() || !args[1].isNumber()) return Value::nil();
    VMArray* arr = args[0].asArray();
    int k = static_cast<int>(args[1].asFloat()) % static_cast<int>(arr->data.size());
    if (k < 0) k += static_cast<int>(arr->data.size());
    if (k == 0 || arr->data.empty()) return args[0];
    std::rotate(arr->data.begin(), arr->data.begin() + k, arr->data.end());
    return args[0];
}

Value arrFill(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::nil();
    VMArray* arr = args[0].asArray();
    Value fillVal = args[1];
    size_t start = 0, end = arr->data.size();
    if (args.size() >= 3 && args[2].isNumber()) start = static_cast<size_t>(args[2].asFloat());
    if (args.size() >= 4 && args[3].isNumber()) end = static_cast<size_t>(args[3].asFloat());
    if (start > arr->data.size()) start = arr->data.size();
    if (end > arr->data.size()) end = arr->data.size();
    for (size_t i = start; i < end; i++) arr->data[i] = fillVal;
    return args[0];
}

Value arrSlice(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray() || !args[1].isNumber()) return Value::nil();
    VMArray* arr = args[0].asArray();
    int start = static_cast<int>(args[1].asFloat());
    int end = (args.size() >= 3 && args[2].isNumber()) ? static_cast<int>(args[2].asFloat()) : static_cast<int>(arr->data.size());
    if (start < 0) start = static_cast<int>(arr->data.size()) + start;
    if (end < 0) end = static_cast<int>(arr->data.size()) + end;
    if (start < 0) start = 0;
    if (end > static_cast<int>(arr->data.size())) end = static_cast<int>(arr->data.size());
    VMArray* result = VMArray::create(0);
    for (int i = start; i < end; i++) result->data.push_back(arr->data[i]);
    return Value::Array(result);
}

Value arrSplice(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray() || !args[1].isNumber()) return Value::nil();
    VMArray* arr = args[0].asArray();
    int start = static_cast<int>(args[1].asFloat());
    int deleteCount = (args.size() >= 3 && args[2].isNumber()) ? static_cast<int>(args[2].asFloat()) : static_cast<int>(arr->data.size());
    if (start < 0) start = static_cast<int>(arr->data.size()) + start;
    if (start < 0) start = 0;
    if (start > static_cast<int>(arr->data.size())) start = static_cast<int>(arr->data.size());
    if (deleteCount < 0) deleteCount = 0;
    if (start + deleteCount > static_cast<int>(arr->data.size())) deleteCount = static_cast<int>(arr->data.size()) - start;
    VMArray* removed = VMArray::create(0);
    for (int i = 0; i < deleteCount; i++) {
        removed->data.push_back(arr->data[start]);
        arr->data.erase(arr->data.begin() + start);
    }
    for (size_t i = 3; i < args.size(); i++) {
        arr->data.insert(arr->data.begin() + start + (i - 3), args[i]);
    }
    return Value::Array(removed);
}

Value arrFind(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::nil();
    VMArray* arr = args[0].asArray();
    for (size_t i = 0; i < arr->data.size(); i++) {
        if (arr->data[i].equals(args[1])) return arr->data[i];
    }
    return Value::nil();
}

Value arrFindIndex(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::Float(-1);
    VMArray* arr = args[0].asArray();
    for (size_t i = 0; i < arr->data.size(); i++) {
        if (arr->data[i].equals(args[1])) return Value::Float(static_cast<double>(i));
    }
    return Value::Float(-1);
}

Value arrUnique(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::nil();
    VMArray* arr = args[0].asArray();
    VMArray* result = VMArray::create(0);
    for (size_t i = 0; i < arr->data.size(); i++) {
        bool found = false;
        for (size_t j = 0; j < result->data.size(); j++) {
            if (arr->data[i].equals(result->data[j])) { found = true; break; }
        }
        if (!found) result->data.push_back(arr->data[i]);
    }
    return Value::Array(result);
}

Value arrFlatten(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::nil();
    VMArray* arr = args[0].asArray();
    int depth = (args.size() >= 2 && args[1].isNumber()) ? static_cast<int>(args[1].asFloat()) : 1;
    VMArray* result = VMArray::create(0);
    std::function<void(VMArray*, int)> flatten = [&](VMArray* a, int d) {
        for (size_t i = 0; i < a->data.size(); i++) {
            if (a->data[i].isArray() && d > 0) {
                flatten(a->data[i].asArray(), d - 1);
            } else {
                result->data.push_back(a->data[i]);
            }
        }
    };
    flatten(arr, depth);
    return Value::Array(result);
}

Value arrZip(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray() || !args[1].isArray()) return Value::nil();
    VMArray* a1 = args[0].asArray();
    VMArray* a2 = args[1].asArray();
    size_t minLen = std::min(a1->data.size(), a2->data.size());
    VMArray* result = VMArray::create(0);
    for (size_t i = 0; i < minLen; i++) {
        VMArray* pair = VMArray::create(0);
        pair->data.push_back(a1->data[i]);
        pair->data.push_back(a2->data[i]);
        result->data.push_back(Value::Array(pair));
    }
    return Value::Array(result);
}

Value arrChunk(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray() || !args[1].isNumber()) return Value::nil();
    VMArray* arr = args[0].asArray();
    size_t size = static_cast<size_t>(args[1].asFloat());
    if (size <= 0) return Value::nil();
    VMArray* result = VMArray::create(0);
    for (size_t i = 0; i < arr->data.size(); i += size) {
        VMArray* chunk = VMArray::create(0);
        for (size_t j = i; j < std::min(i + size, arr->data.size()); j++) {
            chunk->data.push_back(arr->data[j]);
        }
        result->data.push_back(Value::Array(chunk));
    }
    return Value::Array(result);
}
} // namespace arr_more

// ═══════════════════════════════════════════════════════════════════
//  文件操作增强实现
// ═══════════════════════════════════════════════════════════════════

void StdLib::registerFileMore(VM* vm) {
    registerFunction(vm, "fileExists", file_more::fileExists);
    registerFunction(vm, "fileSize", file_more::fileSize);
    registerFunction(vm, "fileCopy", file_more::fileCopy);
    registerFunction(vm, "fileMove", file_more::fileMove);
    registerFunction(vm, "fileDelete", file_more::fileDelete);
    registerFunction(vm, "dirExists", file_more::dirExists);
    registerFunction(vm, "dirCreate", file_more::dirCreate);
    registerFunction(vm, "dirDelete", file_more::dirDelete);
    registerFunction(vm, "dirList", file_more::dirList);
    registerFunction(vm, "getCwd", file_more::getCwd);
    registerFunction(vm, "chDir", file_more::chDir);

    registerAlias(vm, "文件存在", "fileExists");
    registerAlias(vm, "文件大小", "fileSize");
    registerAlias(vm, "复制文件", "fileCopy");
    registerAlias(vm, "移动文件", "fileMove");
    registerAlias(vm, "删除文件", "fileDelete");
    registerAlias(vm, "目录存在", "dirExists");
    registerAlias(vm, "创建目录", "dirCreate");
    registerAlias(vm, "删除目录", "dirDelete");
    registerAlias(vm, "目录列表", "dirList");
    registerAlias(vm, "当前目录", "getCwd");
    registerAlias(vm, "切换目录", "chDir");
    
    registerFunction(vm, "fileGlob", file_more::fileGlob);
    
    registerAlias(vm, "文件通配", "fileGlob");
}

namespace file_more {

static std::string getStr(const Value& v) {
    if (!v.isString()) return "";
    return std::string(v.asString()->data, v.asString()->length);
}

Value fileExists(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Bool(false);
    std::string path = getStr(args[0]);
    return Value::Bool(std::filesystem::exists(path) && std::filesystem::is_regular_file(path));
}

Value fileSize(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Float(-1);
    std::string path = getStr(args[0]);
    if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) return Value::Float(-1);
    return Value::Float(static_cast<double>(std::filesystem::file_size(path)));
}

Value fileCopy(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return Value::Bool(false);
    try {
        std::filesystem::copy_file(getStr(args[0]), getStr(args[1]), std::filesystem::copy_options::overwrite_existing);
        return Value::Bool(true);
    } catch (...) { return Value::Bool(false); }
}

Value fileMove(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return Value::Bool(false);
    try {
        std::filesystem::rename(getStr(args[0]), getStr(args[1]));
        return Value::Bool(true);
    } catch (...) { return Value::Bool(false); }
}

Value fileDelete(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Bool(false);
    try {
        return Value::Bool(std::filesystem::remove(getStr(args[0])));
    } catch (...) { return Value::Bool(false); }
}

Value dirExists(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Bool(false);
    std::string path = getStr(args[0]);
    return Value::Bool(std::filesystem::exists(path) && std::filesystem::is_directory(path));
}

Value dirCreate(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Bool(false);
    try {
        return Value::Bool(std::filesystem::create_directories(getStr(args[0])));
    } catch (...) { return Value::Bool(false); }
}

Value dirDelete(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Bool(false);
    try {
        return Value::Bool(std::filesystem::remove_all(getStr(args[0])) > 0);
    } catch (...) { return Value::Bool(false); }
}

Value dirList(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string path = getStr(args[0]);
    VMArray* result = VMArray::create(0);
    try {
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            result->data.push_back(Value::String(VMString::create(entry.path().string())));
        }
    } catch (...) {}
    return Value::Array(result);
}

Value getCwd(std::vector<Value>& args) {
    return Value::String(VMString::create(std::filesystem::current_path().string()));
}

Value chDir(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Bool(false);
    try {
        std::filesystem::current_path(getStr(args[0]));
        return Value::Bool(true);
    } catch (...) { return Value::Bool(false); }
}

Value fileGlob(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string pattern = getStr(args[0]);
    // 分离目录和模式
    size_t lastSlash = pattern.find_last_of("/\\");
    std::string dir = ".";
    std::string pat = pattern;
    if (lastSlash != std::string::npos) {
        dir = pattern.substr(0, lastSlash);
        if (dir.empty()) dir = "\\";
        pat = pattern.substr(lastSlash + 1);
    }
    
    auto result = VMArray::create(0);
    try {
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            std::string name = entry.path().filename().string();
            // 简单通配：* 和 ?
            auto match = [&](const std::string& name, const std::string& pat) -> bool {
                size_t ni = 0, pi = 0;
                size_t star = std::string::npos, matchPos = 0;
                while (ni < name.size()) {
                    if (pi < pat.size() && (pat[pi] == '?' || pat[pi] == name[ni])) {
                        ni++; pi++;
                    } else if (pi < pat.size() && pat[pi] == '*') {
                        star = pi; pi++; matchPos = ni;
                    } else if (star != std::string::npos) {
                        pi = star + 1; matchPos++; ni = matchPos;
                    } else {
                        return false;
                    }
                }
                while (pi < pat.size() && pat[pi] == '*') pi++;
                return pi == pat.size();
            };
            if (match(name, pat)) {
                std::string full = dir + "\\" + name;
                result->data.push_back(Value::String(VMString::create(full)));
            }
        }
    } catch (...) {}
    return Value::Array(result);
}
} // namespace file_more

// ═══════════════════════════════════════════════════════════════════
//  日期时间增强实现
// ═══════════════════════════════════════════════════════════════════

} // namespace cplang
