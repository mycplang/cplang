#pragma once
#include "common/types.hpp"
#include "vm/vm.hpp"
#include <unordered_map>
#include <vector>
#include <functional>

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  断点
// ═══════════════════════════════════════════════════════════════════

struct Breakpoint {
    String file;
    int line;
    bool enabled = true;
    int hitCount = 0;
    int hitCondition = 0;       // 0 = 无条件, >0 = 命中次数
    String condition;           // 条件表达式
    
    Breakpoint() : line(0) {}
    Breakpoint(const String& f, int l) : file(f), line(l) {}
};

// ═══════════════════════════════════════════════════════════════════
//  监视表达式
// ═══════════════════════════════════════════════════════════════════

struct Watch {
    String expression;
    String lastValue;
    bool enabled = true;
};

// ═══════════════════════════════════════════════════════════════════
//  调试事件
// ═══════════════════════════════════════════════════════════════════

enum class DebugEvent {
    BreakpointHit,      // 命中断点
    StepOver,           // 单步跳过
    StepInto,           // 单步进入
    StepOut,            // 单步跳出
    ExceptionThrown,    // 异常抛出
    FunctionEntry,      // 函数进入
    FunctionExit,       // 函数退出
    VariableChanged     // 变量变化
};

// ═══════════════════════════════════════════════════════════════════
//  调用帧信息
// ═══════════════════════════════════════════════════════════════════

struct DebugFrame {
    String functionName;
    String file;
    int line;
    std::unordered_map<String, Value> locals;
    std::unordered_map<String, Value> upvalues;
};

// ═══════════════════════════════════════════════════════════════════
//  调试器
// ═══════════════════════════════════════════════════════════════════

class Debugger {
public:
    Debugger(VM* vm);
    
    // 生命周期
    bool initialize();
    void shutdown();
    bool isActive() const { return active_; }
    
    // 断点管理
    int addBreakpoint(const String& file, int line);
    bool removeBreakpoint(int id);
    bool enableBreakpoint(int id, bool enable);
    void clearBreakpoints();
    const std::unordered_map<int, Breakpoint>& getBreakpoints() const { return breakpoints_; }
    
    // 监视管理
    int addWatch(const String& expression);
    bool removeWatch(int id);
    void clearWatches();
    
    // 执行控制
    void continueExecution();
    void pauseExecution();
    void stepOver();
    void stepInto();
    void stepOut();
    void runToCursor(const String& file, int line);
    
    // 堆栈检查
    std::vector<DebugFrame> getCallStack() const;
    Value evaluateExpression(const String& expr);
    std::unordered_map<String, Value> getVariables(int frameIndex = 0) const;
    
    // 事件处理
    using EventCallback = std::function<void(DebugEvent, const String&)>;
    void setEventCallback(EventCallback callback) { eventCallback_ = callback; }
    
    // 内部接口（由 VM 调用）
    void onInstruction(const String& file, int line);
    void onFunctionEntry(const String& name);
    void onFunctionExit(const String& name);
    void onException(const String& message);
    
    // 调试协议（DAP 支持）
    String handleDAPRequest(const String& jsonRequest);

private:
    [[maybe_unused]] VM* vm_;
    bool active_ = false;
    int nextBreakpointId_ = 1;
    int nextWatchId_ = 1;
    
    std::unordered_map<int, Breakpoint> breakpoints_;
    std::unordered_map<int, Watch> watches_;
    
    // 执行状态
    enum class StepMode {
        None,
        Over,
        Into,
        Out
    };
    StepMode stepMode_ = StepMode::None;
    int stepDepth_ = 0;
    
    // 当前位置
    String currentFile_;
    int currentLine_ = 0;
    int callDepth_ = 0;
    
    EventCallback eventCallback_;
    
    // 内部方法
    bool shouldBreak(const String& file, int line);
    bool checkBreakpointCondition(const Breakpoint& bp);
    void notifyEvent(DebugEvent event, const String& info = "");
    void updateWatches();
};

// ═══════════════════════════════════════════════════════════════════
//  调试服务器（远程调试）
// ═══════════════════════════════════════════════════════════════════

class DebugServer {
public:
    DebugServer(Debugger* debugger, VM* vm);
    ~DebugServer();

    bool start(int port = 4711);
    void stop();
    bool isRunning() const { return running_; }

    // 由 VM 调用：检查是否应在此指令处暂停
    bool shouldPause(const String& file, int line);
    // 暂停并等待客户端命令
    void waitForCommand();
    // 处理一个客户端请求（非阻塞）
    void poll();

private:
    Debugger* debugger_;
    VM* vm_;
    bool running_ = false;
    int port_ = 4711;
    int serverSocket_ = -1;
    int clientSocket_ = -1;

    // 待处理的命令
    String pendingCommand_;
    bool paused_ = false;

    String readLine(int sock);
    void sendResponse(const String& json);
    void handleCommand(const String& json);
};

// ═══════════════════════════════════════════════════════════════════
//  调试命令
// ═══════════════════════════════════════════════════════════════════

enum class DebugCommand {
    Continue,       // c / continue
    StepOver,       // n / next
    StepInto,       // s / step
    StepOut,        // finish
    Break,          // b / break
    Delete,         // d / delete
    Info,           // i / info
    Print,          // p / print
    Backtrace,      // bt / backtrace
    Frame,          // f / frame
    List,           // l / list
    Quit,           // q / quit
    Help            // h / help
};

// ═══════════════════════════════════════════════════════════════════
//  命令行调试器
// ═══════════════════════════════════════════════════════════════════

class CLIDebugger {
public:
    CLIDebugger(Debugger* debugger);
    
    void run();  // 启动交互式调试会话
    bool executeCommand(const String& cmd);

private:
    Debugger* debugger_;
    
    void showPrompt();
    void showHelp();
    void showSource(const String& file, int line, int context = 5);
    void showBacktrace();
    void showVariables();
};

} // namespace cplang
