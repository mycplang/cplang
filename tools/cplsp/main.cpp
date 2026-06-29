// CPLSP — LSP JSON-RPC stdio 协议处理
#include "cplsp.hpp"
#include <iostream>
#include <sstream>
#include <regex>
#include <cstdio>

namespace cplsp {

// ═══════════════════════════════════════════════════════════════
//  JSON 序列化
// ═══════════════════════════════════════════════════════════════

static std::string jsonEscape(const std::string& s) {
    std::string r;
    for (char c : s) {
        switch (c) {
            case '"':  r += "\\\""; break;
            case '\\': r += "\\\\"; break;
            case '\n': r += "\\n"; break;
            case '\r': r += "\\r"; break;
            case '\t': r += "\\t"; break;
            default:   r += c;
        }
    }
    return r;
}

std::string jsonNull() { return "null"; }
std::string jsonStr(const std::string& s) { return "\"" + jsonEscape(s) + "\""; }
std::string jsonInt(int n) { return std::to_string(n); }
std::string jsonBool(bool b) { return b ? "true" : "false"; }

std::string jsonObj(const std::vector<std::pair<std::string, std::string>>& fields) {
    std::string r = "{";
    for (size_t i = 0; i < fields.size(); i++) {
        if (i > 0) r += ",";
        r += jsonStr(fields[i].first) + ":" + fields[i].second;
    }
    r += "}";
    return r;
}

static std::string jsonArr(const std::vector<std::string>& items) {
    std::string r = "[";
    for (size_t i = 0; i < items.size(); i++) {
        if (i > 0) r += ",";
        r += items[i];
    }
    r += "]";
    return r;
}

std::string jsonPosition(const LspPosition& pos) {
    return jsonObj({{"line", jsonInt(pos.line)}, {"character", jsonInt(pos.character)}});
}

std::string jsonRange(const LspRange& range) {
    return jsonObj({{"start", jsonPosition(range.start)}, {"end", jsonPosition(range.end)}});
}

std::string jsonDiagnostic(const LspDiagnostic& d) {
    return jsonObj({
        {"range", jsonRange(d.range)},
        {"severity", jsonInt(d.severity)},
        {"message", jsonStr(d.message)},
        {"source", jsonStr(d.source)}
    });
}

std::string jsonCompletionItem(const CompletionItem& item) {
    std::vector<std::pair<std::string, std::string>> f = {
        {"label", jsonStr(item.label)},
        {"kind", jsonInt(item.kind)},
        {"detail", jsonStr(item.detail)}
    };
    if (!item.insertText.empty())
        f.push_back({"insertText", jsonStr(item.insertText)});
    return jsonObj(f);
}

// ═══════════════════════════════════════════════════════════════
//  LSP 协议
// ═══════════════════════════════════════════════════════════════

static DocumentManager gDocMgr;

static void sendMessage(const std::string& json) {
    std::string header = "Content-Length: " + std::to_string(json.size()) + "\r\n\r\n";
    fwrite(header.c_str(), 1, header.size(), stdout);
    fwrite(json.c_str(), 1, json.size(), stdout);
    fflush(stdout);
}

// 简单 JSON 字段提取（不依赖完整 JSON 解析器）
static std::string jsonField(const std::string& json, const std::string& key) {
    std::string pat = "\"" + key + "\"";
    size_t pos = json.find(pat);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + pat.size());
    if (pos == std::string::npos) return "";
    pos++; while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    if (pos >= json.size()) return "";
    
    if (json[pos] == '"') {
        pos++;
        size_t end = pos;
        while (end < json.size() && json[end] != '"') {
            if (json[end] == '\\') end++;
            end++;
        }
        return json.substr(pos, end - pos);
    } else if (json[pos] == '{' || json[pos] == '[') {
        int depth = 1;
        size_t end = pos + 1;
        char open = json[pos], close = (open == '{') ? '}' : ']';
        while (end < json.size() && depth > 0) {
            if (json[end] == open) depth++;
            else if (json[end] == close) depth--;
            else if (json[end] == '"') { end++; while (end < json.size() && json[end] != '"') { if (json[end]=='\\') end++; end++; } }
            end++;
        }
        return json.substr(pos, end - pos - 1);
    } else {
        size_t end = pos;
        while (end < json.size() && json[end] != ',' && json[end] != '}' && json[end] != ']') end++;
        return json.substr(pos, end - pos);
    }
}

