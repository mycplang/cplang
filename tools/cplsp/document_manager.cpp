// CPLSP — 文档管理器
#include "cplsp.hpp"

namespace cplsp {

void DocumentManager::open(const std::string& uri, const std::string& text, int version) {
    Document& doc = docs_[uri];
    doc.uri = uri;
    doc.text = text;
    doc.version = version;
    doc.dirty = true;
    
    // 清理旧解析结果
    if (doc.sema) {
        delete doc.sema;
        doc.sema = nullptr;
    }
    doc.ast.reset();
}

void DocumentManager::change(const std::string& uri, const std::string& text, int version) {
    auto it = docs_.find(uri);
    if (it == docs_.end()) return;
    it->second.text = text;
    it->second.version = version;
    it->second.dirty = true;
    
    // 清理旧解析结果
    if (it->second.sema) {
        delete it->second.sema;
        it->second.sema = nullptr;
    }
    it->second.ast.reset();
}

void DocumentManager::close(const std::string& uri) {
    auto it = docs_.find(uri);
    if (it == docs_.end()) return;
    if (it->second.sema) {
        delete it->second.sema;
        it->second.sema = nullptr;
    }
    docs_.erase(it);
}

Document* DocumentManager::get(const std::string& uri) {
    auto it = docs_.find(uri);
    return (it != docs_.end()) ? &it->second : nullptr;
}

Document* DocumentManager::getOrAnalyze(const std::string& uri) {
    auto it = docs_.find(uri);
    if (it == docs_.end()) return nullptr;
    if (it->second.dirty) analyze(it->second);
    return &it->second;
}

void DocumentManager::clear() {
    for (auto& kv : docs_) {
        if (kv.second.sema) { delete kv.second.sema; kv.second.sema = nullptr; }
    }
    docs_.clear();
}

void DocumentManager::analyze(Document& doc) {
    if (!doc.dirty && doc.ast) return;
    doc.dirty = false;
    
    try {
        cplang::Lexer lexer(doc.text);
        cplang::Parser parser(&lexer);
        doc.ast = parser.parse();
        if (parser.hasError()) {
            doc.diagnostics.push_back("语法错误: " + std::string(parser.errorMessage()));
            return;
        }
        
        if (doc.sema) { delete doc.sema; doc.sema = nullptr; }
        doc.sema = new cplang::SemanticAnalyzer(&lexer);
        doc.sema->analyze(doc.ast);
        if (doc.sema->hasError()) {
            doc.diagnostics.push_back("语义错误: " + std::string(doc.sema->errorMessage()));
        }
    } catch (const std::exception& e) {
        doc.diagnostics.push_back("解析异常: " + std::string(e.what()));
        doc.dirty = true;
    }
}

} // namespace cplsp
