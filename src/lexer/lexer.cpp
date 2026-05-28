// CP语言 词法分析器实现 v2 - 简单字符串扫描
#include "lexer/lexer.hpp"
#include <cctype>
#include <unordered_map>
#include <iostream>

namespace cplang {

// ═══════════════════════════════════════════════════════════════
//  静态辅助
// ═══════════════════════════════════════════════════════════════

static inline bool isDigit(char c) { return c >= '0' && c <= '9'; }
static inline bool isAsciiAlpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '$'; }
[[maybe_unused]] static inline bool isAlphaNum(char c) { return isAsciiAlpha(c) || isDigit(c); }
static inline bool isSpace(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

// UTF-8: check if byte starts a multi-byte sequence (Chinese CJK etc.)
static inline bool isUtf8Lead(char c) {
    unsigned char uc = static_cast<unsigned char>(c);
    return uc >= 0xC0 && uc < 0xFE;  // 2-byte..4-byte lead
}

// Is this an identifier-start character? (ASCII letter / _ / $ / UTF-8 lead)
static inline bool isIdentStart(char c) {
    return isAsciiAlpha(c) || isUtf8Lead(c);
}

// Is this an identifier-continuation character?
[[maybe_unused]] static inline bool isIdentCont(char c) {
    return isAsciiAlpha(c) || isDigit(c) || isUtf8Lead(c);
}

// Read one full UTF-8 codepoint from src[pos] and append to out; advance pos by byte count.
// Returns number of bytes consumed (0 if invalid).
[[maybe_unused]] static int readUtf8Char(const String& src, size_t pos, String& out) {
    if (pos >= src.size()) return 0;
    unsigned char b0 = static_cast<unsigned char>(src[pos]);
    int len = 1;
    if      (b0 >= 0xF0) len = 4;  // 4-byte
    else if (b0 >= 0xE0) len = 3;  // 3-byte (Chinese)
    else if (b0 >= 0xC0) len = 2;  // 2-byte
    else return 0;                  // ASCII or invalid — not a UTF-8 lead
    if (pos + len > src.size()) return 0;
    for (int i = 0; i < len; i++) out += src[pos + i];
    return len;
}

// ═══════════════════════════════════════════════════════════════
//  构造 / 析构
// ═══════════════════════════════════════════════════════════════

Lexer::Lexer(const String& s)
    : src_(s), pos_(0), line_(1), col_(1),
      hasError_(false), hasCached_(false) {
    // 跳过 UTF-8 BOM (EF BB BF)
    if (src_.size() >= 3 &&
        static_cast<unsigned char>(src_[0]) == 0xEF &&
        static_cast<unsigned char>(src_[1]) == 0xBB &&
        static_cast<unsigned char>(src_[2]) == 0xBF) {
        pos_ = 3;
    }
    advance_();
}

Lexer::~Lexer() = default;

// ═══════════════════════════════════════════════════════════════
//  内部：advance
// ═══════════════════════════════════════════════════════════════

void Lexer::advance_() {
    prev_ = curr_;
    if (pos_ < src_.size()) {
        curr_ = src_[pos_++];
        if (curr_ == '\n') { line_++; col_ = 1; }
        else col_++;
    } else {
        curr_ = 0;  // EOF
    }
}

char Lexer::peek() const { return curr_; }

char Lexer::peekNext() const {
    return (pos_ < src_.size()) ? src_[pos_] : 0;
}

// ═══════════════════════════════════════════════════════════════
//  公共 API
// ═══════════════════════════════════════════════════════════════

Token Lexer::nextToken() {
    if (hasCached_) {
        hasCached_ = false;
        return cachedToken_;
    }
    while (curr_ && isSpace(curr_)) advance_();
    if (!curr_) return Token(TokenType::END_OF_FILE, "", line_, col_);
    int sl = line_, sc = col_ - 1;
    if (isIdentStart(curr_)) return scanId_(sl, sc);
    if (isDigit(curr_)) return scanNum_(sl, sc);
    if (curr_ == 0x22 || curr_ == 0x27) return scanStr_(sl, sc);
    if (curr_ == '/' && peekNext() == '/') { scanComment_(); return nextToken(); }
    return scanOp_(sl, sc);
}

Token Lexer::peekToken() {
    if (!hasCached_) {
        cachedToken_ = nextToken();
        hasCached_ = true;
    }
    return cachedToken_;
}

void Lexer::reset() {
    pos_ = 0; line_ = 1; col_ = 1;
    hasError_ = false; hasCached_ = false;
    // 跳过 UTF-8 BOM (EF BB BF)
    if (src_.size() >= 3 &&
        static_cast<unsigned char>(src_[0]) == 0xEF &&
        static_cast<unsigned char>(src_[1]) == 0xBB &&
        static_cast<unsigned char>(src_[2]) == 0xBF) {
        pos_ = 3;
    }
    advance_();
}

// ═══════════════════════════════════════════════════════════════
//  scanId_
// ═══════════════════════════════════════════════════════════════

Token Lexer::scanId_(int sl, int sc) {
    String txt;
    // Scan directly from source buffer for proper UTF-8 handling
    while (true) {
        unsigned char uc = static_cast<unsigned char>(curr_);
        if (isAsciiAlpha(curr_) || isDigit(curr_)) {
            // ASCII identifier char
            txt += curr_;
            advance_();
        } else if (uc >= 0xC0 && uc < 0xFE) {
            // UTF-8 multi-byte lead byte — read one full codepoint
            int len = 1;
            if      (uc >= 0xF0) len = 4;
            else if (uc >= 0xE0) len = 3;
            else if (uc >= 0xC0) len = 2;
            // Append all bytes of this UTF-8 character
            for (int i = 0; i < len && curr_ != 0; i++) {
                txt += curr_;
                advance_();
            }
        } else {
            break;
        }
    }
    auto kw = KeywordTable::instance().find(txt);
    if (kw.has_value()) return Token(kw.value(), txt, sl, sc);
    return Token(TokenType::IDENTIFIER, txt, sl, sc);
}

// ═══════════════════════════════════════════════════════════════
//  scanNum_
// ═══════════════════════════════════════════════════════════════

Token Lexer::scanNum_(int sl, int sc) {
    String txt;
    while (curr_ && (isDigit(curr_) || curr_ == '.')) { txt += curr_; advance_(); }
    bool isFloat = (txt.find('.') != String::npos);
    if (isFloat) {
        try { return Token(TokenType::FLOAT, txt, sl, sc, std::stod(txt)); }
        catch (...) { reportError("无效浮点数: " + txt); return Token(TokenType::INVALID, txt, sl, sc); }
    }
    try { return Token(TokenType::INTEGER, txt, sl, sc, static_cast<Int64>(std::stoll(txt))); }
    catch (...) { reportError("无效整数: " + txt); return Token(TokenType::INVALID, txt, sl, sc); }
}

// ═══════════════════════════════════════════════════════════════
//  scanStr_
// ═══════════════════════════════════════════════════════════════

Token Lexer::scanStr_(int sl, int sc) {
    char q = curr_; advance_();
    String txt;
    while (curr_ && curr_ != q) {
        if (curr_ == '\\') {
            advance_();
            if (curr_ == 'n') { txt += '\n'; advance_(); }
            else if (curr_ == 't') { txt += '\t'; advance_(); }
            else if (curr_ == 'r') { txt += '\r'; advance_(); }
            else if (curr_ == '0') { txt += '\0'; advance_(); }
            else if (curr_ == '\\') { txt += '\\'; advance_(); }
            else if (curr_ == '\'') { txt += '\''; advance_(); }
            else if (curr_ == '"') { txt += '"'; advance_(); }
            else { txt += curr_; advance_(); }
        } else { txt += curr_; advance_(); }
    }
    if (curr_ == q) advance_();
    else reportError("字符串未闭合");
    return Token(TokenType::STRING, txt, sl, sc, txt);
}

// ═══════════════════════════════════════════════════════════════
//  scanComment_
// ═══════════════════════════════════════════════════════════════

void Lexer::scanComment_() {
    advance_(); advance_();  // skip //
    while (curr_ && curr_ != '\n') advance_();
}

// ═══════════════════════════════════════════════════════════════
//  scanOp_
// ═══════════════════════════════════════════════════════════════

Token Lexer::scanOp_(int sl, int sc) {
    char c1 = curr_;
    char c2 = peekNext();
    // 2-char ops
    if (c2) {
        String two; two += c1; two += c2;
        if (two == "==" || two == "!=" || two == ">=" || two == "<=" ||
            two == "&&" || two == "||" || two == "++" || two == "--" ||
            two == "+=" || two == "-=" || two == "*=" || two == "/=" ||
            two == "%=" || two == "::" || two == "->" ||
            two == "<<") {
            advance_(); advance_();
            static const std::unordered_map<String, TokenType> m2 = {
                {"==", TokenType::OP_EQ}, {"!=", TokenType::OP_NE},
                {">=", TokenType::OP_GE}, {"<=", TokenType::OP_LE},
                {"&&", TokenType::OP_AND}, {"||", TokenType::OP_OR},
                {"++", TokenType::OP_INC}, {"--", TokenType::OP_DEC},
                {"+=", TokenType::OP_PLUS_ASSIGN}, {"-=", TokenType::OP_MINUS_ASSIGN},
                {"*=", TokenType::OP_MUL_ASSIGN}, {"/=", TokenType::OP_DIV_ASSIGN},
                {"%=", TokenType::OP_MOD_ASSIGN},
                {"::", TokenType::OP_DBL_COLON}, {"->", TokenType::OP_ARROW},
                {"<<", TokenType::OP_LSHIFT},
            };
            return Token(m2.at(two), two, sl, sc);
        }
    }
    // 1-char ops
    static const std::unordered_map<char, TokenType> m1 = {
        {'+', TokenType::OP_PLUS}, {'-', TokenType::OP_MINUS},
        {'*', TokenType::OP_MUL}, {'/', TokenType::OP_DIV},
        {'%', TokenType::OP_MOD}, {'=', TokenType::OP_ASSIGN},
        {'<', TokenType::OP_LT}, {'>', TokenType::OP_GT},
        {'!', TokenType::OP_NOT}, {'&', TokenType::OP_BIT_AND},
        {'|', TokenType::OP_BIT_OR}, {'^', TokenType::OP_BIT_XOR},
        {'(', TokenType::LPAREN}, {')', TokenType::RPAREN},
        {'[', TokenType::LBRACKET}, {']', TokenType::RBRACKET},
        {'{', TokenType::LBRACE}, {'}', TokenType::RBRACE},
        {',', TokenType::COMMA}, {';', TokenType::SEMICOLON},
        {'?', TokenType::OP_QUESTION}, {':', TokenType::OP_COLON},
        {'.', TokenType::OP_DOT},
    };
    advance_();
    auto it = m1.find(c1);
    if (it != m1.end()) return Token(it->second, String(1, c1), sl, sc);
    advance_();  // skip unknown char
    return Token(TokenType::INVALID, String(1, c1), sl, sc);
}

// ═══════════════════════════════════════════════════════════════
//  错误
// ═══════════════════════════════════════════════════════════════

void Lexer::reportError(const String& msg) {
    if (!hasError_) {
        hasError_ = true;
        errorMsg_ = "第" + std::to_string(line_) + "行: " + msg;
    }
}

} // namespace cplang
