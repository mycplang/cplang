# CP语言编译器调试状态

**最后更新**: 2026-05-14 09:30

## 项目路径
`C:\cplang\`

## 当前版本
v0.1.0-beta → 核心编译管线全链路打通，标准库 810 原生函数

---

## 已修复的问题 ✅

| 问题 | 状态 | 修复日期 |
|------|------|----------|
| compiler.compile() 崩溃 (exit code 1) | ✅ 已修复 | 之前 |
| printVal 输出 `<1>` 而非 `真`/`假` | ✅ 已修复 (输出 "true"/"false") | 之前 |
| ast.hpp 重复定义 | ✅ | - |
| `[DBG STORE_NOSLOT]` 遗留调试输出 | ✅ 已从 vm.cpp 移除 | 2026-05-14 |

---

## 当前问题 ⚠️

| 问题 | 严重性 | 阻塞原因 |
|------|--------|----------|
| JIT 回退到字节码执行 | 中 | LLVM-18.1.8 开发库 llvm-dev.tar.xz 损坏（628MB 截断），需重新下载 clang+llvm-18.1.8-x86_64-pc-windows-msvc.tar.xz |
| 优化器调试输出过多 | 低 | escape_analyzer / function_inliner / loop_unroller 等每次编译都打印统计报告 |
| CMake 构建未集成 JIT | 低 | CMakeLists.txt 有 LLVM 检测但实际构建用 build_msvc.bat |
| 根目录 .obj 文件污染 | 低 | 构建产物未清理 |

---

## 2026-05-14 调试进展

### 验证测试
- `build\cplang -c test.cp` → 正常编译并执行
- Fibonacci 递归 → 输出 55，2127 条指令
- 控制流 (如果/否则/当) → 正常工作
- Bool 输出 → "true"/"false"（正确）
- JIT 模式 (-j) → 回退到字节码（LLVM 未配置）
- 回归测试 _run_ci.py → 170 个测试通过

### 修复
1. **vm.cpp:1845** - 移除 `[DBG STORE_NOSLOT]` 调试输出

### 已创建
1. **.github/workflows/ci.yml** - GitHub Actions CI/CD 工作流（Windows MSVC 构建 + 回归测试）
2. **tests/_run_ci.py** - 灵活的 CI 测试运行器（支持命令行参数和 CPLANG_BIN 环境变量）

---

## LLVM JIT 恢复步骤

1. 重新下载 clang+llvm-18.1.8-x86_64-pc-windows-msvc.tar.xz（约 936MB）
2. 用 7-Zip 或 Python tarfile 解压到 C:\cplang\llvm-dev\
3. 设置环境变量 `LLVM_DIR=C:\cplang\llvm-dev\`
4. 或在 CMakeLists.txt 中添加搜索路径
5. 重新构建（build_msvc.bat 需添加 LLVM include/lib 路径）

---

## 待调试输出清理

| 文件 | 行数 | 描述 |
|------|------|------|
| src/jit/jit_compiler.cpp | ~30 | [JIT] 状态消息 |
| src/jit/orc_jit.cpp | ~15 | [OrcJIT] 状态消息 |
| src/optimizer/escape_analyzer.cpp | 401+ | 逃逸分析结果（整段精美排版） |
| src/optimizer/function_inliner.cpp | 275 | 函数内联分析 |
| src/optimizer/loop_unroller.cpp | 436 | 循环展开分析 |
| src/optimizer/bytecode_optimizer.cpp | 41-46 | 优化统计 |
| src/optimizer/optimizer.cpp | 150 | 优化统计 |
| src/optimizer/tail_recursion_optimizer.cpp | 272 | 尾递归分析 |

建议：添加 `--verbose` / `-v` CLI 标志，将上述输出全部 gating

---

## 构建命令

```batch
rem MSVC 构建
call build_msvc.bat

rem CMake 构建（需先配置）
cmake -B build_cmake -G "Visual Studio 17 2022" -A x64
cmake --build build_cmake --config Release
```

---

## 快速恢复指令

新对话中粘贴：
```
请继续完善 CP语言编译器。运行 build\cplang -c tests\test_simple.cp 验证功能，
然后继续 DEBUG_STATUS.md 中记录的待办事项。
```
