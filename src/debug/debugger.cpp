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
    if (!vm_) return stack;

    // 从 VM 的调用帧栈读取帧信息（从底到顶）
    for (size_t i = 0; i < vm_->frames_.size(); i++) {
        const auto& cf = vm_->frames_[i];
        DebugFrame df;
        if (cf.func && cf.func->name) {
            df.functionName = std::string(cf.func->name->data, cf.func->name->length);
        } else {
            df.functionName = "<main>";
        }

        // 从函数源码文件名获取文件信息
        if (cf.func && !cf.func->sourceFile.empty()) {
            df.file = cf.func->sourceFile;
        }

        // 尝试从函数的 lineInfo 获取当前行号
        if (cf.func && !cf.func->lineInfo.empty()) {
            // 查找当前 PC 对应的行（frame 中没有直接存储 PC，使用近似值）
            // 用 returnPcOffset 近似定位
            size_t idx = (size_t)(cf.returnPcOffset / 16);
            if (idx < cf.func->lineInfo.size()) {
                df.line = cf.func->lineInfo[idx];
            } else if (!cf.func->lineInfo.empty()) {
                df.line = cf.func->lineInfo.back();
            }
        }

        stack.push_back(df);
    }

    // 栈顶是当前正在执行的函数 — 使用 VM 的 currentLine
    if (!stack.empty() && currentLine_ > 0) {
        stack.back().line = currentLine_;
        stack.back().file = currentFile_.empty() ? stack.back().file : currentFile_;
    }

    return stack;
}

Value Debugger::evaluateExpression(const String& expr) {
    if (!vm_) return Value::nil();

    // 简单表达式求值：
    // 1. 字面量: 整数、浮点数、字符串、true/false、nil
    // 2. 变量名: 在全局和当前作用域中查找
    // 3. 简单的成员访问: var.field

    // 去除首尾空白
    String trimmed = expr;
    trimmed.erase(0, trimmed.find_first_not_of(" \t"));
    trimmed.erase(trimmed.find_last_not_of(" \t") + 1);

    if (trimmed.empty()) return Value::nil();

    // true / false / nil
    if (trimmed == "真" || trimmed == "true") return Value::Bool(true);
    if (trimmed == "假" || trimmed == "false") return Value::Bool(false);
    if (trimmed == "空" || trimmed == "nil") return Value::nil();

    // 整数
    bool isInt = !trimmed.empty();
    for (char c : trimmed) {
        if (c == '-' && &c == &trimmed[0]) continue;  // leading minus
        if (c < '0' || c > '9') { isInt = false; break; }
    }
    if (isInt) return Value::Int(std::stoll(trimmed));

    // 浮点数
    bool isFloat = true;
    int dotCount = 0;
    for (size_t i = 0; i < trimmed.size(); i++) {
        char c = trimmed[i];
        if (c == '-' && i == 0) continue;
        if (c == '.') { dotCount++; continue; }
        if (c < '0' || c > '9') { isFloat = false; break; }
    }
    if (isFloat && dotCount == 1) return Value::fromFloat(std::stod(trimmed));

    // 字符串字面量 "..." 或 '...'
    if ((trimmed.front() == '"' && trimmed.back() == '"') ||
        (trimmed.front() == '\'' && trimmed.back() == '\'')) {
        String content = trimmed.substr(1, trimmed.size() - 2);
        return makeStringVal(VMString::create(content));
    }

    // 成员访问: obj.field
    size_t dotPos = trimmed.find('.');
    if (dotPos != String::npos) {
        String objName = trimmed.substr(0, dotPos);
        String fieldName = trimmed.substr(dotPos + 1);
        Value obj = evaluateExpression(objName);
        if (obj.asPtr() && obj.asPtr()->typeTag == ObjectHeader::TAG_TABLE) {
            auto* tbl = static_cast<VMTable*>(obj.asPtr());
            return tbl->get(makeStringVal(VMString::create(fieldName)));
        }
        if (obj.asPtr() && obj.asPtr()->typeTag == ObjectHeader::TAG_INSTANCE) {
            auto* inst = static_cast<VMInstance*>(obj.asPtr());
            if (inst->cls) {
                for (size_t i = 0; i < inst->cls->fieldNames.size(); i++) {
                    auto* fn = inst->cls->fieldNames[i];
                    if (fn && std::string(fn->data, fn->length) == fieldName) {
                        return inst->getField(static_cast<Int32>(i));
                    }
                }
            }
        }
        return Value::nil();
    }

    // 变量查找: 在 VM 全局变量中查找
    auto varIt = vm_->globals_.find(trimmed);
    if (varIt != vm_->globals_.end()) {
        return varIt->second;
    }

    // 在 slot 表中查找
    auto slotIt = vm_->globalNameToSlot_.find(trimmed);
    if (slotIt != vm_->globalNameToSlot_.end()) {
        UInt16 slot = slotIt->second;
        if (slot < vm_->globalSlots_.size()) {
            return vm_->globalSlots_[slot];
        }
    }

    return Value::nil();
}

