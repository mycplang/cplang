# CP 语言 v0.1.0-beta 发布说明

## 概述

CP语言首个 Beta 版本。中文编程语言，编译器+VM+JIT 全链路。

## 核心特性

- **中文语法**: `打印` `如果` `当` `函数` `变量`
- **隐式声明**: `x = 10`，无需 var/let
- **渐进类型**: 动态起步，加类型标注获得 JIT 加速
- **JIT 编译**: LLVM 18 ORC JIT，热点函数 95x 加速
- **所有权检查**: Rust 式 `&x` / `&可写 x` 借用规则
- **包管理器**: 中文 `包` CLI，`包 安装 math`
- **交互 REPL**: `-r` 启动，表达式自动打印
- **错误行号**: 运行时错误附带源码行号

## 快速开始

```bash
# 编译执行
./cplang -c hello.cp

# JIT 模式
./cplang -j hello.cp

# 交互 REPL
./cplang -r

# 包管理器
./tools/包 搜索
./tools/包 安装 math
```

## 示例

```cp
函数 斐波那契(n: 整数): 整数 {
    如果 (n < 2) { 返回 n }
    返回 斐波那契(n-1) + 斐波那契(n-2)
}
打印(斐波那契(10))  // 55

// 包管理
导入 math;
打印(立方(5))        // 125
打印(阶乘(6))        // 720

// Crypto
打印(sha256("hello"))  // 2cf24dba...
```

## 已知限制

- 仅支持 Linux x86-64 (CentOS 7+)
- 类/闭包语法未实现
- 浮点函数 JIT 需手动标注类型
- GC 未充分压力测试

## 贡献

- 源码: https://github.com/mycplang/cplang
- 包注册表: https://github.com/mycplang/registry
