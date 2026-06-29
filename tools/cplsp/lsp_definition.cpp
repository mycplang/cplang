// CPLSP — 跳转定义
#include "cplsp.hpp"

namespace cplsp {

LspRange findDefinition(Document* doc, const LspPosition& pos) {
    LspRange notFound;
    notFound.start.line = -1;  // 哨兵：未找到
    if (!doc || !doc->ast) return notFound;
    
    // 获取光标所在的词
    std::string word;
    const auto& lines = doc->text;
    int lineStart = 0;
    int curLine = 0;
    int curChar = 0;
    for (size_t i = 0; i < lines.size(); i++) {
        char c = lines[i];
        if (c == '\n') {
            if (curLine == pos.line) break;
            curLine++;
            curChar = 0;
            lineStart = static_cast<int>(i) + 1;
            continue;
        }
        if (curLine == pos.line && curChar == pos.character) {
            // 从光标位置向左找词起始
            int wStart = static_cast<int>(i);
            while (wStart > lineStart) {
                char prev = lines[wStart - 1];
                if (isalnum(static_cast<unsigned char>(prev)) || (prev & 0x80) || prev == '_')
                    wStart--;
                else break;
            }
            // 向右找词结束
            int wEnd = static_cast<int>(i);
            while (wEnd < static_cast<int>(lines.size())) {
                char nc = lines[wEnd];
                if (isalnum(static_cast<unsigned char>(nc)) || (nc & 0x80) || nc == '_')
                    wEnd++;
                else break;
            }
            word = lines.substr(wStart, wEnd - wStart);
            break;
        }
        curChar++;
    }
    
    if (word.empty()) return notFound;
    
    // 在 AST 中查找函数/变量声明
    for (auto& stmt : doc->ast->statements) {
        // 检查函数声明
        if (auto func = std::dynamic_pointer_cast<cplang::FuncDeclStmt>(stmt)) {
            if (func->name == word) {
                LspRange r;
                r.start.line = func->token.line - 1;
                r.start.character = func->token.column - 1;
                r.end.line = func->token.line - 1;
                r.end.character = r.start.character + static_cast<int>(word.size());
                return r;
            }
        }
        // 检查变量声明
        if (auto var = std::dynamic_pointer_cast<cplang::VarDeclStmt>(stmt)) {
            if (var->name == word) {
                LspRange r;
                r.start.line = var->token.line - 1;
                r.start.character = var->token.column - 1;
                r.end.line = var->token.line - 1;
                r.end.character = r.start.character + static_cast<int>(word.size());
                return r;
            }
        }
    }
    
    return notFound;
}

} // namespace cplsp
