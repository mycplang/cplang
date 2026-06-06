# CP 语言 Code Wiki

> **CP 语言** — 一款支持原生中文语法的编程语言编译器/解释器系统，采用多引擎架构（字节码 VM + JIT + AOT）。

---

## 目录

1. [项目概述](#1-项目概述)
2. [项目架构概览](#2-项目架构概览)
3. [目录结构](#3-目录结构)
4. [模块详解](#4-模块详解)
   - [4.1 基础类型层 (common)](#41-基础类型层-common)
   - [4.2 词法分析器 (lexer)](#42-词法分析器-lexer)
   - [4.3 抽象语法树 (ast)](#43-抽象语法树-ast)
   - [4.4 语法分析器 (parser)](#44-语法分析器-parser)
   - [4.5 语义分析器 (semantic)](#45-语义分析器-semantic)
   - [4.6 代码生成器 (codegen)](#46-代码生成器-codegen)
   - [4.7 虚拟机 (vm)](#47-虚拟机-vm)
   - [4.8 优化器 (optimizer)](#48-优化器-optimizer)
   - [4.9 JIT 编译器 (jit)](#49-jit-编译器-jit)
   - [4.10 标准库 (stdlib)](#410-标准库-stdlib)
   - [4.11 模块系统 (module)](#411-模块系统-module)
   - [4.12 调试器 (debug)](#412-调试器-debug)
   - [4.13 异常处理 (exception)](#413-异常处理-exception)
   - [4.14 REPL 交互式环境 (repl)](#414-repl-交互式环境-repl)
   - [4.15 CLI 入口 (main)](#415-cli-入口-main)
   - [4.16 包管理器 (cpkg)](#416-包管理器-cpkg)
5. [依赖关系图](#5-依赖关系图)
6. [构建与运行](#6-构建与运行)
7. [执行模式](#7-执行模式)
8. [关键设计决策](#8-关键设计决策)
9. [数据流](#9-数据流)

---

## 1. 项目概述

CP 语言是一款**纯中文编程语言**，所有关键字、标准库函数均支持中文别名。采用 C++ 实现，支持多种执行模式：

| 执行模式 | 命令 | 启动速度 | 执行速度 | 适用场景 |
|---------|------|---------|---------|---------|
| 字节码 VM | `-c` | < 0.1s | 基准 | 开发、调试、脚本 |
| 热点 JIT | `-c --hotspot` | < 0.1s | 热点函数 ~95x | 长时间运行任务 |
| 全量 JIT | `-j` | < 0.1s + 预热 | ~95x 加速 | 计算密集型 |
| AOT 编译 | `-a -o app.exe` | 3-6s 编译 | 原生速度 | 发布部署 |

### 核心特性

- **纯中文语法** — 关键字、函数名、变量名均支持中文
- **双引擎架构** — DevEngine（VM+JIT）+ AOTEngine（LLVM 原生编译）
- **NaN-Boxing** — 64位紧凑值表示，Int8/Int16/Int32/Float32 零堆分配
- **渐进类型系统** — 动态类型起步 + 类型标注获得 JIT 加速
- **Rust 式所有权** — `&x` / `&可写 x` 借用规则、移动语义
- **垃圾回收** — 三色标记-清除 GC
- **泛型支持** — 函数泛型 + 结构体泛型（单态化）
- **800+ 标准库函数** — 26 容器模块、算法、加密、网络、数据库、图形等

---

## 2. 项目架构概览

```
源码(.cp)
    │
    ▼
┌─────────── 共享前端 (Compiler) ───────────┐
│  Lexer → Parser → Semantic → AST          │
│  (词法)   (语法)    (语义)                │
└────────────────┬──────────────────────────┘
                 │
    ┌────────────┴────────────┐
    ▼                         ▼
┌───────────────┐    ┌───────────────────┐
│  DevEngine    │    │   AOTEngine       │
│  ┌─────────┐  │    │  ┌─────────────┐  │
│  │ Optimizer│  │    │  │ LLVMCodegen │  │
│  │ (AST优化)│  │    │  │ (LLVM IR)   │  │
│  └────┬────┘  │    │  └──────┬──────┘  │
│       ▼       │    │         ▼         │
│  ┌─────────┐  │    │  ┌─────────────┐  │
│  │ Codegen  │  │    │  │ AOTCompiler │  │
│  │(字节码)  │  │    │  │ (clang+lld) │  │
│  └────┬────┘  │    │  └──────┬──────┘  │
│       ▼       │    │         ▼         │
│  ┌─────────┐  │    │  ┌─────────────┐  │
│  │   VM    │  │    │  │   .exe      │  │
│  │(字节码)  │  │    │  │ (原生代码)  │  │
│  └────┬────┘  │    │  └─────────────┘  │
│       │       │    │                   │
│  ┌────▼────┐  │    │                   │
│  │HybridJIT│  │    │                   │
│  │(热点加速)│  │    │                   │
│  └─────────┘  │    │                   │
└───────────────┘    └───────────────────┘
```

**编译 Pipeline 目标库分层**：

```
00-core:    cplang_common → cplang_token → cplang_core
01-frontend: cplang_lexer → cplang_ast → cplang_parser → cplang_semantic
02-backend:  cplang_vm → cplang_codegen
03-opt/jit:  cplang_optimizer → cplang_jit
04-runtime:  cplang_module → cplang_stdlib → cplang_debug → cplang_exception
99-cli:      cplang_cli / cplang_repl / cplang_cpkg
```

---

## 3. 目录结构

```
c:\CPLANG\
├── src/                        # 源码实现
│   ├── main.cpp                # CLI 入口
│   ├── repl.cpp                # 交互式 REPL
│   ├── lexer/lexer.cpp         # 词法分析器
│   ├── parser/                 # 语法分析器（已拆分）
│   │   ├── parser.cpp          #   核心/程序级
│   │   ├── parser_decl.cpp     #   声明解析
│   │   ├── parser_stmt.cpp     #   语句解析
│   │   └── parser_expr.cpp     #   表达式解析
│   ├── semantic/
│   │   └── semantic_analyzer.cpp  # 语义分析
│   ├── codegen/                # 代码生成
│   │   ├── codegen.cpp         #   字节码生成主文件
│   │   ├── codegen_opt.cpp     #   优化相关
│   │   ├── codegen_stmt.cpp    #   语句编译
│   │   ├── codegen_expr.cpp    #   表达式编译
│   │   ├── bytecode_optimizer.cpp  # 字节码优化
│   │   ├── aot_compiler.cpp    #   AOT 编译器
│   │   └── llvm_codegen.cpp    #   LLVM IR 生成
│   ├── vm/                     # 虚拟机
│   │   ├── vm.cpp              #   VM 核心/GC
│   │   ├── vm_exec.cpp         #   执行循环
│   │   ├── vm_containers.cpp   #   容器操作
│   │   ├── vm_objects.cpp      #   对象操作
│   │   ├── value.cpp           #   NaN-boxing 值
│   │   ├── vm_opt.cpp          #   computed goto 优化
│   │   └── vm_opt_stub.cpp     #   MSVC 兼容桩
│   ├── jit/                    # JIT 编译器
│   │   ├── jit_compiler.cpp    #   外部 JIT 编译器
│   │   ├── jit_runtime.cpp     #   运行时支持
│   │   ├── orc_jit.cpp         #   LLVM ORC JIT
│   │   ├── jit_dispatch.cpp    #   JIT 分派
│   │   └── hybrid_jit.cpp      #   混合 JIT 策略
│   ├── optimizer/              # AST 优化器
│   │   ├── optimizer.cpp       #   优化管理器
│   │   ├── constant_folder.cpp #   常量折叠
│   │   ├── dead_code_eliminator.cpp  # 死代码消除
│   │   ├── function_inliner.cpp      # 函数内联
│   │   ├── loop_unroller.cpp         # 循环展开
│   │   ├── tail_recursion_optimizer.cpp  # 尾递归优化
│   │   ├── escape_analyzer.cpp       # 逃逸分析
│   │   └── llvm_optimizer.cpp        # LLVM 优化pass
│   ├── stdlib/                 # 标准库
│   │   ├── stdlib.cpp          #   注册入口
│   │   ├── stdlib_stubs.cpp    #   桩实现
│   │   ├── stdlib_raylib_unit.cpp  # Raylib 绑定
│   │   ├── stdlib_imgui.cpp    #   ImGui 绑定
│   │   └── stdlib_fix_missing.cpp  # 缺失补全
│   ├── module/module_system.cpp  # 模块系统
│   ├── debug/debugger.cpp      # 调试器
│   ├── exception/exception_handler.cpp  # 异常处理
│   ├── cpkg/cpkg.cpp           # 包管理器
│   ├── miniz.c / miniz_tdef.c / miniz_tinfl.c  # 压缩库
│   ├── crypto/md5_impl.cpp     # MD5 实现
│   └── sqlite/sqlite3.c        # SQLite 嵌入
├── include/                    # 头文件
│   ├── common/types.hpp        # 基础类型别名
│   ├── lexer/
│   │   ├── token.hpp           # Token 类型/关键字表
│   │   └── lexer.hpp           # 词法分析器接口
│   ├── ast/ast.hpp             # 所有 AST 节点定义
│   ├── parser/parser.hpp       # 语法分析器接口
│   ├── semantic/
│   │   ├── semantic_analyzer.hpp  # 语义分析器
│   │   └── typesystem.hpp      # 类型系统
│   ├── codegen/
│   │   ├── codegen.hpp         # 代码生成器/Compiler
│   │   ├── bytecode_optimizer.hpp  # 字节码优化器
│   │   ├── aot_compiler.hpp    # AOT 编译器
│   │   └── llvm_codegen.hpp    # LLVM IR 生成器
│   ├── vm/
│   │   ├── vm_fwd.hpp          # 前向声明/ObjectHeader
│   │   ├── value.hpp           # NaN-boxing Value
│   │   ├── vm_opcodes.hpp      # 字节码指令枚举
│   │   ├── vm_object.hpp       # VM 对象结构体
│   │   ├── vm_types.hpp        # CallFrame/HandlerFrame
│   │   ├── vm_class.hpp        # VM 类定义
│   │   ├── vm_value_helpers.hpp # Value 辅助函数
│   │   ├── vm_optimizations.hpp # 运行时优化
│   │   └── inline_cache.hpp    # 内联缓存
│   ├── optimizer/
│   │   ├── optimizer.hpp       # 优化管理器
│   │   ├── constant_folder.hpp
│   │   ├── dead_code_eliminator.hpp
│   │   ├── function_inliner.hpp
│   │   ├── loop_unroller.hpp
│   │   ├── tail_recursion_optimizer.hpp
│   │   ├── escape_analyzer.hpp
│   │   └── llvm_optimizer.hpp
│   ├── jit/
│   │   ├── jit_compiler.hpp    # JIT 编译器
│   │   ├── hybrid_jit.hpp      # 混合 JIT
│   │   ├── orc_jit.hpp         # ORC JIT
│   │   ├── jit_dispatch.hpp    # JIT 分派
│   │   └── jit_runtime.hpp     # JIT 运行时
│   ├── stdlib/
│   │   ├── stdlib.hpp          # 汇总头文件
│   │   ├── stdlib_fwd.hpp      # StdLib 注册类
│   │   ├── stdlib_math.hpp     # 各模块声明
│   │   ├── stdlib_string.hpp
│   │   ├── stdlib_array.hpp
│   │   ├── stdlib_table.hpp
│   │   ├── stdlib_io.hpp
│   │   ├── stdlib_file.hpp
│   │   ├── stdlib_containers.hpp
│   │   ├── stdlib_time_system.hpp
│   │   ├── stdlib_types_net.hpp
│   │   ├── stdlib_regex_encoding_crypto.hpp
│   │   ├── stdlib_algo_bitwise.hpp
│   │   └── stdlib_variant_utils.hpp
│   ├── module/module_system.hpp
│   ├── debug/debugger.hpp
│   ├── exception/exception_handler.hpp
│   └── repl/repl.hpp
├── tests/                      # 测试文件
│   ├── gtest/                  # Google Test 单元测试
│   │   ├── test_lexer.cpp
│   │   ├── test_parser.cpp
│   │   ├── test_vm.cpp
│   │   ├── test_optimizer.cpp
│   │   ├── test_semantic.cpp
│   │   └── test_e2e.cpp
│   └── test_new_stdlib.cp      # CP 语言集成测试
├── examples/                   # 示例程序
├── docs/                       # 文档
├── third_party/                # 第三方依赖
│   ├── raylib/                 # 图形库
│   └── imgui/                  # GUI 库
├── cmake/                      # CMake 模块
│   ├── CompilerWarnings.cmake
│   ├── FindLLVM.cmake
│   └── Sanitizers.cmake
├── scripts/                    # 辅助脚本
├── CMakeLists.txt              # 主构建文件
└── README.md                   # 项目说明
```

---

## 4. 模块详解

### 4.1 基础类型层 (common)

**文件**: `include/common/types.hpp`

定义编译器全局使用的基础类型别名和枚举。

**关键类型别名**:
```cpp
namespace cplang {
    using Int8    = int8_t;      using Int16   = int16_t;
    using Int32   = int32_t;     using Int64   = int64_t;
    using UInt8   = uint8_t;     using UInt16  = uint16_t;
    using UInt32  = uint32_t;    using UInt64  = uint64_t;
    using Float32 = float;       using Float64 = double;
    using String  = std::string;
    template<typename T> using Optional = std::optional<T>;
    template<typename T> using Unique   = std::unique_ptr<T>;
    template<typename T> using Shared   = std::shared_ptr<T>;
}
```

**关键枚举**:
```cpp
enum class OptLevel : int {
    None = 0,  // 无优化
    O1   = 1,  // 基本优化
    O2   = 2,  // 中等优化（默认推荐）
    O3   = 3   // 激进优化
};
```

---

### 4.2 词法分析器 (lexer)

**文件**: `include/lexer/token.hpp`, `include/lexer/lexer.hpp` | `src/lexer/lexer.cpp`

#### TokenType 枚举

定义约 150 种 Token 类型，涵盖中文关键字、英文关键字、运算符和分隔符。

**中文关键字** (核心):
| 关键字 | Token 类型 | 含义 |
|--------|-----------|------|
| 包名 | K_PACKAGE | package |
| 导入 | K_IMPORT | import |
| 类 | K_CLASS | class |
| 接口 | K_INTERFACE | interface |
| 枚举 | K_ENUM | enum |
| 结构体 | K_STRUCT | struct |
| 函数 | K_FUNC | function |
| 返回 | K_RETURN | return |
| 如果/否则 | K_IF/K_ELSE | if/else |
| 选择/情况/其他 | K_SWITCH/K_CASE/K_DEFAULT | switch/case/default |
| 循环/遍历 | K_FOR/K_FOREACH | for/foreach |
| 当/为 | K_WHILE/K_DO | while/do |
| 跳出/继续 | K_BREAK/K_CONTINUE | break/continue |
| 尝试/捕获/抛出/最终 | K_TRY/K_CATCH/K_THROW/K_FINALLY | try/catch/throw/finally |
| 变量/常量 | K_VAR/K_CONST | var/const |
| 设 | K_LET | let |
| 且/或/非 | K_AND/K_OR/K_NOT | and/or/not |
| 是/等于 | K_EQ | equality |
| 可写 | K_BORROW_MUT | mutable borrow |
| 可信 | K_TRUST | unsafe block |
| 移动 | K_MOVE | move |
| 释放 | K_DROP | drop |
| 推迟 | K_DEFER | defer |

**所有中文关键字同时支持英文备选形式**（如 `if`、`while`、`var`）。

#### Token 结构
```cpp
struct Token {
    TokenType type;                          // Token 类型
    String text;                             // 文本表示
    String raw;                              // 原始文本
    Int32 line, column;                      // 源码位置
    Variant<Int64, Float64, String, bool> value;  // 字面量值
};
```

#### KeywordTable 类

单例模式的关键字映射表，包含中英文关键字到 `TokenType` 的映射。通过 `find()` 方法查找关键字对应的 Token 类型。

#### Lexer 类

| 方法 | 功能 |
|------|------|
| `Lexer(source)` | 构造函数，初始化词法分析器 |
| `nextToken()` | 读取下一个 Token |
| `peekToken()` | 预读下一个 Token（不消费） |
| `reset()` | 重置词法分析器状态 |
| `hasError()` | 是否有词法错误 |
| `errorMessage()` | 获取错误信息 |
| `currentLine()` / `currentColumn()` | 当前位置 |

**内部扫描器**:
- `scanId_()` — 扫描标识符和关键字
- `scanNum_()` — 扫描数字字面量（整数/浮点）
- `scanStr_()` — 扫描字符串字面量
- `scanComment_()` — 扫描注释（`//` 和 `/* */`）
- `scanOp_()` — 扫描运算符

---

### 4.3 抽象语法树 (ast)

**文件**: `include/ast/ast.hpp`

#### 节点层次结构

```
ASTNode (基类: token)
├── Expr (表达式基类)
│   ├── LiteralExpr         — 字面量 (整数/浮点/字符串/布尔)
│   ├── IdentifierExpr      — 标识符引用
│   ├── BinaryExpr          — 二元表达式 (left op right)
│   ├── UnaryExpr           — 一元表达式 (op operand)，支持前置/后置
│   ├── CallExpr            — 函数调用 (callee, arguments, typeArgs)
│   ├── MemberExpr          — 成员访问 (object.member)
│   ├── IndexExpr           — 数组访问 (array[index])
│   ├── ArrayExpr           — 数组字面量 [a, b, c]
│   ├── StructLiteralExpr   — 结构体字面量 {字段: 值, ...}
│   ├── NewExpr             — 新建表达式 (new 类名(args))
│   ├── BorrowExpr          — 借用表达式 (&x, &可写 x)
│   ├── MoveExpr            — 移动表达式 (移动 x)
│   └── DropExpr            — 释放表达式 (释放 x)
│
└── Stmt (语句基类)
    ├── ExprStmt            — 表达式语句
    ├── EmptyStmt           — 空语句
    ├── BlockStmt           — 块语句 { statements }
    ├── VarDeclStmt         — 变量声明 (name, type?, init, isConst)
    ├── FuncDeclStmt        — 函数声明 (name, typeParams, params, returnType, body)
    ├── ClassDeclStmt       — 类声明 (name, baseClass, members)
    ├── InterfaceDeclStmt   — 接口声明 (name, methods)
    ├── EnumDeclStmt        — 枚举声明 (name, values)
    ├── StructDeclStmt      — 结构体声明 (name, typeParams, members)
    ├── IfStmt              — if 语句 (condition, thenBranch, elseBranch?)
    ├── SwitchStmt          — switch 语句 (expr, cases, defaultCase?)
    ├── ForStmt             — for 循环 (init, condition, update, body)
    ├── ForEachStmt         — for-each 循环 (varName, iterable, body)
    ├── WhileStmt           — while 循环 (condition, body)
    ├── DoWhileStmt         — do-while 循环 (body, condition)
    ├── BreakStmt           — break 语句
    ├── ContinueStmt        — continue 语句
    ├── ReturnStmt          — return 语句 (value?)
    ├── ThrowStmt           — throw 语句 (exception)
    ├── TryStmt             — try-catch-finally (tryBlock, catchBlocks, finallyBlock?)
    ├── DeferStmt           — defer 语句 (body)
    ├── ImportStmt          — import 语句 (moduleName, alias?)
    ├── PackageStmt         — package 语句 (name)
    └── TrustBlockStmt      — 可信块 (unsafe { body })
```

**程序根节点**:
```cpp
struct Program : ASTNode {
    Optional<Shared<PackageStmt>> package;
    std::vector<Shared<Stmt>> statements;
};
```

---

### 4.4 语法分析器 (parser)

**文件**: `include/parser/parser.hpp` | `src/parser/parser*.cpp`

`Parser` 类使用**递归下降**解析方法，采用三层 lookahead（`current_`, `peek_`, `peek2_`, `peek3_`）来消除歧义（如泛型调用 `函数<类型>()` 与比较 `a < b`）。

#### 关键方法

| 方法 | 功能 |
|------|------|
| `parse()` | 解析入口，返回 `Shared<Program>` |
| `parseExpression()` | 公开的表达式解析接口（用于 `${}` 插值） |
| `parseProgram()` | 解析整个程序 |
| `parseStatement()` | 解析任意语句 |
| `parseDeclaration()` | 解析声明（函数/类/接口/枚举/结构体/变量） |
| `parseFunctionDecl()` | 解析函数声明 |
| `parseClassDecl()` | 解析类声明 |
| `parsePackage()` / `parseImport()` | 解析包/导入语句 |

**表达式解析** (按优先级从低到高):
```
parseAssignment()      → =, +=, -=, *=, /=, %=
parseTernary()         → ?:
parseOr()              → ||, 或
parseAnd()             → &&, 且
parseBitOr()           → |
parseBitXor()          → ^
parseBitAnd()          → &
parseEquality()        → ==, !=, 是, 等于, 不等于
parseComparison()      → <, >, <=, >=, 大于, 小于, ...
parseShift()           → <<, >>
parseAdditive()        → +, -
parseMultiplicative()  → *, /, %
parseUnary()           → -, !, ~, ++, --
parsePostfix()         → ., [], (), ++, --
parsePrimary()         → 字面量, 标识符, 括号
```

**便捷函数**:
```cpp
Shared<Program> parseString(const String& source);   // 解析字符串
Shared<Program> parseFile(const String& filename);   // 解析文件
Shared<Expr>   parseExprString(const String& source); // 解析表达式
```

**错误恢复**: `synchronize()` 方法在遇到错误时跳过 Token 直到找到同步点（分号或关键字），使解析器能继续解析后续代码。

---

### 4.5 语义分析器 (semantic)

**文件**: `include/semantic/semantic_analyzer.hpp`, `include/semantic/typesystem.hpp` | `src/semantic/semantic_analyzer.cpp`

#### 类型系统

**BuiltinType 枚举**:
```cpp
enum class BuiltinType {
    UNKNOWN, VOID, INT, FLOAT, BOOL, STRING, CHAR,
    INT8, INT16, INT32, INT64,
    UINT8, UINT16, UINT32, UINT64,
    FLOAT32, FLOAT64,
    OBJECT, ARRAY, FUNCTION, ENUM, STRUCT
};
```

**Type 结构**: 描述类型信息，包含 `kind`、`name`、`innerType`（数组元素类型）、`generics`（泛型参数）、`isConst` 等属性。提供静态工厂方法 (`int_()`, `int8_()`, `string_()` 等) 和 `equals()` 比较方法。

**TypeRegistry**: 单例类型缓存，避免重复分配 Type 对象。

#### 符号表

**Symbol 结构**: 表示一个符号（变量/函数/类/参数/字段/常量/枚举值/类型别名/标签），包含：
- `kind` — 符号类型
- `name` — 符号名
- `type` — 类型信息
- `node` — 原始 AST 节点
- `scopeLevel` — 作用域层级
- 函数特有: `params`, `returnType`, `isVariadic`
- 类特有: `classType`, `baseClass`

**Scope 结构**: 作用域，支持嵌套。通过 `find()` 递归查找符号，`define()` 定义符号，`findLocal()` 仅在当前作用域查找。

#### SemanticAnalyzer 类

**核心方法**:

| 类别 | 方法 | 功能 |
|------|------|------|
| 入口 | `analyze(program)` | 分析整个程序 |
| 声明 | `analyzeFuncDecl()` | 分析函数声明 |
| 声明 | `analyzeClassDecl()` | 分析类声明 |
| 声明 | `analyzeStructDecl()` | 分析结构体声明 |
| 声明 | `analyzeVarDecl()` | 分析变量声明 |
| 语句 | `analyzeStmt()` | 分析任意语句 |
| 语句 | `analyzeBlock()` | 分析块语句 |
| 语句 | `analyzeIf()` / `analyzeFor()` / `analyzeWhile()` | 分析控制流 |
| 表达式 | `analyzeExpr()` | 分析任意表达式 |
| 表达式 | `analyzeBinaryExpr()` | 分析二元表达式 |
| 表达式 | `analyzeCallExpr()` | 分析函数调用 |
| 表达式 | `analyzeMemberExpr()` | 分析成员访问 |
| 类型 | `getExprType(expr)` | 获取表达式类型 |
| 类型 | `isAssignableTo(from, to)` | 类型兼容性检查 |
| 所有权 | `checkBorrow()` | 借用检查 |
| 所有权 | `moveVariable()` | 移动变量所有权 |
| 所有权 | `checkVariableMoved()` | 检查变量是否已移动 |
| 泛型 | `instantiateGenericFunc()` | 泛型函数单态化 |
| 泛型 | `ensureGenericStructInstantiated()` | 泛型结构体实例化 |
| 泛型 | `checkTypeConstraints()` | 泛型约束检查 |

**所有权/借用追踪**:
- `borrowCounts_` — 跟踪每个变量的可变/不可变借用计数
- `movedVars_` — 跟踪已移动的变量
- `trustDepth_` — 可信块嵌套深度（>0 时跳过借用检查）
- 支持 Rust 式借用规则：不可变借用可多个，可变借用独占

**泛型单态化**: 为每个泛型函数的具体类型参数组合生成独立的函数声明，存储在 `monomorphizedFunctions_` 中供代码生成器使用。

---

### 4.6 代码生成器 (codegen)

**文件**: `include/codegen/codegen.hpp`, `include/codegen/bytecode_optimizer.hpp`, `include/codegen/aot_compiler.hpp`, `include/codegen/llvm_codegen.hpp` | `src/codegen/*.cpp`

#### Codegen 类 — 字节码生成器

将 AST 转换为字节码指令序列。

**关键方法**:

| 类别 | 方法 | 功能 |
|------|------|------|
| 入口 | `compile(program)` | 从 AST 生成字节码函数 |
| 配置 | `setOptLevel(level)` | 设置优化级别 |
| 配置 | `setEnableBytecodeOpt(bool)` | 启用/禁用字节码优化 |
| 指令发射 | `emit(op, a, b, c)` | 发射 4 字节指令 |
| 指令发射 | `emitInt(op, a, imm)` | 发射带立即数的指令 |
| 跳转 | `emitJump(op, offset)` | 发射跳转指令 |
| 跳转 | `emitJumpPlaceholder(op, a)` | 发射占位跳转（后续回填） |
| 跳转 | `patchJump(pos, target)` | 回填跳转目标 |
| 寄存器 | `allocReg()` / `freeRegs(n)` | 寄存器分配/释放 |
| 常量 | `addConstant(v)` | 添加常量到常量池 |
| 语句编译 | `compileStmt()` / `compileIf()` / `compileFor()` / `compileWhile()` / `compileTry()` / `compileSwitch()` 等 | 各语句编译 |
| 表达式编译 | `compileExpr()` / `compileBinary()` / `compileCall()` / `compileMember()` 等 | 各表达式编译 |
| 常量折叠 | `canFold()` / `foldConstant()` | 编译时常量折叠 |
| 类型化 | `emitTypedArithmetic()` / `emitTypedComparison()` | 类型化指令发射（渐进类型） |

**内部数据结构**:
- `constants_` — 常量池（`std::vector<Value>`）
- `localScopes_` — 局部变量作用域栈
- `labels_` — 跳转标签管理
- `loopStack_` — 循环上下文栈（break/continue 目标）
- `deferStack_` — Defer 语句栈
- `classMeta_` — 类元信息（字段名、方法）

#### Compiler 类 — 完整编译器入口

封装 Lexer → Parser → SemanticAnalyzer → Codegen 的完整编译流程。

```cpp
class Compiler {
    VMFunction* compile(const String& source, const String& sourceFile = "<string>");
    VMFunction* compileFile(const String& filename);
    VM* vm();
    void setOptLevel(OptLevel level);
    void setEnableBytecodeOpt(bool enable);
    void setTraceVM(bool v);
};
```

#### BytecodeOptimizer 类 — 字节码优化器

在字节码层面进行优化，提升 VM 执行效率。

**优化通道**:
| 通道 | 方法 | 功能 |
|------|------|------|
| 窥孔优化 | `peepholeOptimize()` | 查找并替换常见指令模式 |
| 死指令消除 | `eliminateDeadInstructions()` | 移除不可达/无用指令 |
| 常量传播 | `propagateConstants()` | 在字节码中传播常量值 |
| 基本块重排 | `reorderBasicBlocks()` | 优化指令缓存局部性 |
| 寄存器分配 | `allocateRegisters()` | 寄存器分配优化 |
| 辅助 | `buildCFG()` | 构建控制流图 |
| 辅助 | `mergeLoads()` | 合并连续 LOAD 指令 |
| 辅助 | `removeRedundantStores()` | 移除冗余 STORE 指令 |

**统计信息** (`BytecodeOptStats`): `peepholesApplied`, `deadInstructionsRemoved`, `constantPropagations`, `registersAllocated`, `totalBytesSaved`

#### LLVMCodegen 类 — LLVM IR 生成器

使用 LLVM C++ API 将 AST 直接编译为 LLVM IR，支持：
- 完整程序 IR 生成: `generate(program, opt)`
- 单函数 IR 生成（热点编译）: `generateSingleFunction(program, funcName, opt)`
- IR 字符串输出: `generateIRString(program, opt)`

#### AOTCompiler 类 — 提前编译器

将 CP 代码编译为独立可执行文件。

**配置** (`AOTConfig`):
- `optLevel` — 优化级别
- `outputFile` — 输出文件名
- `emitLLVM` — 只输出 LLVM IR
- `pureMath` — 纯数学模式（跳过 NaN-boxing 分派）
- `llvmToolsDir` / `msvcToolsDir` — 工具链路径

**编译流程**:
1. `generateLLVMIR()` — 生成 LLVM IR
2. `compileIRToObject()` — 使用 clang 编译为 .obj
3. `linkToExecutable()` — 使用 lld-link 链接为 .exe

---

### 4.7 虚拟机 (vm)

**文件**: `include/vm/*.hpp` | `src/vm/*.cpp`

#### Value — NaN-Boxing 值表示

**文件**: `include/vm/value.hpp` | `src/vm/value.cpp`

64 位紧凑值表示，核心设计：

```
bits 48-63 = 0xFFFF → 标记为非浮点值
  bit 47 = 1 → 立即值 (immediate)
    bit 46 = 0 → 特殊值 (Nil/False/True)
    bit 46 = 1 → 类型化立即值 (Int8/16/32, Float32)
  bit 47 = 0 → 对象指针 (bits 0-47 = 48-bit 指针)

编码布局:
  0xFFFF_8000_0000_0000 = Nil
  0xFFFF_8000_0000_0001 = False
  0xFFFF_8000_0000_0002 = True
  0xFFFF_C000_0000_iiii = Int8   (低8位)
  0xFFFF_C000_0001_iiii = Int16  (低16位)
  0xFFFF_C000_1000_iiii = Int32  (低32位)
  0xFFFF_C000_2000_ffff = Float32 (装箱)
  0xFFFF_C000_4000_pppp = 对象指针 (低48位)
```

**核心优势**: Int8/Int16/Int32/Float32/Bool/Nil 完全不分配堆内存，sizeof(Value) = 8 字节。

**关键工厂方法**:
| 方法 | 说明 |
|------|------|
| `nil()` / `Bool(b)` / `True()` / `False()` | 特殊值 |
| `fromInt8(x)` / `fromInt16(x)` / `fromInt32(x)` | 内联整数 |
| `Int(x)` | 通用整数（Int32 内内联，否则装箱） |
| `fromFloat32(x)` | 内联 Float32 |
| `fromFloat(x)` / `fromFloat64(x)` | Float64（直接存 IEEE 754） |
| `Ptr(obj)` | 对象指针 |
| `String(s)` / `Array(a)` / `Table(t)` / ... | 各对象类型 |

**类型检测**: `isInt8()`, `isInt16()`, `isInt32()`, `isInt()`, `isFloat32()`, `isFloat64()`, `isDouble()`, `isNil()`, `isBool()`, `isNumber()`, `isPtr()`, `isObject()`, `isString()`, `isArray()`, `isTable()`, `isFunction()`, `isClosure()`, `isClass()`, `isInstance()` 等 30+ 方法。

#### Opcode — 字节码指令集

**文件**: `include/vm/vm_opcodes.hpp`

约 130 条指令，按功能分组：

| 类别 | 指令 | 说明 |
|------|------|------|
| 加载/存储 | `OP_LOADNIL`, `OP_LOADBOOL`, `OP_LOADINT`, `OP_LOADFLT`, `OP_LOADSTR`, `OP_LOADCONST`, `OP_MOVE` | 常量加载 |
| 全局变量 | `OP_LOADGLOBAL`, `OP_STOREGLOBAL` | 全局变量访问 |
| 局部变量 | `OP_LOADLOCAL`, `OP_STORELOCAL` | 局部变量访问 |
| 算术 (通用) | `OP_ADD`, `OP_SUB`, `OP_MUL`, `OP_DIV`, `OP_IDIV`, `OP_MOD`, `OP_POW`, `OP_NEG` | 通用算术 |
| 算术 (Int32) | `OP_IADD` ~ `OP_INEG` | 类型化 Int32 算术 |
| 算术 (Float64) | `OP_FADD` ~ `OP_FNEG` | 类型化 Float64 算术 |
| 算术 (Int8) | `OP_I8ADD` ~ `OP_I8NEG` | 类型化 Int8 算术 |
| 算术 (Int16) | `OP_I16ADD` ~ `OP_I16NEG` | 类型化 Int16 算术 |
| 算术 (Float32) | `OP_F32ADD` ~ `OP_F32NEG` | 类型化 Float32 算术 |
| 比较 (通用) | `OP_CMPEQ` ~ `OP_CMPGE` | 通用比较 |
| 比较 (类型化) | `OP_ICMPEQ` ~ `OP_FCMPGE`, `OP_I8CMPEQ` ~ `OP_F32CMPGE` | 类型化比较 |
| 位运算 | `OP_BAND`, `OP_BOR`, `OP_BXOR`, `OP_BSHL`, `OP_BSHR`, `OP_BNOT` | 位操作 |
| 逻辑 | `OP_NOT` | 逻辑非 |
| 跳转 | `OP_JUMP`, `OP_JUMPIF`, `OP_JUMPNIF` | 控制流 |
| 调用 | `OP_CALL`, `OP_CALLMETHOD`, `OP_RETURN` | 函数调用 |
| 数组 | `OP_NEWARRAY`, `OP_GETELEM`, `OP_SETELEM`, `OP_GETIDX`, `OP_SETIDX` | 数组操作 |
| 字符串 | `OP_CONCAT`, `OP_STRLEN`, `OP_GETLEN` | 字符串操作 |
| 类型转换 | `OP_TONUM`, `OP_TOSTR`, `OP_TOBool`, `OP_TYPEOF`, `OP_ISNULL` | 类型转换 |
| 异常 | `OP_TRY`, `OP_ENDTRY`, `OP_THROW` | 异常处理 |
| 对象 | `OP_NEWCLASS`, `OP_NEWSTRUCT`, `OP_GETFIELD`, `OP_SETFIELD` | 类/结构体 |
| 模块 | `OP_IMPORT` | 模块导入 |
| 其他 | `OP_NOP` | 空操作 |

#### VM 对象模型

**文件**: `include/vm/vm_fwd.hpp`, `include/vm/vm_object.hpp`

**ObjectHeader** — 所有堆对象的基类：
```cpp
struct ObjectHeader {
    VMObject* next = nullptr;   // GC 链表
    UInt32    size = 0;         // 对象大小
    GCColor   color = WHITE;    // GC 三色标记
    UInt8     typeTag = 0;      // 对象类型标签
    UInt16    flags = 0;
};
```

**GC 三色标记**: `WHITE`（未访问）→ `GRAY`（已访问，子节点未处理）→ `BLACK`（已处理）

**对象类型标签** (Tag):
| 标签 | 名称 | 说明 |
|------|------|------|
| 0 | TAG_STRING | 字符串 |
| 1 | TAG_ARRAY | 动态数组 |
| 2 | TAG_TABLE | 哈希表 |
| 3 | TAG_SET | 集合 |
| 4-7 | TAG_STACK/QUEUE/DEQUE/PRIORITY_QUEUE | 栈/队列/双端队列/优先队列 |
| 8 | TAG_FUNCTION | CP 函数 |
| 9 | TAG_CLOSURE | 闭包 |
| 10 | TAG_CLASS | 类 |
| 11 | TAG_INSTANCE | 实例 |
| 12 | TAG_UPVALUE | 上值 |
| 13 | TAG_NATIVE | 原生函数 |
| 16-19 | LINKEDLIST/SLINKEDLIST/MULTISET/MULTIMAP | 链表/多重集/多重映射 |
| 20-23 | UNORDERED_SET/MULTISET/MAP/MULTIMAP | 无序容器 |
| 40-41 | ORDERED_SET/MAP | 有序容器 |
| 24-33 | THREAD/MUTEX/CONDITION/SEMAPHORE/ATOMIC_INT/FUTURE/CHANNEL/RWLOCK/WEBSOCKET | 并发 |
| 35-39 | TEXTURE2D/IMAGE/SOUND/MUSIC/FONT | 图形资源 |
| 42-43 | BOXED_INT64/FLOAT | 装箱数值 |

**主要 VM 对象**:
- `VMString` — 字符串 (hash, length, data)
- `VMArray` — 动态数组 (std::vector<Value>)
- `VMTable` — 哈希表 (开放寻址法)
- `VMSet` — 集合 (哈希表实现)
- `VMStack` — 栈 (std::vector<Value>)
- `VMQueue` — 队列 (std::deque<Value>)
- `VMDeque` — 双端队列
- `VMPriorityQueue` — 优先队列
- `VMLinkedList` / `VMSLinkedList` — 双向/单向链表
- `VMMultiSet` / `VMMultiMap` — 多重集/映射
- `VMUnorderedSet` / `VMUnorderedMap` — 无序容器
- `VMOrderedSet` / `VMOrderedMap` — 有序容器

#### VM 类 — 虚拟机核心

**文件**: `include/vm/vm_class.hpp` | `src/vm/vm.cpp`, `src/vm/vm_exec.cpp`

| 类别 | 方法/成员 | 功能 |
|------|----------|------|
| 模块 | `loadModule(func)` | 加载并执行字节码模块 |
| 全局 | `registerGlobal(name, val)` | 注册全局变量 |
| 全局 | `registerNative(name, fn)` | 注册原生函数 |
| 全局 | `getOrCreateGlobalSlot(name)` | 获取/创建全局槽位 |
| 调用 | `callFunction(func, args)` | 调用函数 |
| 导入 | `doImport(filename)` | 动态导入模块 |
| 字符串 | `internString(s, len)` | 字符串驻留 |
| JIT | `setJIT(jit)` / `getJIT()` | JIT 集成 |
| 调试 | `setBreakpoint(line)` / `removeBreakpoint(line)` | 断点管理 |
| 调试 | `debugContinue()` / `debugStepOver()` / `debugStop()` | 调试控制 |
| 调试 | `debugCallStack()` / `debugLocals()` | 调试信息 |
| 查询 | `totalInstructions()` / `gcCount()` | 统计信息 |
| 错误 | `raiseError(msg)` / `hasError()` / `error()` | 错误处理 |
| 静态 | `current()` | 获取当前线程 VM 实例 |

**核心常量**:
- `MAX_REGISTERS = 256` — 最大寄存器数
- `MAX_STACK = 65536` — 最大栈深度
- `GC_THRESHOLD = 1MB` — GC 触发阈值

**内部结构**:
- `stack_` — 操作数栈 (`std::vector<Value>`)
- `frames_` — 调用帧栈 (`std::vector<CallFrame>`)
- `globalSlots_` — 全局槽位数组（最多 65535 个）
- `allObjects_` — GC 对象链表头
- `stringTable_` — 字符串驻留表
- `handlerStack_` — 异常处理帧栈

**GC 算法** — 三色标记-清除:
1. `gcMarkRoots()` — 标记根对象（栈、全局变量、调用帧）
2. `gcMarkObject(obj)` — 递归标记对象引用的子对象
3. `gcMarkValue(v)` — 标记 Value 引用的对象
4. `gcSweepPhase()` — 清除白色对象
5. `gcCleanup()` — 清理完毕

**执行循环** (`run()` 方法, `src/vm/vm_exec.cpp`):
- 主循环读取 `OP_*` 指令并执行
- 支持 `computed goto` (GCC/Clang) 优化分发
- 在执行前检查 JIT 编译的函数入口，通过 `jitTryCallDispatch()` 分派
- 支持调试断点检查

#### CallFrame 结构

```cpp
struct CallFrame {
    VMFunction* func;          // 当前函数
    VMClosure*  closure;       // 闭包（如果有）
    Value*      base;          // 栈基址
    const UInt8* pc;           // 程序计数器
    const UInt8* returnPC;     // 返回地址
    Int32       resultReg;     // 返回值寄存器
    Int32       resultCount;   // 返回值数量
};
```

#### HandlerFrame 结构

```cpp
struct HandlerFrame {
    Int32  catchPC;       // catch 块 PC
    Value* savedBase;     // 异常前的栈基址
    Int32  resultReg;     // 异常值寄存器
};
```

---

### 4.8 优化器 (optimizer)

**文件**: `include/optimizer/*.hpp` | `src/optimizer/*.cpp`

#### Optimizer 类 — 优化管理器

统一管理所有优化 Pass，根据 `OptLevel` 决定执行哪些优化。

```cpp
class Optimizer {
    Optimizer(OptLevel level = OptLevel::O2);
    Shared<Program> optimize(Shared<Program> program);
    Shared<Stmt>   optimizeStmt(Shared<Stmt> stmt);
    const OptStats& getStats() const;
};
```

**优化统计** (`OptStats`): `constantsFolded`, `deadCodeRemoved`, `functionsInlined`, `tailRecOptimized`, `loopsUnrolled`, `stackAllocs`, `heapAllocsAvoided`, `iterations`

#### 优化 Pass 详解

| Pass | 类 | 功能 |
|------|-----|------|
| 常量折叠 | `ConstantFolder` | 编译时计算常量表达式，如 `3+5*2` → `13` |
| 死代码消除 | `DeadCodeEliminator` | 移除不可达代码和无用赋值 |
| 函数内联 | `FunctionInliner` | 将小函数调用替换为函数体 |
| 尾递归优化 | `TailRecursionOptimizer` | 将尾递归转换为循环 |
| 循环展开 | `LoopUnroller` | 展开小循环减少分支 |
| 逃逸分析 | `EscapeAnalyzer` | 分析对象是否逃逸，栈上分配优化 |

**ConstantFolder**: 支持 `tryEval()` 尝试在编译时求值表达式的值，支持二元运算（算术/比较/逻辑）和一元运算求值。

**EscapeAnalyzer**: 分析 `ProgramEscapeResult`，支持栈上分配优化，减少堆分配。

---

### 4.9 JIT 编译器 (jit)

**文件**: `include/jit/*.hpp` | `src/jit/*.cpp`

#### JITCompiler 类 — 外部工具链 JIT

使用外部 LLVM 工具链（clang + lld-link）将热点函数编译为机器码并加载。

| 方法 | 功能 |
|------|------|
| `initialize()` | 初始化 JIT 编译器 |
| `shouldCompile(func)` | 检查函数是否达到热点阈值 |
| `compile(func)` | 编译热点函数 |
| `compileFromAST(program, funcName)` | 从 AST 直接编译 |
| `recordCall(func)` | 记录函数调用（热点检测） |
| `setHotThreshold(n)` | 设置热点阈值（默认 100 次） |
| `isCompiled(func)` | 检查是否已编译 |
| `clearCache()` | 清空编译缓存 |

**编译流程**:
1. `generateLLVMIR()` — 生成 LLVM IR 文本
2. `compileToObjectFile()` — 使用 clang 编译为 .obj
3. `loadCompiledCode()` — 加载编译后的机器码

#### HybridJIT 类 — 混合 JIT 策略

自动选择最佳 JIT 模式：优先使用 ORC JIT（内存中编译），回退到外部工具链 JIT。

```cpp
class HybridJIT {
    bool initialize();
    void* compile(shared_ptr<Program> program, const string& funcName);
    bool shouldCompile(VMFunction*);
    void recordCall(VMFunction*);
    void* compileHotFunction(VMFunction*);
    void compileAll(shared_ptr<Program>);  // 全量预编译
    void storeProgram(shared_ptr<Program>);
    void setHotThreshold(int);
    JITRegistry& getJITRegistry();
};
```

**模式**: `None` / `Orc` (LLVM ORC JIT) / `External` (外部 clang+lld)

#### JITRegistry 类 — JIT 信息注册表

外部映射表，将 JIT 元数据从 VMFunction 解耦：

```cpp
struct JITInfo {
    void* entry = nullptr;    // JIT 编译的函数入口
    bool  compiled = false;   // 是否已编译
};
```

#### OrcJIT 类 — LLVM ORC JIT

使用 LLVM ORC JIT API 进行内存中即时编译，避免写入临时文件。

#### jitTryCallDispatch() — JIT 分派

```cpp
bool jitTryCallDispatch(VM* vm, VMFunction* func, int argc, Value* args, Value& result);
```

VM 在执行 `OP_CALL` 时检查函数是否有 JIT 入口，如果有则通过 JIT 分派执行，否则回退到字节码执行。

---

### 4.10 标准库 (stdlib)

**文件**: `include/stdlib/*.hpp` | `src/stdlib/*.cpp`

#### StdLib 类

静态注册类，将所有标准库函数注册到 VM。

```cpp
class StdLib {
    static void registerAll(VM* vm);  // 注册所有标准库
};
```

**注册类别** (80+ 注册函数):

| 类别 | 注册方法 | 内容 |
|------|---------|------|
| 基础 | `registerMath`, `registerMathMore`, `registerMathConst`, `registerMathSpecial` | 数学运算 |
| 字符串 | `registerString`, `registerStringExt`, `registerStringMore`, `registerStringCase`, `registerStringSearch`, `registerStrCi` | 字符串操作 |
| 数组 | `registerArray`, `registerArrayMore` | 数组操作 |
| 表 | `registerTable`, `registerMap` | 哈希表操作 |
| 容器 | `registerSet`, `registerStack`, `registerQueue`, `registerDeque`, `registerPriorityQueue`, `registerLinkedList`, `registerSLinkedList`, `registerMultiSet`, `registerMultiMap`, `registerUnorderedSet`, `registerUnorderedMultiSet`, `registerUnorderedMap`, `registerUnorderedMultiMap`, `registerOrderedSet`, `registerOrderedMap` | 26 种容器类型 |
| IO | `registerIO`, `registerFile`, `registerFileMore`, `registerFileSeek`, `registerFileWalk`, `registerFileStat`, `registerBinaryIO` | 文件 IO |
| 时间 | `registerTime`, `registerTimeMore`, `registerDuration` | 时间处理 |
| 系统 | `registerSystem`, `registerSystemMore`, `registerProcess`, `registerConsole`, `registerDir`, `registerLogger`, `registerLogPlus`, `registerTemp` | 系统操作 |
| 网络 | `registerNetwork`, `registerHTTP`, `registerSocket`, `registerHttp` | 网络通信 |
| 加密 | `registerCrypto`, `registerCryptoPlus`, `registerAes` | AES/加密 |
| 编码 | `registerEncoding`, `registerCharset`, `registerCharconv`, `registerCharconvFloat` | Base64/编码 |
| 正则 | `registerRegex` | 正则表达式 |
| 随机 | `registerRandom` | 随机数 |
| 算法 | `registerAlgorithms`, `registerAlgoExt`, `registerAlgoMissing` | 排序/查找 |
| 位运算 | `registerBitwise` | 位操作 |
| JSON | `registerJSON` | JSON 解析 |
| 反射 | `registerReflection` | 类型反射 |
| 并发 | `registerThreading`, `registerCallOnce` | 线程/互斥/信号量 |
| WebSocket | `registerWebSocket` | WebSocket |
| 数据库 | `registerSqlite`, `registerMysql`, `registerPg`, `registerRedis` | SQLite/MySQL/PostgreSQL/Redis |
| 图形 | `registerRaylib`, `registerImGui`, `registerMatrix`, `registerColor` | Raylib + ImGui |
| 类型 | `registerTypes`, `registerOptional`, `registerVariant`, `registerAny`, `registerTuple`, `registerTupleEnhance`, `registerResult`, `registerResultMonad`, `registerPair`, `registerBitset`, `registerComplex`, `registerNumericLimits`, `registerSpan`, `registerSpanEnhance`, `registerIterator` | 类型系统扩展 |
| 工具 | `registerUtils`, `registerMoreUtils`, `registerFormat`, `registerFunctional`, `registerMemory`, `registerSourceLoc`, `registerFFI` | 工具函数 |
| 其他 | `registerR10Misc`, `registerR11Misc`, `registerFixMissing`, `registerCSVWrite` | 杂项 |

**总计**: 800+ 函数，775+ 中文别名。

---

### 4.11 模块系统 (module)

**文件**: `include/module/module_system.hpp` | `src/module/module_system.cpp`

#### Module 类

表示一个模块，支持依赖管理。

```cpp
class Module {
    String name() const;
    bool isLoaded() const;
    bool addDependency(moduleName);
    bool hasCircularDependency(moduleName, path);  // 循环依赖检测
};
```

#### ModuleLoader 类

模块加载器，负责查找和加载模块。

```cpp
class ModuleLoader {
    void addSearchPath(path);
    Shared<Module> loadModule(moduleName, vm);
    VMFunction* compileModule(moduleName, errorMsg);  // 只编译不执行
    String resolveModulePath(moduleName);             // 查找模块文件路径
};
```

**搜索路径优先级**:
1. 源文件所在目录
2. 当前工作目录
3. `packages/模块名/index.cp`
4. `~/.cpkg/packages/模块名/index.cp`
5. `tests/` 目录
6. HTTP/HTTPS 远程导入

#### ModuleManager 类

模块管理器，封装加载器并管理已导入模块。

```cpp
class ModuleManager {
    bool import(moduleName, alias);
    Shared<Module> getModule(name);
    bool isImported(moduleName);
};
```

---

### 4.12 调试器 (debug)

**文件**: `include/debug/debugger.hpp` | `src/debug/debugger.cpp`

#### Debugger 类

| 功能 | 方法 |
|------|------|
| 断点管理 | `addBreakpoint(file, line)`, `removeBreakpoint(id)`, `enableBreakpoint(id, enable)`, `clearBreakpoints()` |
| 监视 | `addWatch(expr)`, `removeWatch(id)`, `clearWatches()` |
| 执行控制 | `continueExecution()`, `stepOver()`, `stepInto()`, `stepOut()`, `runToCursor(file, line)` |
| 堆栈 | `getCallStack()`, `getVariables(frameIndex)` |
| 表达式 | `evaluateExpression(expr)` |
| DAP | `handleDAPRequest(json)` — Debug Adapter Protocol 支持 |

**Breakpoint 结构**: `file`, `line`, `enabled`, `hitCount`, `hitCondition`（命中次数条件）, `condition`（条件表达式）

**DebugEvent 枚举**: `BreakpointHit`, `StepOver`, `StepInto`, `StepOut`, `ExceptionThrown`, `FunctionEntry`, `FunctionExit`, `VariableChanged`

#### DebugServer 类

远程调试服务器，支持 TCP 连接（默认端口 4711）。

#### CLIDebugger 类

命令行调试器，支持命令：`continue`, `step`, `next`, `finish`, `break`, `delete`, `info`, `print`, `backtrace`, `frame`, `list`, `quit`, `help`

---

### 4.13 异常处理 (exception)

**文件**: `include/exception/exception_handler.hpp` | `src/exception/exception_handler.cpp`

#### ExceptionType 枚举

```cpp
enum class ExceptionType {
    RuntimeError, TypeError, IndexError, KeyError, ValueError,
    ZeroDivisionError, OverflowError, MemoryError, IOError,
    ImportError, NameError, AttributeError, NotImplementedError, Custom
};
```

#### Exception 结构

```cpp
struct Exception {
    ExceptionType type;
    String message;
    String stackTrace;
    Value value;
    String file;
    int line, column;
};
```

#### ExceptionHandler 类

| 方法 | 功能 |
|------|------|
| `throwException(ex)` | 抛出异常 |
| `hasException()` | 是否有待处理异常 |
| `pushFrame(frame)` / `popFrame()` | 异常帧栈管理 |
| `findHandler(type)` | 查找匹配的 catch 处理器 |
| `generateStackTrace()` | 生成堆栈跟踪 |
| `matches(handler, ex)` | 检查异常是否匹配 handler |

#### CatchHandler 结构

```cpp
struct CatchHandler {
    ExceptionType type;      // 捕获的异常类型
    String typeName;         // 自定义类型名
    String varName;          // catch(e) 变量名
    int handlerPC;           // handler 代码起始 PC
    int endPC;               // try 块结束 PC
};
```

#### CPLangException 类

C++ 异常类，继承自 `std::exception`，用于编译器内部异常抛出。

**辅助宏**:
```cpp
THROW_TYPE_ERROR(msg)    // 类型错误
THROW_INDEX_ERROR(msg)   // 索引错误
THROW_KEY_ERROR(msg)     // 键错误
THROW_VALUE_ERROR(msg)   // 值错误
THROW_ZERO_DIVISION()    // 除零错误
THROW_NAME_ERROR(name)   // 未定义名称
```

---

### 4.14 REPL 交互式环境 (repl)

**文件**: `include/repl/repl.hpp` | `src/repl.cpp`

#### ReplEngine 类

交互式 Read-Eval-Print Loop 环境。

**特性**:
- 增强行编辑（光标移动、历史记录、Tab 补全）
- 语法提示和函数签名显示
- 内置命令（`%help`, `%history`, `%save`, `%load`, `%type`, `%time`, `%vars`, `%clear`, `%exit`）
- 持久化历史记录
- ANSI 颜色支持
- 表达式自动打印（`3+5` → `打印(3+5)`）
- 多行输入支持（检测未闭合的括号/块）

**核心方法**:
| 方法 | 功能 |
|------|------|
| `run()` | 启动 REPL 循环 |
| `readLineEnhanced(prompt)` | 增强行编辑输入 |
| `doCompletion(prefix)` | Tab 补全 |
| `evaluate(source, result)` | 编译并执行代码 |
| `wrapExpression(source)` | 表达式自动包装 |
| `dispatchCommand(cmd)` | 处理 REPL 命令 |

---

### 4.15 CLI 入口 (main)

**文件**: `src/main.cpp`

#### 命令行选项

| 选项 | 功能 |
|------|------|
| `-l, --lex` | 仅词法分析 |
| `-p, --parse` | 仅语法分析 |
| `-c, --compile` | 编译并执行（字节码 VM） |
| `-c --hotspot` | 字节码 VM + 热点 JIT 检测 |
| `--hotspot-threshold=N` | 热点阈值（默认 100） |
| `-j, --jit` | 全量 JIT 编译执行 |
| `-a, --aot` | AOT 编译为原生可执行文件 |
| `-o <file>` | 指定输出文件 |
| `-O0/-O1/-O2/-O3` | 优化级别 |
| `--no-bytecode-opt` | 禁用字节码优化 |
| `--emit-llvm` | 输出 LLVM IR |
| `-v, --verbose` | 详细输出 |
| `-r, --repl` | 交互式 REPL |
| `-h, --help` | 帮助信息 |

#### 核心函数

| 函数 | 功能 |
|------|------|
| `runLexer(source)` | 运行词法分析，输出 Token 列表 |
| `runParser(source)` | 运行语法分析，报告语句数 |
| `runFullCompile(source, filepath, useJit, useHotspot, ...)` | 完整编译执行流程 |
| `runAOTCompile(source, filepath, outputFile, ...)` | AOT 编译 |

**`runFullCompile()` 流程**:
1. 创建 `Compiler` 实例，设置优化级别
2. `compiler.compile(source, srcFile)` → 编译为字节码
3. 获取 `VM` 实例
4. 注册标准库（已在 Compiler 构造时完成）
5. 设置模块导入回调（支持本地/远程/包注册表导入）
6. 根据模式初始化 JIT（全量/热点）
7. `vm->loadModule(func)` → 执行字节码

---

### 4.16 包管理器 (cpkg)

**文件**: `src/cpkg/cpkg.cpp` | `include/cpkg/cpkg.hpp`

独立的包管理器可执行文件 (`cpkg`)，负责 CP 语言包的安装、管理和分发。

---

## 5. 依赖关系图

```
                         ┌──────────────┐
                         │ cplang_common│ (纯头文件, 基础类型)
                         └──────┬───────┘
                                │
                    ┌───────────┼───────────┐
                    ▼           ▼           ▼
            ┌──────────┐ ┌──────────┐ ┌──────────┐
            │cplang_   │ │cplang_core│ │cplang_vm │
            │token     │ │(verbose)  │ │(字节码VM)│
            └────┬─────┘ └──────────┘ └────┬─────┘
                 │                         │
                 ▼                         │
            ┌──────────┐                   │
            │cplang_   │                   │
            │lexer     │                   │
            └────┬─────┘                   │
                 │                         │
    ┌────────────┼────────────┐            │
    ▼            ▼            ▼            │
┌──────────┐ ┌──────────┐ ┌──────────┐    │
│cplang_ast│ │cplang_   │ │cplang_   │    │
│(AST节点) │ │parser    │ │semantic  │    │
└────┬─────┘ └────┬─────┘ └────┬─────┘    │
     │            │            │           │
     └────────────┼────────────┘           │
                  ▼                        │
          ┌──────────────┐                 │
          │ cplang_      │                 │
          │ codegen      │─────────────────┘
          │ (字节码+LLVM)│
          └──────┬───────┘
                 │
    ┌────────────┼────────────┬────────────┐
    ▼            ▼            ▼            ▼
┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐
│cplang_jit│ │cplang_   │ │cplang_   │ │cplang_   │
│(JIT编译) │ │stdlib    │ │module    │ │optimizer │
└────┬─────┘ └────┬─────┘ └────┬─────┘ └──────────┘
     │            │            │
     └────────────┼────────────┘
                  ▼
         ┌────────────────┐
         │   cplang_cli   │ ← 可执行文件
         │   cplang_repl  │
         └────────────────┘
```

**依赖层次**:
1. **第 0 层**: `cplang_common`, `cplang_token`, `cplang_core` — 无外部依赖
2. **第 1 层**: `cplang_lexer`, `cplang_ast`, `cplang_vm` — 依赖第 0 层
3. **第 2 层**: `cplang_parser`, `cplang_semantic` — 依赖第 1 层
4. **第 3 层**: `cplang_codegen` — 依赖第 2 层和 VM
5. **第 4 层**: `cplang_optimizer`, `cplang_jit`, `cplang_stdlib`, `cplang_module`, `cplang_debug`, `cplang_exception` — 依赖第 3 层
6. **第 5 层**: `cplang_cli`, `cplang_repl` — 依赖所有层

---

## 6. 构建与运行

### 构建要求

- **Windows 10+**
- **Visual Studio 2022** (MSVC v143)
- **CMake 3.20+**
- **可选**: LLVM 18+ (启用 JIT 和 AOT)

### 构建步骤

```bash
# 使用 MSVC 构建脚本
build_msvc.bat

# 或手动 CMake 构建
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### CMake 选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `CPLANG_BUILD_TESTS` | ON | 构建测试 |
| `CPLANG_BUILD_EXAMPLES` | ON | 构建示例 |
| `CPLANG_USE_LLVM` | ON | 启用 LLVM JIT |
| `CPLANG_ENABLE_SANITIZERS` | OFF | 启用 Sanitizers |
| `CPLANG_WARNINGS_AS_ERRORS` | OFF | 警告即错误 |

### 构建产物

| 产物 | 输出 | 说明 |
|------|------|------|
| `cplang` | `build/bin/cplang.exe` | 主 CLI 工具 |
| `cplang_repl` | `build/bin/cplang_repl.exe` | 独立 REPL |
| `cpkg` | `build/bin/cpkg.exe` | 包管理器 |
| `cplang_tests` | 测试可执行文件 | Google Test 测试 |

### 运行测试

```bash
# C++ 单元测试
ctest --test-dir build

# CP 语言集成测试
cplang -c tests/test_new_stdlib.cp
```

---

## 7. 执行模式

### 模式对比

| 模式 | 命令示例 | 前端 | 后端 | 速度 |
|------|---------|------|------|------|
| 词法分析 | `cplang -l test.cp` | Lexer | - | - |
| 语法分析 | `cplang -p test.cp` | Lexer + Parser | - | - |
| 字节码 VM | `cplang -c test.cp` | 完整前端 | 字节码解释器 | 基准 |
| 热点 JIT | `cplang -c --hotspot test.cp` | 完整前端 | VM + JIT | 热点 ~95x |
| 全量 JIT | `cplang -j test.cp` | 完整前端 | 全量预编译 | ~95x |
| AOT 编译 | `cplang -a -o app.exe test.cp` | 完整前端 | LLVM → .exe | 原生 |
| LLVM IR | `cplang -a --emit-llvm test.cp` | 完整前端 | LLVM IR 输出 | - |
| REPL | `cplang -r` | 逐行编译 | VM + JIT | 即时 |

### 执行流程详解（字节码 VM 模式）

```
1. readFile()              → 读取源码文件，去除 UTF-8 BOM
2. Compiler::compile()     → 完整编译流程
   ├── Lexer::nextToken()  → 词法分析，生成 Token 流
   ├── Parser::parse()     → 递归下降解析，生成 AST
   ├── SemanticAnalyzer::analyze()  → 语义分析，类型检查
   │   ├── 作用域分析
   │   ├── 类型推断/检查
   │   ├── 所有权/借用检查
   │   └── 泛型单态化
   ├── Optimizer::optimize()  → AST 优化（多轮）
   └── Codegen::compile()     → 字节码生成
       └── BytecodeOptimizer::optimize()  → 字节码优化
3. StdLib::registerAll()   → 注册标准库函数
4. VM::loadModule()        → 加载字节码模块
   └── VM::run()           → 执行循环
       ├── 读取 OP_* 指令
       ├── 执行操作（算术/比较/跳转/调用）
       ├── jitTryCallDispatch() → JIT 分派（如果可用）
       └── GC 触发检查
```

### 执行流程详解（JIT 模式）

```
1. 完整编译（同字节码模式）
2. HybridJIT::initialize()       → 初始化 JIT 引擎
3. jit.compileAll(program)       → 全量预编译所有函数
   └── LLVMCodegen::generate()   → AST → LLVM IR
       └── OrcJIT / ExternalJIT  → LLVM IR → 机器码
4. VM::loadModule()              → 加载并执行
   └── VM::run() → jitTryCallDispatch()
       └── 调用 JIT 编译的函数入口（机器码直接执行）
```

### 执行流程详解（AOT 模式）

```
1. Lexer + Parser + Semantic  → 生成 AST
2. LLVMCodegen::generate()    → AST → LLVM IR
3. clang                       → LLVM IR → .obj
4. lld-link                    → .obj → .exe
5. 输出独立可执行文件（零运行时依赖）
```

---

## 8. 关键设计决策

### 8.1 NaN-Boxing 值表示

**决策**: 使用 64 位 NaN-Boxing 而非 Tagged Union。

**原因**:
- `sizeof(Value) = 8` 字节（原来 Tagged Union 需 12+padding=16）
- Int8/Int16/Int32/Float32 内联存储，零堆分配
- 类型检查只需检查 high 16 bits（单次比较）
- 指针提取只需位掩码操作（单次 AND）

### 8.2 字节码 + JIT 双引擎

**决策**: 保留字节码 VM 作为基础执行引擎，JIT 作为加速层。

**原因**:
- 字节码模式启动快（< 0.1s），适合开发和脚本
- JIT 在热点函数上提供 ~95x 加速
- 可通过热点检测自动切换，无需用户干预
- AOT 编译提供原生部署能力

### 8.3 渐进类型系统

**决策**: 动态类型默认支持，类型标注可选。

**实现**:
- 无标注: 使用通用指令（`OP_ADD`, `OP_CMPEQ` 等），运行时类型检查
- 有标注: 使用类型化指令（`OP_IADD`, `OP_FADD` 等），直接操作，更快
- 代码生成器通过 `allTyped_` 标志追踪函数是否全类型化

### 8.4 递归下降解析器

**决策**: 使用手写递归下降解析器而非 Parser Generator。

**原因**:
- 更好的错误恢复和中文错误信息
- 灵活处理中文语法的特殊性
- 三层 lookahead 消除泛型 vs 比较歧义

### 8.5 三色标记-清除 GC

**决策**: 使用简单的三色标记-清除 GC。

**实现**:
- 触发阈值: 1MB 分配量
- 根对象: 栈、全局变量、调用帧
- 增量: 当前为 Stop-the-World 模式

### 8.6 Rust 式所有权系统

**决策**: 实现借用检查和移动语义。

**实现**:
- `&x` — 不可变借用（允许多个）
- `&可写 x` — 可变借用（独占）
- `移动 x` — 转移所有权
- `释放 x` — 手动释放
- `可信 { ... }` — 跳过检查的 unsafe 块
- 在语义分析阶段进行检查

### 8.7 泛型单态化

**决策**: 使用单态化（Monomorphization）实现泛型。

**实现**:
- 语义分析阶段为每个具体类型组合生成独立的函数声明
- 存储在 `monomorphizedFunctions_` 中
- 代码生成器优先编译单态化后的函数
- 支持函数泛型和结构体泛型

---

## 9. 数据流

### 编译数据流

```
源码字符串 (.cp)
    │
    ▼
[Lexer] ──→ Token 流
    │
    ▼
[Parser] ──→ AST (Program)
    │
    ▼
[SemanticAnalyzer] ──→ 类型标注 + 符号表 + 单态化 AST
    │
    ├──────────────────────────────────┐
    ▼                                  ▼
[Optimizer] ──→ 优化后 AST      [LLVMCodegen] ──→ LLVM IR
    │                                  │
    ▼                                  ├──→ [OrcJIT] ──→ 机器码 (内存)
[Codegen] ──→ 字节码 (VMFunction)      │
    │                                  └──→ [AOTCompiler] ──→ .exe
    ▼
[BytecodeOptimizer] ──→ 优化后字节码
    │
    ▼
[VM] ──→ 执行
    │
    ├──→ [GC] 三色标记-清除
    └──→ [JITDispatch] ──→ 热点函数 → JIT 编译
```

### VM 执行数据流

```
VMFunction (字节码 + 常量池)
    │
    ▼
VM::loadModule()
    │
    ▼
VM::run()  ← 执行循环
    │
    ├── fetch: 读取 OP_* 指令
    ├── decode: 解析操作数
    ├── execute: 执行操作
    │   ├── 算术/比较: 操作 Value (NaN-boxing)
    │   ├── 跳转: 修改 PC
    │   ├── 调用: 压入 CallFrame
    │   ├── 返回: 弹出 CallFrame
    │   └── 异常: 查找 HandlerFrame
    ├── JIT check: jitTryCallDispatch()
    └── GC check: gcAllocated_ >= GC_THRESHOLD → gc()
```

### 模块导入数据流

```
导入 "模块名"
    │
    ▼
ModuleLoader::resolveModulePath()
    ├── 源文件目录/模块名.cp
    ├── ./模块名.cp
    ├── ./packages/模块名/index.cp
    ├── ~/.cpkg/packages/模块名/index.cp
    └── HTTP/HTTPS 远程下载
    │
    ▼
Compiler::compileFile()  → VMFunction
    │
    ▼
VM::registerGlobal()  → 注册函数到全局槽
    │
    ▼
HybridJIT::compile()  → 预热 JIT 编译
```

---

> **文档版本**: 基于 CP 语言 v1.0.0 代码库分析生成
> **生成日期**: 2026-06-05