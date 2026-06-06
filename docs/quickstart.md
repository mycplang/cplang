# CP 语言快速入门

CP 语言是一门**中文编程语言**，专为少儿编程入门设计，支持从零基础到高级开发的渐进学习。

## 安装与运行

### Linux
```bash
# 构建
g++ -std=c++17 -Iinclude -include posix_compat.h \
  src/vm/value.cpp src/vm/vm.cpp src/lexer/lexer.cpp \
  src/parser/parser.cpp src/semantic/semantic_analyzer.cpp \
  src/codegen/codegen.cpp src/main.cpp src/stdlib/stdlib_linux.cpp \
  -lpthread -ldl -o build/cplang_linux

# 运行文件
./build/cplang_linux -c 程序.cp

# 交互式 REPL
./build/cplang_linux -r
```

### Windows
```bat
build_portable_msvc.bat
build\cplang_win.exe -c 程序.cp
```

## 第一步：你好世界

创建文件 `hello.cp`：

```
打印("你好，世界！")
```

运行：
```bash
./build/cplang_linux -c hello.cp
```

输出：`你好，世界！`

## 第二步：变量与运算

CP 语言支持**隐式声明**——不需要写 `变量` 关键字：

```
x = 10
y = 20
打印(x + y)      // 30
打印(x * y)      // 200
打印("结果是：" + x)  // 字符串拼接
```

也支持显式声明（推荐用于大程序）：

```
变量 姓名 = "小明"
变量 年龄 = 12
打印(姓名 + "今年" + 年龄 + "岁")
```

## 第三步：条件判断

```
分数 = 85

如果 (分数 >= 90) {
    打印("优秀！")
} 否则 如果 (分数 >= 60) {
    打印("及格")
} 否则 {
    打印("需要努力")
}
```

## 第四步：循环

```
// 打印 1 到 10
i = 1
当 (i <= 10) {
    打印(i)
    i = i + 1
}
```

## 第五步：函数

```
函数 打招呼(名字) {
    打印("你好，" + 名字 + "！")
}

函数 平方(x) {
    返回 x * x
}

打招呼("小明")
打印(平方(5))  // 25
```

### 带类型的函数（更快！）

```
函数 累加(n: 整数): 整数 {
    变量 s = 0
    变量 i = 0
    当 (i < n) {
        s = s + i
        i = i + 1
    }
    返回 s
}

打印(累加(100))  // 4950
```

## 第六步：数组

```
水果 = ["苹果", "香蕉", "橘子"]
打印(水果[0])     // 苹果
打印(长度(水果))   // 3

// 遍历
i = 0
当 (i < 长度(水果)) {
    打印(水果[i])
    i = i + 1
}
```

## 第七步：表（字典）

```
学生 = {}
表设(学生, "姓名", "小明")
表设(学生, "年龄", 12)

打印(表取(学生, "姓名"))  // 小明
```

## 交互式 REPL

不需要写文件！直接在终端里试：

```bash
$ ./build/cplang_linux -r

>>> 3 + 5
8
>>> x = 100
>>> x * 2
200
>>> 函数 加倍(n) { 返回 n * 2 }
>>> 打印(加倍(50))
100
>>> :q
再见！
```

## 下一步

- [功能参考](features.md) — 全部语法和内置函数
- [教程](tutorial/) — 循序渐进学编程
- [11 个教学示例](../examples/cp_demos/) — 从 Hello World 到成绩管理系统
