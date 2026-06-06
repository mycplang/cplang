# CP 语言编译器/虚拟机 — 当前状态

**最后更新**: 2026-05-29

---

## 架构总览（双引擎架构）

```
源码(.cp)
    │
    ▼
── Compiler（共享前端）──
   Lexer → Parser → Semantic → AST + 字节码
    │
    ├──→ DevEngine ───────── 开发/学习/脚本
    │       DevMode::Bytecode  → 字节码 VM 解释执行
    │       DevMode::Hotspot   → 字节码 + 热点 JIT（95x 加速）
    │       DevMode::JitAll    → 全量 JIT 预编译
    │
    └──→ AOTEngine ───────── 部署/发布
            LLVM IR → .obj → .exe
```

### 2026-05-29 重构（架构梳理）

1. **前端统一**：AOT 不再自建 Lexer+Parser，走同一 Compiler 前端
2. **VM-JIT 解耦**：80 行 JIT 分派从 `vm_exec.cpp` 提取到独立 `jit_dispatch.cpp`
3. **JIT 字段外部化**：`jitEntry`/`jitCompiled` 从 VMFunction 移到外部 `JITRegistry`
4. **模块导入提取**：130 行 import 回调提取为独立 `resolveImport()` 函数
5. **死代码清理**：`vm_exec.cpp` 空循环、ExternalJIT 警告输出
6. **模式语义整理**：`-c`/`-j` 通过 `DevMode` 枚举明确分离

---

## ✅ 已完成功能

### 核心编译管线
- ✅ Lexer（词法分析器）- 支持 UTF-8 中文关键字
- ✅ Parser（语法分析器）- 生成 AST（已拆分为 4 个文件）
- ✅ Semantic Analyzer（语义分析器）- 类型检查 + 作用域
- ✅ Codegen（代码生成器）- 16字节指令格式
- ✅ Optimizer（优化器）- 常数折叠/死代码消除/内联/循环展开/尾递归/逃逸分析
- ✅ VM（虚拟机）- 栈式虚拟机，三色标记 GC
- ✅ LLVM ORC JIT（即时编译）- 内存中编译，95x 加速
- ✅ AOT（预编译）- LLVM IR → .obj → .exe

### 数据类型
- ✅ 整数（Int8/16/32/64）、浮点（Float32/64）、布尔（真/假）
- ✅ 字符串（UTF-8）、数组、表（Map）、结构体、Nil（空）
- ✅ NaN-Boxing：64位紧凑值表示，小整数/Nil/Bool 零堆分配

### 运算符
- ✅ 算术：`+` `-` `*` `/` `%` `^`
- ✅ 比较：`==` `!=` `<` `>` `<=` `>=`
- ✅ 逻辑：`&&` `||` `!`
- ✅ 位运算：`&` `|` `^` `~` `<<` `>>`

### 控制流
- ✅ if/else（`如果/否则`）、while（`当`）、for（`循环`）、for-each（`遍历`）
- ✅ return（`返回`）、try/catch/throw（异常处理）
- ✅ switch/case、break/continue、defer

### 函数
- ✅ 函数声明（`函数`）、函数对象、λ 谓词
- ✅ 递归、多参数、可选类型标注
- ✅ 函数泛型 + 结构体泛型 + 类型约束

### 中文支持
- ✅ 中英文双语关键字
- ✅ 中文变量名、中文函数名
- ✅ 中文错误信息（词法/语法/语义/运行时）
- ✅ UTF-8 字符串字面量

### 内存管理
- ✅ 三色标记-清除 GC（自动回收）
- ✅ NaN-Boxing 64位紧凑值表示

### 面向对象
- ✅ 类定义 + 继承 + 字段 + 方法
- ✅ 结构体定义 + 实例化 + 字段访问

## 🔧 标准库（800+ 函数，775+ 中文别名）

| 类别 | 模块数 | 内容 |
|------|--------|------|
| 容器 | 26 | 数组、表、栈、队列、双端队列、优先队列、链表、集合、映射等 |
| 算法 | 4 | 排序/查找/分区/全排列/随机洗牌/逆序 |
| 字符串 | 12 | 正则/Base64/Hex/URL/SHA256/MD5/CRC32/SHA512/UUID/CSV/编码 |
| 并发 | 10 | 线程/互斥/条件/信号量/原子/Barrier/Future/Channel/RWLock/TLS |
| I/O | 8 | 基础IO/文件/时间/进程/JSON/环境变量/网络(HTTP/TCP/UDP/DNS) |
| 加密 | — | AES-128/256-CBC、BCryptGenRandom、压缩(gzip/deflate) |
| 字符集 | — | UTF-8 ↔ GBK/Big5/Shift-JIS |
| 数据库 | 3 | SQLite/MySQL/Redis |
| 图形 | 2 | Raylib 2D/3D、ImGui UI |

## ⚡ 性能指标

| 场景 | 字节码 | JIT 编译 | 加速比 |
|------|--------|---------|--------|
| 1亿次循环 | 超时 | 0.35s | 100x+ |
| 1000万次函数调用 | 5.4s | 0.057s | 95x |
| 类型化函数执行 | 6.8s | 0.10s | 65x |

## 📝 已知问题

1. JIT 模式对中文标准库函数的符号链接不完全（`__u957F__` 编码名未全部注册）
2. AOT 生成的 .exe 链接时有多重 `LNK4006` 警告（`/FORCE:MULTIPLE` 绕过）
3. `printVal` 无 Bool 显式输出（已修复：显示"真"/"假"）
4. 部分调试输出待清理

## 📊 测试状态

- 回归测试：193/197 通过（4 个超时为基准测试）
- 教学示例：11 个全部通过
- JIT 基准：LLVM ORC JIT 100x 加速验证通过
- AOT：独立 .exe 生成 + 链接通过
