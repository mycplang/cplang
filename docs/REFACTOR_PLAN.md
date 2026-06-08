# CP语言 — 大厂级模块化重构方案

> 创建日期：2026-04-29
> 目标：将 cplang 从批处理脚本构建升级为 CMake 模块化构建体系

---

## 📊 现状诊断

| 问题 | 严重度 | 说明 |
|------|--------|------|
| 无构建系统 | 🔴 P0 | 只有 bat 脚本硬编码文件列表 |
| 循环依赖 | 🟡 P1 | optimizer.hpp → jit_compiler.hpp，纯静态优化不应依赖 JIT |
| OptLevel 位置不当 | 🟡 P1 | 枚举定义在 llvm_codegen.hpp，但 optimizer 需要使用 |
| Demo 文件混入 src/ | 🟢 P2 | escape_demo.cpp 等测试性代码应在 examples/ |
| 无头文件命名空间 | 🟢 P2 | include/ 下直接放子目录，公共库使用时可能冲突 |
| 无代码规范工具 | 🟢 P2 | 无 .clang-format / .clang-tidy |
| 无 CI/CD | 🟢 P2 | 每次手动构建，无自动化测试 |

---

## 🗺️ 重构分 3 个阶段

### Phase 1（立即可做）：CMake 基础 + 清理
- 写 CMakeLists.txt，定义 14 个 library target
- 写 .clang-format / .clang-tidy / .gitignore
- 移动 demo 文件到 examples/
- 修复 optimizer.hpp → jit 的循环依赖
- 将 OptLevel 提升到 common/types.hpp

### Phase 2（Phase 1 稳定后）：测试 + CI
- 结构化 tests/ 目录（unit / integration / benchmarks）
- 集成 Google Test
- GitHub Actions CI/CD 流水线
- CMakePresets.json 多配置构建

### Phase 3（进阶）：公共库规范
- include/cplang/ 命名空间前缀
- Doxygen 文档生成
- CPack 打包
- Conan/vcpkg 依赖管理

---

## 🏗️ Phase 1：CMake 库目标依赖图

```
                         cplang_common (header-only)
                               |
                         cplang_token (header-only)
                               |
                         cplang_lexer
                          /         \
                 cplang_ast       cplang_parser
                 (header-only)         |
                      |         cplang_semantic
                      +-------+-------+
                              |
                         cplang_vm
                    /   /   |   \   \           \
              codegen module debug exception stdlib
                 |                              |
            cplang_codegen               cplang_stdlib
                 |       \
                 |   cplang_optimizer (纯 AST 变换, 不依赖 JIT)
                 |
             cplang_jit (LLVM ORC)
                 |
             cplang_cli (main + repl)
```

**关键修复**：
- `optimizer/optimizer.hpp` 移除 `#include "jit/jit_compiler.hpp"`
- `OptLevel` 枚举从 `codegen/llvm_codegen.hpp` 移动到 `common/types.hpp`
- JIT 集成改由 `cplang_cli` 层负责编排

---

## 📁 新目录结构（Phase 1）

```
cplang/
├── cmake/
│   ├── CompilerWarnings.cmake
│   ├── Sanitizers.cmake
│   └── FindLLVM.cmake
├── include/                    # 保持现有结构
│   ├── common/types.hpp
│   ├── lexer/{token,lexer}.hpp
│   ├── ast/ast.hpp
│   ├── parser/parser.hpp
│   ├── semantic/semantic_analyzer.hpp
│   ├── vm/{vm,vm_opt,vm_optimizations}.hpp
│   ├── codegen/{codegen,bytecode_optimizer,llvm_codegen}.hpp
│   ├── optimizer/{optimizer,constant_folder,dead_code_eliminator,escape_analyzer,
│   │               function_inliner,loop_unroller,tail_recursion_optimizer}.hpp
│   ├── jit/{jit_compiler,orc_jit}.hpp
│   ├── module/module_system.hpp
│   ├── stdlib/stdlib.hpp
│   ├── debug/debugger.hpp
│   └── exception/exception_handler.hpp
├── src/
│   ├── lexer/lexer.cpp
│   ├── parser/parser.cpp
│   ├── semantic/semantic_analyzer.cpp
│   ├── vm/{vm,vm_opt,vm_switch}.cpp
│   ├── codegen/{codegen,codegen_opt,llvm_codegen}.cpp
│   ├── optimizer/{optimizer,constant_folder,dead_code_eliminator,escape_analyzer,
│   │               function_inliner,loop_unroller,tail_recursion_optimizer}.cpp
│   ├── jit/{jit_compiler,orc_jit}.cpp
│   ├── module/module_system.cpp
│   ├── stdlib/stdlib.cpp
│   ├── debug/debugger.cpp
│   ├── exception/exception_handler.cpp
│   ├── cli/                    # 【NEW】CLI 入口
│   │   ├── main.cpp
│   │   └── repl.cpp
│   └── cplang_build.cpp       # 单文件整合（可选保留）
├── examples/                   # 【NEW】从 src/ 移入
│   ├── escape_demo.cpp
│   ├── inline_demo.cpp
│   ├── jit_demo.cpp
│   ├── llvm_opt_demo.cpp
│   ├── loop_unroll_demo.cpp
│   ├── optimizer_demo.cpp
│   └── tail_rec_demo.cpp
├── tests/                      # 保持现有结构（Phase 2 重构）
│   ├── unit/                   # 【NEW】Google Test
│   ├── integration/            # 【NEW】端到端测试
│   └── benchmarks/             # 移入 benchmarks/ 内容
├── docs/
├── CMakeLists.txt              # 【NEW】根构建文件
├── CMakePresets.json           # 【NEW】构建预设
├── .clang-format               # 【NEW】
├── .clang-tidy                 # 【NEW】
├── .gitignore                  # 【NEW】
└── README.md                   # 【NEW】
```

---

## 📝 代码修改清单

### 必须修改的源码

1. **`include/common/types.hpp`** — 添加 OptLevel 枚举
2. **`include/codegen/llvm_codegen.hpp`** — 移除 OptLevel 定义，改用 common
3. **`include/optimizer/optimizer.hpp`** — 移除 `#include "jit/jit_compiler.hpp"`
4. **`include/optimizer/optimizer.hpp`** — 将 `std::unique_ptr<JITCompiler>` 改为 std::any 或移除
5. **`src/cli/main.cpp`** — 从 `src/main.cpp` 移动
6. **`src/cli/repl.cpp`** — 从 `src/repl.cpp` 移动
7. **`scripts/fixes/`** — 归档或删除（40+ 个临时修复脚本）

### 可选优化

8. 统一使用 `#pragma once`（当前是混合风格，部分用 include guard）
9. 清理代码中的临时调试 std::cout
10. 修复 DEBUG_STATUS.md 中记录的路劲不一致问题

---

## ✅ 验收标准

- [ ] `cmake --preset debug && cmake --build --preset debug` 一次性通过
- [ ] `cmake --preset release && cmake --build --preset release` 一次性通过
- [ ] 生成的 `cplang_cli.exe` 功能与原 `cplang_debug.exe` 一致
- [ ] 所有 14 个 library target 编译为独立的 .lib/.a
- [ ] 无编译警告（-Wall -Wextra -Wpedantic）
- [ ] .clang-format 已配置，代码风格统一
