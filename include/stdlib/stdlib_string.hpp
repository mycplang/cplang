#pragma once
#include "vm/value.hpp"
#include <vector>

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  字符串函数
// ═══════════════════════════════════════════════════════════════════

namespace string {
    Value len(std::vector<Value>& args);
    Value substr(std::vector<Value>& args);
    Value concat(std::vector<Value>& args);
    Value find(std::vector<Value>& args);
    Value replace(std::vector<Value>& args);
    Value split(std::vector<Value>& args);
    Value join(std::vector<Value>& args);
    Value trim(std::vector<Value>& args);
    Value lower(std::vector<Value>& args);
    Value upper(std::vector<Value>& args);
    Value startsWith(std::vector<Value>& args);
    Value endsWith(std::vector<Value>& args);
    Value contains(std::vector<Value>& args);
    Value format(std::vector<Value>& args);
    Value repeat(std::vector<Value>& args);
    Value reverse(std::vector<Value>& args);
    Value padLeft(std::vector<Value>& args);
    Value padRight(std::vector<Value>& args);
}

// ═══════════════════════════════════════════════════════════════════
//  字符串扩展函数
// ═══════════════════════════════════════════════════════════════════

namespace str_ext {
    Value format(std::vector<Value>& args);
    Value parseInt(std::vector<Value>& args);
    Value parseFloat(std::vector<Value>& args);
    Value toHex(std::vector<Value>& args);
    Value toOct(std::vector<Value>& args);
    Value toBin(std::vector<Value>& args);
}

// ═══════════════════════════════════════════════════════════════════
//  字符串增强函数
// ═══════════════════════════════════════════════════════════════════

namespace str_more {
    Value strTrim(std::vector<Value>& args);
    Value strTrimLeft(std::vector<Value>& args);
    Value strTrimRight(std::vector<Value>& args);
    Value strPadLeft(std::vector<Value>& args);
    Value strPadRight(std::vector<Value>& args);
    Value strSplit(std::vector<Value>& args);
    Value strJoin(std::vector<Value>& args);
    Value strStartsWith(std::vector<Value>& args);
    Value strEndsWith(std::vector<Value>& args);
    Value strContains(std::vector<Value>& args);
    Value strReverse(std::vector<Value>& args);
    Value strReplace(std::vector<Value>& args);
    Value strCount(std::vector<Value>& args);
    Value strIndexOf(std::vector<Value>& args);
    Value strLastIndexOf(std::vector<Value>& args);
    Value strSlice(std::vector<Value>& args);
    Value strEscape(std::vector<Value>& args);
    Value strUnescape(std::vector<Value>& args);
    Value strPadCenter(std::vector<Value>& args);
}

// ═══════════════════════════════════════════════════════════════════
//  字符串大小写与字符判断
// ═══════════════════════════════════════════════════════════════════

namespace string_case {
    Value strToUpper(std::vector<Value>& args);
    Value strToLower(std::vector<Value>& args);
    Value strIsDigit(std::vector<Value>& args);
    Value strIsAlpha(std::vector<Value>& args);
    Value strIsAlnum(std::vector<Value>& args);
    Value strIsSpace(std::vector<Value>& args);
    Value strCapitalize(std::vector<Value>& args);
    Value strTitle(std::vector<Value>& args);
}

} // namespace cplang
