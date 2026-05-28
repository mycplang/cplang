// 调试器实现

#include "debug/debugger.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  Debugger 实现
// ═══════════════════════════════════════════════════════════════════

Debugger::Debugger(VM* vm) : vm_(vm) {}

bool Debugger::initialize() {
    active_ = true;
    std::cout << "[Debugger] 调试器已启动" << std::endl;
    return true;
}

void Debugger::shutdown() {
    active_ = false;
    std::cout << "[Debugger] 调试器已停止" << std::endl;
}

int Debugger::addBreakpoint(const String& file, int line) {
    int id = nextBreakpointId_++;
    breakpoints_[id] = Breakpoint(file, line);
    std::cout << "[Debugger] 断点 " << id << " 已设置: " << file << ":" << line << std::endl;
    return id;
}

bool Debugger::removeBreakpoint(int id) {
    auto it = breakpoints_.find(id);
    if (it != breakpoints_.end()) {
        breakpoints_.erase(it);
        std::cout << "[Debugger] 断点 " << id << " 已删除" << std::endl;
        return true;
    }
    return false;
}

bool Debugger::enableBreakpoint(int id, bool enable) {
    auto it = breakpoints_.find(id);
    if (it != breakpoints_.end()) {
        it->second.enabled = enable;
        std::cout << "[Debugger] 断点 " << id 
                  << (enable ? " 已启用" : " 已禁用") << std::endl;
        return true;
    }
    return false;
}

void Debugger::clearBreakpoints() {
    breakpoints_.clear();
    std::cout << "[Debugger] 所有断点已清除" << std::endl;
}

int Debugger::addWatch(const String& expression) {
    int id = nextWatchId_++;
    Watch watch;
    watch.expression = expression;
    watches_[id] = watch;
    std::cout << "[Debugger] 监视 " << id << " 已添加: " << expression << std::endl;
    return id;
}

bool Debugger::removeWatch(int id) {
    auto it = watches_.find(id);
    if (it != watches_.end()) {
        watches_.erase(it);
        return true;
    }
    return false;
}

void Debugger::clearWatches() {
    watches_.clear();
}

void Debugger::continueExecution() {
    stepMode_ = StepMode::None;
    std::cout << "[Debugger] 继续执行..." << std::endl;
}

void Debugger::pauseExecution() {
    std::cout << "[Debugger] 执行已暂停" << std::endl;
    notifyEvent(DebugEvent::BreakpointHit, "手动暂停");
}

void Debugger::stepOver() {
    stepMode_ = StepMode::Over;
    stepDepth_ = callDepth_;
    std::cout << "[Debugger] 单步跳过..." << std::endl;
}

void Debugger::stepInto() {
    stepMode_ = StepMode::Into;
    std::cout << "[Debugger] 单步进入..." << std::endl;
}

void Debugger::stepOut() {
    stepMode_ = StepMode::Out;
    stepDepth_ = callDepth_;
    std::cout << "[Debugger] 单步跳出..." << std::endl;
}

void Debugger::runToCursor(const String& file, int line) {
    addBreakpoint(file, line);
    continueExecution();
}

std::vector<DebugFrame> Debugger::getCallStack() const {
    std::vector<DebugFrame> stack;
    // 从 VM 获取调用栈
    return stack;
}

Value Debugger::evaluateExpression(const String& /*expr*/) {
    // 在 VM 上下文中求值表达式
    // 需要实现表达式解析和求值
    return Value::nil();
}

std::unordered_map<String, Value> Debugger::getVariables(int /*frameIndex*/) const {
    std::unordered_map<String, Value> vars;
    // 从指定帧获取变量
    return vars;
}

void Debugger::onInstruction(const String& file, int line) {
    if (!active_) return;
    
    currentFile_ = file;
    currentLine_ = line;
    
    // 检查是否应该中断
    if (shouldBreak(file, line)) {
        notifyEvent(DebugEvent::BreakpointHit, file + ":" + std::to_string(line));
    }
    
    // 更新监视
    updateWatches();
}

void Debugger::onFunctionEntry(const String& name) {
    callDepth_++;
    if (stepMode_ == StepMode::Into) {
        notifyEvent(DebugEvent::StepInto, name);
    }
}

void Debugger::onFunctionExit(const String& name) {
    callDepth_--;
    if (stepMode_ == StepMode::Out && callDepth_ < stepDepth_) {
        notifyEvent(DebugEvent::StepOut, name);
    }
}

void Debugger::onException(const String& message) {
    notifyEvent(DebugEvent::ExceptionThrown, message);
}

String Debugger::handleDAPRequest(const String& /*jsonRequest*/) {
    // 处理 Debug Adapter Protocol 请求
    // 返回 JSON 响应
    return "{}";
}

bool Debugger::shouldBreak(const String& file, int line) {
    // 检查断点
    for (const auto& [id, bp] : breakpoints_) {
        if (bp.enabled && bp.file == file && bp.line == line) {
            if (checkBreakpointCondition(bp)) {
                return true;
            }
        }
    }
    
    // 检查单步模式
    switch (stepMode_) {
        case StepMode::Over:
            if (callDepth_ <= stepDepth_) {
                return true;
            }
            break;
        case StepMode::Into:
            return true;
        case StepMode::Out:
            if (callDepth_ < stepDepth_) {
                return true;
            }
            break;
        default:
            break;
    }
    
    return false;
}

