# AOT 编译 — 将 CP 语言程序编译为独立可执行文件

## 概述

AOT（Ahead-of-Time）编译器将 CP 语言源码直接编译为 Windows x64 可执行文件 (`.exe`)，无需 CP 运行时虚拟机即可独立运行。

**工作流程：**
```
.cp 源文件 → LLVM IR → llc (.obj) → MSVC link.exe (.exe)
```

## 使用方法

```bash
# 基本 AOT 编译（支持打印、字符串拼接）
cplang --aot input.cp -o output.exe

# 纯数学模式（零运行时依赖，适合形式化验证/分析工具消费）
cplang --aot --pure-math input.cp -o output.exe

# 仅生成 LLVM IR（不含链接步骤）
cplang --emit-llvm input.cp -o output.ll
```

### 示例

```bash
cplang --aot hello.cp -o hello.exe
hello.exe
# 输出: Hello, World!
```

## 兼容性

### ✅ 支持的 CP 语言特性

| 特性 | 说明 |
|------|------|
| 算术运算 | `+ - * / %` 全部支持 |
| 逻辑运算 | `&& \|\| !` 短路求值 |
| 控制流 | `如果/if`、`否则/else`、`当/while`、`对于/for` |
| 函数定义 | `func/函数 name()` 完整支持 |
| `func main()` | 自动适配为入口点 |
| 结构体 | `struct/结构` 完整支持 |
| `打印/print/println` | 通过运行时库支持 |
| 字符串拼接 | `"Hello, " + x` 通过运行时库支持 |
| 中文关键字 | `如果`、`否则`、`当`、`对于`、`变量` 等 |
| 双语言语法 | `for (var n in arr)` 英文语法 |
| UTF-8 BOM | 自动跳过 BOM 标记 |
| 数学常量 | `tau()`、`sqrt2()`、`goldenRatio()` |

### ❌ 不支持的特性

| 特性 | 原因 | 替代方案 |
|------|------|----------|
| 标准库原生函数 | `fmt()`、`resOk()` 等 VM 原生函数无独立实现 | 用纯 CP 重写 |
| Raylib 图形 | 依赖动态库 raylib.dll | 无 |
| Lambda/闭包 | LLVM codegen 未实现闭包捕获 | — |
| 类/对象 | 字节码 VM 独有特性 | — |
| 数组字面量 | LLVM codegen 未实现 | 手动展开 |
| 多线程 | 无 AOT 运行时线程支持 | — |
| SQLite/MySQL/Redis | 依赖外部 C 库 | — |
| 模块/import | 依赖 VM 模块加载器 | — |

> **经验法则：** 只要不依赖标准库原生函数、图形库或高级 VM 特性，AOT 编译就能工作。

## 技术细节

### 依赖的工具链

AOT 流水线需要以下工具（自动检测路径）：

| 工具 | 位置 | 用途 |
|------|------|------|
| `llc.exe` | `llvm-dev\bin\` (LLVM 18.1.8) | LLVM IR → COFF 目标文件 |
| `link.exe` | MSVC 2022 `bin\Hostx64\x64\` | 链接 .obj → .exe |
| `cl.exe` | MSVC 2022 工具链 | 编译 `jit_runtime_standalone.cpp` 为 .lib |
| vswhere.exe | VS Installer | 自动检测 MSVC 安装路径 |

### NaN-boxing 运行时

AOT 可执行文件链接 `jit_runtime.lib`（来自 `src/jit/jit_runtime_standalone.cpp`），提供：

- **`jit_printv`** — 处理 `打印()`/`print()` 的变参输出
- **`jit_strcat`** — 处理字符串拼接的运行时分配
- **`tau`/`sqrt2`/`goldenRatio`** — 内联数学常量

`jit_runtime_standalone.cpp` 是**纯 C 实现**（无 C++ 标准库依赖），仅链接 `libcmt.lib` + `libucrt.lib` + `libvcruntime.lib`。

### 链接策略

| 模式 | 链接器 | CRT 处理 |
|------|--------|----------|
| 纯数学 (`--pure-math`) | `lld-link` | 无 CRT 依赖 |
| 标准 (`--aot`) | MSVC `link.exe` | 静态链接 `/MT`（`libcmt.lib`） |

### 入口点处理

```
源文件：func main() { ... }
                ↓ (LLVM codegen 重命名)
       LLVM IR: @__cp_main()
                ↓ (generateMainWrapper)
       LLVM IR: @main(i32, ptr) { call @__cplang_entry(); ret 0 }
                ↓ (__cplang_entry 调用 __cp_main)
       可执行文件入口：mainCRTStartup → main → __cplang_entry → __cp_main
```

## 与 JIT 模式的关系

```
AOT 编译器 = LLVM codegen（与 JIT 共享） + 目标文件生成 + 链接
JIT 编译器 = LLVM codegen（与 AOT 共享） + ORC JIT 即时编译
```

两者的 LLVM IR 代码生成器完全相同，区别仅在于执行路径：
- **JIT**：ORC JIT 在运行时编译并执行 IR，可解析 VM 原生符号
- **AOT**：一次性编译 IR → .obj → .exe，需要链接时提供所有符号

## 测试

```bash
# 编译 AOT 测试
cplang --aot tests/test_func.cp -o test.exe
test.exe
# 输出: 5

# 纯数学 AOT
cplang --aot --pure-math tests/test_func.cp -o test.exe
# 生成的 .exe 仅 3.5KB（零运行时依赖）
```

已知通过 AOT 编译的 20/25 个测试文件位于 `tests/` 目录。失败的文件均因使用了标准库原生函数或 raylib 图形库。

## 已知限制

1. **Double 输出格式**：运行时 `valueToString` 无法区分原始 IEEE 754 双精度和整数，打印 `tau()` 等浮点返回值时会显示整数位模式
2. **链接警告 LNK4088**：因 `/FORCE:MULTIPLE` 生成，无害
3. **`--pure-math` 模式**：但顶层脚本语句（`打印()`、变量声明等）被跳过，入口函数直接返回 0
4. **stdlib native 函数**: `fmt()`、`resOk()` 等仅限于 VM 运行时
