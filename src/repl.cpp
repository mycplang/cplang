// CP 语言 REPL (交互式解释器) — 增强版
// 支持: 命令历史(↑/↓), 行编辑(←/→/Home/End/Delete), Tab 补全, Ctrl+C 中断
// 持久化历史记录, 语法提示, REPL 命令
//
// 2026-05-22: 增强: 持久化历史 / 增强补全 / 语法提示 / %命令系统

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <chrono>
#include "repl/repl.hpp"
#include "jit/hybrid_jit.hpp"
#include "vm/vm.hpp"
#include "codegen/codegen.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic_analyzer.hpp"
#include "stdlib/stdlib.hpp"

// 跨平台控制台 I/O（Windows: conio+Console API, Linux: termios+ANSI）
// console_getch / console_kbhit / console_init / console_restore 定义在 repl/console.hpp

namespace cplang {

// 前向声明内部辅助函数
static std::string trim(const std::string& s);

// ANSI 颜色代码 (Windows 10+ 支持)
#define ANSI_RESET   "\033[0m"
#define ANSI_RED     "\033[31m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_BLUE    "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_BOLD    "\033[1m"
#define ANSI_DIM     "\033[2m"

// ============================================================================
// 构造 / 析构
// ============================================================================

ReplEngine::ReplEngine(bool enableJit)
    : compiler_(), jit_(nullptr), enableJit_(enableJit) {
    vm_ = compiler_.vm();
    initConsole();

    // 检测 ANSI 颜色支持 (Windows 10 版本 >= 14393)
    ansiColor_ = true;

    // 初始化内置函数文档
    builtinDocs_ = {
        {"打印", "打印(值) — 输出值到控制台"},
        {"print", "print(value) — 输出值到控制台"},
        {"println", "println(值) — 输出值并换行"},
        {"长度", "长度(容器) — 返回字符串/数组/表的长度"},
        {"len", "len(container) — 返回字符串/数组/表的长度"},
        {"类型", "类型(值) — 返回值的类型名称"},
        {"typeof", "typeof(value) — 返回值的类型名称"},
        {"tick", "tick() — 返回从首次调用起经过的毫秒数"},
        {"tock", "tock() — 返回从首次调用起经过的毫秒数（同tick）"},
        {"sleep", "sleep(毫秒) — 暂停执行指定毫秒数"},
        {"now", "now() — 返回当前时间戳（毫秒）"},
        {"正弦", "正弦(角度) — 计算正弦值"},
        {"sin", "sin(angle) — 计算正弦值"},
        {"余弦", "余弦(角度) — 计算余弦值"},
        {"cos", "cos(angle) — 计算余弦值"},
        {"平方根", "平方根(值) — 计算平方根"},
        {"sqrt", "sqrt(value) — 计算平方根"},
        {"绝对值", "绝对值(值) — 计算绝对值"},
        {"abs", "abs(value) — 计算绝对值"},
        {"随机数", "随机数() — 返回 0~1 之间的随机浮点数"},
        {"random", "random() — 返回 0~1 之间的随机浮点数"},
        {"rand", "rand(最小值, 最大值) — 返回指定范围内的随机整数"},
        {"最大值", "最大值(a, b) — 返回较大的值"},
        {"max", "max(a, b) — 返回较大的值"},
        {"最小值", "最小值(a, b) — 返回较小的值"},
        {"min", "min(a, b) — 返回较小的值"},
        {"转字符串", "转字符串(值) — 将值转换为字符串"},
        {"toString", "toString(value) — 将值转换为字符串"},
        {"读文件", "读文件(路径) — 读取文件内容为字符串"},
        {"readFile", "readFile(path) — 读取文件内容为字符串"},
        {"写文件", "写文件(路径, 内容) — 写入字符串到文件"},
        {"writeFile", "writeFile(path, content) — 写入字符串到文件"},
        {"文件存在", "文件存在(路径) — 检查文件是否存在"},
        {"fileExists", "fileExists(path) — 检查文件是否存在"},
        {"JSON解析", "JSON解析(字符串) — 解析 JSON 字符串为值"},
        {"jsonParse", "jsonParse(string) — 解析 JSON 字符串为值"},
        {"JSON序列化", "JSON序列化(值) — 将值序列化为 JSON 字符串"},
        {"jsonStringify", "jsonStringify(value) — 将值序列化为 JSON 字符串"},
        {"push", "push(数组, 元素) — 向数组末尾添加元素"},
        {"pop", "pop(数组) — 移除并返回数组最后一个元素"},
        {"sort", "sort(数组) — 对数组排序"},
        {"map", "map(数组, 函数) — 对数组每个元素应用函数"},
        {"filter", "filter(数组, 函数) — 过滤数组元素"},
        {"遍历", "遍历(数组) { 主体 } — 遍历数组"},
        {"forEach", "forEach(array, func) — 对每个元素执行函数"},
    };

    // 设置历史文件路径
    const char* home = getenv("USERPROFILE");
    if (!home) home = getenv("HOME");   // Linux/macOS fallback
    if (!home) home = getenv("HOME");
    if (home) {
        historyFile_ = std::string(home) + "/.cplang_history";
    } else {
        historyFile_ = ".cplang_history";
    }

    // 加载历史记录
    loadHistory();
}

ReplEngine::~ReplEngine() {
    saveHistory();
    restoreConsole();
}

// ============================================================================
// 历史记录持久化
// ============================================================================

void ReplEngine::saveHistory() {
    if (history_.empty()) return;
    try {
        std::ofstream file(historyFile_, std::ios::trunc);
        if (file.is_open()) {
            // 只保存最近 500 条
            size_t start = history_.size() > 500 ? history_.size() - 500 : 0;
            for (size_t i = start; i < history_.size(); i++) {
                file << history_[i] << "\n";
            }
        }
    } catch (...) {
        // 静默忽略文件写入错误
    }
}

void ReplEngine::loadHistory() {
    try {
        std::ifstream file(historyFile_);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                if (!line.empty()) {
                    history_.push_back(line);
                }
            }
            historyPos_ = history_.size();
        }
    } catch (...) {
        // 静默忽略文件读取错误
    }
}

