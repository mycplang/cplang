// CP语言 词法分析器
#pragma once

#include "lexer/token.hpp"

namespace cplang {

// 词法分析器
class Lexer {
public:
    explicit Lexer(const String& source);
    ~Lexer();

    // 词法分析
    Token nextToken();
    Token peekToken();
    void reset();

    // 状态查询
    bool hasError() const { return hasError_; }
    const String& errorMessage() const { return errorMsg_; }
    Int32 currentLine() const { return line_; }
    Int32 currentColumn() const { return col_; }

private:
    // 内部工具
    void advance_();
    char peek() const;
    char peekNext() const;

    // 扫描器
    Token scanId_(int sl, int sc);
    Token scanNum_(int sl, int sc);
    Token scanStr_(int sl, int sc);
    void scanComment_();
    Token scanOp_(int sl, int sc);

    // 错误
    void reportError(const String& msg);

    // 数据
    String src_;
    size_t pos_;
    Int32 line_;
    Int32 col_;
    Char curr_;
    Char prev_;
    bool hasError_;
    String errorMsg_;

    // 缓存
    Token cachedToken_;
    bool hasCached_;
};

} // namespace cplang
