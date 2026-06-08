#include "stdlib/stdlib.hpp"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace cplang {

// P2 remaining: temp files/dirs, file stat, duration, file delete
// #include'd from stdlib.cpp, already inside namespace cplang

// ── Temp files and directories (Windows) ──
namespace temp_ns {

Value tempFile_(std::vector<Value>& args) {
    std::string prefix = "cp_";
    std::string suffix = ".tmp";
    if (args.size() >= 1 && args[0].isString())
        prefix.assign(args[0].asString()->data, args[0].asString()->length);
    if (args.size() >= 2 && args[1].isString())
        suffix.assign(args[1].asString()->data, args[1].asString()->length);
    
    wchar_t tempPath[MAX_PATH];
    if (GetTempPathW(MAX_PATH, tempPath) == 0) return Value::nil();
    
    wchar_t tempFile[MAX_PATH];
    // Convert prefix to wide
    std::wstring wprefix;
    int wlen = MultiByteToWideChar(CP_UTF8, 0, prefix.c_str(), -1, nullptr, 0);
    if (wlen > 0) { std::vector<wchar_t> wb(wlen); MultiByteToWideChar(CP_UTF8, 0, prefix.c_str(), -1, wb.data(), wlen); wprefix = wb.data(); }
    
    if (GetTempFileNameW(tempPath, wprefix.c_str(), 0, tempFile) == 0) return Value::nil();
    
    // Convert back to UTF-8
    int u8len = WideCharToMultiByte(CP_UTF8, 0, tempFile, -1, nullptr, 0, nullptr, nullptr);
    if (u8len <= 0) return Value::nil();
    std::vector<char> u8(u8len);
    WideCharToMultiByte(CP_UTF8, 0, tempFile, -1, u8.data(), u8len, nullptr, nullptr);
    return makeStringVal(VMString::create(std::string(u8.data(), u8len - 1)));
}

Value tempDir_(std::vector<Value>& args) {
    std::string prefix = "cp_";
    if (args.size() >= 1 && args[0].isString())
        prefix.assign(args[0].asString()->data, args[0].asString()->length);
    
    wchar_t tempPath[MAX_PATH];
    if (GetTempPathW(MAX_PATH, tempPath) == 0) return Value::nil();
    
    // Generate unique name: prefix + random hex
    wchar_t dirName[MAX_PATH];
    for (int attempt = 0; attempt < 100; attempt++) {
        int rnd = rand() ^ (GetTickCount() << 8);
        // Use wide prefix
        std::wstring wprefix;
        int wlen = MultiByteToWideChar(CP_UTF8, 0, prefix.c_str(), -1, nullptr, 0);
        if (wlen > 0) { std::vector<wchar_t> wb(wlen); MultiByteToWideChar(CP_UTF8, 0, prefix.c_str(), -1, wb.data(), wlen); wprefix = wb.data(); }
        swprintf_s(dirName, MAX_PATH, L"%s%s%08x", tempPath, wprefix.c_str(), rnd);
        if (CreateDirectoryW(dirName, nullptr)) {
            int u8len = WideCharToMultiByte(CP_UTF8, 0, dirName, -1, nullptr, 0, nullptr, nullptr);
            if (u8len <= 0) return Value::nil();
            std::vector<char> u8(u8len);
            WideCharToMultiByte(CP_UTF8, 0, dirName, -1, u8.data(), u8len, nullptr, nullptr);
            return makeStringVal(VMString::create(std::string(u8.data(), u8len - 1)));
        }
    }
    return Value::nil();
}

Value dirRemove_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Bool(false);
    std::string path(args[0].asString()->data, args[0].asString()->length);
    int wlen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return Value::Bool(false);
    std::vector<wchar_t> wb(wlen);
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wb.data(), wlen);
    return Value::Bool(RemoveDirectoryW(wb.data()) != 0);
}

} // namespace temp_ns

void StdLib::registerTemp(VM* vm) {
    using namespace temp_ns;
    registerFunction(vm, "tempFile",   tempFile_);
    registerFunction(vm, "tempDir",    tempDir_);
    registerFunction(vm, "dirRemove",  dirRemove_);
    registerAlias(vm, "临时文件",      "tempFile");
    registerAlias(vm, "临时目录",      "tempDir");
    registerAlias(vm, "删除目录",      "dirRemove");
}