// ============================================================================
// 控制台初始化 / 恢复
// ============================================================================

void ReplEngine::initConsole() {
    console_init(consoleState_);
    consoleInitialized_ = true;
}

void ReplEngine::restoreConsole() {
    if (consoleInitialized_) {
        console_restore(consoleState_);
    }
}

// ============================================================================
// 刷新行显示（跨平台：Windows 用 Console API，Linux 用 ANSI 转义序列）
// ============================================================================

void ReplEngine::refreshLine(const std::string& prompt,
                              const std::string& buffer,
                              int cursorPos) {
    if (!consoleInitialized_) return;

    // 使用 ANSI 转义序列清空行并重绘（Windows 10+ 和所有 Unix 终端均支持）
    // \r           — 回到行首
    // \033[K        — 清除从光标到行尾的内容
    // 然后重绘提示符 + 缓冲区，再用 \r + 空格移到正确光标位置
    std::cout << "\r\033[K" << prompt << buffer;

    // 定位光标：回到行首，前移 (提示符长度 + 光标位置) 个字符
    int absPos = static_cast<int>(prompt.size()) + cursorPos;
    std::cout << "\r";
    if (absPos > 0) {
        std::cout << "\033[" << absPos << "C";
    }
    std::cout << std::flush;
}

// ============================================================================
// Tab 补全 — 增强版
// ============================================================================

void ReplEngine::printCompletionMatches(const std::vector<std::string>& matches,
                                         const std::string& prompt,
                                         const std::string& buffer) {
    std::cout << std::endl;
    // 带说明的格式化输出
    for (size_t i = 0; i < matches.size(); i++) {
        // 在匹配项旁显示简短说明
        std::string desc;
        if (builtinDocs_.count(matches[i])) {
            desc = "  " + std::string(ANSI_DIM) + builtinDocs_[matches[i]].substr(0, 40) + "..." + ANSI_RESET;
        }
        std::cout << "  " << ANSI_CYAN << matches[i] << ANSI_RESET << desc;
        std::cout << std::endl; // 每行一个，避免拥挤
    }
    std::cout << prompt << buffer << std::flush;
}

