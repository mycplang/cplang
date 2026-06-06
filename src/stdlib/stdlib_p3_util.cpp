#include "stdlib/stdlib.hpp"

namespace cplang {

// CSV write, enhanced logging with levels/timestamps, case-insensitive compare
// #include'd from stdlib.cpp, already inside namespace cplang

namespace csv_write_ns {

// csvWrite(data, delimiter?) — serialize array of arrays to CSV string
// Each row is an array of values, values will be auto-quoted if they contain
// the delimiter, double quotes, or newlines
static std::string csvEscape(const std::string& cell, char delim) {
    bool needsQuote = cell.find(delim) != std::string::npos ||
                      cell.find('"') != std::string::npos ||
                      cell.find('\n') != std::string::npos ||
                      cell.find('\r') != std::string::npos;
    if (!needsQuote) return cell;
    std::string out = "\"";
    for (char c : cell) {
        if (c == '"') out += "\"\"";
        else out += c;
    }
    out += "\"";
    return out;
}

Value csvWrite_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return makeStringVal(VMString::create(""));
    char delim = ',';
    if (args.size() >= 2 && args[1].isString()) {
        auto ds = args[1].asString();
        if (ds->length > 0) delim = ds->data[0];
    }
    
    std::string out;
    auto rows = args[0].asArray();
    for (size_t r = 0; r < rows->data.size(); r++) {
        auto& rowVal = rows->data[r];
        if (!rowVal.isArray()) continue;
        auto row = rowVal.asArray();
        for (size_t c = 0; c < row->data.size(); c++) {
            if (c > 0) out += delim;
            std::string cell;
            if (row->data[c].isString()) {
                auto s = row->data[c].asString();
                cell.assign(s->data, s->length);
            } else if (row->data[c].isInt()) {
                cell = std::to_string(row->data[c].asInt());
            } else if (row->data[c].isFloat()) {
                cell = std::to_string(row->data[c].asFloat());
            } else if (row->data[c].isBool()) {
                cell = row->data[c].isTrue() ? "true" : "false";
            } else if (!row->data[c].isNil()) {
                // Use toString-like
                char buf[64];
                snprintf(buf, 64, "%g", row->data[c].asFloat());
                cell = buf;
            }
            out += csvEscape(cell, delim);
        }
        out += "\r\n";
    }
    return makeStringVal(VMString::create(out));
}

} // namespace csv_write_ns

void StdLib::registerCsvWrite(VM* vm) {
    using namespace csv_write_ns;
    registerFunction(vm, "csvWrite", csvWrite_);
    registerAlias(vm, "CSV写入", "csvWrite");
}

