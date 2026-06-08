#include "stdlib/stdlib.hpp"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace cplang {

// File functions (read, write, append, exists, size, copy, move, delete, etc.)
// #include'd from stdlib.cpp, already inside namespace cplang

// 前向声明
namespace file {
    Value readBytes(std::vector<Value>& args);
    Value writeBytes(std::vector<Value>& args);
}

void StdLib::registerFile(VM* vm) {
    registerFunction(vm, "readFile", file::read);
    registerFunction(vm, "writeFile", file::write);
    registerFunction(vm, "appendFile", file::append);
    registerFunction(vm, "fileExists", file::exists);
    registerFunction(vm, "fileSize", file::size);
    registerFunction(vm, "fileCopy", file::copy);
    registerFunction(vm, "fileMove", file::move);
    registerFunction(vm, "fileDelete", file::delete_);
    registerFunction(vm, "dirExists", file::isDir);
    registerFunction(vm, "dirCreate", file::mkdir);
    registerFunction(vm, "dirDelete", file::rmdir);
    registerFunction(vm, "dirList", file::listDir);
    registerFunction(vm, "isFile", file::isFile);
    registerFunction(vm, "fileTime", file::time);
    registerAlias(vm, "读取文件", "readFile");
    registerAlias(vm, "写入文件", "writeFile");
    registerAlias(vm, "追加文件", "appendFile");
    registerAlias(vm, "文件存在", "fileExists");
    registerAlias(vm, "文件大小", "fileSize");
    registerAlias(vm, "复制文件", "fileCopy");
    registerAlias(vm, "移动文件", "fileMove");
    registerAlias(vm, "删除文件", "fileDelete");
    registerAlias(vm, "目录存在", "dirExists");
    registerAlias(vm, "创建目录", "dirCreate");
    registerAlias(vm, "删除目录", "dirDelete");
    registerAlias(vm, "目录列表", "dirList");
    registerAlias(vm, "是文件", "isFile");
    registerAlias(vm, "文件时间", "fileTime");
    registerFunction(vm, "readFileBytes", file::readBytes);
    registerFunction(vm, "writeFileBytes", file::writeBytes);
    registerAlias(vm, "读取文件字节", "readFileBytes");
    registerAlias(vm, "写入文件字节", "writeFileBytes");
}

namespace file {
    Value read(std::vector<Value>& args) {
        if (args.empty() || !args[0].isString()) return Value::nil();
        std::string path(args[0].asString()->data, args[0].asString()->length);
        std::ifstream f(path, std::ios::binary);
        if (!f) return Value::nil();
        std::string data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        if (data.size() >= 3 && (unsigned char)data[0] == 0xEF && (unsigned char)data[1] == 0xBB && (unsigned char)data[2] == 0xBF)
            data = data.substr(3);
        return Value::String(VMString::create(data.c_str(), (UInt32)data.size()));
    }
    Value write(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return Value::Bool(false);
        std::string path(args[0].asString()->data, args[0].asString()->length);
        std::string data(args[1].asString()->data, args[1].asString()->length);
        std::ofstream f(path, std::ios::binary);
        if (!f) return Value::Bool(false);
        f.write(data.c_str(), data.size());
        return Value::Bool(true);
    }
    Value append(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return Value::Bool(false);
        std::string path(args[0].asString()->data, args[0].asString()->length);
        std::string data(args[1].asString()->data, args[1].asString()->length);
        std::ofstream f(path, std::ios::app | std::ios::binary);
        if (!f) return Value::Bool(false);
        f.write(data.c_str(), data.size());
        return Value::Bool(true);
    }
    Value exists(std::vector<Value>& args) {
        if (args.empty() || !args[0].isString()) return Value::Bool(false);
        std::string path(args[0].asString()->data, args[0].asString()->length);
        struct stat st;
        return Value::Bool(stat(path.c_str(), &st) == 0);
    }
    Value size(std::vector<Value>& args) {
        if (args.empty() || !args[0].isString()) return Value::Int(-1);
        std::string path(args[0].asString()->data, args[0].asString()->length);
        struct stat st;
        if (stat(path.c_str(), &st) != 0) return Value::Int(-1);
        return Value::Int((Int64)st.st_size);
    }
    Value copy(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return Value::Bool(false);
        std::string src(args[0].asString()->data, args[0].asString()->length);
        std::string dst(args[1].asString()->data, args[1].asString()->length);
        std::ifstream in(src, std::ios::binary);
        if (!in) return Value::Bool(false);
        std::ofstream out(dst, std::ios::binary);
        if (!out) return Value::Bool(false);
        out << in.rdbuf();
        return Value::Bool(true);
    }
    Value move(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return Value::Bool(false);
        std::string src(args[0].asString()->data, args[0].asString()->length);
        std::string dst(args[1].asString()->data, args[1].asString()->length);
        return Value::Bool(rename(src.c_str(), dst.c_str()) == 0);
    }
    Value delete_(std::vector<Value>& args) {
        if (args.empty() || !args[0].isString()) return Value::Bool(false);
        std::string path(args[0].asString()->data, args[0].asString()->length);
        return Value::Bool(remove(path.c_str()) == 0);
    }
    Value isFile(std::vector<Value>& args) {
        if (args.empty() || !args[0].isString()) return Value::Bool(false);
        std::string path(args[0].asString()->data, args[0].asString()->length);
        struct stat st;
        return Value::Bool(stat(path.c_str(), &st) == 0 && (st.st_mode & S_IFREG));
    }
    Value isDir(std::vector<Value>& args) {
        if (args.empty() || !args[0].isString()) return Value::Bool(false);
        std::string path(args[0].asString()->data, args[0].asString()->length);
        struct stat st;
        return Value::Bool(stat(path.c_str(), &st) == 0 && (st.st_mode & S_IFDIR));
    }
    Value mkdir(std::vector<Value>& args) {
        if (args.empty() || !args[0].isString()) return Value::Bool(false);
        std::string path(args[0].asString()->data, args[0].asString()->length);
        try { return Value::Bool(std::filesystem::create_directories(path)); }
        catch (...) { return Value::Bool(false); }
    }
    Value rmdir(std::vector<Value>& args) {
        if (args.empty() || !args[0].isString()) return Value::Bool(false);
        std::string path(args[0].asString()->data, args[0].asString()->length);
        std::error_code ec;
        return Value::Bool(std::filesystem::remove_all(path, ec) > 0);
    }
    Value listDir(std::vector<Value>& args) {
        if (args.empty() || !args[0].isString()) return Value::nil();
        std::string path(args[0].asString()->data, args[0].asString()->length);
        VMArray* result = VMArray::create();
        try {
            for (auto& entry : std::filesystem::directory_iterator(path)) {
                std::string name = entry.path().filename().string();
                result->data.push_back(Value::String(VMString::create(name.c_str(), (UInt32)name.size())));
            }
        } catch (...) {}
        return Value::Array(result);
    }
    Value time(std::vector<Value>& args) {
        if (args.empty() || !args[0].isString()) return Value::Int(0);
        std::string path(args[0].asString()->data, args[0].asString()->length);
        struct stat st;
        if (stat(path.c_str(), &st) != 0) return Value::Int(0);
        return Value::Int((Int64)st.st_mtime);
    }