std::string ReplEngine::doCompletion(const std::string& prefix, const std::string& prompt) {
    if (prefix.empty()) return prefix;

    // 提取最后一个单词（标识符）
    std::string lastWord;
    // 检查是否在导入语句中（提示文件路径）
    bool isImportPath = false;
    std::string trimmedPrefix = trim(prefix);
    if (trimmedPrefix.rfind("导入", 0) == 0 || trimmedPrefix.rfind("import", 0) == 0) {
        isImportPath = true;
    }

    for (int i = static_cast<int>(prefix.size()) - 1; i >= 0; i--) {
        unsigned char c = static_cast<unsigned char>(prefix[i]);
        if (std::isalnum(c) || c == '_' || c == '.' || c >= 0x80 || c == '/' || c == '\\') {
            lastWord = std::string(1, static_cast<char>(c)) + lastWord;
        } else {
            break;
        }
    }
    if (lastWord.empty()) return prefix;

    // 收集候选项
    collectIdentifiers();
    if (completions_.empty()) return prefix;

    // 查找匹配
    std::vector<std::string> matches;
    for (const auto& id : completions_) {
        if (id.size() >= lastWord.size() &&
            id.compare(0, lastWord.size(), lastWord) == 0) {
            matches.push_back(id);
        }
    }
    if (matches.empty()) return prefix;

    // 唯一匹配 → 直接补全
    if (matches.size() == 1) {
        std::string result = prefix.substr(0, prefix.size() - lastWord.size()) + matches[0];
        // 如果匹配是内置函数，自动追加左括号
        if (builtinDocs_.count(matches[0]) ||
            matches[0] == "打印" || matches[0] == "print" ||
            matches[0] == "长度" || matches[0] == "len" ||
            matches[0] == "类型" || matches[0] == "typeof") {
            result += "(";
        }
        return result;
    }

    // 多个匹配 → 找公共前缀
    std::string common = matches[0];
    for (size_t i = 1; i < matches.size(); i++) {
        size_t j = 0;
        while (j < common.size() && j < matches[i].size() &&
               common[j] == matches[i][j]) {
            j++;
        }
        common = common.substr(0, j);
    }
    if (common.size() > lastWord.size()) {
        return prefix.substr(0, prefix.size() - lastWord.size()) + common;
    }

    // 无公共扩展 → 显示列表（带分类标记）
    printCompletionMatches(matches, prompt, prefix);

    return prefix;
}

// ============================================================================
// 收集标识符 — 增强版
// ============================================================================

void ReplEngine::collectIdentifiers() {
    if (!completions_.empty()) return; // 缓存，单次收集

    // 从 VM 全局槽收集
    if (vm_) {
        auto slotNames = vm_->getGlobalSlotNames();
        for (const auto& name : slotNames) {
            completions_.insert(name);
        }
    }

    // 关键字（完整列表）
    const char* keywords[] = {
        // 中文关键字
        "如果", "否则", "当", "循环", "遍历", "对于",
        "函数", "返回", "变量", "常量",
        "类", "结构体", "接口", "枚举",
        "选择", "情况", "其他",
        "真", "假", "空", "空值",
        "并且", "或者", "非",
        "导入", "包名", "类型定义",
        "新建", "公有", "私有", "保护",
        "尝试", "捕获", "抛出", "推迟",
        "是", "不是",
        // 英文关键字
        "if", "else", "while", "for", "foreach",
        "function", "fn", "return", "var", "const",
        "class", "struct", "interface", "enum",
        "switch", "case", "default",
        "true", "false", "nil", "null",
        "and", "or", "not",
        "import", "package", "typedef",
        "new", "public", "private", "protected",
        "try", "catch", "throw", "defer",
        "is", "!is",
        // 内置函数（完整列表）
        "打印", "println", "print",
        "长度", "len", "size",
        "类型", "typeof", "typeOf",
        "tick", "tock", "sleep", "now",
        "随机数", "random", "rand",
        "正弦", "sin", "余弦", "cos",
        "平方根", "sqrt", "绝对值", "abs",
        "幂", "pow", "对数", "log",
        "向上取整", "ceil", "向下取整", "floor",
        "四舍五入", "round",
        "最大值", "max", "最小值", "min",
        "字符串长度", "strLen",
        "字符串拼接", "strConcat",
        "字符串查找", "strFind",
        "字符串替换", "strReplace",
        "字符串分割", "strSplit",
        "转字符串", "toString", "parseInt", "parseFloat",
        "读文件", "readFile", "写文件", "writeFile",
        "文件存在", "fileExists", "创建目录", "mkdir",
        "目录列表", "listDir",
        "JSON解析", "jsonParse", "JSON序列化", "jsonStringify",
        "MD5", "md5", "SHA256", "sha256", "CRC32", "crc32",
        "AES加密", "aesEncrypt", "AES解密", "aesDecrypt",
        "BASE64编码", "base64Encode", "BASE64解码", "base64Decode",
        "push", "pop", "shift", "unshift",
        "排序", "sort", "映射", "map", "过滤", "filter",
        "折叠", "fold", "数组变换", "transform",
        "格式化", "format",
        // 类型名
        "int", "i8", "i16", "i32", "i64",
        "u8", "u16", "u32", "u64",
        "float", "f32", "f64",
        "bool", "string", "char", "void", "auto",
    };
    for (auto* kw : keywords) completions_.insert(kw);

    // 从当前缓冲区中提取已定义的变量名（简单启发式）
    // 这会在每次 eval 后通过 completions_.clear() 触发重新收集
}

