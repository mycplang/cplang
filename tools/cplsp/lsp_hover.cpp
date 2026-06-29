// CPLSP — 悬停类型提示
#include "cplsp.hpp"

namespace cplsp {

// 标准库函数的悬停文档
static const std::unordered_map<std::string, std::string> HOVER_DOCS = {
    {"打印", "**打印(任意值...)** — 将值输出到控制台，多个参数间用空格分隔"},
    {"print", "**print(any...)** — Print values to console"},
    {"函数", "**函数 名称(参数) { 体 }** — 定义一个函数"},
    {"function", "**function name(params) { body }** — Define a function"},
    {"变量", "**变量 名称 = 值** — 声明一个可重新赋值的变量"},
    {"var", "**var name = value** — Declare a reassignable variable"},
    {"常量", "**常量 名称 = 值** — 声明一个不可变的常量"},
    {"const", "**const name = value** — Declare an immutable constant"},
    {"如果", "**如果 (条件) { ... }** — 条件分支语句"},
    {"if", "**if (condition) { ... }** — Conditional branch"},
    {"当", "**当 (条件) { ... }** — while 循环"},
    {"while", "**while (condition) { ... }** — While loop"},
    {"循环", "**循环 (初始化; 条件; 递增) { ... }** — for 循环"},
    {"for", "**for (init; condition; inc) { ... }** — For loop"},
    {"遍历", "**遍历 (元素 : 容器) { ... }** — 遍历容器每个元素"},
    {"匹配", "**匹配 (表达式) { 情况 模式 => 结果; }** — 模式匹配"},
    {"match", "**match (expr) { case pattern => result; }** — Pattern matching"},
    {"创建字节数组", "**创建字节数组(大小)** — 分配指定大小的零填充字节缓冲区"},
    {"字节数组获取", "**字节数组获取(buf, 索引)** — 读取字节数组中指定位置的字节"},
    {"字节数组设置", "**字节数组设置(buf, 索引, 值)** — 设置字节数组中指定位置的字节"},
    {"字节数组长度", "**字节数组长度(buf)** — 获取字节数组的字节数"},
    {"字节数组转字符串", "**字节数组转字符串(buf)** — 将字节数组解码为 UTF-8 字符串"},
    {"字节数组转十六进制", "**字节数组转十六进制(buf)** — 将字节数组转为十六进制字符串"},
    {"字节数组自十六进制", "**字节数组自十六进制(hex)** — 从十六进制字符串创建字节数组"},
};

std::string getHover(Document* doc, const LspPosition& pos) {
    // 获取光标所在的词
    std::string word;
    const auto& lines = doc->text;
    int curLine = 0;
    for (size_t i = 0; i < lines.size(); i++) {
        char c = lines[i];
        if (c == '\n') {
            if (curLine == pos.line) break;
            curLine++;
            continue;
        }
        if (curLine == pos.line) {
            int col = 0;
            // 计算当前列
            for (size_t j = i; j > 0 && lines[j-1] != '\n'; j--) col++;
            // 简化：用字符位置匹配
            if (col >= pos.character - 1 && col <= pos.character + 1) {
                // 提取整词
                size_t ws = i;
                while (ws > 0 && lines[ws-1] != '\n' && 
                       (isalnum((unsigned char)lines[ws-1]) || (lines[ws-1] & 0x80) || lines[ws-1] == '_'))
                    ws--;
                size_t we = i;
                while (we < lines.size() && lines[we] != '\n' &&
                       (isalnum((unsigned char)lines[we]) || (lines[we] & 0x80) || lines[we] == '_'))
                    we++;
                word = lines.substr(ws, we - ws);
                break;
            }
        }
    }
    
    if (word.empty()) return "";
    
    // 查静态文档
    auto it = HOVER_DOCS.find(word);
    if (it != HOVER_DOCS.end()) return it->second;
    
    // 查 AST 中用户定义的符号
    if (doc && doc->ast) {
        for (auto& stmt : doc->ast->statements) {
            if (auto func = std::dynamic_pointer_cast<cplang::FuncDeclStmt>(stmt)) {
                if (func->name == word) {
                    std::string sig = "**函数 " + func->name + "(";
                    for (size_t pi = 0; pi < func->params.size(); pi++) {
                        if (pi > 0) sig += ", ";
                        sig += func->params[pi].first;
                        if (func->params[pi].second.has_value())
                            sig += ": " + *func->params[pi].second;
                    }
                    sig += ")** — 用户定义函数";
                    return sig;
                }
            }
            if (auto var = std::dynamic_pointer_cast<cplang::VarDeclStmt>(stmt)) {
                if (var->name == word) {
                    std::string s = "**变量 " + var->name + "**";
                    if (var->type.has_value()) s += ": " + *var->type;
                    return s;
                }
            }
        }
    }
    
    return "";
}

} // namespace cplsp
