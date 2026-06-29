#pragma once
// CPLSP — CP 语言语义级 LSP 服务器（原生 C++）
#include "common/types.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic_analyzer.hpp"
#include "ast/ast.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <sstream>

namespace cplsp {

// ═══════════════════════════════════════════════════════════════
//  文档管理器
// ═══════════════════════════════════════════════════════════════

struct Document {
    std::string uri;
    std::string text;
    int         version = 0;
    // 解析结果缓存
    cplang::Shared<cplang::Program> ast;
    cplang::SemanticAnalyzer*       sema = nullptr;
    bool                            dirty = true;
    std::vector<std::string>        diagnostics;  // 错误/警告消息
};

class DocumentManager {
public:
    void open(const std::string& uri, const std::string& text, int version);
    void change(const std::string& uri, const std::string& text, int version);
    void close(const std::string& uri);
    Document* get(const std::string& uri);
    Document* getOrAnalyze(const std::string& uri);  // 按需解析
    void clear();

private:
    std::unordered_map<std::string, Document> docs_;
    void analyze(Document& doc);
};

// ═══════════════════════════════════════════════════════════════
//  LSP 消息结构
// ═══════════════════════════════════════════════════════════════

struct LspPosition {
    int line = 0;
    int character = 0;
};

struct LspRange {
    LspPosition start;
    LspPosition end;
};

struct LspDiagnostic {
    LspRange range;
    int severity = 1;  // 1=error, 2=warning
    std::string message;
    std::string source = "cplsp";
};

struct CompletionItem {
    std::string label;
    std::string detail;
    std::string insertText;
    int kind = 14;  // LSP CompletionItemKind
};

// ═══════════════════════════════════════════════════════════════
//  功能接口
// ═══════════════════════════════════════════════════════════════

std::vector<LspDiagnostic> computeDiagnostics(Document* doc);
std::vector<CompletionItem> computeCompletion(Document* doc, const LspPosition& pos);
LspRange                   findDefinition(Document* doc, const LspPosition& pos);
std::string                getHover(Document* doc, const LspPosition& pos);
std::vector<LspRange>      findReferences(Document* doc, const LspPosition& pos);

// ═══════════════════════════════════════════════════════════════
//  JSON 序列化辅助
// ═══════════════════════════════════════════════════════════════

std::string jsonObj(const std::vector<std::pair<std::string, std::string>>& fields);
std::string jsonNull();
std::string jsonStr(const std::string& s);
std::string jsonInt(int n);
std::string jsonBool(bool b);
std::string jsonPosition(const LspPosition& pos);
std::string jsonRange(const LspRange& range);
std::string jsonDiagnostic(const LspDiagnostic& d);
std::string jsonCompletionItem(const CompletionItem& item);

// ═══════════════════════════════════════════════════════════════
//  LSP 协议
// ═══════════════════════════════════════════════════════════════

void lspRun();  // 主循环

} // namespace cplsp