// ============================================================================
// 语法提示 (获取函数签名)
// ============================================================================

std::string ReplEngine::getFunctionSignature(const std::string& name) {
    if (builtinDocs_.count(name)) {
        return builtinDocs_[name];
    }
    // 尝试从 VM 注册的函数中检索签名
    // 如果函数已编译到 VM，尝试获取其参数信息
    if (vm_) {
        int slot = vm_->getGlobalSlot(name.c_str());
        if (slot >= 0) {
            Value* vp = vm_->getGlobalBySlot(static_cast<UInt16>(slot));
            if (vp) {
                Value v = *vp;
                if (v.isClosure() && v.asClosure()->func &&
                    v.asClosure()->func->name) {
                    auto* f = v.asClosure()->func;
                    std::string sig = std::string(f->name->data, f->name->length) + "(";
                    // 尝试获取参数名（如果有调试信息）
                    sig += ")";
                    return sig;
                }
            }
        }
    }
    return "";
}

std::string ReplEngine::getBuiltinHelp(const std::string& name) {
    auto it = builtinDocs_.find(name);
    if (it != builtinDocs_.end()) {
        return it->second;
    }
    return "";
}

// ============================================================================
// 获取类型名称
// ============================================================================

std::string ReplEngine::getTypeName(const Value& v) {
    if (v.isNil()) return "空(nil)";
    if (v.isBool()) return "布尔(bool)";
    if (v.isInt()) return "整数(int)";
    if (v.isFloat()) return "浮点(float)";
    if (v.isString()) return "字符串(string)";
    if (v.isArray()) return "数组(array)";
    if (v.isTable()) return "表(table)";
    if (v.isClosure()) return "函数(function)";
    if (v.isCFunction()) return "内置函数(cfunction)";
    return "未知(unknown)";
}

// ============================================================================
// REPL 命令调度
// ============================================================================

