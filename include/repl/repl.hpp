// CP语言 REPL (Read-Eval-Print Loop)
#pragma once
#include "codegen/codegen.hpp"
#include "vm/vm.hpp"
#include "jit/orc_jit.hpp"
#include <string>
#include <vector>
#include <set>
#include <map>

namespace cplang {

class ReplEngine {
public:
    ReplEngine(bool enableJit = true);
    ~ReplEngine();

    // Run the REPL loop
    void run();

private:
    // Read one line with enhanced editing (history, tab-complete, arrow keys)
    std::string readLineEnhanced(const std::string& prompt);

    // Refresh the current line on screen after editing
    void refreshLine(const std::string& prompt, const std::string& buffer, int cursorPos);

    // Tab completion: find common prefix among all known identifiers
    std::string doCompletion(const std::string& prefix, const std::string& prompt);

    // Collect identifiers from VM global slots for tab completion
    void collectIdentifiers();

    // Check if input is incomplete (needs more lines)
    bool isIncomplete(const std::string& source);

    // Wrap expression for auto-print: "3+5" → "打印(3+5)"
    std::string wrapExpression(const std::string& source);

    // Compile and evaluate one complete input
    // Returns (success, isExpression)
    std::pair<bool, bool> evaluate(const std::string& source, Value& result);

    // Convert Value to display string
    std::string valueDisplay(const Value& v);

    // Print welcome banner
    void printBanner();

    // Initialize console handles
    void initConsole();

    // Restore original console mode
    void restoreConsole();

    // === 新增方法 ===

    // 保存/加载历史记录
    void saveHistory();
    void loadHistory();

    // 处理 REPL 命令（以 % 开头的命令）
    bool dispatchCommand(const std::string& cmd);

    // 显示帮助信息
    void printHelp(const std::string& topic = "");

    // 显示函数签名
    std::string getFunctionSignature(const std::string& name);

    // 显示内置函数帮助
    std::string getBuiltinHelp(const std::string& name);

    // 获取类型名称
    std::string getTypeName(const Value& v);

    // 生成范围提示条（用于 Tab 多匹配时显示）
    void printCompletionMatches(const std::vector<std::string>& matches,
                                 const std::string& prompt,
                                 const std::string& buffer);

    Compiler compiler_;
    VM* vm_;
    std::unique_ptr<HybridJIT> jit_;
    bool enableJit_;

    // Enhanced input state
    std::vector<std::string> history_;
    size_t historyPos_ = 0;
    std::set<std::string> completions_;
    bool consoleInitialized_ = false;
    void* hConsoleIn_ = nullptr;
    void* hConsoleOut_ = nullptr;
    unsigned long oldConsoleMode_ = 0;
    bool usingEnhancedInput_ = false;  // true if console supports enhanced mode

    // 新增成员
    std::string historyFile_;  // 历史记录文件路径
    std::map<std::string, std::string> builtinDocs_;  // 内置函数文档
    bool ansiColor_ = false;  // 是否支持 ANSI 颜色
};

} // namespace cplang