// ── Enhanced logger with levels and timestamps ──
namespace log_ns {

static int currentLevel_ = 0; // 0=DEBUG, 1=INFO, 2=WARN, 3=ERROR
static std::string logFilePath_;
static std::vector<std::string> logBuffer_;
static const size_t maxBufferSize_ = 1000;

static void flushLogBuffer() {
    if (logFilePath_.empty() || logBuffer_.empty()) return;
    FILE* f = nullptr;
    f = fopen( logFilePath_.c_str(), "ab");
    if (!f) return;
    for (auto& line : logBuffer_) {
        fwrite(line.data(), 1, line.size(), f);
    }
    fclose(f);
    logBuffer_.clear();
}

static void writeLog(const std::string& level, const std::string& msg) {
    // Get current time
    SYSTEMTIME st;
    GetLocalTime(&st);
    char ts[32];
    snprintf(ts, 32, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
             st.wYear, st.wMonth, st.wDay,
             st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    
    std::string line = std::string(ts) + " [" + level + "] " + msg + "\n";
    
    // Print to console
    DWORD written;
    HANDLE h = GetStdHandle(level == "ERROR" ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
    WriteFile(h, line.data(), (DWORD)line.size(), &written, nullptr);
    
    // Buffer for file
    if (!logFilePath_.empty()) {
        logBuffer_.push_back(line);
        if (logBuffer_.size() >= maxBufferSize_) flushLogBuffer();
    }
}

Value logDebug_(std::vector<Value>& args) {
    if (currentLevel_ > 0) return Value::nil();
    std::string msg;
    if (!args.empty() && args[0].isString()) {
        auto s = args[0].asString();
        msg.assign(s->data, s->length);
    }
    writeLog("DEBUG", msg);
    return Value::nil();
}

Value logInfo_(std::vector<Value>& args) {
    if (currentLevel_ > 1) return Value::nil();
    std::string msg;
    if (!args.empty() && args[0].isString()) {
        auto s = args[0].asString();
        msg.assign(s->data, s->length);
    }
    writeLog("INFO", msg);
    return Value::nil();
}

Value logWarn_(std::vector<Value>& args) {
    if (currentLevel_ > 2) return Value::nil();
    std::string msg;
    if (!args.empty() && args[0].isString()) {
        auto s = args[0].asString();
        msg.assign(s->data, s->length);
    }
    writeLog("WARN", msg);
    return Value::nil();
}

Value logError_(std::vector<Value>& args) {
    std::string msg;
    if (!args.empty() && args[0].isString()) {
        auto s = args[0].asString();
        msg.assign(s->data, s->length);
    }
    writeLog("ERROR", msg);
    return Value::nil();
}

Value logSetLevel_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    auto s = args[0].asString();
    std::string lv(s->data, s->length);
    if (lv == "DEBUG" || lv == "debug") currentLevel_ = 0;
    else if (lv == "INFO" || lv == "info") currentLevel_ = 1;
    else if (lv == "WARN" || lv == "warn") currentLevel_ = 2;
    else if (lv == "ERROR" || lv == "error") currentLevel_ = 3;
    return Value::nil();
}

Value logSetFile_(std::vector<Value>& args) {
    flushLogBuffer(); // flush old file before switching
    if (args.empty() || !args[0].isString()) {
        logFilePath_.clear();
        return Value::Bool(true);
    }
    auto s = args[0].asString();
    logFilePath_.assign(s->data, s->length);
    return Value::Bool(true);
}

Value logFlush_(std::vector<Value>& args) {
    flushLogBuffer();
    return Value::nil();
}

} // namespace log_ns

void StdLib::registerLogPlus(VM* vm) {
    using namespace log_ns;
    registerFunction(vm, "logDebug",    logDebug_);
    registerFunction(vm, "logInfo",     logInfo_);
    registerFunction(vm, "logWarn",     logWarn_);
    registerFunction(vm, "logError",    logError_);
    registerFunction(vm, "logSetLevel", logSetLevel_);
    registerFunction(vm, "logSetFile",  logSetFile_);
    registerFunction(vm, "logFlush",    logFlush_);
    registerAlias(vm, "调试日志",       "logDebug");
    registerAlias(vm, "信息日志",       "logInfo");
    registerAlias(vm, "警告日志",       "logWarn");
    registerAlias(vm, "错误日志",       "logError");
    registerAlias(vm, "设置日志级别",   "logSetLevel");
    registerAlias(vm, "设置日志文件",   "logSetFile");
    registerAlias(vm, "刷新日志",       "logFlush");
}

// ── Case-insensitive string compare ──
namespace str_ci_ns {

Value strCompareI_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isString())
        return Value::Int(0);
    auto a = args[0].asString();
    auto b = args[1].asString();
    // Simple ASCII case-insensitive compare
    auto ic = [](char c) -> char { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; };
    size_t minLen = (a->length < b->length) ? a->length : b->length;
    for (size_t i = 0; i < minLen; i++) {
        char ca = ic(a->data[i]), cb = ic(b->data[i]);
        if (ca < cb) return Value::Int(-1);
        if (ca > cb) return Value::Int(1);
    }
    if (a->length < b->length) return Value::Int(-1);
    if (a->length > b->length) return Value::Int(1);
    return Value::Int(0);
}

Value strEqualsI_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isString())
        return Value::Bool(false);
    auto a = args[0].asString();
    auto b = args[1].asString();
    if (a->length != b->length) return Value::Bool(false);
    auto ic = [](char c) -> char { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; };
    for (size_t i = 0; i < a->length; i++) {
        if (ic(a->data[i]) != ic(b->data[i])) return Value::Bool(false);
    }
    return Value::Bool(true);
}

Value strContainsI_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isString())
        return Value::Bool(false);
    auto haystack = args[0].asString();
    auto needle = args[1].asString();
    if (needle->length == 0) return Value::Bool(true);
    if (needle->length > haystack->length) return Value::Bool(false);
    auto ic = [](char c) -> char { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; };
    for (size_t i = 0; i <= haystack->length - needle->length; i++) {
        bool match = true;
        for (size_t j = 0; j < needle->length; j++) {
            if (ic(haystack->data[i+j]) != ic(needle->data[j])) {
                match = false;
                break;
            }
        }
        if (match) return Value::Bool(true);
    }
    return Value::Bool(false);
}

} // namespace str_ci_ns

void StdLib::registerStrCi(VM* vm) {
    using namespace str_ci_ns;
    registerFunction(vm, "strCompareI",  strCompareI_);
    registerFunction(vm, "strEqualsI",   strEqualsI_);
    registerFunction(vm, "strContainsI", strContainsI_);
    registerAlias(vm, "不区分大小写比较",  "strCompareI");
    registerAlias(vm, "不区分大小写等于",  "strEqualsI");
    registerAlias(vm, "不区分大小写包含",  "strContainsI");
}

} // namespace cplang
