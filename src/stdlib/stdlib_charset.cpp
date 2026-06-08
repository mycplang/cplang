#include "stdlib/stdlib.hpp"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace cplang {

// Character set encoding: GBK, Big5, Shift-JIS ↔ UTF-8
// Uses Windows MultiByteToWideChar / WideCharToMultiByte
// #include'd from stdlib.cpp, already inside namespace cplang

namespace charset_ns {

// Helper: UTF-8 string → wide string → codepage string
static std::string utf8ToMultiByte(const std::string& utf8, UINT codepage) {
    if (utf8.empty()) return "";
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return "";
    std::vector<wchar_t> wide(wlen);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, wide.data(), wlen);
    int mblen = WideCharToMultiByte(codepage, 0, wide.data(), -1, nullptr, 0, nullptr, nullptr);
    if (mblen <= 0) return "";
    std::vector<char> mb(mblen);
    WideCharToMultiByte(codepage, 0, wide.data(), -1, mb.data(), mblen, nullptr, nullptr);
    return std::string(mb.data(), mblen - 1); // exclude null terminator
}

// Helper: codepage string → wide string → UTF-8 string
static std::string multiByteToUtf8(const std::string& mb, UINT codepage) {
    if (mb.empty()) return "";
    int wlen = MultiByteToWideChar(codepage, 0, mb.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return "";
    std::vector<wchar_t> wide(wlen);
    MultiByteToWideChar(codepage, 0, mb.c_str(), -1, wide.data(), wlen);
    int u8len = WideCharToMultiByte(CP_UTF8, 0, wide.data(), -1, nullptr, 0, nullptr, nullptr);
    if (u8len <= 0) return "";
    std::vector<char> u8(u8len);
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), -1, u8.data(), u8len, nullptr, nullptr);
    return std::string(u8.data(), u8len - 1);
}

// ── GBK ↔ UTF-8 ──
Value utf8ToGbk_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return makeStringVal(VMString::create(""));
    std::string input(args[0].asString()->data, args[0].asString()->length);
    return makeStringVal(VMString::create(utf8ToMultiByte(input, 936)));
}
Value gbkToUtf8_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return makeStringVal(VMString::create(""));
    std::string input(args[0].asString()->data, args[0].asString()->length);
    return makeStringVal(VMString::create(multiByteToUtf8(input, 936)));
}

// ── Big5 ↔ UTF-8 ──
Value utf8ToBig5_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return makeStringVal(VMString::create(""));
    std::string input(args[0].asString()->data, args[0].asString()->length);
    return makeStringVal(VMString::create(utf8ToMultiByte(input, 950)));
}
Value big5ToUtf8_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return makeStringVal(VMString::create(""));
    std::string input(args[0].asString()->data, args[0].asString()->length);
    return makeStringVal(VMString::create(multiByteToUtf8(input, 950)));
}

// ── Shift-JIS ↔ UTF-8 ──
Value utf8ToShiftJis_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return makeStringVal(VMString::create(""));
    std::string input(args[0].asString()->data, args[0].asString()->length);
    return makeStringVal(VMString::create(utf8ToMultiByte(input, 932)));
}
Value shiftJisToUtf8_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return makeStringVal(VMString::create(""));
    std::string input(args[0].asString()->data, args[0].asString()->length);
    return makeStringVal(VMString::create(multiByteToUtf8(input, 932)));
}

// ── 通用编码转换 ──
Value convertEncoding_(std::vector<Value>& args) {
    if (args.size() < 3 || !args[0].isString()) return makeStringVal(VMString::create(""));
    std::string input(args[0].asString()->data, args[0].asString()->length);
    Int64 fromCp = args[1].asInt();
    Int64 toCp = args[2].asInt();
    // Long path: from CP → wide → to CP
    int wlen = MultiByteToWideChar(static_cast<UINT>(fromCp), 0, input.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return makeStringVal(VMString::create(""));
    std::vector<wchar_t> wide(wlen);
    MultiByteToWideChar(static_cast<UINT>(fromCp), 0, input.c_str(), -1, wide.data(), wlen);
    int outlen = WideCharToMultiByte(static_cast<UINT>(toCp), 0, wide.data(), -1, nullptr, 0, nullptr, nullptr);
    if (outlen <= 0) return makeStringVal(VMString::create(""));
    std::vector<char> out(outlen);
    WideCharToMultiByte(static_cast<UINT>(toCp), 0, wide.data(), -1, out.data(), outlen, nullptr, nullptr);
    return makeStringVal(VMString::create(std::string(out.data(), outlen - 1)));
}

// ── UTF-8 合法性检测 ──
Value isValidUtf8_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Bool(false);
    std::string s(args[0].asString()->data, args[0].asString()->length);
    int wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                    s.c_str(), static_cast<int>(s.size()), nullptr, 0);
    return Value::Bool(wlen > 0);
}

