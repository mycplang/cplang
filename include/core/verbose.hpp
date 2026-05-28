#pragma once

// 跨模块的 verbose 状态控制
// 使用方法：
//   #include "core/verbose.hpp"
//   if (cplang::verbose()) { std::cout << "info...\n"; }

namespace cplang {

// 全局 verbose 状态（默认 false）
bool verboseEnabled();

// 运行时设置 verbose 状态
void setVerbose(bool v);

// 便捷宏：VERBOSE(x) 仅在 verbose 开启时执行 x
#define VERBOSE(x) do { if (cplang::verboseEnabled()) { x; } } while(0)

} // namespace cplang