    // ═══════════════════════════════════════════════════════════════
    //  读取字节 → 整数数组（纯二进制，无 BOM 剥离）
    //  readFileBytes(path) → [byte1, byte2, ...]
    // ═══════════════════════════════════════════════════════════════
    Value readBytes(std::vector<Value>& args) {
        if (args.empty() || !args[0].isString()) return Value::nil();
        std::string path(args[0].asString()->data, args[0].asString()->length);
        std::ifstream f(path, std::ios::binary);
        if (!f) return Value::nil();
        // 读全部字节
        f.seekg(0, std::ios::end);
        std::streampos len = f.tellg();
        f.seekg(0, std::ios::beg);
        if (len <= 0) return Value::Array(VMArray::create());
        // 读入 vector
        std::vector<uint8_t> buf(static_cast<size_t>(len));
        f.read(reinterpret_cast<char*>(buf.data()), len);
        if (!f) return Value::nil();
        // 转为 CP 数组
        VMArray* arr = VMArray::create();
        arr->data.reserve(buf.size());
        for (uint8_t b : buf)
            arr->data.push_back(Value::Int(b));
        return Value::Array(arr);
    }

    // ═══════════════════════════════════════════════════════════════
    //  写入字节（从整数数组）
    //  writeFileBytes(path, [65, 66, 67])  → 写入 "ABC"
    // ═══════════════════════════════════════════════════════════════
    Value writeBytes(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].isString()) return Value::Bool(false);
        std::string path(args[0].asString()->data, args[0].asString()->length);
        // 第二个参数可以是数组或整数
        std::ofstream f(path, std::ios::binary);
        if (!f) return Value::Bool(false);
        if (args[1].isArray()) {
            VMArray* arr = args[1].asArray();
            for (size_t i = 0; i < arr->data.size(); i++) {
                int64_t byteVal = 0;
                Value& v = arr->data[i];
                if (v.isInt() || v.isInt64()) byteVal = v.asInt();
                else if (v.isBool())         byteVal = v.asBool() ? 1 : 0;
                else continue;
                f.put(static_cast<char>(byteVal & 0xFF));
            }
        } else if (args[1].isInt() || args[1].isInt64()) {
            int64_t byteVal = args[1].asInt();
            f.put(static_cast<char>(byteVal & 0xFF));
        }
        return Value::Bool(f.good());
    }
}

} // namespace cplang
