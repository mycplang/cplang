#include "stdlib/stdlib.hpp"

namespace cplang {

// File enhancements: seek, recursive dir walk, temp files, logger
// #include'd from stdlib.cpp, already inside namespace cplang

// ── File Seek ──
namespace file_seek_ns {

// File handle wrapper — stored as VMTable with _fh key
Value fileOpenRead_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string path(args[0].asString()->data, args[0].asString()->length);
    FILE* f = nullptr;
    fopen_s(&f, path.c_str(), "rb");
    if (!f) return Value::nil();
    VMTable* t = VMTable::create();
    t->set(makeStringVal(VMString::create("_fh")), makePtrVal(reinterpret_cast<VMObject*>(f)));
    return makeTableVal(t);
}
Value fileOpenWrite_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string path(args[0].asString()->data, args[0].asString()->length);
    FILE* f = nullptr;
    fopen_s(&f, path.c_str(), "wb");
    if (!f) return Value::nil();
    VMTable* t = VMTable::create();
    t->set(makeStringVal(VMString::create("_fh")), makePtrVal(reinterpret_cast<VMObject*>(f)));
    return makeTableVal(t);
}
Value fileClose_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::Bool(false);
    Value fh = args[0].asTable()->get(makeStringVal(VMString::create("_fh")));
    if (fh.isUserData() && fh.asPtr()) {
        fclose(reinterpret_cast<FILE*>(fh.asPtr()));
        args[0].asTable()->set(makeStringVal(VMString::create("_fh")), Value::nil());
        return Value::Bool(true);
    }
    return Value::Bool(false);
}
Value fileSeek_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isTable()) return Value::Bool(false);
    Value fh = args[0].asTable()->get(makeStringVal(VMString::create("_fh")));
    if (!fh.isUserData() || !fh.asPtr()) return Value::Bool(false);
    Int64 offset = args[1].asInt();
    return Value::Bool(fseek(reinterpret_cast<FILE*>(fh.asPtr()), static_cast<long>(offset), SEEK_SET) == 0);
}
Value fileTell_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::Int(-1);
    Value fh = args[0].asTable()->get(makeStringVal(VMString::create("_fh")));
    if (!fh.isUserData() || !fh.asPtr()) return Value::Int(-1);
    return Value::Int(ftell(reinterpret_cast<FILE*>(fh.asPtr())));
}
Value fileReadChunk_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isTable()) return Value::nil();
    Value fh = args[0].asTable()->get(makeStringVal(VMString::create("_fh")));
    if (!fh.isUserData() || !fh.asPtr()) return Value::nil();
    Int64 sz = args[1].asInt();
    std::vector<char> buf(static_cast<size_t>(sz));
    size_t rd = fread(buf.data(), 1, static_cast<size_t>(sz), reinterpret_cast<FILE*>(fh.asPtr()));
    return makeStringVal(VMString::create(std::string(buf.data(), rd)));
}
Value fileWriteChunk_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isTable()) return Value::Bool(false);
    Value fh = args[0].asTable()->get(makeStringVal(VMString::create("_fh")));
    if (!fh.isUserData() || !fh.asPtr()) return Value::Bool(false);
    std::string data(args[1].asString()->data, args[1].asString()->length);
    size_t wr = fwrite(data.data(), 1, data.size(), reinterpret_cast<FILE*>(fh.asPtr()));
    return Value::Bool(wr == data.size());
}
Value fileReadLine_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::nil();
    Value fh = args[0].asTable()->get(makeStringVal(VMString::create("_fh")));
    if (!fh.isUserData() || !fh.asPtr()) return Value::nil();
    FILE* f = reinterpret_cast<FILE*>(fh.asPtr());
    char buf[4096];
    if (fgets(buf, sizeof(buf), f)) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len-1] == '\n') buf[--len] = 0;
        if (len > 0 && buf[len-1] == '\r') buf[--len] = 0;
        return makeStringVal(VMString::create(std::string(buf, len)));
    }
    return Value::nil();
}
Value fileEof_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::Bool(true);
    Value fh = args[0].asTable()->get(makeStringVal(VMString::create("_fh")));
    if (!fh.isUserData() || !fh.asPtr()) return Value::Bool(true);
    return Value::Bool(feof(reinterpret_cast<FILE*>(fh.asPtr())) != 0);
}

} // namespace file_seek_ns

