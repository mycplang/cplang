// File functions (read, write, append, exists, size, copy, move, delete, etc.)
// #include'd from stdlib.cpp, already inside namespace cplang

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
}
