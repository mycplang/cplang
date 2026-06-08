#include "stdlib/stdlib.hpp"
#include <chrono>
#include <thread>

namespace cplang {

// Time and System functions
// #include'd from stdlib.cpp, already inside namespace cplang

void StdLib::registerTime(VM* vm) {
    registerFunction(vm, "now", time::now);
    registerFunction(vm, "sleep", time::sleep);
    registerFunction(vm, "tick", time::tick);
    registerAlias(vm, "时间戳", "now");
    registerAlias(vm, "延时", "sleep");
    registerAlias(vm, "计时器", "tick");
}

Value time::now(std::vector<Value>& /*args*/) {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return Value::Int(ms);
}

Value time::sleep(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    
    Int64 ms = args[0].asInt();
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    return Value::nil();
}

Value time::tick(std::vector<Value>& /*args*/) {
    static auto start = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
    return Value::Int(ms);
}

void StdLib::registerSystem(VM* vm) {
    registerFunction(vm, "exit", system::exit);
    registerAlias(vm, "程序退出", "exit");
    registerFunction(vm, "getEnv", system::getEnv);
    registerAlias(vm, "环境变量", "getEnv");
    registerFunction(vm, "platform", system::platform);
    registerAlias(vm, "平台", "platform");
    registerFunction(vm, "cwd", system::cwd);
    registerAlias(vm, "当前目录", "cwd");
    registerFunction(vm, "setEnv", system::setEnv);
    registerAlias(vm, "设置环境变量", "setEnv");
    registerFunction(vm, "exec", system::exec);
    registerAlias(vm, "执行命令", "exec");
    registerFunction(vm, "spawn", system::spawn);
    registerAlias(vm, "启动进程", "spawn");
    registerFunction(vm, "arch", system::arch);
    registerAlias(vm, "架构", "arch");
    registerFunction(vm, "pid", system::pid);
    registerAlias(vm, "进程ID", "pid");
    registerFunction(vm, "chdir", system::chdir);
    registerAlias(vm, "切换目录", "chdir");
}

Value system::exit(std::vector<Value>& args) {
    Int64 code = args.empty() ? 0 : args[0].asInt();
    std::exit(static_cast<int>(code));
    return Value::nil();
}

Value system::getEnv(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    
    std::string name(args[0].asString()->data, args[0].asString()->length);
    const char* value = std::getenv(name.c_str());
    
    if (value) {
        return Value::String(VMString::create(value));
    }
    return Value::nil();
}

Value system::platform(std::vector<Value>& /*args*/) {
    #ifdef _WIN32
        return Value::String(VMString::create("windows"));
    #elif __APPLE__
        return Value::String(VMString::create("macos"));
    #elif __linux__
        return Value::String(VMString::create("linux"));
    #else
        return Value::String(VMString::create("unknown"));
    #endif
}

Value system::cwd(std::vector<Value>& /*args*/) {
    // 简化实现
    return Value::String(VMString::create("."));
}

} // namespace cplang