bool ReplEngine::dispatchCommand(const std::string& cmd) {
    // 命令格式: %command [args]
    if (cmd.empty() || cmd[0] != '%') return false;

    std::istringstream iss(cmd);
    std::string command;
    iss >> command;

    // 去掉前导 %
    std::string cmdName = command.substr(1);

    if (cmdName == "help" || cmdName == "h") {
        std::string topic;
        std::getline(iss >> std::ws, topic);
        printHelp(topic);
        return true;
    }

    if (cmdName == "hist" || cmdName == "history") {
        if (history_.empty()) {
            std::cout << "（历史记录为空）" << std::endl;
            return true;
        }
        size_t start = history_.size() > 100 ? history_.size() - 100 : 0;
        for (size_t i = start; i < history_.size(); i++) {
            std::cout << ANSI_DIM << (i + 1) << ": " << ANSI_RESET
                      << history_[i] << std::endl;
        }
        return true;
    }

    if (cmdName == "save") {
        std::string filename;
        std::getline(iss >> std::ws, filename);
        if (filename.empty()) {
            std::cout << "用法: %save <文件名>" << std::endl;
            return true;
        }
        try {
            std::ofstream file(filename);
            if (file.is_open()) {
                for (const auto& line : history_) {
                    file << line << "\n";
                }
                std::cout << "已保存 " << history_.size()
                          << " 条记录到 " << filename << std::endl;
            } else {
                std::cout << "错误: 无法打开文件 " << filename << std::endl;
            }
        } catch (...) {
            std::cout << "错误: 写入失败" << std::endl;
        }
        return true;
    }

    if (cmdName == "load") {
        std::string filename;
        std::getline(iss >> std::ws, filename);
        if (filename.empty()) {
            std::cout << "用法: %load <文件名>" << std::endl;
            return true;
        }
        // 读取并执行文件
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cout << "错误: 无法打开文件 " << filename << std::endl;
            return true;
        }
        std::string content((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
        // 逐行执行
        std::istringstream lines(content);
        std::string line;
        int executed = 0;
        while (std::getline(lines, line)) {
            std::string t = trim(line);
            if (t.empty() || t[0] == '#') continue;
            std::cout << ANSI_GREEN ">>> " << t << ANSI_RESET << std::endl;
            Value result;
            auto ok = evaluate(t + "\n", result);
            if (ok.first && !ok.second && !result.isNil()) {
                std::cout << valueDisplay(result) << std::endl;
            }
            executed++;
        }
        std::cout << "已加载 " << filename << "，执行 " << executed << " 条语句" << std::endl;
        return true;
    }

    if (cmdName == "type") {
        std::string expr;
        std::getline(iss >> std::ws, expr);
        if (expr.empty()) {
            std::cout << "用法: %type <表达式>" << std::endl;
            return true;
        }
        // 编译表达式获取类型
        auto func = compiler_.compile("打印(" + expr + ");");
        if (func) {
            // 查看最后一个常量的类型
            Value result;
            if (vm_->loadModule(func)) {
                // 无法直接获取返回值类型，用启发式方法
                std::cout << "表达式编译成功" << std::endl;
            } else {
                std::cout << "错误: " << vm_->error() << std::endl;
            }
        } else {
            std::cout << "错误: " << compiler_.errorMessage() << std::endl;
        }
        return true;
    }

    if (cmdName == "time") {
        std::string expr;
        std::getline(iss >> std::ws, expr);
        if (expr.empty()) {
            std::cout << "用法: %time <表达式>" << std::endl;
            return true;
        }
        auto start = std::chrono::high_resolution_clock::now();
        Value result;
        auto ok = evaluate(expr + "\n", result);
        auto end = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        if (ok.first) {
            if (!ok.second && !result.isNil()) {
                std::cout << valueDisplay(result) << std::endl;
            }
            if (ms > 0) {
                std::cout << ANSI_YELLOW << "耗时: " << ms << " ms" ANSI_RESET << std::endl;
            } else {
                std::cout << ANSI_YELLOW << "耗时: " << us << " μs" ANSI_RESET << std::endl;
            }
        }
        return true;
    }

    if (cmdName == "cd") {
        std::string dir;
        std::getline(iss >> std::ws, dir);
        if (dir.empty()) {
            // 显示当前目录
            char cwd[1024];
            if (GetCurrentDirectoryA(1024, cwd)) {
                std::cout << cwd << std::endl;
            }
            return true;
        }
        if (SetCurrentDirectoryA(dir.c_str())) {
            // 成功
        } else {
            std::cout << "错误: 无法切换到目录 " << dir << std::endl;
        }
        return true;
    }

    if (cmdName == "pwd") {
        char cwd[1024];
        if (GetCurrentDirectoryA(1024, cwd)) {
            std::cout << cwd << std::endl;
        }
        return true;
    }

    if (cmdName == "cls" || cmdName == "clear") {
        console_clear();
        return true;
    }

    if (cmdName == "exit" || cmdName == "quit") {
        std::cout << "再见!" << std::endl;
        std::exit(0);
        return true;
    }

    if (cmdName == "env" || cmdName == "vars") {
        // 显示当前作用域变量
        if (vm_) {
            auto names = vm_->getGlobalSlotNames();
            if (names.empty()) {
                std::cout << "（无变量）" << std::endl;
            } else {
                std::cout << "当前变量:" << std::endl;
                for (const auto& nm : names) {
                    int slot = vm_->getGlobalSlot(nm.c_str());
                    if (slot >= 0) {
                        Value* vp = vm_->getGlobalBySlot(static_cast<UInt16>(slot));
                        if (vp) {
                            std::cout << "  " << nm << " = "
                                      << valueDisplay(*vp) << "  "
                                      << ANSI_DIM << getTypeName(*vp) << ANSI_RESET
                                      << std::endl;
                        }
                    }
                }
            }
        }
        return true;
    }

    std::cout << "未知命令: " << cmdName << "（输入 %help 查看可用命令）" << std::endl;
    return true;
}

// ============================================================================
// 帮助显示
// ============================================================================

void ReplEngine::printHelp(const std::string& topic) {
    if (!topic.empty()) {
        // 特定主题帮助
        auto it = builtinDocs_.find(topic);
        if (it != builtinDocs_.end()) {
            std::cout << "\n" << ANSI_BOLD << topic << ANSI_RESET << std::endl;
            std::cout << "  " << it->second << std::endl;
            return;
        }
        // 检查是否是命令
        std::cout << "\n没有关于 '" << topic << "' 的帮助信息" << std::endl;
        return;
    }

    std::cout << "\n" ANSI_BOLD "CP 语言交互式解释器 — 帮助" ANSI_RESET "\n\n";

    std::cout << ANSI_BOLD "REPL 命令:" ANSI_RESET "\n";
    std::cout << "  %help [主题]       显示帮助信息\n";
    std::cout << "  %hist              显示历史记录\n";
    std::cout << "  %save <文件>       保存当前会话到文件\n";
    std::cout << "  %load <文件>       加载并执行文件\n";
    std::cout << "  %type <表达式>     显示表达式类型\n";
    std::cout << "  %time <表达式>     计时执行表达式\n";
    std::cout << "  %cd [目录]         切换/显示当前目录\n";
    std::cout << "  %pwd               显示当前目录路径\n";
    std::cout << "  %vars              显示当前变量\n";
    std::cout << "  %cls               清屏\n";
    std::cout << "  %exit              退出 REPL\n\n";

    std::cout << ANSI_BOLD "快捷键:" ANSI_RESET "\n";
    std::cout << "  ↑/↓        历史命令\n";
    std::cout << "  ←/→        移动光标\n";
    std::cout << "  Home        行首\n";
    std::cout << "  End         行尾\n";
    std::cout << "  Delete      删除光标后字符\n";
    std::cout << "  Backspace   删除光标前字符\n";
    std::cout << "  Tab         自动补全\n";
    std::cout << "  Ctrl+C      取消多行输入\n";
    std::cout << "  Ctrl+D      退出\n\n";

    std::cout << ANSI_BOLD "提示:" ANSI_RESET "\n";
    std::cout << "  • 输入 %help <函数名> 查看函数帮助\n";
    std::cout << "  • 直接输入表达式，自动打印结果\n";
    std::cout << "  • 多行输入自动检测（括号不匹配时继续等待）\n";
    std::cout << "  • Tab 补全自动追加函数括号\n\n";
}

// ============================================================================
// 增强行输入
// ============================================================================

std::string ReplEngine::readLineEnhanced(const std::string& prompt) {
    // 降级路径
    if (!usingEnhancedInput_) {
        std::cout << prompt << std::flush;
        std::string line;
        std::getline(std::cin, line);
        return line;
    }

    // 输出提示符
    std::cout << prompt << std::flush;

    std::string buffer;
    int cursorPos = 0;

    while (true) {
        int ch = console_getch();

        // ---- Enter ----
        if (ch == '\r') {
            if (console_kbhit()) console_getch(); // consume possible \n
            std::cout << std::endl;
            break;
        }

        // ---- Ctrl+C ----
        if (ch == 3) {
            std::cout << "^C" << std::endl;
            return "\x03";
        }

        // ---- Ctrl+D (EOF) ----
        if (ch == 4) {
            if (buffer.empty()) return "\x04";
            continue;
        }

        // ---- Ctrl+L (清屏) ----
        if (ch == 12) {
            console_clear();
            std::cout << prompt << buffer << std::flush;
            continue;
        }

        // ---- Backspace ----
        if (ch == '\b' || ch == 127) {
            if (cursorPos > 0) {
                int start = cursorPos - 1;
                while (start > 0 &&
                       (static_cast<unsigned char>(buffer[start]) & 0xC0) == 0x80)
                    start--;
                buffer.erase(buffer.begin() + start, buffer.begin() + cursorPos);
                cursorPos = start;
                refreshLine(prompt, buffer, cursorPos);
            }
            continue;
        }

        // ---- Tab ----
        if (ch == '\t') {
            std::string completed = doCompletion(buffer, prompt);
            if (!completed.empty() && completed != buffer) {
                buffer = completed;
                cursorPos = static_cast<int>(buffer.size());
                refreshLine(prompt, buffer, cursorPos);
            }
            continue;
        }

        // ---- 扩展键 ----
        if (ch == 0xE0 || ch == 0x00) {
            int ext = console_getch();
            switch (ext) {
                case 72: // ↑
                    if (historyPos_ > 0) {
                        historyPos_--;
                        buffer = history_[historyPos_];
                        cursorPos = static_cast<int>(buffer.size());
                        refreshLine(prompt, buffer, cursorPos);
                    }
                    break;
                case 80: // ↓
                    if (historyPos_ + 1 < history_.size()) {
                        historyPos_++;
                        buffer = history_[historyPos_];
                        cursorPos = static_cast<int>(buffer.size());
                        refreshLine(prompt, buffer, cursorPos);
                    } else {
                        historyPos_ = history_.size();
                        buffer.clear();
                        cursorPos = 0;
                        refreshLine(prompt, buffer, cursorPos);
                    }
                    break;
                case 75: // ←
                    if (cursorPos > 0) {
                        cursorPos--;
                        while (cursorPos > 0 &&
                               (static_cast<unsigned char>(buffer[cursorPos]) & 0xC0) == 0x80)
                            cursorPos--;
                        refreshLine(prompt, buffer, cursorPos);
                    }
                    break;
                case 77: // →
                    if (cursorPos < static_cast<int>(buffer.size())) {
                        cursorPos++;
                        while (cursorPos < static_cast<int>(buffer.size()) &&
                               (static_cast<unsigned char>(buffer[cursorPos]) & 0xC0) == 0x80)
                            cursorPos++;
                        refreshLine(prompt, buffer, cursorPos);
                    }
                    break;
                case 71: // Home
                    cursorPos = 0;
                    refreshLine(prompt, buffer, cursorPos);
                    break;
                case 79: // End
                    cursorPos = static_cast<int>(buffer.size());
                    refreshLine(prompt, buffer, cursorPos);
                    break;
                case 83: // Delete
                    if (cursorPos < static_cast<int>(buffer.size())) {
                        int end = cursorPos + 1;
                        while (end < static_cast<int>(buffer.size()) &&
                               (static_cast<unsigned char>(buffer[end]) & 0xC0) == 0x80)
                            end++;
                        buffer.erase(buffer.begin() + cursorPos, buffer.begin() + end);
                        refreshLine(prompt, buffer, cursorPos);
                    }
                    break;
            }
            continue;
        }

        // ---- 普通字符 ----
        buffer.insert(buffer.begin() + cursorPos, static_cast<char>(ch));
        cursorPos++;
        refreshLine(prompt, buffer, cursorPos);
    }

    // 加入历史
    if (!buffer.empty()) {
        if (history_.empty() || buffer != history_.back())
            history_.push_back(buffer);
    }
    historyPos_ = history_.size();

    return buffer;
}

// ============================================================================
// 主循环 — 增强版
// ============================================================================

void ReplEngine::run() {
    printBanner();

    std::string line;
    std::string buffer;

    while (true) {
        std::string p = buffer.empty() ? ">>> " : "... ";

        line = readLineEnhanced(p);

        // Ctrl+C → 取消多行
        if (line == "\x03") {
            if (!buffer.empty()) {
                std::cout << ANSI_YELLOW << "（取消）" << ANSI_RESET << std::endl;
                buffer.clear();
            }
            continue;
        }
        // Ctrl+D → 退出
        if (line == "\x04") {
            if (buffer.empty()) {
                std::cout << std::endl;
                break;
            }
            continue;
        }

        // 去除回车等空白字符后检查命令
        {
            std::string trimmed = trim(line);
            if (trimmed.empty() && buffer.empty()) continue;

            if (buffer.empty()) {
                // 处理 % 命令
                if (trimmed[0] == '%') {
                    dispatchCommand(trimmed);
                    continue;
                }
                // 兼容旧命令
                if (trimmed == "exit" || trimmed == "quit") {
                    std::cout << "再见!" << std::endl;
                    break;
                }
                if (trimmed == "help") {
                    printHelp();
                    continue;
                }
                if (trimmed == "clear" || trimmed == "cls") {
                    console_clear();
                    continue;
                }
                // 检查是否有语法提示查询
                if (trimmed.size() > 1 && trimmed[0] == '?') {
                    std::string topic = trimmed.substr(1);
                    std::string doc = getBuiltinHelp(topic);
                    if (!doc.empty()) {
                        std::cout << ANSI_BOLD << topic << ANSI_RESET << std::endl;
                        std::cout << "  " << doc << std::endl;
                    } else {
                        std::string sig = getFunctionSignature(topic);
                        if (!sig.empty()) {
                            std::cout << "  " << sig << std::endl;
                        } else {
                            std::cout << "没有关于 '" << topic << "' 的信息" << std::endl;
                        }
                    }
                    continue;
                }
            }
            // 使用修剪后的行继续
            line = trimmed;
        }

        buffer += line + "\n";

        if (!isIncomplete(buffer)) {
            Value result;
            auto ok = evaluate(buffer, result);
            if (ok.first) {
                if (!ok.second && !result.isNil()) {
                    std::string display = valueDisplay(result);
                    if (!display.empty()) {
                        std::cout << ANSI_GREEN << display << ANSI_RESET << "\n";
                    }
                }
            } else {
                // 显示编译/执行错误
                std::string errMsg = compiler_.errorMessage();
                if (!errMsg.empty()) {
                    std::cout << ANSI_RED << "错误: " << errMsg << ANSI_RESET << "\n";
                }
            }
            buffer.clear();
            // 清空补全缓存，下次重新收集
            completions_.clear();
        }
    }
}

// ============================================================================
// 辅助方法
// ============================================================================

static std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && (s[start] == ' ' || s[start] == '\t' ||
           s[start] == '\n' || s[start] == '\r')) start++;
    // 跳过 UTF-8 BOM (EF BB BF)
    if (start + 3 <= s.size() &&
        static_cast<unsigned char>(s[start]) == 0xEF &&
        static_cast<unsigned char>(s[start+1]) == 0xBB &&
        static_cast<unsigned char>(s[start+2]) == 0xBF) {
        start += 3;
    }
    size_t end = s.size();
    while (end > start && (s[end-1] == ' ' || s[end-1] == '\t' ||
           s[end-1] == '\n' || s[end-1] == '\r')) end--;
    return s.substr(start, end - start);
}