// ── File stat (size, modification time, exists check) ──
namespace file_stat_ns {

Value fileSize_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Int(-1);
    std::string path(args[0].asString()->data, args[0].asString()->length);
    int wlen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return Value::Int(-1);
    std::vector<wchar_t> wb(wlen);
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wb.data(), wlen);
    
    WIN32_FILE_ATTRIBUTE_DATA attr;
    if (!GetFileAttributesExW(wb.data(), GetFileExInfoStandard, &attr))
        return Value::Int(-1);
    if (attr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        return Value::Int(0);
    ULONGLONG fs = ((ULONGLONG)attr.nFileSizeHigh << 32) | attr.nFileSizeLow;
    return Value::Int(static_cast<Int64>(fs));
}

// Convert FILETIME to milliseconds since Unix epoch
static Int64 filetimeToMs(const FILETIME& ft) {
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    // FILETIME is 100-ns intervals since 1601-01-01
    // Unix epoch is 1970-01-01. Difference: 11644473600 seconds
    return static_cast<Int64>((uli.QuadPart / 10000ULL) - 11644473600000ULL);
}

Value fileMtime_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Int(0);
    std::string path(args[0].asString()->data, args[0].asString()->length);
    int wlen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return Value::Int(0);
    std::vector<wchar_t> wb(wlen);
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wb.data(), wlen);
    
    WIN32_FILE_ATTRIBUTE_DATA attr;
    if (!GetFileAttributesExW(wb.data(), GetFileExInfoStandard, &attr))
        return Value::Int(0);
    return Value::Int(filetimeToMs(attr.ftLastWriteTime));
}

Value fileCtime_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Int(0);
    std::string path(args[0].asString()->data, args[0].asString()->length);
    int wlen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return Value::Int(0);
    std::vector<wchar_t> wb(wlen);
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wb.data(), wlen);
    
    WIN32_FILE_ATTRIBUTE_DATA attr;
    if (!GetFileAttributesExW(wb.data(), GetFileExInfoStandard, &attr))
        return Value::Int(0);
    return Value::Int(filetimeToMs(attr.ftCreationTime));
}

Value fileDelete_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Bool(false);
    std::string path(args[0].asString()->data, args[0].asString()->length);
    int wlen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return Value::Bool(false);
    std::vector<wchar_t> wb(wlen);
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wb.data(), wlen);
    return Value::Bool(DeleteFileW(wb.data()) != 0);
}

Value fileRename_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return Value::Bool(false);
    std::string oldpath(args[0].asString()->data, args[0].asString()->length);
    std::string newpath(args[1].asString()->data, args[1].asString()->length);
    int wlen1 = MultiByteToWideChar(CP_UTF8, 0, oldpath.c_str(), -1, nullptr, 0);
    int wlen2 = MultiByteToWideChar(CP_UTF8, 0, newpath.c_str(), -1, nullptr, 0);
    if (wlen1 <= 0 || wlen2 <= 0) return Value::Bool(false);
    std::vector<wchar_t> wb1(wlen1), wb2(wlen2);
    MultiByteToWideChar(CP_UTF8, 0, oldpath.c_str(), -1, wb1.data(), wlen1);
    MultiByteToWideChar(CP_UTF8, 0, newpath.c_str(), -1, wb2.data(), wlen2);
    return Value::Bool(MoveFileW(wb1.data(), wb2.data()) != 0);
}

Value fileCopy_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return Value::Bool(false);
    std::string src(args[0].asString()->data, args[0].asString()->length);
    std::string dst(args[1].asString()->data, args[1].asString()->length);
    int wlen1 = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), -1, nullptr, 0);
    int wlen2 = MultiByteToWideChar(CP_UTF8, 0, dst.c_str(), -1, nullptr, 0);
    if (wlen1 <= 0 || wlen2 <= 0) return Value::Bool(false);
    std::vector<wchar_t> wb1(wlen1), wb2(wlen2);
    MultiByteToWideChar(CP_UTF8, 0, src.c_str(), -1, wb1.data(), wlen1);
    MultiByteToWideChar(CP_UTF8, 0, dst.c_str(), -1, wb2.data(), wlen2);
    bool failIfExists = (args.size() >= 3 && args[2].isTrue());
    return Value::Bool(CopyFileW(wb1.data(), wb2.data(), failIfExists ? TRUE : FALSE) != 0);
}