// ── 编码检测（启发式） ──
Value detectEncoding_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return makeStringVal(VMString::create("unknown"));
    std::string s(args[0].asString()->data, args[0].asString()->length);
    if (s.empty()) return makeStringVal(VMString::create("empty"));
    // Check UTF-8 BOM
    if (s.size() >= 3 && (unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB && (unsigned char)s[2] == 0xBF)
        return makeStringVal(VMString::create("utf8-bom"));
    // Check UTF-16 LE BOM
    if (s.size() >= 2 && (unsigned char)s[0] == 0xFF && (unsigned char)s[1] == 0xFE)
        return makeStringVal(VMString::create("utf16le"));
    // Check UTF-16 BE BOM
    if (s.size() >= 2 && (unsigned char)s[0] == 0xFE && (unsigned char)s[1] == 0xFF)
        return makeStringVal(VMString::create("utf16be"));
    // Try UTF-8 decode
    int wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                    s.c_str(), static_cast<int>(s.size()), nullptr, 0);
    if (wlen > 0) return makeStringVal(VMString::create("utf8"));
    // Try GBK
    wlen = MultiByteToWideChar(936, MB_ERR_INVALID_CHARS,
                                s.c_str(), static_cast<int>(s.size()), nullptr, 0);
    if (wlen > 0) return makeStringVal(VMString::create("gbk"));
    // Try Big5
    wlen = MultiByteToWideChar(950, MB_ERR_INVALID_CHARS,
                                s.c_str(), static_cast<int>(s.size()), nullptr, 0);
    if (wlen > 0) return makeStringVal(VMString::create("big5"));
    // Try Shift-JIS
    wlen = MultiByteToWideChar(932, MB_ERR_INVALID_CHARS,
                                s.c_str(), static_cast<int>(s.size()), nullptr, 0);
    if (wlen > 0) return makeStringVal(VMString::create("shiftjis"));
    return makeStringVal(VMString::create("binary"));
}

// ── 宽字符辅助（UTF-8 ↔ wstring） ──
Value utf8ToWide_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return makeStringVal(VMString::create(""));
    std::string input(args[0].asString()->data, args[0].asString()->length);
    int wlen = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return makeStringVal(VMString::create(""));
    std::vector<wchar_t> wide(wlen);
    MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, wide.data(), wlen);
    // Return as UTF-16LE bytes (CP strings are UTF-8, so store as "binary" encoded string)
    std::string result(reinterpret_cast<char*>(wide.data()), (wlen - 1) * sizeof(wchar_t));
    return makeStringVal(VMString::create(result));
}
Value wideToUtf8_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return makeStringVal(VMString::create(""));
    std::string input(args[0].asString()->data, args[0].asString()->length);
    if (input.size() % 2 != 0) return makeStringVal(VMString::create(""));
    size_t wlen = input.size() / 2;
    int u8len = WideCharToMultiByte(CP_UTF8, 0,
        reinterpret_cast<const wchar_t*>(input.data()), static_cast<int>(wlen),
        nullptr, 0, nullptr, nullptr);
    if (u8len <= 0) return makeStringVal(VMString::create(""));
    std::vector<char> u8(u8len);
    WideCharToMultiByte(CP_UTF8, 0,
        reinterpret_cast<const wchar_t*>(input.data()), static_cast<int>(wlen),
        u8.data(), u8len, nullptr, nullptr);
    return makeStringVal(VMString::create(std::string(u8.data(), u8len - 1)));
}

} // namespace charset_ns

void StdLib::registerCharset(VM* vm) {
    using namespace charset_ns;
    registerFunction(vm, "utf8ToGbk",      utf8ToGbk_);
    registerFunction(vm, "gbkToUtf8",      gbkToUtf8_);
    registerFunction(vm, "utf8ToBig5",     utf8ToBig5_);
    registerFunction(vm, "big5ToUtf8",     big5ToUtf8_);
    registerFunction(vm, "utf8ToShiftJis", utf8ToShiftJis_);
    registerFunction(vm, "shiftJisToUtf8", shiftJisToUtf8_);
    registerFunction(vm, "convertEncoding", convertEncoding_);
    registerFunction(vm, "isValidUtf8",    isValidUtf8_);
    registerFunction(vm, "detectEncoding", detectEncoding_);
    registerFunction(vm, "utf8ToWide",     utf8ToWide_);
    registerFunction(vm, "wideToUtf8",     wideToUtf8_);
    registerAlias(vm, "UTF8转GBK",         "utf8ToGbk");
    registerAlias(vm, "GBK转UTF8",         "gbkToUtf8");
    registerAlias(vm, "UTF8转Big5",        "utf8ToBig5");
    registerAlias(vm, "Big5转UTF8",        "big5ToUtf8");
    registerAlias(vm, "UTF8转ShiftJIS",    "utf8ToShiftJis");
    registerAlias(vm, "ShiftJIS转UTF8",    "shiftJisToUtf8");
    registerAlias(vm, "转换编码",          "convertEncoding");
    registerAlias(vm, "是否合法UTF8",      "isValidUtf8");
    registerAlias(vm, "检测编码",          "detectEncoding");
    registerAlias(vm, "UTF8转宽字符",      "utf8ToWide");
    registerAlias(vm, "宽字符转UTF8",      "wideToUtf8");
}

} // namespace cplang