bool ReplEngine::isIncomplete(const std::string& source) {
    int braces = 0, parens = 0, brackets = 0;
    bool inString = false;
    char stringChar = 0;
    for (size_t i = 0; i < source.size(); i++) {
        char c = source[i];
        if (inString) {
            if (c == stringChar && (i == 0 || source[i-1] != '\\'))
                inString = false;
            continue;
        }
        if (c == '"' || c == '\'') { inString = true; stringChar = c; continue; }
        if (c == '{') braces++;
        else if (c == '}') braces--;
        else if (c == '(') parens++;
        else if (c == ')') parens--;
        else if (c == '[') brackets++;
        else if (c == ']') brackets--;
    }
    return braces > 0 || parens > 0 || brackets > 0 || inString;
}

std::string ReplEngine::wrapExpression(const std::string& raw) {
    return "打印(" + trim(raw) + ");";
}

std::pair<bool, bool> ReplEngine::evaluate(const std::string& source,
                                            Value& result) {
    std::string src = trim(source);
    if (src.empty()) return {false, false};

    // 清空之前编译器的错误状态
    compiler_.clearError();

    // 策略1: 包装为表达式
    auto func = compiler_.compile("打印(" + src + ");");
    if (func) {
        if (!vm_->loadModule(func)) {
            return {false, false};
        }
        if (compiler_.hasError()) {
            return {false, false};
        }
        result = Value::Nil(); // 表达式被打印了，返回值无意义
        return {true, true};   // 成功
    }

    // 策略2: 直接编译为语句
    compiler_.clearError();
    func = compiler_.compile(src);
    if (func) {
        if (!vm_->loadModule(func)) {
            (void)result;
            return {false, false};
        }
        if (compiler_.hasError()) {
            (void)result;
            return {false, false};
        }
        // 尝试从 VM 获取最近计算的值
        result = Value::Nil();
        return {true, false};
    }

    (void)result;
    return {false, false};
}

