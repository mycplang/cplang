#include "stdlib/stdlib.hpp"

namespace cplang {

// Time, System, Process extension functions
// #include'd from stdlib.cpp, already inside namespace cplang

void StdLib::registerTimeMore(VM* vm) {
    registerFunction(vm, "timeFormat", time_more::timeFormat);
    registerFunction(vm, "timeParse", time_more::timeParse);
    registerFunction(vm, "timeSleep", time_more::timeSleep);
    registerFunction(vm, "timeDiff", time_more::timeDiff);
    registerFunction(vm, "timeAdd", time_more::timeAdd);
    registerFunction(vm, "timeNowMs", time_more::timeNowMs);
    registerFunction(vm, "timeTimerStart", time_more::timeTimerStart);
    registerFunction(vm, "timeTimerElapsed", time_more::timeTimerElapsed);

    registerAlias(vm, "时间格式化", "timeFormat");
    registerAlias(vm, "时间解析", "timeParse");
    registerAlias(vm, "睡眠", "timeSleep");
    registerAlias(vm, "时间差", "timeDiff");
    registerAlias(vm, "时间加", "timeAdd");
    registerAlias(vm, "当前毫秒", "timeNowMs");
    registerAlias(vm, "计时开始", "timeTimerStart");
    registerAlias(vm, "计时经过", "timeTimerElapsed");
}

namespace time_more {

static std::string getStr(const Value& v) {
    if (!v.isString()) return "";
    return std::string(v.asString()->data, v.asString()->length);
}

// 简单计时器存储（单线程，用静态变量）
static std::chrono::steady_clock::time_point g_timerStart;

Value timeFormat(std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::nil();
    double val = args[0].asFloat();
    // 自动检测毫秒时间戳（> 1e10 视为毫秒）
    if (val > 1e10) val /= 1000.0;
    time_t t = static_cast<time_t>(val);
    std::string fmt = (args.size() >= 2 && args[1].isString()) ? getStr(args[1]) : "%Y-%m-%d %H:%M:%S";
    struct tm timeinfo;
    localtime_s(&timeinfo, &t);
    char buf[256];
    strftime(buf, sizeof(buf), fmt.c_str(), &timeinfo);
    return Value::String(VMString::create(buf));
}

Value timeParse(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return Value::Float(-1);
    std::string s = getStr(args[0]);
    std::string fmt = getStr(args[1]);
    struct tm timeinfo = {};
    std::istringstream ss(s);
    ss >> std::get_time(&timeinfo, fmt.c_str());
    if (ss.fail()) return Value::Float(-1);
    return Value::Float(static_cast<double>(mktime(&timeinfo)));
}

Value timeSleep(std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::nil();
    int ms = static_cast<int>(args[0].asFloat());
    if (ms > 0) std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    return Value::nil();
}

Value timeDiff(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isNumber() || !args[1].isNumber()) return Value::Float(0);
    return Value::Float(args[1].asFloat() - args[0].asFloat());
}

Value timeAdd(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isNumber() || !args[1].isNumber()) return Value::Float(0);
    return Value::Float(args[0].asFloat() + args[1].asFloat());
}

Value timeNowMs(std::vector<Value>& args) {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return Value::Float(static_cast<double>(ms));
}

Value timeTimerStart(std::vector<Value>& args) {
    g_timerStart = std::chrono::steady_clock::now();
    return Value::nil();
}

Value timeTimerElapsed(std::vector<Value>& args) {
    auto now = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_timerStart).count();
    return Value::Float(static_cast<double>(ms));
}
} // namespace time_more

// ═══════════════════════════════════════════════════════════════════
//  系统增强实现
// ═══════════════════════════════════════════════════════════════════

void StdLib::registerSystemMore(VM* vm) {
    registerFunction(vm, "sysGetEnv", sys_more::sysGetEnv);
    registerFunction(vm, "sysSetEnv", sys_more::sysSetEnv);
    registerFunction(vm, "sysExec", sys_more::sysExec);
    registerFunction(vm, "sysShell", sys_more::sysShell);

    registerAlias(vm, "获取环境变量", "sysGetEnv");
    registerAlias(vm, "设置环境变量", "sysSetEnv");
    registerAlias(vm, "执行命令", "sysExec");
    registerAlias(vm, "执行脚本", "sysShell");
    
    registerFunction(vm, "cpuCount", sys_more::cpuCount);
    registerFunction(vm, "osVersion", sys_more::osVersion);
    
    registerAlias(vm, "CPU核数", "cpuCount");
    registerAlias(vm, "操作系统", "osVersion");
}

