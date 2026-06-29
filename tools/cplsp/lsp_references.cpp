// CPLSP — 查找引用
#include "cplsp.hpp"
#include <set>

namespace cplsp {

std::vector<LspRange> findReferences(Document* doc, const LspPosition& pos) {
    std::vector<LspRange> result;
    if (!doc) return result;
    
    // 获取光标所在的词
    std::string word;
    const auto& text = doc->text;
    int curLine = 0, curChar = 0;
    for (size_t i = 0; i < text.size(); i++) {
        if (text[i] == '\n') { if (curLine == pos.line) break; curLine++; curChar = 0; continue; }
        if (curLine == pos.line && curChar == pos.character) {
            // 向左扩展
            size_t ws = i;
            while (ws > 0 && (isalnum((unsigned char)text[ws-1]) || (text[ws-1] & 0x80) || text[ws-1] == '_')) ws--;
            size_t we = i;
            while (we < text.size() && (isalnum((unsigned char)text[we]) || (text[we] & 0x80) || text[we] == '_')) we++;
            word = text.substr(ws, we - ws);
            break;
        }
        curChar++;
    }
    
    if (word.empty()) return result;
    
    // 全文档搜索词出现位置（基于文本，避免匹配到关键字内部）
    std::set<std::pair<int,int>> seen;  // (line, col)
    int line = 0, col = 0;
    for (size_t i = 0; i < text.size(); i++) {
        if (text[i] == '\n') { line++; col = 0; continue; }
        
        // 检查是否匹配词
        bool match = true;
        for (size_t j = 0; j < word.size(); j++) {
            if (i + j >= text.size() || text[i + j] != word[j]) { match = false; break; }
        }
        
        if (match) {
            // 检查词的边界（前后不是标识符字符）
            bool leftOk = (i == 0 || text[i-1] == '\n' || 
                          !(isalnum((unsigned char)text[i-1]) || (text[i-1] & 0x80) || text[i-1] == '_'));
            size_t after = i + word.size();
            bool rightOk = (after >= text.size() || text[after] == '\n' ||
                           !(isalnum((unsigned char)text[after]) || (text[after] & 0x80) || text[after] == '_'));
            
            if (leftOk && rightOk) {
                auto key = std::make_pair(line, col);
                if (seen.insert(key).second) {
                    LspRange r;
                    r.start.line = line;
                    r.start.character = col;
                    r.end.line = line;
                    r.end.character = col + static_cast<int>(word.size());
                    result.push_back(r);
                }
            }
        }
        col++;
    }
    
    return result;
}

} // namespace cplsp
