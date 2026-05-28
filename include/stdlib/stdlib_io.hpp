#pragma once
#include "vm/value.hpp"
#include <vector>

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  IO 函数
// ═══════════════════════════════════════════════════════════════════

namespace io {
    Value print(std::vector<Value>& args);
    Value println(std::vector<Value>& args);
    Value input(std::vector<Value>& args);
    Value readFile(std::vector<Value>& args);
    Value writeFile(std::vector<Value>& args);
    Value appendFile(std::vector<Value>& args);
    Value exists(std::vector<Value>& args);
    Value deleteFile(std::vector<Value>& args);
    Value copyFile(std::vector<Value>& args);
    Value moveFile(std::vector<Value>& args);
    Value mkdir(std::vector<Value>& args);
    Value rmdir(std::vector<Value>& args);
    Value listDir(std::vector<Value>& args);
    Value isFile(std::vector<Value>& args);
    Value isDir(std::vector<Value>& args);
}

// ═══════════════════════════════════════════════════════════════════
//  控制台函数
// ═══════════════════════════════════════════════════════════════════

namespace console {
    Value color(std::vector<Value>& args);
    Value reset(std::vector<Value>& args);
    Value inputHidden(std::vector<Value>& args);
    Value clear(std::vector<Value>& args);
    Value size(std::vector<Value>& args);
    Value cursor(std::vector<Value>& args);
}

} // namespace cplang
