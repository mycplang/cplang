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
    Value isDirFunc(std::vector<Value>& args);
    
    // 新增基础函数
    Value strFunc(std::vector<Value>& args);
    Value typeFunc(std::vector<Value>& args);
    Value lenFunc(std::vector<Value>& args);
    Value absFunc(std::vector<Value>& args);
    Value arrNewFunc(std::vector<Value>& args);
    Value arrPushFunc(std::vector<Value>& args);
    Value arrLenFunc(std::vector<Value>& args);
    Value arrGetFunc(std::vector<Value>& args);
    Value strConcatFunc(std::vector<Value>& args);
    Value tableFunc(std::vector<Value>& args);
    Value tableSetFunc(std::vector<Value>& args);
    Value tableGetFunc(std::vector<Value>& args);
    Value tableHasFunc(std::vector<Value>& args);
    Value tableKeysFunc(std::vector<Value>& args);
    Value tableRemoveFunc(std::vector<Value>& args);
    Value fileExistsFunc(std::vector<Value>& args);
    Value fileSizeBytesFunc(std::vector<Value>& args);
    Value fileModifiedFunc(std::vector<Value>& args);
    Value rleCompressFunc(std::vector<Value>& args);
    Value rleDecompressFunc(std::vector<Value>& args);
    Value fileWatchCreateFunc(std::vector<Value>& args);
    Value fileWatchPollFunc(std::vector<Value>& args);
    Value fileWatchCloseFunc(std::vector<Value>& args);
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