std::unordered_map<String, Value> Debugger::getVariables(int frameIndex) const {
    std::unordered_map<String, Value> vars;
    if (!vm_) return vars;

    // 添加全局变量
    for (const auto& [name, val] : vm_->globals_) {
        if (val.isFunction() || val.isCFunction() || val.isClosure()) {
            continue;  // 跳过函数，只显示数据变量
        }
        vars[name] = val;
    }

    // 如果指定了帧索引，尝试读取该帧的局部变量
    if (frameIndex >= 0 && (size_t)frameIndex < vm_->frames_.size()) {
        const auto& cf = vm_->frames_[frameIndex];
        if (cf.func && cf.base) {
            // 读取函数参数和局部变量
            // 参数从 base[0] 开始
            for (UInt32 i = 0; i < cf.func->numParams; i++) {
                Value paramVal = cf.base[i];
                if (!paramVal.isNil()) {
                    // 尝试从函数签名获取参数名
                    String paramName = "参数" + std::to_string(i);
                    vars[paramName] = paramVal;
                }
            }
            // 局部变量从 base[numParams] 开始
            for (UInt32 i = cf.func->numParams; i < cf.func->numLocals + cf.func->numParams && i < 256; i++) {
                Value localVal = cf.base[i];
                if (!localVal.isNil()) {
                    String localName = "局部" + std::to_string(i - cf.func->numParams);
                    vars[localName] = localVal;
                }
            }
        }
    }

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

// ═══════════════════════════════════════════════════════════════════
//  DebugServer 实现 — TCP 调试协议
// ═══════════════════════════════════════════════════════════════════

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  using socklen_t = int;
  #define CLOSE_SOCKET closesocket
  #define SOCKET_ERRNO WSAGetLastError()
  static bool initWinsock() {
      WSADATA wsa;
      return WSAStartup(MAKEWORD(2,2), &wsa) == 0;
  }
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <errno.h>
  #define CLOSE_SOCKET close
  #define SOCKET_ERRNO errno
  static bool initWinsock() { return true; }
#endif

DebugServer::DebugServer(Debugger* debugger, VM* vm)
    : debugger_(debugger), vm_(vm) {}

DebugServer::~DebugServer() { stop(); }

bool DebugServer::start(int port) {
    if (running_) return true;
    if (!initWinsock()) return false;

    port_ = port;
    serverSocket_ = (int)socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) return false;

    int reuse = 1;
    setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((unsigned short)port);

    if (bind(serverSocket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        CLOSE_SOCKET(serverSocket_); serverSocket_ = -1; return false;
    }
    if (listen(serverSocket_, 1) < 0) {
        CLOSE_SOCKET(serverSocket_); serverSocket_ = -1; return false;
    }

    // 设置为非阻塞
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(serverSocket_, FIONBIO, &mode);
#else
    fcntl(serverSocket_, F_SETFL, O_NONBLOCK);
#endif

    running_ = true;
    std::cout << "[DebugServer] 监听端口 " << port << "，等待客户端连接..." << std::endl;
    return true;
}

void DebugServer::stop() {
    running_ = false;
    paused_ = false;
    if (clientSocket_ >= 0) { CLOSE_SOCKET(clientSocket_); clientSocket_ = -1; }
    if (serverSocket_ >= 0) { CLOSE_SOCKET(serverSocket_); serverSocket_ = -1; }
}

void DebugServer::poll() {
    if (!running_) return;

    // 接受新连接
    if (clientSocket_ < 0) {
        struct sockaddr_in clientAddr;
        socklen_t len = sizeof(clientAddr);
        int client = (int)accept(serverSocket_, (struct sockaddr*)&clientAddr, &len);
        if (client >= 0) {
            clientSocket_ = client;
            std::cout << "[DebugServer] 客户端已连接" << std::endl;
            sendResponse("{\"type\":\"connected\"}");
        }
        return;
    }

    // 读取客户端命令
    String line = readLine(clientSocket_);
    if (!line.empty()) {
        handleCommand(line);
    }

    // 检查客户端断开
    if (line.empty() && clientSocket_ >= 0) {
        // 用 peek 检查连接状态
        char c;
        int r = recv(clientSocket_, &c, 1, MSG_PEEK);
        if (r == 0 || (r < 0 && SOCKET_ERRNO != EWOULDBLOCK && SOCKET_ERRNO != EAGAIN)) {
            std::cout << "[DebugServer] 客户端已断开" << std::endl;
            CLOSE_SOCKET(clientSocket_); clientSocket_ = -1;
            paused_ = false;
        }
    }
}

bool DebugServer::shouldPause(const String& file, int line) {
    if (!running_ || clientSocket_ < 0) return false;

    // 检查是否有断点
    if (debugger_) {
        for (const auto& [id, bp] : debugger_->getBreakpoints()) {
            if (bp.enabled && bp.file == file && bp.line == line) {
                // 发送暂停事件
                std::ostringstream oss;
                oss << "{\"type\":\"paused\",\"reason\":\"breakpoint\",\"file\":\""
                    << file << "\",\"line\":" << line << ",\"breakpointId\":" << id << "}";
                sendResponse(oss.str());
                paused_ = true;
                return true;
            }
        }
    }
    return false;
}

void DebugServer::waitForCommand() {
    if (!paused_ || clientSocket_ < 0) return;

    // 阻塞等待命令
    while (paused_ && running_) {
        String line = readLine(clientSocket_);
        if (line.empty()) {
            // 检查连接
            char c;
            int r = recv(clientSocket_, &c, 1, MSG_PEEK);
            if (r <= 0 && SOCKET_ERRNO != EWOULDBLOCK && SOCKET_ERRNO != EAGAIN) {
                paused_ = false;
                break;
            }
#ifdef _WIN32
            Sleep(50);
#else
            usleep(50000);
#endif
            continue;
        }
        handleCommand(line);
    }
}

String DebugServer::readLine(int sock) {
    String result;
    char c;
    while (true) {
        int n = recv(sock, &c, 1, 0);
        if (n <= 0) return result; // 无数据或错误
        if (c == '\n') break;
        if (c != '\r') result += c;
    }
    return result;
}

void DebugServer::sendResponse(const String& json) {
    if (clientSocket_ < 0) return;
    String msg = json + "\n";
    send(clientSocket_, msg.c_str(), (int)msg.size(), 0);
}

void DebugServer::handleCommand(const String& json) {
    // 简单 JSON 解析（提取 cmd 字段）
    auto extractStr = [](const String& s, const String& key) -> String {
        size_t pos = s.find("\"" + key + "\"");
        if (pos == String::npos) return "";
        pos = s.find(":", pos);
        if (pos == String::npos) return "";
        pos = s.find("\"", pos);
        if (pos == String::npos) return "";
        size_t end = s.find("\"", pos + 1);
        if (end == String::npos) return "";
        return s.substr(pos + 1, end - pos - 1);
    };

    String cmd = extractStr(json, "cmd");

    if (cmd == "continue") {
        paused_ = false;
        sendResponse("{\"type\":\"continued\"}");
    }
    else if (cmd == "stepOver") {
        if (debugger_) debugger_->stepOver();
        paused_ = false;
        sendResponse("{\"type\":\"continued\"}");
    }
    else if (cmd == "stepInto") {
        if (debugger_) debugger_->stepInto();
        paused_ = false;
        sendResponse("{\"type\":\"continued\"}");
    }
    else if (cmd == "stepOut") {
        if (debugger_) debugger_->stepOut();
        paused_ = false;
        sendResponse("{\"type\":\"continued\"}");
    }
    else if (cmd == "getStack") {
        auto stack = debugger_ ? debugger_->getCallStack() : std::vector<DebugFrame>{};
        std::ostringstream oss;
        oss << "{\"type\":\"stack\",\"frames\":[";
        for (size_t i = 0; i < stack.size(); i++) {
            if (i > 0) oss << ",";
            oss << "{\"name\":\"" << stack[i].functionName << "\""
                << ",\"file\":\"" << stack[i].file << "\""
                << ",\"line\":" << stack[i].line << "}";
        }
        oss << "]}";
        sendResponse(oss.str());
    }
    else if (cmd == "getVars") {
        auto vars = debugger_ ? debugger_->getVariables(0) : std::unordered_map<String, Value>{};
        std::ostringstream oss;
        oss << "{\"type\":\"variables\",\"vars\":{";
        bool first = true;
        for (const auto& [name, val] : vars) {
            if (!first) oss << ",";
            first = false;
            oss << "\"" << name << "\":\"" << val.toString() << "\"";
        }
        oss << "}}";
        sendResponse(oss.str());
    }
    else if (cmd == "setBreakpoints") {
        // 解析: {"cmd":"setBreakpoints","file":"...","lines":[1,2,3]}
        String file = extractStr(json, "file");
        if (debugger_ && !file.empty()) {
            debugger_->clearBreakpoints();
            // 简单解析 lines 数组
            size_t arrStart = json.find("\"lines\"");
            if (arrStart != String::npos) {
                arrStart = json.find("[", arrStart);
                if (arrStart != String::npos) {
                    size_t arrEnd = json.find("]", arrStart);
                    String arr = json.substr(arrStart + 1, arrEnd - arrStart - 1);
                    std::istringstream iss(arr);
                    String token;
                    while (std::getline(iss, token, ',')) {
                        // 去除空白
                        token.erase(0, token.find_first_not_of(" \t"));
                        if (!token.empty()) {
                            debugger_->addBreakpoint(file, std::stoi(token));
                        }
                    }
                }
            }
        }
        sendResponse("{\"type\":\"breakpointsSet\"}");
    }
    else if (cmd == "evaluate") {
        String expr = extractStr(json, "expr");
        Value result = debugger_ ? debugger_->evaluateExpression(expr) : Value::nil();
        sendResponse("{\"type\":\"evaluateResult\",\"value\":\"" + result.toString() + "\"}");
    }
    else {
        sendResponse("{\"type\":\"error\",\"message\":\"unknown command: " + cmd + "\"}");
    }
}

} // namespace cplang