namespace sys_more {

static std::string getStr(const Value& v) {
    if (!v.isString()) return "";
    return std::string(v.asString()->data, v.asString()->length);
}

Value sysGetEnv(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string name = getStr(args[0]);
    char* buf = nullptr;
    size_t len = 0;
    if (_dupenv_s(&buf, &len, name.c_str()) == 0 && buf != nullptr) {
        std::string result(buf);
        free(buf);
        return Value::String(VMString::create(result));
    }
    return Value::nil();
}

Value sysSetEnv(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return Value::Bool(false);
    std::string name = getStr(args[0]);
    std::string value = getStr(args[1]);
    return Value::Bool(_putenv_s(name.c_str(), value.c_str()) == 0);
}

Value sysExec(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string cmd = getStr(args[0]);
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe) return Value::nil();
    std::string result;
    char buf[1024];
    while (fgets(buf, sizeof(buf), pipe) != nullptr) {
        result += buf;
    }
    _pclose(pipe);
    // 去除末尾换行
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) result.pop_back();
    return Value::String(VMString::create(result));
}

Value sysShell(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Float(-1);
    std::string cmd = getStr(args[0]);
    return Value::Float(static_cast<double>(std::system(cmd.c_str())));
}

Value cpuCount(std::vector<Value>& args) {
    return Value::Int(static_cast<int>(std::thread::hardware_concurrency()));
}

Value osVersion(std::vector<Value>& args) {
    #ifdef _WIN32
        OSVERSIONINFOW osvi;
        ZeroMemory(&osvi, sizeof(OSVERSIONINFOW));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
        #pragma warning(suppress: 4996)
        if (GetVersionExW(&osvi)) {
            char buf[64];
            snprintf(buf, sizeof(buf), "Windows %lu.%lu.%lu", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);
            return Value::String(VMString::create(buf));
        }
    #endif
    return Value::String(VMString::create("Unknown"));
}
} // namespace sys_more

// ═══════════════════════════════════════════════════════════════════
//  进程信息实现
// ═══════════════════════════════════════════════════════════════════

void StdLib::registerProcess(VM* vm) {
    registerFunction(vm, "procId", proc::procId);
    registerFunction(vm, "procParentId", proc::procParentId);
    registerFunction(vm, "procName", proc::procName);
    registerFunction(vm, "procArgs", proc::procArgs);

    registerAlias(vm, "进程ID", "procId");
    registerAlias(vm, "父进程ID", "procParentId");
    registerAlias(vm, "进程名", "procName");
    registerAlias(vm, "进程参数", "procArgs");
}

namespace proc {

Value procId(std::vector<Value>& args) {
    return Value::Float(static_cast<double>(GetCurrentProcessId()));
}

Value procParentId(std::vector<Value>& args) {
    return Value::Float(static_cast<double>(GetCurrentProcessId())); // 简化实现
}

Value procName(std::vector<Value>& args) {
    char buf[MAX_PATH];
    DWORD len = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (len > 0) {
        std::string path(buf, len);
        size_t pos = path.find_last_of("\\/");
        if (pos != std::string::npos) path = path.substr(pos + 1);
        return Value::String(VMString::create(path));
    }
    return Value::nil();
}

Value procArgs(std::vector<Value>& args) {
    VMArray* result = VMArray::create(0);
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv) {
        for (int i = 0; i < argc; i++) {
            int len = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, nullptr, 0, nullptr, nullptr);
            std::string arg(len - 1, '\0');
            WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, &arg[0], len, nullptr, nullptr);
            result->data.push_back(Value::String(VMString::create(arg)));
        }
        LocalFree(argv);
    }
    return Value::Array(result);
}
} // namespace proc

// ═══════════════════════════════════════════════════════════════════
//  数学扩展实现
// ═══════════════════════════════════════════════════════════════════

} // namespace cplang
