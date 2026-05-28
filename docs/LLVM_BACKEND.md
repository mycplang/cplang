# LLVM后端使用文档

## 概述

CP语言编译器现已支持生成LLVM IR代码，可直接编译为机器码。

## 使用方法

### 1. 生成LLVM IR

```bash
# 编译CP源文件为LLVM IR
test_llvm.exe <source.cp>

# 输出文件: source.cp.ll
```

### 2. 编译LLVM IR为可执行文件

需要安装LLVM工具链（llc和clang）：

```bash
# 方法1: 使用LLVM官方安装包
# 下载: https://github.com/llvm/llvm-project/releases

# 方法2: 使用包管理器
# winget install LLVM.LLVM

# 编译步骤
llc test.ll -o output.s        # LLVM IR → 汇编
clang output.s -o output.exe   # 汇编 → 可执行文件
```

## 支持的功能

### ✅ 已完全支持

- [x] 函数定义（支持中文函数名）
- [x] 函数调用（支持递归）
- [x] 变量声明和初始化
- [x] 赋值操作符（=）
- [x] 算术运算（+、-、*、/、%）
- [x] 比较运算（==、!=、<、>、<=、>=）
- [x] 逻辑运算（&&、||、!）
- [x] if/else条件分支
- [x] while循环
- [x] for循环
- [x] return语句
- [x] break语句（跳出循环）
- [x] continue语句（继续下一次循环）
- [x] 数组索引访问（读/写）
- [x] 结构体成员访问（obj.member）
- [x] for-each循环（简化实现）
- [x] 递归函数调用

### 🔄 部分支持

- [ ] 数组字面量分配
- [ ] 结构体多成员访问（当前简化实现）
- [ ] for-each数组长度获取

### ⏳ 待实现

- [ ] 字符串操作
- [ ] 浮点数运算

## 示例代码

### 示例1: 基本函数

```cp
函数 加(a, b) {
    返回 a + b;
}

函数 主() {
    变量 x = 10;
    变量 y = 20;
    变量 z = 加(x, y);
    返回 z;
}
```

生成LLVM IR:
```llvm
define i64 @加(i64 %a, i64 %b) {
entry:
  %a.addr = alloca i64
  store i64 %a, i64* %a.addr
  %b.addr = alloca i64
  store i64 %b, i64* %b.addr
  %t0 = load i64, i64* %a
  %t1 = load i64, i64* %b
  %t2 = add i64 %t0, %t1
  ret i64 %t2
}
```

### 示例2: 控制流

```cp
函数 最大值(a, b) {
    如果 (a > b) {
        返回 a;
    } 否则 {
        返回 b;
    }
}

函数 阶乘(n) {
    如果 (n <= 1) {
        返回 1;
    }
    返回 n * 阶乘(n - 1);
}

函数 求和(n) {
    变量 sum = 0;
    变量 i = 1;
    当 (i <= n) {
        sum = sum + i;
        i = i + 1;
    }
    返回 sum;
}
```

### 示例3: break/continue

```cp
函数 测试跳出() {
    变量 i = 0;
    当 (i < 100) {
        如果 (i == 50) {
            跳出;  // break
        }
        i = i + 1;
    }
    返回 i;  // 返回 50
}

函数 测试继续() {
    变量 sum = 0;
    变量 i = 0;
    当 (i < 10) {
        i = i + 1;
        如果 (i % 2 == 0) {
            继续;  // continue
        }
        sum = sum + i;
    }
    返回 sum;  // 返回 25 (1+3+5+7+9)
}
```

### 示例4: 数组索引

```cp
函数 数组求和(arr, n) {
    变量 sum = 0;
    变量 i = 0;
    当 (i < n) {
        sum = sum + arr[i];   // 数组索引读取
        i = i + 1;
    }
    返回 sum;
}

函数 查找元素(arr, n, target) {
    变量 i = 0;
    当 (i < n) {
        如果 (arr[i] == target) {
            返回 i;
        }
        i = i + 1;
    }
    返回 -1;
}
```

## 架构设计

```
CP源代码 (.cp)
    ↓
Lexer (词法分析)
    ↓
Parser (语法分析) → AST
    ↓
LLVMCodegen (LLVM IR生成)
    ↓
LLVM IR (.ll文件)
    ↓
LLVM工具链 (llc/clang)
    ↓
可执行文件 (.exe)
```

## 技术细节

### 类型映射

| CP类型 | LLVM类型 |
|--------|----------|
| 整数 | i64 |
| 布尔 | i64 (0/1) |
| 浮点数 | double (待实现) |

### 调用约定

- 所有函数返回 i64
- 所有参数传递 i64
- 内部使用alloca存储局部变量

## 开发计划

1. **短期目标**
   - 完善结构体成员访问
   - 实现数组索引访问
   - 添加break/continue支持

2. **中期目标**
   - 浮点数完整支持
   - 字符串操作库
   - 垃圾回收集成

3. **长期目标**
   - JIT编译支持
   - 优化Pass集成
   - 调试信息生成

## 注意事项

1. 当前所有数值类型使用64位整数（i64）
2. 函数参数在函数入口被存储到栈上（.addr后缀）
3. 生成的LLVM IR是有效的，可以用标准LLVM工具链编译
4. 中文标识符完全支持

## 调试技巧

```bash
# 查看生成的LLVM IR
cat test.cp.ll

# 验证LLVM IR语法
llvm-as test.cp.ll -o test.bc

# 查看汇编输出
llc test.cp.ll -o test.s
cat test.s
```

## 参考链接

- [LLVM Language Reference](https://llvm.org/docs/LangRef.html)
- [LLVM IR Tutorial](https://llvm.org/docs/tutorial/)
- [CP语言文档](./README.md)
