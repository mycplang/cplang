#pragma once
#include "vm/value.hpp"
#include <vector>

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  表函数
// ═══════════════════════════════════════════════════════════════════

namespace table {
    Value create(std::vector<Value>& args);
    Value set(std::vector<Value>& args);
    Value get(std::vector<Value>& args);
    Value len(std::vector<Value>& args);
    Value keys(std::vector<Value>& args);
    Value values(std::vector<Value>& args);
    Value entries(std::vector<Value>& args);
    Value has(std::vector<Value>& args);
    Value delete_(std::vector<Value>& args);
    Value clear(std::vector<Value>& args);
    Value merge(std::vector<Value>& args);
    Value clone(std::vector<Value>& args);
    Value toArray(std::vector<Value>& args);
    Value fromArray(std::vector<Value>& args);
}

} // namespace cplang