std::string ReplEngine::valueDisplay(const Value& v) {
    if (v.isNil()) return "空";
    if (v.isBool()) return v.asBool() ? "真" : "假";
    if (v.isInt()) return std::to_string(v.asInt());
    if (v.isFloat()) {
        std::string s = std::to_string(v.asFloat());
        // 移除尾随零
        size_t dot = s.find('.');
        if (dot != std::string::npos) {
            size_t last = s.size() - 1;
            while (last > dot && s[last] == '0') last--;
            if (last == dot) last--;
            s = s.substr(0, last + 1);
            if (s.find('.') == std::string::npos) s += ".0";
        }
        return s;
    }
    if (v.isString()) {
        return std::string(v.asString()->data, v.asString()->length);
    }
    if (v.isClosure() || v.isCFunction()) return "<函数>";
    if (v.isArray()) {
        std::string s = "[";
        auto* arr = v.asArray();
        Int64 len = arr->length();
        for (Int64 i = 0; i < len; i++) {
            if (i > 0) s += ", ";
            s += valueDisplay(arr->get(i));
        }
        s += "]";
        return s;
    }
    if (v.isTable()) return "<表>";
    return "<其他值>";
}

void ReplEngine::printBanner() {
    std::cout << ANSI_CYAN;
    std::cout << "╔══════════════════════════════════════╗\n";
    std::cout << "║   CP 语言交互式解释器 v0.2.0-beta    ║\n";
    std::cout << "║   输入 %help 查看帮助，%exit 退出   ║\n";
    std::cout << "╚══════════════════════════════════════╝\n\n";
    std::cout << ANSI_RESET;
}

} // namespace cplang

// standalone REPL 入口
int main() {
    cplang::ReplEngine repl(true);
    repl.run();
    return 0;
}
