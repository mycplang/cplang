# CP语言架构重新设计方案

> 目标：5个月内交付Beta，支持从少儿编程到系统编程的渐进成长路径  
> 核心原则：x=10即整数，不写类型也能原生性能，需要时可选Rust式所有权

---

## 设计决策

| 决策 | 方案 |
|------|------|
| 类型声明 | 默认推断，可选标注 `整数 x = 10` |
| 值表示 | NaN-boxing：Int/Float/Bool/Nil内联，无需堆分配 |
| 类型变 | 允许但有提示"类型变化可能影响性能" |
| 内存模型 | GC默认，所有权可选（可信块） |
| 字符串 | 用户只看到一种 `字符串` 类型 |
| 错误消息 | 全中文，含建议修复 |

---

## Phase 1: NaN-boxing 值表示（基础层）

**目标**：小值(Int/Float/Bool/Nil)不再走GC堆分配，直接存在 Value 的 8 字节中。

**当前问题**：
- struct Value { union{Int64 i, Float64 f, void* ptr}; UInt16 tag; }  —— 12字节 + 每次Int都要包装  
- 整数操作仍需check tag

**新设计**：
```
Value = 64-bit tagged union (NaN-boxing)
  - IEEE 754 double: 1 sign + 11 exponent + 52 mantissa
  - NaN有 2^51 个表示可用作tagged pointer
  - 非NaN = Float64直接存
  - NaN带payload = 指针/特殊值(Int/Bool/Nil)
  
布局：
  ptr: [48-bit pointer][16-bit tag]  利用x64只用了低48位
  小整数: [Int53][tag bits]           53位足够各种整数
  Bool/Nil: 特殊magic值
```

### Phase 1 任务

1. 新 Value 结构体（vm/value.hpp）
2. Int/Float/Bool/Nil 的 NaN-boxing 工厂方法和访问器
3. 更新 vm.cpp 中的 OP_ADD/SUB/MUL/DIV 等直接操作裸值
4. 更新所有 stdlib 中创建 Value 的地方
5. 更新 OP_LOADINT 直接存值到寄存器而非常量池
6. 添加基准测试对比旧版性能

---

## Phase 2: 类型推断系统

**目标**：`x = 10` 自动推断为 Int，生成原生指令。

### Phase 2 任务

1. 在 SemanticAnalyzer 添加类型推断 pass
2. AST 节点加 inferredType 字段
3. 推断规则：
   - 字面量直接推断
   - 二元运算根据操作数推断
   - 函数参数从调用点推断
   - 推断失败 → 标记为 Dynamic

---

## Phase 3: 双轨指令集

**目标**：Codegen 根据推断类型选择原生指令或动态指令。

### Phase 3 任务

1. 新增 typed opcodes：OP_IADD, ISUB, IMUL, IDIV, FADD, FSUB, FMUL, FDIV
2. Codegen::compileExpr 根据 inferredType 选择指令
3. 推断成功的用 typed 指令，失败的退化到通用 OP_ADD 等
4. VM 执行 typed 指令时跳过类型检查

---

## Phase 4: 简化语法（去变量关键字）

**目标**：`x = 10` 等价于 `变量 x = 10`

### Phase 4 任务

1. Parser 支持无关键字赋值语句
2. 区分赋值(=)和声明（首次出现为声明，再次出现为赋值）
3. 变量 x = 10 仍可用，与 x = 10 等价

---

## Phase 5: 中文错误消息系统

**目标**：所有编译/运行时错误都是中文，带修复建议。

### Phase 5 任务

1. 错误码系统
2. 错误消息模板(含行号、上下文、建议)
3. 常见错误检测（拼写建议："prnit" → 你是想说"打印"吗？）

---

## Phase 6: Rust式所有权（可选专家模式）

**目标**：类型标注后可启用所有权检查，零GC开销。

### Phase 6 任务

1. 借用检查器（编译时扫描）
2. `可信块` 语法支持
3. 所有权标注 `&T` / `&可写 T`
4. 自动析构插入

---

## 执行顺序

Phase 1 → Phase 2 → Phase 3 → Phase 4 → Phase 5 → Phase 6

Phase 1 是一切的基础，必须最先完成。
