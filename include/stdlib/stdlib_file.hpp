#pragma once
#include "vm/value.hpp"
#include <vector>

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  文件操作函数
// ═══════════════════════════════════════════════════════════════════

namespace file {
    Value read(std::vector<Value>& args);
    Value write(std::vector<Value>& args);
    Value append(std::vector<Value>& args);
    Value exists(std::vector<Value>& args);
    Value delete_(std::vector<Value>& args);
    Value copy(std::vector<Value>& args);
    Value move(std::vector<Value>& args);
    Value mkdir(std::vector<Value>& args);
    Value rmdir(std::vector<Value>& args);
    Value listDir(std::vector<Value>& args);
    Value isFile(std::vector<Value>& args);
    Value isDir(std::vector<Value>& args);
    Value size(std::vector<Value>& args);
    Value time(std::vector<Value>& args);
}

// ═══════════════════════════════════════════════════════════════════
//  文件操作增强函数
// ═══════════════════════════════════════════════════════════════════

namespace file_more {
    Value fileExists(std::vector<Value>& args);
    Value fileSize(std::vector<Value>& args);
    Value fileCopy(std::vector<Value>& args);
    Value fileMove(std::vector<Value>& args);
    Value fileDelete(std::vector<Value>& args);
    Value dirExists(std::vector<Value>& args);
    Value dirCreate(std::vector<Value>& args);
    Value dirDelete(std::vector<Value>& args);
    Value dirList(std::vector<Value>& args);
    Value getCwd(std::vector<Value>& args);
    Value chDir(std::vector<Value>& args);
    Value fileGlob(std::vector<Value>& args);
}

// ═══════════════════════════════════════════════════════════════════
//  路径操作
// ═══════════════════════════════════════════════════════════════════

namespace path {
    Value basename(std::vector<Value>& args);
    Value dirname(std::vector<Value>& args);
    Value extname(std::vector<Value>& args);
    Value join(std::vector<Value>& args);
}

} // namespace cplang
