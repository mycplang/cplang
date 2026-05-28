#pragma once
#include "vm/value.hpp"
#include <vector>

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  数组函数
// ═══════════════════════════════════════════════════════════════════

namespace array {
    Value len(std::vector<Value>& args);
    Value push(std::vector<Value>& args);
    Value pop(std::vector<Value>& args);
    Value shift(std::vector<Value>& args);
    Value unshift(std::vector<Value>& args);
    Value insert(std::vector<Value>& args);
    Value remove(std::vector<Value>& args);
    Value slice(std::vector<Value>& args);
    Value splice(std::vector<Value>& args);
    Value reverse(std::vector<Value>& args);
    Value sort(std::vector<Value>& args);
    Value map(std::vector<Value>& args);
    Value filter(std::vector<Value>& args);
    Value reduce(std::vector<Value>& args);
    Value find(std::vector<Value>& args);
    Value findIndex(std::vector<Value>& args);
    Value includes(std::vector<Value>& args);
    Value indexOf(std::vector<Value>& args);
    Value lastIndexOf(std::vector<Value>& args);
    Value fill(std::vector<Value>& args);
    Value copy(std::vector<Value>& args);
    Value flatten(std::vector<Value>& args);
    Value unique(std::vector<Value>& args);
    Value zip(std::vector<Value>& args);
    Value unzip(std::vector<Value>& args);
}

// ═══════════════════════════════════════════════════════════════════
//  数组增强函数
// ═══════════════════════════════════════════════════════════════════

namespace arr_more {
    Value arrReverse(std::vector<Value>& args);
    Value arrRotate(std::vector<Value>& args);
    Value arrFill(std::vector<Value>& args);
    Value arrSlice(std::vector<Value>& args);
    Value arrSplice(std::vector<Value>& args);
    Value arrFind(std::vector<Value>& args);
    Value arrFindIndex(std::vector<Value>& args);
    Value arrUnique(std::vector<Value>& args);
    Value arrFlatten(std::vector<Value>& args);
    Value arrZip(std::vector<Value>& args);
    Value arrChunk(std::vector<Value>& args);
}

} // namespace cplang