static int jsonFieldInt(const std::string& json, const std::string& key) {
    std::string s = jsonField(json, key);
    return s.empty() ? 0 : std::stoi(s);
}

static void handleMessage(const std::string& method, const std::string& id, const std::string& params) {
    if (method == "initialize") {
        std::string result = jsonObj({
            {"capabilities", jsonObj({
                {"textDocumentSync", jsonObj({{"openClose", jsonBool(true)}, {"change", jsonInt(1)}})},
                {"completionProvider", jsonObj({{"triggerCharacters", jsonStr(".")}})},
                {"hoverProvider", jsonBool(true)},
                {"definitionProvider", jsonBool(true)},
                {"referencesProvider", jsonBool(true)},
                {"documentSymbolProvider", jsonBool(true)}
            })},
            {"serverInfo", jsonObj({
                {"name", jsonStr("cplsp")},
                {"version", jsonStr("0.10.0")}
            })}
        });
        sendMessage(jsonObj({{"jsonrpc", jsonStr("2.0")}, {"id", id}, {"result", result}}));
        return;
    }
    
    if (method == "initialized") {
        return;  // 无需响应
    }
    
    if (method == "shutdown") {
        sendMessage(jsonObj({{"jsonrpc", jsonStr("2.0")}, {"id", id}, {"result", jsonNull()}}));
        return;
    }
    
    if (method == "exit") {
        exit(0);
    }
    
    // ── 文档同步 ──
    if (method == "textDocument/didOpen") {
        std::string uri = jsonField(params, "uri");
        std::string text = jsonField(params, "text");
        int version = jsonFieldInt(params, "version");
        gDocMgr.open(uri, text, version);
        // 发送诊断
        Document* doc = gDocMgr.get(uri);
        if (doc) {
            auto diags = computeDiagnostics(doc);
            std::vector<std::string> items;
            for (auto& d : diags) items.push_back(jsonDiagnostic(d));
            sendMessage(jsonObj({
                {"jsonrpc", jsonStr("2.0")},
                {"method", jsonStr("textDocument/publishDiagnostics")},
                {"params", jsonObj({
                    {"uri", jsonStr(uri)},
                    {"diagnostics", jsonArr(items)}
                })}
            }));
        }
        return;
    }
    
    if (method == "textDocument/didChange") {
        std::string uri = jsonField(params, "uri");
        std::string text = jsonField(params, "text");
        // 从 contentChanges 中提取
        std::string changes = jsonField(params, "contentChanges");
        if (!changes.empty()) text = jsonField(changes, "text");
        int version = jsonFieldInt(params, "version");
        gDocMgr.change(uri, text, version);
        return;
    }
    
    if (method == "textDocument/didClose") {
        std::string uri = jsonField(params, "uri");
        gDocMgr.close(uri);
        return;
    }
    
    // ── 补全 ──
    if (method == "textDocument/completion") {
        std::string uri = jsonField(params, "uri");
        std::string posStr = jsonField(params, "position");
        LspPosition pos;
        pos.line = jsonFieldInt(posStr, "line");
        pos.character = jsonFieldInt(posStr, "character");
        
        Document* doc = gDocMgr.getOrAnalyze(uri);
        std::vector<CompletionItem> items;
        if (doc) items = computeCompletion(doc, pos);
        
        std::vector<std::string> itemStrs;
        for (auto& it : items) itemStrs.push_back(jsonCompletionItem(it));
        
        sendMessage(jsonObj({
            {"jsonrpc", jsonStr("2.0")}, {"id", id},
            {"result", jsonObj({{"isIncomplete", jsonBool(false)}, {"items", jsonArr(itemStrs)}})}
        }));
        return;
    }
    
    // ── 跳转定义 ──
    if (method == "textDocument/definition") {
        std::string uri = jsonField(params, "uri");
        std::string posStr = jsonField(params, "position");
        LspPosition pos;
        pos.line = jsonFieldInt(posStr, "line");
        pos.character = jsonFieldInt(posStr, "character");
        
        Document* doc = gDocMgr.getOrAnalyze(uri);
        if (doc) {
            LspRange loc = findDefinition(doc, pos);
            if (loc.start.line >= 0) {
                sendMessage(jsonObj({
                    {"jsonrpc", jsonStr("2.0")}, {"id", id},
                    {"result", jsonObj({
                        {"uri", jsonStr(uri)},
                        {"range", jsonRange(loc)}
                    })}
                }));
                return;
            }
        }
        sendMessage(jsonObj({{"jsonrpc", jsonStr("2.0")}, {"id", id}, {"result", jsonNull()}}));
        return;
    }
    
    // ── 悬停 ──
    if (method == "textDocument/hover") {
        std::string uri = jsonField(params, "uri");
        std::string posStr = jsonField(params, "position");
        LspPosition pos;
        pos.line = jsonFieldInt(posStr, "line");
        pos.character = jsonFieldInt(posStr, "character");
        
        Document* doc = gDocMgr.getOrAnalyze(uri);
        if (doc) {
            std::string hover = getHover(doc, pos);
            if (!hover.empty()) {
                sendMessage(jsonObj({
                    {"jsonrpc", jsonStr("2.0")}, {"id", id},
                    {"result", jsonObj({
                        {"contents", jsonObj({
                            {"kind", jsonStr("markdown")},
                            {"value", jsonStr(hover)}
                        })}
                    })}
                }));
                return;
            }
        }
        sendMessage(jsonObj({{"jsonrpc", jsonStr("2.0")}, {"id", id}, {"result", jsonNull()}}));
        return;
    }
    
    // ── 查找引用 ──
    if (method == "textDocument/references") {
        std::string uri = jsonField(params, "uri");
        std::string posStr = jsonField(params, "position");
        LspPosition pos;
        pos.line = jsonFieldInt(posStr, "line");
        pos.character = jsonFieldInt(posStr, "character");
        
        Document* doc = gDocMgr.getOrAnalyze(uri);
        if (doc) {
            auto refs = findReferences(doc, pos);
            std::vector<std::string> refStrs;
            for (auto& r : refs) {
                refStrs.push_back(jsonObj({
                    {"uri", jsonStr(uri)},
                    {"range", jsonRange(r)}
                }));
            }
            sendMessage(jsonObj({
                {"jsonrpc", jsonStr("2.0")}, {"id", id},
                {"result", jsonArr(refStrs)}
            }));
            return;
        }
        sendMessage(jsonObj({{"jsonrpc", jsonStr("2.0")}, {"id", id}, {"result", jsonArr({})}}));
        return;
    }
    
    // ── 未知方法 ──
    sendMessage(jsonObj({
        {"jsonrpc", jsonStr("2.0")}, {"id", id},
        {"error", jsonObj({
            {"code", jsonInt(-32601)},
            {"message", jsonStr("Method not found: " + method)}
        })}
    }));
}