void StdLib::registerFileSeek(VM* vm) {
    using namespace file_seek_ns;
    registerFunction(vm, "fileOpenRead",  fileOpenRead_);
    registerFunction(vm, "fileOpenWrite", fileOpenWrite_);
    registerFunction(vm, "fileClose",     fileClose_);
    registerFunction(vm, "fileSeek",      fileSeek_);
    registerFunction(vm, "fileTell",      fileTell_);
    registerFunction(vm, "fileReadChunk", fileReadChunk_);
    registerFunction(vm, "fileWriteChunk", fileWriteChunk_);
    registerFunction(vm, "fileReadLine",  fileReadLine_);
    registerFunction(vm, "fileEof",       fileEof_);
    registerAlias(vm, "文件打开读",       "fileOpenRead");
    registerAlias(vm, "文件打开写",       "fileOpenWrite");
    registerAlias(vm, "文件关闭",         "fileClose");
    registerAlias(vm, "文件定位",         "fileSeek");
    registerAlias(vm, "文件位置",         "fileTell");
    registerAlias(vm, "文件读块",         "fileReadChunk");
    registerAlias(vm, "文件写块",         "fileWriteChunk");
    registerAlias(vm, "文件读行",         "fileReadLine");
    registerAlias(vm, "文件结束",         "fileEof");
}

// ── Recursive Directory Walk ──
// Recursive file system walk using Windows FindFirstFile/FindNextFile
namespace file_walk_ns {

static std::string wstrToUtf8(const std::wstring& ws) {
    if (ws.empty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return "";
    std::vector<char> buf(len);
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, buf.data(), len, nullptr, nullptr);
    return std::string(buf.data(), len - 1);
}

static void recWalk(const std::wstring& dir, VMArray* result, int maxDepth, int depth) {
    if (depth > maxDepth) return;
    std::wstring pattern = dir + L"\\*";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;
    do {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
        std::string fullPath = wstrToUtf8(dir) + "\\" + wstrToUtf8(fd.cFileName);
        VMTable* entry = VMTable::create();
        entry->set(makeStringVal(VMString::create("path")), makeStringVal(VMString::create(fullPath)));
        entry->set(makeStringVal(VMString::create("name")), makeStringVal(VMString::create(wstrToUtf8(fd.cFileName))));
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            entry->set(makeStringVal(VMString::create("isDir")), Value::Bool(true));
            result->data.push_back(makeTableVal(entry));
            recWalk(dir + L"\\" + fd.cFileName, result, maxDepth, depth + 1);
        } else {
            entry->set(makeStringVal(VMString::create("isDir")), Value::Bool(false));
            U64 fs = ((U64)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;
            entry->set(makeStringVal(VMString::create("size")), Value::Int(static_cast<Int64>(fs)));
            result->data.push_back(makeTableVal(entry));
        }
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);
}

Value fileWalk_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string root(args[0].asString()->data, args[0].asString()->length);
    int maxDepth = args.size() >= 2 && args[1].isInt() ? static_cast<int>(args[1].asInt()) : 99;
    // Convert UTF-8 root to wstring
    std::wstring wroot;
    int wlen = MultiByteToWideChar(CP_UTF8, 0, root.c_str(), -1, nullptr, 0);
    if (wlen > 0) {
        std::vector<wchar_t> wbuf(wlen);
        MultiByteToWideChar(CP_UTF8, 0, root.c_str(), -1, wbuf.data(), wlen);
        wroot.assign(wbuf.data(), wlen - 1);
    }
    VMArray* result = VMArray::create();
    recWalk(wroot, result, maxDepth, 0);
    return makeArrayVal(result);
}

} // namespace file_walk_ns

