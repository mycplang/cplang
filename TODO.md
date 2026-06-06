# CPLANG 开发TODO清单

## ✅ 已完成

### 核心编译器
- ✅ 编译管线：Lexer → Parser → Semantic → Codegen → VM
- ✅ 优化器：常数折叠/死代码消除/函数内联/循环展开/尾递归/逃逸分析
- ✅ LLVM ORC JIT 集成，100x 加速验证通过
- ✅ AOT 预编译：LLVM IR → .obj → .exe

### 语言特性
- ✅ 中英文双语关键字
- ✅ 渐进类型系统（动态 + 可选类型标注）
- ✅ 所有权/借用检查（Rust 式 `&x` / `&可写 x`）
- ✅ 异常处理（try/catch/throw）
- ✅ 函数泛型 + 结构体泛型
- ✅ 类 + 继承 + 结构体
- ✅ switch/case + defer

### 标准库（800+ 函数）
- ✅ 容器全覆盖（26 模块）
- ✅ 算法（4 模块）
- ✅ 字符串工具（12 模块）
- ✅ 并发（10 模块 39 函数）
- ✅ I/O 与系统（8 模块）
- ✅ 加密（AES/随机数/HMAC）
- ✅ 字符集（UTF-8 ↔ GBK/Big5/Shift-JIS）
- ✅ 数据库（SQLite/MySQL/Redis）
- ✅ 图形（Raylib + ImGui）

### 工具链
- ✅ VSCode 插件（语法高亮 + LSP 客户端 + 代码片段）
- ✅ 语言服务器（cplsp.py：补全/跳转/悬停/诊断）
- ✅ 包管理器（cpkg，中英文双语命令）
- ✅ 代码格式化（cpfmt.py，幂等性验证通过）
- ✅ REPL（持久化历史/补全/命令系统/计时）
- ✅ 包管理器（中英文双语 `包` CLI）

### 错误处理
- ✅ 错误信息全汉化（词法/语法/语义/运行时）
- ✅ 错误行号标准化（"第X行第Y列"）
- ✅ 编译器前缀汉化（"语法分析:" / "语义分析:" / "代码生成:"）

### 2026-05-29 架构重构
- ✅ **前端统一**：AOT 不再自建 Lexer+Parser，走统一 Compiler 前端
- ✅ **VM-JIT 解耦**：JIT 分派从 `vm_exec.cpp` 提取到独立 `jit_dispatch.cpp`
- ✅ **JIT 字段外部化**：`jitEntry`/`jitCompiled` 从 VMFunction 移到 `JITRegistry`
- ✅ **模块导入提取**：130 行 import 回调提取为独立 `resolveImport()` 函数
- ✅ **死代码清理**：空循环、ExternalJIT 静默失败改为警告
- ✅ **模式语义整理**：`-c`/`-j` 通过 `DevMode` 枚举明确分离（Bytecode/Hotspot/JitAll）

## 🔴 高优先级

1. **JIT 中文符号链接修复**
   - 中文标准库函数（`长度`、`转字符串` 等）在 ORC JIT 中符号名编码为 `__u957F__u5EA6__`，
     这些编码名在 JIT 运行时中未注册，导致 `-j` 模式下的 `Symbols not found` 错误

2. **AOT 链接警告清理**
   - `LNK4006`（main 重复定义）和运行时 CRT 库冲突，由 `/FORCE:MULTIPLE` 绕过
   - 需清理 `cplang_full.lib` 中的重复符号

3. **回归测试通过率从 94% 提升到 100%**
   - 当前 193/197 通过，4 个基准测试因超时被跳过

## 🟠 中优先级

1. 完善 REPL 错误恢复（当前编译失败后 VM 状态有时不一致）
2. VSCode 插件适配新架构的 `DevMode` 枚举（语法提示中反映 -c / -j / --hotspot 语义）
3. 补充 AOT 对数组/类/多文件模块的支持文档限制说明

## 🟡 低优先级

1. 移除 `ExternalJIT` 的 clang 进程模式（维护成本 > 使用价值，已有 ORC JIT）
2. 增加更多示例项目：Web 服务、GUI 应用、游戏实战
3. 支持 macOS/Linux 平台
4. 建设社区生态：官方网站、文档站、论坛
5. 完善 `JITRegistry` 生命周期管理（VMFunction 销毁时清理对应条目）