bool Debugger::checkBreakpointCondition(const Breakpoint& bp) {
    // 检查命中次数
    if (bp.hitCondition > 0 && bp.hitCount < bp.hitCondition) {
        return false;
    }
    
    // 检查条件表达式
    if (!bp.condition.empty()) {
        // 求值条件表达式
        // Value result = evaluateExpression(bp.condition);
        // return result.isTrue();
    }
    
    return true;
}

void Debugger::notifyEvent(DebugEvent event, const String& info) {
    if (eventCallback_) {
        eventCallback_(event, info);
    }
    
    // 输出到控制台
    switch (event) {
        case DebugEvent::BreakpointHit:
            std::cout << "[Debugger] 命中断点: " << info << std::endl;
            break;
        case DebugEvent::ExceptionThrown:
            std::cout << "[Debugger] 异常: " << info << std::endl;
            break;
        default:
            break;
    }
}

void Debugger::updateWatches() {
    for (auto& [id, watch] : watches_) {
        if (!watch.enabled) continue;
        
        Value current = evaluateExpression(watch.expression);
        String currentStr = current.toString();
        
        if (currentStr != watch.lastValue) {
            std::cout << "[Debugger] 监视 " << id << " 变化: " 
                      << watch.expression << " = " << currentStr << std::endl;
            watch.lastValue = currentStr;
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
//  CLIDebugger 实现
// ═══════════════════════════════════════════════════════════════════

CLIDebugger::CLIDebugger(Debugger* debugger) : debugger_(debugger) {}

void CLIDebugger::run() {
    std::cout << "\n╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║        CP 语言调试器                                ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "命令: c(继续) n(跳过) s(进入) finish(跳出)\n";
    std::cout << "       b file:line(断点) d id(删除) p expr(打印)\n";
    std::cout << "       bt(堆栈) q(退出) h(帮助)\n\n";
    
    String cmd;
    while (true) {
        showPrompt();
        std::getline(std::cin, cmd);
        
        if (cmd == "q" || cmd == "quit") {
            break;
        }
        
        executeCommand(cmd);
    }
}

bool CLIDebugger::executeCommand(const String& cmd) {
    if (cmd.empty()) return true;
    
    std::istringstream iss(cmd);
    String op;
    iss >> op;
    
    if (op == "c" || op == "continue") {
        debugger_->continueExecution();
    }
    else if (op == "n" || op == "next") {
        debugger_->stepOver();
    }
    else if (op == "s" || op == "step") {
        debugger_->stepInto();
    }
    else if (op == "finish") {
        debugger_->stepOut();
    }
    else if (op == "b" || op == "break") {
        String location;
        iss >> location;
        size_t pos = location.find(':');
        if (pos != String::npos) {
            String file = location.substr(0, pos);
            int line = std::stoi(location.substr(pos + 1));
            debugger_->addBreakpoint(file, line);
        }
    }
    else if (op == "d" || op == "delete") {
        int id;
        iss >> id;
        debugger_->removeBreakpoint(id);
    }
    else if (op == "p" || op == "print") {
        String expr;
        std::getline(iss, expr);
        // 去除前导空格
        expr.erase(0, expr.find_first_not_of(" "));
        Value result = debugger_->evaluateExpression(expr);
        std::cout << result.toString() << std::endl;
    }
    else if (op == "bt" || op == "backtrace") {
        showBacktrace();
    }
    else if (op == "l" || op == "list") {
        // showSource(currentFile, currentLine);
    }
    else if (op == "h" || op == "help") {
        showHelp();
    }
    else {
        std::cout << "未知命令: " << op << std::endl;
    }
    
    return true;
}

void CLIDebugger::showPrompt() {
    std::cout << "(cplang-dbg) ";
}

void CLIDebugger::showHelp() {
    std::cout << "\n调试命令:\n";
    std::cout << "  c, continue    - 继续执行\n";
    std::cout << "  n, next        - 单步跳过\n";
    std::cout << "  s, step        - 单步进入\n";
    std::cout << "  finish         - 单步跳出\n";
    std::cout << "  b, break loc   - 设置断点 (file:line)\n";
    std::cout << "  d, delete id   - 删除断点\n";
    std::cout << "  p, print expr  - 打印表达式值\n";
    std::cout << "  bt, backtrace  - 显示调用堆栈\n";
    std::cout << "  l, list        - 显示源代码\n";
    std::cout << "  h, help        - 显示帮助\n";
    std::cout << "  q, quit        - 退出调试器\n\n";
}

void CLIDebugger::showSource(const String& file, int line, int /*context*/) {
    // 读取并显示源代码
    std::cout << "显示 " << file << " 第 " << line << " 行附近代码\n";
}

void CLIDebugger::showBacktrace() {
    auto stack = debugger_->getCallStack();
    std::cout << "\n调用堆栈:\n";
    int frameNum = 0;
    for (const auto& frame : stack) {
        std::cout << "#" << frameNum++ << " " << frame.functionName;
        if (!frame.file.empty()) {
            std::cout << " at " << frame.file << ":" << frame.line;
        }
        std::cout << std::endl;
    }
}

void CLIDebugger::showVariables() {
    auto vars = debugger_->getVariables(0);
    std::cout << "\n变量:\n";
    for (const auto& [name, value] : vars) {
        std::cout << "  " << name << " = " << value.toString() << std::endl;
    }
}

} // namespace cplang