void StdLib::registerFileWalk(VM* vm) {
    using namespace file_walk_ns;
    registerFunction(vm, "fileWalk",   fileWalk_);
    registerAlias(vm, "文件遍历",      "fileWalk");
}

// ── Logger ──
namespace logger_ns {

// Simple log levels
enum LogLevel { LOG_TRACE=0, LOG_DEBUG=1, LOG_INFO=2, LOG_WARN=3, LOG_ERROR=4 };

static LogLevel currentLevel = LOG_INFO;
static std::string logFilePath;
static bool logToFile = false;
static bool logToConsole = true;

Value logSetLevel_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string lv(args[0].asString()->data, args[0].asString()->length);
    if (lv == "trace") currentLevel = LOG_TRACE;
    else if (lv == "debug") currentLevel = LOG_DEBUG;
    else if (lv == "info")  currentLevel = LOG_INFO;
    else if (lv == "warn")  currentLevel = LOG_WARN;
    else if (lv == "error") currentLevel = LOG_ERROR;
    return Value::nil();
}
Value logSetFile_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) { logToFile = false; return Value::nil(); }
    logFilePath.assign(args[0].asString()->data, args[0].asString()->length);
    logToFile = !logFilePath.empty();
    return Value::nil();
}
Value logSetConsole_(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    logToConsole = args[0].isTrue();
    return Value::nil();
}

static const char* levelName(LogLevel lv) {
    switch(lv) { case LOG_TRACE: return "TRACE"; case LOG_DEBUG: return "DEBUG";
        case LOG_INFO: return "INFO"; case LOG_WARN: return "WARN"; case LOG_ERROR: return "ERROR"; }
    return "UNKNOWN";
}

static void writeLog(LogLevel lv, const std::string& msg) {
    if (lv < currentLevel) return;
    std::string line = std::string("[") + levelName(lv) + "] " + msg + "\n";
    if (logToConsole) fputs(line.c_str(), stderr);
    if (logToFile && !logFilePath.empty()) {
        FILE* f = nullptr;
        fopen_s(&f, logFilePath.c_str(), "ab");
        if (f) { fwrite(line.data(), 1, line.size(), f); fclose(f); }
    }
}

Value log_(std::vector<Value>& args, LogLevel lv) {
    if (args.empty()) return Value::nil();
    std::string msg(args[0].asString()->data, args[0].asString()->length);
    writeLog(lv, msg);
    return Value::nil();
}

Value logTrace_(std::vector<Value>& args) { return log_(args, LOG_TRACE); }
Value logDebug_(std::vector<Value>& args) { return log_(args, LOG_DEBUG); }
Value logInfo_(std::vector<Value>& args)  { return log_(args, LOG_INFO); }
Value logWarn_(std::vector<Value>& args)  { return log_(args, LOG_WARN); }
Value logError_(std::vector<Value>& args) { return log_(args, LOG_ERROR); }

} // namespace logger_ns

void StdLib::registerLogger(VM* vm) {
    using namespace logger_ns;
    registerFunction(vm, "logSetLevel",   logSetLevel_);
    registerFunction(vm, "logSetFile",    logSetFile_);
    registerFunction(vm, "logSetConsole", logSetConsole_);
    registerFunction(vm, "logTrace",      logTrace_);
    registerFunction(vm, "logDebug",      logDebug_);
    registerFunction(vm, "logInfo",       logInfo_);
    registerFunction(vm, "logWarn",       logWarn_);
    registerFunction(vm, "logError",      logError_);
    registerAlias(vm, "日志设置级别",      "logSetLevel");
    registerAlias(vm, "日志设置文件",      "logSetFile");
    registerAlias(vm, "日志设置控制台",    "logSetConsole");
    registerAlias(vm, "日志追踪",          "logTrace");
    registerAlias(vm, "日志调试",          "logDebug");
    registerAlias(vm, "日志信息",          "logInfo");
    registerAlias(vm, "日志警告",          "logWarn");
    registerAlias(vm, "日志错误",          "logError");
}

} // namespace cplang
