# CP 语言 (CPLang) — Code Wiki 结构化文档

> **版本**: v0.1.0-beta | **语言**: C++17 | **构建系统**: CMake 3.20+ | **平台**: Windows (MSVC / Clang)

---

## 目录

1. [项目概述](#1-项目概述)
2. [整体架构](#2-整体架构)
3. [编译管线详解](#3-编译管线详解)
4. [核心模块说明](#4-核心模块说明)
5. [运行时系统](#5-运行时系统)
6. [JIT 编译系统](#6-jit-编译系统)
7. [标准库与包管理](#7-标准库与包管理)
8. [工具链与IDE支持](#8-工具链与ide支持)
9. [构建系统](#9-构建系统)
10. [项目运行方式](#10-项目运行方式)
11. [目录结构总览](#11-目录结构总览)
12. [依赖关系图](#12-依赖关系图)
13. [关键设计决策](#13-关键设计决策)
14. [测试体系](#14-测试体系)

---

## 1. 项目概述

CP 语言是一款**支持原生中文语法的编程语言**。所有关键字、标准库函数均有完整的中文别名，可以使用纯中文编写程序。编译器采用经典的 **Lexer → Parser → Semantic Analyzer → Codegen → VM** 编译管线，同时支持 **LLVM JIT 即时编译**和 **AOT 预编译**。

### 核心特性

| 特性 | 说明 |
|------|------|
| 中文语法 | 所有关键字支持中文/英文双语，变量名/函数名可用中文 |
| 双执行引擎 | 字节码栈式VM (解释执行) + LLVM ORC JIT (机器码执行) |
| NaN-Boxing | 64位紧凑值表示，Int8/16/32/Bool/Nil 零堆分配 |
| 三色GC | 增量式三色标记-清除垃圾回收，1MB阈值自动触发 |
| 图形支持 | Raylib + ImGui 中文绑定 |
| 网络支持 | TCP/HTTP/WebSocket |
| 数据库支持 | SQLite / MySQL / Redis 内建驱动 |
| 包管理 | `cpkg` 包管理器，支持远程 URL 导入 |
| 中文错误 | 编译器错误、运行时异常均为中文 |

---

## 2. 整体架构

### 2.1 高层架构图

```
┌──────────────────────────────────────────────────────────────────────┐
│                        CP 编译器管道                                  │
├──────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  源码(.cp)                                                           │
│     │                                                                │
│     ▼  [Lexer]  词法分析器                                           │
│  Token流                                                             │
│     │                                                                │
│     ▼  [Parser]  语法分析器 (递归下降)                                │
│  AST (Program → Stmt[] → Expr[])                                    │
│     │                                                                │
│     ▼  [SemanticAnalyzer]  语义分析器                                │
│  带类型/符号信息的 AST                                                 │
│     │                                                                │
│     ├──▶ [Codegen] ──▶ 字节码 ──▶ [BytecodeOptimizer] ──▶ [VM]      │
│     │                                                      │         │
│     └──▶ [LLVMCodegen] ──▶ LLVM IR ──▶ [ORC JIT] ──▶ 机器码        │
│                                        │                             │
│                                        └──▶ [AOTCompiler] ──▶ .exe   │
│                                                                      │
└──────────────────────────────────────────────────────────────────────┘
```

### 2.2 数据流

```
源文件(.cp) → Lexer → Token流 → Parser → AST → SemanticAnalyzer
                                                    ├──→ Codegen → 字节码 → VM → 输出
                                                    └──→ LLVMCodegen → LLVM IR → JIT/可执行文件
```

---

## 3. 编译管线详解

### 3.1 词法分析器 (Lexer)

**文件**: [include/lexer/lexer.hpp](file:///d:/CPLANG/include/lexer/lexer.hpp) | [src/lexer/lexer.cpp](file:///d:/CPLANG/src/lexer/lexer.cpp)

| 属性 | 说明 |
|------|------|
| 编码 | 纯 UTF-8 (不依赖 wchar_t) |
| 关键字 | 中文 + 英文双关键字系统 (~80+ 关键字) |
| 字符串 | 双引号字符串，支持 `\n \t \\ \" \0` 转义 |
| 数字 | 十进制 / 十六进制 `0x` / 二进制 `0b` / 八进制 `0o` |
| 注释 | `//` 行注释、`/* */` 块注释 |
| 错误 | 全汉化错误消息: `"第X行: 消息"` |

**核心方法**:

```cpp
class Lexer {
    Token nextToken();     // 获取下一个 Token
    Token peekToken();     // 预览下一个 Token (缓冲一个)
    void reset();          // 重置到开头
    // 内部扫描器: scanId_, scanNum_, scanStr_, scanComment_, scanOp_
};
```

### 3.2 Token 系统

**文件**: [include/lexer/token.hpp](file:///d:/CPLANG/include/lexer/token.hpp)

Token 类型分为几个大类:

- **特殊**: `END_OF_FILE`, `IDENTIFIER`, `INVALID`
- **字面量**: `INTEGER`, `FLOAT`, `STRING`, `BOOLEAN`, `CHAR`
- **中文关键字**: `K_PACKAGE(包名)`, `K_IMPORT(导入)`, `K_CLASS(类)`, `K_FUNC(函数)`, `K_RETURN(返回)`, `K_IF(如果)`, `K_FOR(循环)` 等 40+ 个
- **双语运算符关键词**: `K_AND(且/and)`, `K_OR(或/or)`, `K_LET(设/let)`, `K_MOVE(移动/move)` 等
- **运算符**: `OP_PLUS`, `OP_MINUS`, `OP_ASSIGN`, `OP_EQ` 等 30+ 个
- **分隔符**: `LPAREN`, `RBRACE`, `COMMA`, `SEMICOLON` 等

`KeywordTable` 单例提供关键字查找: `keywords_` 映射包含所有中英文关键字的双向映射。

### 3.3 语法分析器 (Parser)

**文件**: [include/parser/parser.hpp](file:///d:/CPLANG/include/parser/parser.hpp) |
[src/parser/parser.cpp](file:///d:/CPLANG/src/parser/parser.cpp) (核心) |
[src/parser/parser_decl.cpp](file:///d:/CPLANG/src/parser/parser_decl.cpp) (声明) |
[src/parser/parser_stmt.cpp](file:///d:/CPLANG/src/parser/parser_stmt.cpp) (语句) |
[src/parser/parser_expr.cpp](file:///d:/CPLANG/src/parser/parser_expr.cpp) (表达式)

**递归下降**解析器，支持三层 lookahead (`current_`, `peek_`, `peek2_`, `peek3_`)，用于泛型调用 `排序<整数>()` 与比较 `a<b>` 的歧义消除。

表达式解析按**优先级从低到高**:
```
parseAssignment → parseTernary → parseOr → parseAnd → parseBitOr → parseBitXor
→ parseBitAnd → parseEquality → parseComparison → parseShift → parseAdditive
→ parseMultiplicative → parseUnary → parsePostfix → parsePrimary
```

**核心方法**:

| 方法 | 级别 | 说明 |
|------|------|------|
| `parse()` | 入口 | 解析整个程序 |
| `parseProgram()` | 程序 | 包装入口 |
| `parseDeclaration()` | 声明 | 分发到函数/类/结构体/枚举/接口声明 |
| `parseStatement()` | 语句 | 分发到 if/for/while/return/break/defer/try 等 |
| `parseExpression()` | 表达式 | 完整表达式解析入口 |
| `synchronize()` | 恢复 | 错误恢复: 跳到下一个语句边界 |

便捷函数: `parseString()`, `parseFile()`, `parseExprString()` 提供简化的解析入口。

### 3.4 AST 节点体系

**文件**: [include/ast/ast.hpp](file:///d:/CPLANG/include/ast/ast.hpp)

```
ASTNode (基类)
├── Expr (表达式基类)
│   ├── LiteralExpr        字面量 (整数/浮点/字符串/布尔)
│   ├── IdentifierExpr     标识符引用
│   ├── BinaryExpr         二元运算 (left op right)
│   ├── UnaryExpr          一元运算 (op operand, 支持前后缀 ++/--)
│   ├── CallExpr           函数调用 (callee, arguments, typeArgs)
│   ├── MemberExpr         成员访问 (object.member)
│   ├── IndexExpr          索引访问 (array[index])
│   ├── ArrayExpr          数组字面量 [1, 2, 3]
│   ├── StructLiteralExpr  结构体字面量
│   ├── NewExpr            new 表达式
│   ├── BorrowExpr         借用表达式 (& / &可写)
│   ├── MoveExpr           移动表达式
│   └── DropExpr           释放表达式
├── Stmt (语句基类)
│   ├── ExprStmt           表达式语句
│   ├── EmptyStmt          空语句
│   ├── BlockStmt          块语句 { ... }
│   ├── VarDeclStmt        变量声明 (变量/常量/let)
│   ├── FuncDeclStmt       函数声明 (含泛型参数)
│   ├── ClassDeclStmt      类声明
│   ├── InterfaceDeclStmt  接口声明
│   ├── EnumDeclStmt       枚举声明
│   ├── StructDeclStmt     结构体声明 (含泛型)
│   ├── IfStmt             if/else 语句
│   ├── SwitchStmt         switch/case 语句
│   ├── ForStmt            for 循环 (init; cond; update)
│   ├── ForEachStmt        for-each 循环 (遍历 x : arr)
│   ├── WhileStmt          while 循环
│   ├── DoWhileStmt        do-while 循环
│   ├── BreakStmt          break 语句
│   ├── ContinueStmt       continue 语句
│   ├── DeferStmt          defer 语句 (作用域退出时执行)
│   ├── ReturnStmt         return 语句
│   ├── ThrowStmt          throw 语句
│   ├── TryStmt            try-catch-finally 语句
│   ├── ImportStmt         导入语句
│   ├── PackageStmt        包声明
│   └── TrustBlockStmt     可信块 (unsafe)
└── Program (程序根节点)
    ├── package: Optional<PackageStmt>
    └── statements: vector<Stmt>
```

---

## 4. 核心模块说明

### 4.1 公共类型 (cplang_common)

**文件**: [include/common/types.hpp](file:///d:/CPLANG/include/common/types.hpp)

```cpp
namespace cplang {
    // 基本类型别名
    using Int8 = int8_t;     using UInt8 = uint8_t;
    using Int16 = int16_t;   using UInt16 = uint16_t;
    using Int32 = int32_t;   using UInt32 = uint32_t;
    using Int64 = int64_t;   using UInt64 = uint64_t;
    using Float32 = float;   using Float64 = double;
    using String = std::string;
    using Char = char;

    // 标准库别名
    template<typename T> using Optional = std::optional<T>;
    template<typename T> using Unique = std::unique_ptr<T>;
    template<typename T> using Shared = std::shared_ptr<T>;
    template<typename T> using Weak = std::weak_ptr<T>;
    template<typename... Types> using Variant = std::variant<Types...>;

    // 优化等级
    enum class OptLevel : int { None = 0, O1 = 1, O2 = 2, O3 = 3 };
}
```

### 4.2 语义分析器 (cplang_semantic)

**文件**: [src/semantic/semantic_analyzer.cpp](file:///d:/CPLANG/src/semantic/semantic_analyzer.cpp)

三阶段分析:

| 阶段 | 说明 |
|------|------|
| 符号收集 | 遍历 AST 收集所有函数/变量/类型声明 |
| 类型检查 | 验证类型标注的一致性 |
| 所有权检查 | 验证借用规则 (`&` / `&可写`) |

负责作用域管理、重复定义检测、未定义引用检测、类型推断。

### 4.3 代码生成器 (cplang_codegen)

**文件**: [include/codegen/codegen.hpp](file:///d:/CPLANG/include/codegen/codegen.hpp) |
[src/codegen/codegen.cpp](file:///d:/CPLANG/src/codegen/codegen.cpp) (核心) |
[src/codegen/codegen_opt.cpp](file:///d:/CPLANG/src/codegen/codegen_opt.cpp) (优化) |
[src/codegen/codegen_stmt.cpp](file:///d:/CPLANG/src/codegen/codegen_stmt.cpp) (语句) |
[src/codegen/codegen_expr.cpp](file:///d:/CPLANG/src/codegen/codegen_expr.cpp) (表达式) |
[src/codegen/bytecode_optimizer.cpp](file:///d:/CPLANG/src/codegen/bytecode_optimizer.cpp) (字节码优化)

**Codegen 类** — 字节码生成:
```cpp
class Codegen {
    // 核心编译
    VMFunction* compile(Shared<Program> program);

    // 字节码发射 (16字节对齐指令)
    void emit(UInt8 op, UInt8 a=0, UInt8 b=0, UInt8 c=0);
    void emitInt(UInt8 op, UInt8 a, Int32 imm);    // 带32位立即数
    void emitJump(UInt8 op, int offset);            // 跳转指令

    // 寄存器管理 (最多256个)
    int allocReg();    void freeRegs(int n);
    // 作用域管理
    void pushScope();  void popScope();
    // 编译分发
    void compileStmt(Shared<Stmt>);          // 语句编译
    int  compileExpr(Shared<Expr>);          // 表达式编译 → 返回寄存器号

    // 类型化指令 (直接JIT编译)
    void emitTypedArithmetic(TokenType, Type*, int ra, int rb, int rc);
    void emitTypedComparison(TokenType, Type*, int ra, int rb, int rc);

    // 常量折叠
    bool canFold(Shared<Expr>);  Value foldConstant(Shared<Expr>);

    // defer 栈管理
    std::vector<std::vector<Shared<Stmt>>> deferStack_;
    void emitDeferCleanup();  // 作用域退出时逆序执行 defer 体
};
```

**Compiler 类** — 完整编译器入口:
```cpp
class Compiler {
    VMFunction* compile(const String& source, const String& sourceFile);
    VMFunction* compileFile(const String& filename);
    VM* vm();
    void setOptLevel(OptLevel level);
    void setEnableBytecodeOpt(bool enable);
};
```

### 4.4 优化器 (cplang_optimizer)

**文件**: [src/optimizer/optimizer.cpp](file:///d:/CPLANG/src/optimizer/optimizer.cpp) (入口) |
[src/optimizer/constant_folder.cpp](file:///d:/CPLANG/src/optimizer/constant_folder.cpp) |
[src/optimizer/dead_code_eliminator.cpp](file:///d:/CPLANG/src/optimizer/dead_code_eliminator.cpp) |
[src/optimizer/escape_analyzer.cpp](file:///d:/CPLANG/src/optimizer/escape_analyzer.cpp) |
[src/optimizer/function_inliner.cpp](file:///d:/CPLANG/src/optimizer/function_inliner.cpp) |
[src/optimizer/loop_unroller.cpp](file:///d:/CPLANG/src/optimizer/loop_unroller.cpp) |
[src/optimizer/tail_recursion_optimizer.cpp](file:///d:/CPLANG/src/optimizer/tail_recursion_optimizer.cpp) |
[src/optimizer/llvm_optimizer.cpp](file:///d:/CPLANG/src/optimizer/llvm_optimizer.cpp)

纯 AST 变换优化器，不依赖 JIT:

| Pass | 说明 |
|------|------|
| ConstantFolder | 常量折叠 (编译期计算) |
| DeadCodeEliminator | 死代码消除 |
| EscapeAnalyzer | 逃逸分析 |
| FunctionInliner | 函数内联 |
| LoopUnroller | 循环展开 |
| TailRecursionOptimizer | 尾递归优化 |
| LLVMOptimizer | LLVM Pass 优化 |

---

## 5. 运行时系统

### 5.1 值系统 (NaN-Boxing)

**文件**: [include/vm/value.hpp](file:///d:/CPLANG/include/vm/value.hpp)

所有 CP 值使用 64 位 NaN-boxing 表示:

```
┌──────────────────────────────────────────────────────────────────┐
│  IEEE 754 NaN-boxing 值编码 (64-bit)                              │
├──────────────────┬───────────────────────────────────────────────┤
│ bits 48-63       │ bits 0-47                                      │
├──────────────────┼───────────────────────────────────────────────┤
│ 0xFFFF + bit47=1 │ 立即值: Int8/Int16/Int32/Float32/Bool/Nil     │
│ 0xFFFF + bit47=0 │ 对象指针: 指向 VMObject 的48位指针              │
│ 非 0xFFFF         │ IEEE 754 Float64 (直接浮点数)                  │
└──────────────────┴───────────────────────────────────────────────┘
```

| 编码 (bits 48-63 = 0xFFFF) | 类型 | 说明 |
|------|------|------|
| `0x0000_0000_0000` | Nil | bit47=1, 全零 |
| `0x0000_0000_0001` | Bool(false) | bit47=1, LSB=1 |
| `0x0000_0000_0002` | Bool(true) | bit47=1, LSB=2 |
| `0x0000_iiii_iiii` | Int8 | bit47=1, bit46=0, 低8位存值 |
| `0x0001_iiii_iiii` | Int16 | bit47=1, bit46=0, 低16位存值 |
| `0x1000_iiii_iiii` | Int32 | bit47=1, bit46=1, 低32位存值 |
| `0x2000_ffff_ffff` | Float32 | bit47=1, 装箱浮点 |
| pointer (48-bit) | 引用类型 | bit47=0, 指向堆对象 |

**核心优势**: Int8/16/32/Float64/Bool/Nil 完全不分配堆内存; `sizeof(Value) = 8` 字节。

### 5.2 虚拟机 (VM)

**文件**: [include/vm/vm_class.hpp](file:///d:/CPLANG/include/vm/vm_class.hpp) |
[src/vm/vm.cpp](file:///d:/CPLANG/src/vm/vm.cpp) (核心) |
[src/vm/vm_exec.cpp](file:///d:/CPLANG/src/vm/vm_exec.cpp) (执行循环) |
[src/vm/vm_containers.cpp](file:///d:/CPLANG/src/vm/vm_containers.cpp) (容器) |
[src/vm/vm_objects.cpp](file:///d:/CPLANG/src/vm/vm_objects.cpp) (对象) |
[src/vm/vm_opt.cpp](file:///d:/CPLANG/src/vm/vm_opt.cpp) (优化执行, computed goto)

**核心参数**:

| 参数 | 值 | 说明 |
|------|-----|------|
| `MAX_REGISTERS` | 256 | 函数内最大寄存器数 |
| `MAX_STACK` | 65536 | 最大栈深度 |
| `GC_THRESHOLD` | 1 MB | GC 触发阈值 |
| `MAX_GLOBAL_SLOTS` | 65535 | 最大全局槽位数 |

**核心方法**:

```cpp
class VM {
    bool loadModule(VMFunction* func);         // 加载并执行模块
    void registerGlobal(const char* name, Value val);   // 注册全局变量
    void registerNative(const char* name, Fn fn);       // 注册原生函数
    Int32 getOrCreateGlobalSlot(const char* name);      // 获取/创建全局槽位

    // JIT 集成
    void setJIT(HybridJIT* jit);
    HybridJIT* getJIT() const;

    // 调试
    void setBreakpoint(int line);
    void debugContinue();
    void debugStepOver();
    Value debugGetVariable(const std::string& name);

    // 模块导入
    std::function<bool(const std::string&)> importCallback;

    // 调用
    Value callFunction(Value func, std::vector<Value>& args);

private:
    // 三色标记-清除 GC
    void gc(); void gcMarkRoots(); void gcMarkObject(); void gcSweepPhase();

    std::vector<Value> stack_;                  // 值栈
    std::vector<CallFrame> frames_;             // 调用帧栈
    std::vector<Value> globalSlots_;            // 全局槽位
    VMObject* allObjects_;                      // 所有堆对象链表
    std::unordered_map<std::string, VMString*> stringTable_; // 字符串驻留表
    HybridJIT* jit_;                            // JIT 编译器
};
```

### 5.3 VM 对象模型

**文件**: [include/vm/vm_object.hpp](file:///d:/CPLANG/include/vm/vm_object.hpp) |
[include/vm/vm_fwd.hpp](file:///d:/CPLANG/include/vm/vm_fwd.hpp)

所有堆对象继承自 `VMObject` → `ObjectHeader`:

```
ObjectHeader (next, size, color, typeTag, flags)
├── VMString         字符串 (data, length, hash)
├── VMArray          数组 (vector<Value>)
├── VMTable          表/字典 (buckets + rehash)
├── VMSet            集合 (buckets)
├── VMStack          栈 (vector<Value>)
├── VMQueue          队列 (deque<Value>)
├── VMDeque          双端队列
├── VMPriorityQueue  优先队列 (堆)
├── VMLinkedList     双向链表
├── VMSLinkedList    单向链表
├── VMMultiSet       多重集合
├── VMMultiMap       多重映射
├── VMOrderedSet     有序集合 (std::set)
├── VMOrderedMap     有序映射 (std::map)
├── VMUnorderedSet   无序集合 (std::unordered_set)
├── VMUnorderedMap   无序映射 (std::unordered_map)
├── VMUnorderedMultiSet/MultiMap
├── VMMap            自定义排序映射
├── VMFunction       函数体 (code, constants, lineInfo)
├── VMClosure        闭包 (func + upvalues)
├── VMClass          类 (name, base, fields, methods)
├── VMInstance       类实例 (cls + fields)
├── VMStructDef      结构体定义
├── VMStruct         结构体实例
├── VMUpvalue        上值 (闭包捕获的外部变量)
├── VMNativeFunc     原生函数 (C++ lambda)
├── VMThread         线程
├── VMMutex          互斥锁
├── VMCondition      条件变量
├── VMSemaphore      信号量
├── VMAtomicInt      原子整数
├── VMBarrier        屏障
├── VMFuture         Future/Promise
├── VMChannel        Channel (带缓冲)
├── VMRWLock         读写锁
├── VMWebSocket      WebSocket 连接
├── VMTexture2D      2D 纹理
├── VMImage          图像
├── VMSound          音效
├── VMMusic          音乐
├── VMFont           字体
├── VMBoxedInt64     装箱 Int64
└── VMBoxedFloat     装箱 Float64
```

### 5.4 字节码指令集

**文件**: [include/vm/vm_opcodes.hpp](file:///d:/CPLANG/include/vm/vm_opcodes.hpp)

**字节码格式**: 16 字节对齐指令 = `[op(1)][0(1)][a(1)][b(1)][c(1)][imm32(4)][padding(4)]`

| 类别 | 操作码 | 说明 |
|------|--------|------|
| **加载/存储** | `OP_LOADNIL` `OP_LOADBOOL` `OP_LOADINT` `OP_LOADFLT` `OP_LOADSTR` `OP_LOADCONST` `OP_MOVE` | 常量加载与寄存器移动 |
| **全局/局部** | `OP_LOADGLOBAL` `OP_STOREGLOBAL` `OP_LOADLOCAL` `OP_STORELOCAL` | 变量访问 |
| **通用算术** | `OP_ADD` `OP_SUB` `OP_MUL` `OP_DIV` `OP_IDIV` `OP_MOD` `OP_POW` `OP_NEG` | 运行时类型分发 |
| **类型化算术** | `OP_IADD`(0x90) `OP_ISUB` `OP_FADD`(0xA0) `OP_FSUB` 等 | Int32/Float64 直接运算 (JIT 友好) |
| **窄类型算术** | `OP_I8ADD`(0xB0) `OP_I16ADD`(0xC0) | Int8/Int16 直接运算 |
| **比较** | `OP_CMPEQ` `OP_CMPLT` `OP_CMPGT` `OP_ICMPEQ` `OP_FCMPEQ` 等 | 通用 + 类型化比较 |
| **控制流** | `OP_JUMP` `OP_JUMPIF` `OP_JUMPNIF` | 无条件/条件跳转 |
| **函数** | `OP_CALL` `OP_CALLMETHOD` `OP_RETURN` | 函数调用与返回 |
| **数组/索引** | `OP_NEWARRAY` `OP_GETELEM` `OP_SETELEM` `OP_GETIDX` `OP_SETIDX` | 数组操作 |
| **字符串** | `OP_CONCAT` `OP_STRLEN` `OP_TONUM` `OP_TOSTR` `OP_TOBOOL` `OP_TYPEOF` | 字符串与类型转换 |
| **异常** | `OP_TRY` `OP_ENDTRY` `OP_THROW` | 异常处理 |
| **OOP** | `OP_NEWCLASS` `OP_NEWSTRUCT` `OP_GETFIELD` `OP_SETFIELD` | 类与结构体 |
| **模块** | `OP_IMPORT` | 模块导入 |
| **空操作** | `OP_NOP`(0xFF) | 无操作 |

### 5.5 垃圾回收 (GC)

**算法**: 三色标记-清除 (Tri-color Mark-Sweep)

```
标记阶段:
1. gcMarkRoots()  →  栈上所有 Value + 全局槽位 + 所有帧 → 标记为 GRAY
2. gcMarkObject() →  BFS 遍历 GRAY 对象引用 → 标记为 BLACK

清除阶段:
3. gcSweepPhase() → 遍历 allObjects_ 链表
   - WHITE (不可达) → delete
   - BLACK (可达) → 重置为 WHITE (为下一轮准备)

触发条件:
- gcAllocated_ >= GC_THRESHOLD (1 MB)
- 手动调用 GC (VM::gc())
```

---

## 6. JIT 编译系统

### 6.1 HybridJIT 三层架构

**文件**: [include/jit/hybrid_jit.hpp](file:///d:/CPLANG/include/jit/hybrid_jit.hpp) |
[src/jit/hybrid_jit.cpp](file:///d:/CPLANG/src/jit/hybrid_jit.cpp)

```
┌───────────────────────────────────────────────────┐
│  第1层: LLVM ORC JIT (LLJIT)       最快 ← 首选   │
│  第2层: External JIT (Clang→DLL)    较快 ← 回退   │
│  第3层: 字节码 VM (解释执行)         最慢 ← 保底   │
└───────────────────────────────────────────────────┘
```

### 6.2 ORC JIT 编译器

**文件**: [include/jit/orc_jit.hpp](file:///d:/CPLANG/include/jit/orc_jit.hpp) |
[src/jit/orc_jit.cpp](file:///d:/CPLANG/src/jit/orc_jit.cpp)

```cpp
class OrcJIT {
    bool initialize();      // 初始化 LLVM ORC JIT
    void* compileIR(const std::string& ir, const std::string& funcName);
    void* compileAST(Shared<Program> program, const std::string& funcName);
    void compileAll(Shared<Program> program);  // 全量预编译
    void* compileHotFunction(VMFunction* func);  // 热点编译
    void* lookup(const std::string& name);    // 符号查找
    bool addSymbol(const std::string& name, void* address);  // 注册外部符号

    // 热点检测
    void recordCall(VMFunction* func);
    bool shouldCompile(VMFunction* func) const;
    void setHotThreshold(int threshold);  // 默认 100

    // 后台编译线程
    void startBackgroundThread();
    void submitCompileTask(const CompileTask& task);
};
```

### 6.3 LLVM IR 代码生成

**文件**: [src/codegen/llvm_codegen.cpp](file:///d:/CPLANG/src/codegen/llvm_codegen.cpp)

将 AST 转换为 LLVM IR，支持:
- 算术/比较/逻辑运算
- if/else 分支
- while/for 循环
- 函数调用 (含递归)
- break/continue
- 类型化整数运算

### 6.4 AOT 编译

**文件**: [include/codegen/aot_compiler.hpp](file:///d:/CPLANG/src/aot/aot_compiler.cpp) |
[src/codegen/aot_compiler.cpp](file:///d:/CPLANG/src/codegen/aot_compiler.cpp)

支持将 CP 源码直接编译为原生可执行文件 (`.exe`)，或输出 LLVM IR (`.ll`)。

---

## 7. 标准库与包管理

### 7.1 标准库 (cplang_stdlib)

**文件**: [include/stdlib/stdlib.hpp](file:///d:/CPLANG/include/stdlib/stdlib.hpp) (汇总头文件) |
[src/stdlib/stdlib.cpp](file:///d:/CPLANG/src/stdlib/stdlib.cpp) (总注册) |

**子模块**:

| 模块 | 文件 | 功能 |
|------|------|------|
| stdlib_math | [stdlib_math.hpp](file:///d:/CPLANG/include/stdlib/stdlib_math.hpp) / [stdlib_math.cpp](file:///d:/CPLANG/src/stdlib/stdlib_math.cpp) | 数学函数 (sin/cos/sqrt/log/random/pi 等) |
| stdlib_string | [stdlib_string.hpp](file:///d:/CPLANG/include/stdlib/stdlib_string.hpp) | 字符串操作 (split/join/trim/replace/substr 等) |
| stdlib_array | [stdlib_array.hpp](file:///d:/CPLANG/include/stdlib/stdlib_array.hpp) | 数组操作 (push/pop/sort/filter/map 等) |
| stdlib_table | [stdlib_table.hpp](file:///d:/CPLANG/include/stdlib/stdlib_table.hpp) | 表/字典操作 |
| stdlib_io | [stdlib_io.hpp](file:///d:/CPLANG/include/stdlib/stdlib_io.hpp) / [stdlib_io.cpp](file:///d:/CPLANG/src/stdlib/stdlib_io.cpp) | 输入输出 (打印/输入/格式化) |
| stdlib_file | [stdlib_file.hpp](file:///d:/CPLANG/include/stdlib/stdlib_file.hpp) / [stdlib_file.cpp](file:///d:/CPLANG/src/stdlib/stdlib_file.cpp) | 文件操作 (读写/目录遍历) |
| stdlib_time_system | [stdlib_time_system.hpp](file:///d:/CPLANG/include/stdlib/stdlib_time_system.hpp) | 时间与系统 |
| stdlib_types_net | [stdlib_types_net.hpp](file:///d:/CPLANG/include/stdlib/stdlib_types_net.hpp) | 类型系统 + 网络 |
| stdlib_containers | [stdlib_containers.hpp](file:///d:/CPLANG/include/stdlib/stdlib_containers.hpp) | 容器 (Set/Map/Queue/Stack/Deque/Channel 等) |
| stdlib_algo_bitwise | [stdlib_algo_bitwise.hpp](file:///d:/CPLANG/include/stdlib/stdlib_algo_bitwise.hpp) | 算法 + 位运算 |
| stdlib_regex_encoding_crypto | [stdlib_regex_encoding_crypto.hpp](file:///d:/CPLANG/include/stdlib/stdlib_regex_encoding_crypto.hpp) | 正则/编码/加密 |
| stdlib_variant_utils | [stdlib_variant_utils.hpp](file:///d:/CPLANG/include/stdlib/stdlib_variant_utils.hpp) | 变体/工具函数 |
| stdlib_db | [stdlib_db.cpp](file:///d:/CPLANG/src/stdlib/stdlib_db.cpp) | 数据库 (SQLite/MySQL/Redis) |
| stdlib_http | [stdlib_http.cpp](file:///d:/CPLANG/src/stdlib/stdlib_http.cpp) | HTTP 客户端/服务器 |
| stdlib_aes | [stdlib_aes.cpp](file:///d:/CPLANG/src/stdlib/stdlib_aes.cpp) | AES 加密 |
| stdlib_heap | [stdlib_heap.cpp](file:///d:/CPLANG/src/stdlib/stdlib_heap.cpp) | 堆操作 |
| stdlib_map | [stdlib_map.cpp](file:///d:/CPLANG/src/stdlib/stdlib_map.cpp) | 映射操作 |
| stdlib_raylib | [stdlib_raylib_unit.cpp](file:///d:/CPLANG/src/stdlib/stdlib_raylib_unit.cpp) | Raylib 图形绑定 |
| stdlib_imgui | [stdlib_imgui.cpp](file:///d:/CPLANG/src/stdlib/stdlib_imgui.cpp) | ImGui 绑定 |

**注册机制**:
```cpp
class StdLib {
    static void registerAll(VM* vm);  // 在 Compiler 初始化时自动调用
};
```

通过 `registerNative()` 将 C++ lambda 注册为 CP 函数，所有函数均有中文别名。

### 7.2 模块系统 (cplang_module)

**文件**: [src/module/module_system.cpp](file:///d:/CPLANG/src/module/module_system.cpp)

导入搜索路径 (优先级从高到低):
1. 源文件所在目录
2. 当前工作目录
3. `packages/` 注册表目录
4. `~/.cpkg/packages/` 用户安装目录
5. `tests/` 测试目录
6. URL 远程导入 (`https://...`)

### 7.3 包管理器 (cpkg)

**文件**: [include/cpkg/cpkg.hpp](file:///d:/CPLANG/include/cpkg/cpkg.hpp) |
[src/cpkg/cpkg.cpp](file:///d:/CPLANG/src/cpkg/cpkg.cpp)

独立的命令行工具，支持中英双语命令，甚至可执行文件本身也可用 `包` 命令调用:

| 中文命令 | 英文命令 | 功能 |
|----------|----------|------|
| `安装` | `install` / `i` | 安装包（支持本地 .cp 文件 / 仓库名 / URL） |
| `卸载` | `remove` / `rm` / `uninstall` | 卸载包 |
| `列表` | `list` / `ls` | 列出已安装的包 |
| `搜索` | `search` / `s` | 搜索注册表中的包 |
| `更新` | `update` / `up` | 更新包（无包名则更新全部） |
| `信息` | `info` | 查看包详情 |
| `注册表` | `registry` | 查看/设置注册表地址 |
| `帮助` | `help` / `-h` / `--help` | 显示帮助信息 |

安装源支持三种方式:
1. 本地 `.cp` 文件路径
2. 远程 URL (`https://...`)
3. 注册表仓库名（从 GitHub registry 查询）

**注册表文件**: [registry/index.json](file:///d:/CPLANG/registry/index.json)

**已发布的包**:
- [packages/math/](file:///d:/CPLANG/packages/math/) — 数学扩展
- [packages/string/](file:///d:/CPLANG/packages/string/) — 字符串扩展
- [packages/mysql/](file:///d:/CPLANG/packages/mysql/) — MySQL 驱动
- [packages/redis/](file:///d:/CPLANG/packages/redis/) — Redis 驱动

---

## 8. 工具链与 IDE 支持

| 工具 | 路径 | 功能 |
|------|------|------|
| VSCode 插件 | [tools/vscode-cp/](file:///d:/CPLANG/tools/vscode-cp/) | 语法高亮、代码片段、调试集成 |
| Electron IDE | [tools/electron-ide/](file:///d:/CPLANG/tools/electron-ide/) | 图形化桌面IDE (CP 语言自举) |
| CP LSP | [tools/cplsp.py](file:///d:/CPLANG/tools/cplsp.py) | 语言服务器协议 |
| CP 格式化 | [tools/cpfmt.py](file:///d:/CPLANG/tools/cpfmt.py) | 代码格式化工具 |
| CP IDE (Python) | [tools/cp_ide.py](file:///d:/CPLANG/tools/cp_ide.py) | 基于 tkinter 的简易 IDE |
| 反汇编器 | [tools/disasm.cpp](file:///d:/CPLANG/tools/disasm.cpp) | 字节码反汇编工具 |

### 调试器

**文件**: [include/debug/debugger.hpp](file:///d:/CPLANG/include/debug/debugger.hpp) |
[src/debug/debugger.cpp](file:///d:/CPLANG/src/debug/debugger.cpp)

支持:
- 断点管理 (文件+行号, 命中条件)
- 单步执行 (StepInto/StepOver/StepOut)
- 调用栈查看
- 变量监视 (Watch)
- DAP 协议支持 (Debug Adapter Protocol)
- 远程调试服务器 (`DebugServer`, 端口 4711)
- 命令行调试器 (`CLIDebugger`)

### 异常处理

**文件**: [src/exception/exception_handler.cpp](file:///d:/CPLANG/src/exception/exception_handler.cpp)

try-catch-finally 的运行时实现。

---

## 9. 构建系统

### 9.1 CMake 目标层次

```
cplang_common (INTERFACE)          类型别名头文件
  ├── cplang_token (INTERFACE)     Token定义
  │     └── cplang_lexer (STATIC)  词法分析器
  │           └── cplang_ast (INTERFACE)     AST节点定义
  │                 └── cplang_parser (STATIC)  语法分析器
  ├── cplang_vm (STATIC)          虚拟机核心
  │     ├── cplang_stdlib (STATIC) 标准库
  │     ├── cplang_codegen (STATIC) 代码生成
  │     │     └── cplang_jit (STATIC)  JIT编译器
  │     ├── cplang_module (STATIC)   模块系统
  │     ├── cplang_debug (STATIC)    调试器
  │     └── cplang_exception (STATIC) 异常处理
  ├── cplang_semantic (STATIC)    语义分析器
  ├── cplang_optimizer (STATIC)   优化器
  └── cplang_core (STATIC)        运行时配置

可执行文件:
  cplang_cli    →  src/main.cpp + src/repl.cpp   →  cplang.exe
  cplang_repl   →  src/repl.cpp                   →  cplang_repl.exe
  cplang_cpkg   →  src/cpkg/cpkg.cpp              →  cpkg.exe
  cplang_tests  →  tests/gtest/*.cpp              →  测试可执行文件
```

### 9.2 构建配置

```cmake
cmake_minimum_required(VERSION 3.20)
project(cplang VERSION 1.0.0 LANGUAGES C CXX)

# 关键选项
option(CPLANG_USE_LLVM  "启用 LLVM JIT"  ON)
option(CPLANG_BUILD_TESTS "构建测试"     ON)
option(CPLANG_BUILD_EXAMPLES "构建示例"   ON)

# MSVC 下: vm_opt.cpp (computed goto) → vm_opt_stub.cpp
# LLVM 查找: cmake/FindLLVM.cmake
# Google Test: FetchContent 从 GitHub 获取 v1.15.2
```

### 9.3 第三方依赖

| 依赖 | 路径 | 用途 |
|------|------|------|
| LLVM 18+ | [llvm-dev/](file:///d:/CPLANG/llvm-dev/) | JIT/AOT 编译后端 |
| Raylib | [third_party/raylib/](file:///d:/CPLANG/third_party/raylib/) | 2D/3D 图形 |
| ImGui | [third_party/imgui/](file:///d:/CPLANG/third_party/imgui/) | GUI 框架 |
| SQLite 3 | [include/sqlite/sqlite3.h](file:///d:/CPLANG/include/sqlite/sqlite3.h) / [src/sqlite/sqlite3.c](file:///d:/CPLANG/src/sqlite/sqlite3.c) | 嵌入式数据库 |
| MySQL | [include/mysql/](file:///d:/CPLANG/include/mysql/) / [lib/libmysql.lib](file:///d:/CPLANG/lib/libmysql.lib) | MySQL 客户端 |
| miniz | [include/miniz.h](file:///d:/CPLANG/include/miniz.h) / [src/miniz.c](file:///d:/CPLANG/src/miniz.c) | 压缩/解压 |
| Google Test | (FetchContent) | C++ 单元测试框架 |
| MD5 | [include/crypto/md5_impl.h](file:///d:/CPLANG/include/crypto/md5_impl.h) / [src/crypto/md5_impl.cpp](file:///d:/CPLANG/src/crypto/md5_impl.cpp) | MD5 哈希 |

---

## 10. 项目运行方式

### 10.1 Windows 构建

```bash
# 方法1: MSVC (需要 Visual Studio 2022)
build_msvc.bat

# 方法2: 手动 CMake
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release

# 方法3: Clang (需要 LLVM 安装)
_build_clang.bat
```

### 10.2 使用方式

```bash
# 交互式 REPL
cplang.exe -r

# 词法分析
cplang.exe -l hello.cp

# 语法分析
cplang.exe -p hello.cp

# 编译并执行 (字节码 VM)
cplang.exe -c hello.cp

# JIT 全量编译执行
cplang.exe -j hello.cp

# 热点 JIT (阈值 100)
cplang.exe -c hello.cp --hotspot

# AOT 编译为原生可执行文件
cplang.exe -a hello.cp -o hello.exe

# 输出 LLVM IR
cplang.exe --emit-llvm hello.cp

# 优化级别
cplang.exe -c hello.cp -O3

# 禁用字节码优化
cplang.exe -c hello.cp --no-bytecode-opt

# 详细输出
cplang.exe -c hello.cp -v
```

### 10.3 包管理器

```bash
# 中文命令
包 安装 math                 # 安装包
包 搜索 db                   # 搜索包
包 列表                      # 列出已安装
包 更新 math                 # 更新包
包 卸载 math                 # 卸载包
包 信息 math                 # 查看包详情
包 注册表                    # 查看当前注册表

# 英文命令（等价）
cpkg install math
cpkg search db
cpkg list
cpkg update math
cpkg remove math
cpkg info math
cpkg registry
```

---

## 11. 目录结构总览

```
CPLANG/
├── include/                   # C++ 头文件
│   ├── ast/ast.hpp           AST 节点定义 (Program, Stmt, Expr)
│   ├── lexer/                词法分析器 (lexer.hpp, token.hpp)
│   ├── parser/               语法分析器 (parser.hpp)
│   ├── semantic/             语义分析器
│   ├── codegen/              代码生成器 (codegen.hpp, bytecode_optimizer.hpp)
│   ├── vm/                   虚拟机 (vm.hpp, value.hpp, vm_opcodes.hpp, vm_object.hpp)
│   ├── jit/                  JIT (hybrid_jit.hpp, orc_jit.hpp, jit_runtime.hpp)
│   ├── common/               公共类型 (types.hpp)
│   ├── core/                 核心配置 (verbose.hpp)
│   ├── stdlib/               标准库声明 (20+ 子模块)
│   ├── module/               模块系统
│   ├── debug/                调试器
│   ├── exception/            异常处理
│   ├── cpkg/                 包管理器
│   ├── crypto/               加密 (md5_impl.h)
│   ├── sqlite/               SQLite 头文件
│   ├── mysql/                MySQL 头文件
│   ├── miniz*.h              miniz 压缩库
│   └── stubs/                Windows 兼容性桩
├── src/                       # C++ 实现文件 (与 include/ 对应)
│   ├── main.cpp              CLI 入口
│   ├── repl.cpp              REPL 实现
│   ├── lexer/lexer.cpp       词法分析
│   ├── parser/               语法分析 (4个文件)
│   ├── semantic/             语义分析
│   ├── codegen/              代码生成 (6个文件)
│   ├── optimizer/            优化器 (8个文件)
│   ├── vm/                   虚拟机 (9个文件)
│   ├── jit/                  JIT (4个文件)
│   ├── stdlib/               标准库 (18个文件)
│   ├── debug/                调试器
│   ├── exception/            异常处理
│   ├── module/               模块系统
│   ├── cpkg/                 包管理器
│   ├── crypto/               加密
│   ├── sqlite/sqlite3.c      SQLite 源码
│   ├── miniz*.c              miniz 压缩库
│   └── aot/                  AOT 编译器
├── tools/                     # 工具链
│   ├── cplang.exe            编译好的编译器
│   ├── vscode-cp/            VSCode 扩展
│   ├── electron-ide/         Electron IDE
│   ├── cp_ide.py / cp_ide_v1.py  Python IDE
│   ├── cplsp.py              LSP 服务器
│   ├── cpfmt.py              代码格式化
│   ├── disasm.cpp            反汇编器
│   └── version.json          版本信息
├── examples/                  # 示例程序
│   ├── cp_demos/             教学示例 (01-11)
│   ├── tutorial/             教程示例
│   ├── *.cp                  独立示例 (贪吃蛇/2048/Web服务器等)
│   └── *.cpp                 C++ 嵌入示例
├── tests/                     # 测试
│   ├── cp/                   CP 语言测试用例
│   ├── gtest/                Google Test C++ 测试
│   └── scratch/              临时测试文件
├── packages/                  # 官方包注册表
│   ├── math/ string/ mysql/ redis/
├── third_party/               # 第三方库
│   ├── raylib/               图形库
│   └── imgui/                GUI 库
├── llvm-dev/                  # LLVM 开发包 (bin/lib)
├── docs/                      # 文档 (10+ 篇)
├── cmake/                     # CMake 模块
├── benchmarks/                # 性能基准
├── data/                      # 网站数据
├── web/ www/                  # 官网前端
├── assets/                    # 资源文件
├── CMakeLists.txt             # 主构建文件
├── build_msvc.bat             # MSVC 构建脚本
├── build.ps1                  # PowerShell 构建脚本
├── VERSION                    # 版本号
└── README.md                  # 项目说明
```

---

## 12. 依赖关系图

### 12.1 编译时依赖 (CMake 目标)

```
                         ┌─────────────────┐
                         │   cplang_common  │ (INTERFACE)
                         └────────┬────────┘
          ┌───────────────────────┼───────────────────────┐
          │                       │                       │
  ┌───────▼───────┐     ┌────────▼────────┐    ┌─────────▼─────────┐
  │  cplang_token  │     │   cplang_core   │    │    cplang_vm      │
  │  (INTERFACE)   │     │   (STATIC)      │    │    (STATIC)       │
  └───────┬───────┘     └─────────────────┘    └─────────┬─────────┘
          │                                              │
  ┌───────▼───────┐                         ┌────────────┼────────────┐
  │  cplang_lexer │                         │            │            │
  │  (STATIC)     │                  ┌──────▼──────┐ ┌───▼─────┐ ┌───▼────────┐
  └───────┬───────┘                  │ cplang_     │ │cplang_  │ │cplang_      │
          │                          │ stdlib      │ │debug    │ │exception    │
  ┌───────▼───────┐                  └──────┬──────┘ └─────────┘ └─────────────┘
  │   cplang_ast  │                         │
  │  (INTERFACE)  │                  ┌──────▼──────────┐
  └───────┬───────┘                  │  cplang_codegen  │
          │                          │  (STATIC)        │
  ┌───────▼────────┐                 └──────┬───────────┘
  │ cplang_parser  │                        │
  │ (STATIC)       │                 ┌──────▼──────┐
  └───────┬────────┘                 │  cplang_jit  │
          │                          │  (STATIC)    │
  ┌───────▼──────────┐              └──────────────┘
  │ cplang_semantic  │
  │ (STATIC)         │
  └──────────────────┘

  可执行文件: cplang_cli, cplang_repl, cplang_cpkg
```

### 12.2 运行时数据流

```
main() → readFile() → Lexer → Parser → SemanticAnalyzer
                                             │
                    ┌────────────────────────┤
                    ▼                        ▼
              Compiler.compile()    AOTCompiler.compile()
                    │                        │
                    ▼                        ▼
              Codegen.compile()     LLVMCodegen.compile()
                    │                        │
                    ▼                        ▼
              VMFunction (字节码)    LLVM Module (IR)
                    │                        │
                    ▼                        ▼
              VM::loadModule()      OrcJIT::compileIR()
                    │                        │
                    ▼                        ▼
              vm_exec.cpp           机器码执行
              解释执行循环
```

---

## 13. 关键设计决策

| 决策 | 选择 | 原因 |
|------|------|------|
| 值表示 | NaN-boxing (64-bit) | 零开销类型标签，`sizeof(Value)=8`，Int8/16/32/Bool/Nil 不分配堆内存 |
| 内存管理 | 三色 GC + 可选所有权 | 降低入门门槛，生产级项目可启用所有权系统 |
| 编码 | 纯 UTF-8 (不依赖 wchar_t) | 跨平台一致性，Windows/Linux 统一 |
| JIT 后端 | LLVM ORC JIT v2 (LLJIT) | 行业标准，持续优化，支持 Lazy Compilation |
| 字节码 | 16 字节对齐指令 | 缓存行对齐优化，简化解码器 |
| 关键字 | 中文 + 英文双语系统 | 降低中文用户入门门槛，保留英文兼容性 |
| 编译策略 | 先字节码解释，热点 JIT | 快速启动 (零编译开销) + 热点函数机器码执行 |
| 解析器 | 递归下降 + 3-token lookahead | 简单可维护，支持泛型 `排序<整数>()` 歧义消除 |
| 错误消息 | 全汉化 | `"第X行第Y列: 语法分析: 错误描述"` |
| 执行模式 | 双模式 (VM + JIT + AOT) | 开发期解释执行，发布期 AOT 编译为原生 exe |

---

## 14. 测试体系

### 14.1 C++ 单元测试 (Google Test)

**文件**: [tests/gtest/](file:///d:/CPLANG/tests/gtest/)

| 测试文件 | 覆盖内容 |
|----------|----------|
| `test_lexer.cpp` | 词法分析: Token 识别、关键字、字符串、数字、注释 |
| `test_parser.cpp` | 语法分析: 表达式解析、语句解析、AST 结构验证 |
| `test_vm.cpp` | 虚拟机: 字节码执行、GC、算术运算、控制流 |
| `test_optimizer.cpp` | 优化器: 常量折叠、死代码消除、循环展开 |
| `test_semantic.cpp` | 语义分析: 类型检查、符号解析、作用域 |
| `test_e2e.cpp` | 端到端: 完整编译管线测试 |

运行方式:
```bash
# 通过 CMake
cmake --build . --target cplang_tests
ctest

# 或直接运行
./build/bin/<Config>/cplang_tests.exe
```

### 14.2 CP 语言集成测试

**目录**: [tests/cp/](file:///d:/CPLANG/tests/cp/) — 包含 20+ 个 .cp 测试文件，覆盖:
- 基础运算 (`test_add_only.cp`, `test_div_only.cp`)
- 控制流 (`test_if.cp`, `test_for.cp`, `test_while.cp`)
- 函数 (`test_functions.cp`, `test_recursion.cp`)
- 数据结构 (`test_array.cp`, `test_string.cp`, `test_boolean.cp`)
- REPL 模式 (`test_repl_expr.cp`, `test_repl_sim.cp`)
- 边界情况 (`test_nested.cp`, `test_variables.cp`)

### 14.3 基准测试

**目录**: [benchmarks/](file:///d:/CPLANG/benchmarks/) — 性能基准测试:
- `bench_10k.cp` — 万次循环
- `bench_pure.cp` — 纯计算
- `bench_simple.cp` — 基础运算
- `bench_cpp.cpp` / `bench_micro.cpp` — C++ 对照测试

### 14.4 游戏测试

**目录**: [games/](file:///d:/CPLANG/games/) — 完整游戏作为集成测试:
- `贪吃蛇.cp` — Snake 游戏
- `2048.cp` — 2048 数字游戏
- `打方块.cp` — Breakout 打砖块
- `扫雷.cp` — 扫雷
- `俄罗斯方块.cp` — Tetris

---

> **文档生成时间**: 2026-05-27 | **项目版本**: v0.1.0-beta