Value fileIsDir_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Bool(false);
    std::string path(args[0].asString()->data, args[0].asString()->length);
    int wlen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return Value::Bool(false);
    std::vector<wchar_t> wb(wlen);
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wb.data(), wlen);
    DWORD attr = GetFileAttributesW(wb.data());
    return Value::Bool(attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
}

Value fileExists_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Bool(false);
    std::string path(args[0].asString()->data, args[0].asString()->length);
    int wlen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return Value::Bool(false);
    std::vector<wchar_t> wb(wlen);
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wb.data(), wlen);
    DWORD attr = GetFileAttributesW(wb.data());
    return Value::Bool(attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

} // namespace file_stat_ns

void StdLib::registerFileStat(VM* vm) {
    using namespace file_stat_ns;
    registerFunction(vm, "fileSize",    fileSize_);
    registerFunction(vm, "fileMtime",   fileMtime_);
    registerFunction(vm, "fileCtime",   fileCtime_);
    registerFunction(vm, "fileDelete",  fileDelete_);
    registerFunction(vm, "fileRename",  fileRename_);
    registerFunction(vm, "fileCopy",    fileCopy_);
    registerFunction(vm, "fileIsDir",   fileIsDir_);
    registerFunction(vm, "fileExists",  fileExists_);
    registerAlias(vm, "文件大小",       "fileSize");
    registerAlias(vm, "文件修改时间",   "fileMtime");
    registerAlias(vm, "文件创建时间",   "fileCtime");
    registerAlias(vm, "删除文件",       "fileDelete");
    registerAlias(vm, "文件重命名",     "fileRename");
    registerAlias(vm, "文件复制",       "fileCopy");
    registerAlias(vm, "是目录",         "fileIsDir");
    registerAlias(vm, "文件存在",       "fileExists");
}

// ── Duration / time helpers ──
namespace duration_ns {

Value durationMs_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isInt()) return Value::Int(0);
    return Value::Int(args[0].asInt()); // Already in ms, just pass through as named semantic
}

Value durationSince_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isInt()) return Value::Int(0);
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    Int64 nowMs = static_cast<Int64>((uli.QuadPart / 10000ULL) - 11644473600000ULL);
    return Value::Int(nowMs - args[0].asInt());
}

Value durationFormat_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isInt()) return makeStringVal(VMString::create("0ms"));
    Int64 ms = args[0].asInt();
    bool negative = ms < 0;
    if (negative) ms = -ms;
    Int64 hours = ms / 3600000;
    ms %= 3600000;
    Int64 minutes = ms / 60000;
    ms %= 60000;
    Int64 seconds = ms / 1000;
    Int64 millis = ms % 1000;
    char buf[64];
    if (hours > 0) {
        snprintf(buf, sizeof(buf), "%s%lldh %lldm %lld.%03llds",
            negative?"-":"", hours, minutes, seconds, millis);
    } else if (minutes > 0) {
        snprintf(buf, sizeof(buf), "%s%lldm %lld.%03llds",
            negative?"-":"", minutes, seconds, millis);
    } else {
        snprintf(buf, sizeof(buf), "%s%lld.%03llds",
            negative?"-":"", seconds, millis);
    }
    return makeStringVal(VMString::create(std::string(buf)));
}

Value elapsed_(std::vector<Value>& args) {
    // Simple elapsed timer — returns ms since start of process
    static ULONGLONG startTick = 0;
    if (startTick == 0) startTick = GetTickCount64();
    return Value::Int(static_cast<Int64>(GetTickCount64() - startTick));
}

} // namespace duration_ns

void StdLib::registerDuration(VM* vm) {
    using namespace duration_ns;
    registerFunction(vm, "durationMs",     durationMs_);
    registerFunction(vm, "durationSince",  durationSince_);
    registerFunction(vm, "durationFormat", durationFormat_);
    registerFunction(vm, "elapsed",        elapsed_);
    registerAlias(vm, "时间间隔",          "durationMs");
    registerAlias(vm, "距离时间",          "durationSince");
    registerAlias(vm, "格式化时间间隔",    "durationFormat");
    registerAlias(vm, "已用时间",          "elapsed");
}

} // namespace cplang
