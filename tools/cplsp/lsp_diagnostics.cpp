// CPLSP — 诊断（基于 SemanticAnalyzer）
#include "cplsp.hpp"

namespace cplsp {

std::vector<LspDiagnostic> computeDiagnostics(Document* doc) {
    std::vector<LspDiagnostic> result;
    if (!doc) return result;
    
    // 使用文档管理器解析
    DocumentManager tmp;
    // 直接内联解析（不依赖 DocumentManager）
    try {
        cplang::Lexer lexer(doc->text);
        cplang::Parser parser(&lexer);
        auto ast = parser.parse();
        
        if (parser.hasError()) {
            LspDiagnostic d;
            d.severity = 1;  // error
            d.message = std::string(parser.errorMessage());
            // 尝试从错误消息中提取行号
            std::string err = parser.errorMessage();
            size_t linePos = err.find("第");
            if (linePos != std::string::npos) {
                try {
                    size_t endPos = err.find("行", linePos);
                    if (endPos != std::string::npos) {
                        int line = std::stoi(err.substr(linePos + 3, endPos - linePos - 3)) - 1;
                        if (line < 0) line = 0;
                        d.range.start.line = line;
                        d.range.end.line = line;
                    }
                } catch (...) {}
            }
            result.push_back(d);
        }
    } catch (...) {
        LspDiagnostic d;
        d.severity = 1;
        d.message = "编译器内部错误";
        result.push_back(d);
    }
    
    return result;
}

} // namespace cplsp