void lspRun() {
    FILE* logFile = nullptr;
    // 调试日志（写入文件而非 stderr，因为 stderr 被 VSCode 监控）
    // logFile = fopen("cplsp_debug.log", "w");
    
    std::string buf;
    char ch;
    while (fread(&ch, 1, 1, stdin) == 1) {
        buf += ch;
        // 查找 Content-Length 头
        size_t hdrEnd = buf.find("\r\n\r\n");
        if (hdrEnd == std::string::npos) continue;
        
        size_t clPos = buf.find("Content-Length: ");
        if (clPos == std::string::npos || clPos > hdrEnd) {
            buf.clear();
            continue;
        }
        
        int contentLen = 0;
        try { contentLen = std::stoi(buf.substr(clPos + 16, hdrEnd - clPos - 16)); }
        catch (...) { buf.clear(); continue; }
        
        size_t totalLen = hdrEnd + 4 + contentLen;
        if (buf.size() < totalLen) continue;  // 还没收完
        
        std::string body = buf.substr(hdrEnd + 4, contentLen);
        buf = buf.substr(totalLen);  // 剩余数据保留
        
        // 提取 method, id, params
        std::string method = jsonField(body, "method");
        std::string id = jsonField(body, "id");
        std::string params = jsonField(body, "params");
        
        if (logFile) fprintf(logFile, "[LSP] method=%s\n", method.c_str());
        
        handleMessage(method, id, params);
    }
    
    if (logFile) fclose(logFile);
}

} // namespace cplsp
