# CP 语言标准库 API 参考

> 版本: v0.1.0-beta | 函数总数: 800+ | 中文别名: 775+

---

## 目录

1. [数学模块](#1-数学模块) — 算术、三角函数、统计、随机数
2. [字符串模块](#2-字符串模块) — 查找、替换、分割、格式化
3. [数组模块](#3-数组模块) — 增删改查、排序、切片
4. [表模块](#4-表模块) — 键值对、哈希表操作
5. [IO 模块](#5-io-模块) — 打印、输入、文件读写
6. [文件系统模块](#6-文件系统模块) — 文件操作、路径处理
7. [时间与系统模块](#7-时间与系统模块) — 时间、进程、环境
8. [容器模块](#8-容器模块) — 集合、栈、队列、链表
9. [算法与位运算模块](#9-算法与位运算模块) — 排序、查找、位操作
10. [网络模块](#10-网络模块) — TCP/UDP、HTTP、WebSocket
11. [正则表达式模块](#11-正则表达式模块) — 匹配、搜索、替换
12. [编码与加密模块](#12-编码与加密模块) — Base64、MD5、SHA、AES
13. [变体与工具模块](#13-变体与工具模块) — Optional、Variant、Tuple
14. [并发模块](#14-并发模块) — 线程、Channel、Mutex
15. [数据库模块](#15-数据库模块) — SQLite、MySQL、Redis
16. [图形模块](#16-图形模块) — Raylib 2D/3D、ImGui

---

## 使用约定

文档中函数以 **中文名(参数)** 形式给出，括号内附英文原名。
所有函数同时支持中文和英文调用。

```
类型标记:
  任意      — 任意类型的值
  整数      — 整数值 (Int64)
  浮点数    — 浮点值 (Float64)  
  字符串    — 字符串类型
  数组      — 数组类型
  表        — 表/字典类型
  布尔      — 布尔值 (真/假)
  函数      — 函数/闭包类型
```

---

## 1. 数学模块

### 1.1 基本运算

| 函数 | 别名 | 说明 |
|------|------|------|
| `绝对值(n)` | `abs(n)` | 返回 n 的绝对值 |
| `平方根(n)` | `sqrt(n)` | 返回 n 的平方根 |
| `幂(a, b)` | `pow(a, b)` | 返回 a 的 b 次方 |
| `向下取整(n)` | `floor(n)` | 向下取整，返回整数 |
| `向上取整(n)` | `ceil(n)` | 向上取整，返回整数 |
| `四舍五入(n)` | `round(n)` | 四舍五入到最近整数 |

### 1.2 三角函数

| 函数 | 别名 | 说明 |
|------|------|------|
| `正弦(x)` | `sin(x)` | 正弦 (弧度) |
| `余弦(x)` | `cos(x)` | 余弦 (弧度) |
| `正切(x)` | `tan(x)` | 正切 (弧度) |
| `反正弦(x)` | `asin(x)` | 反正弦 |
| `反余弦(x)` | `acos(x)` | 反余弦 |
| `反正切(x)` | `atan(x)` | 反正切 |
| `反正切2(y, x)` | `atan2(y, x)` | 双参数反正切 |

### 1.3 对数和指数

| 函数 | 别名 | 说明 |
|------|------|------|
| `自然对数(x)` | `log(x)`, `ln(x)` | 自然对数 (以 e 为底) |
| `对数10(x)` | `log10(x)` | 以 10 为底的对数 |
| `对数2(x)` | `log2(x)` | 以 2 为底的对数 |
| `自然指数(x)` | `exp(x)` | e 的 x 次方 |
| `指数2(x)` | `exp2(x)` | 2 的 x 次方 |
| `指数减1(x)` | `expm1(x)` | e^x - 1 |

### 1.4 双曲函数

| 函数 | 说明 |
|------|------|
| `sinh(x)` | 双曲正弦 |
| `cosh(x)` | 双曲余弦 |
| `tanh(x)` | 双曲正切 |
| `asinh(x)` | 反双曲正弦 |
| `acosh(x)` | 反双曲余弦 |
| `atanh(x)` | 反双曲正切 |

### 1.5 其他数学函数

| 函数 | 别名 | 说明 |
|------|------|------|
| `立方根(x)` | `cbrt(x)` | 立方根 |
| `斜边(a, b)` | `hypot(a, b)` | √(a² + b²) |
| `截断(x)` | `trunc(x)` | 向零取整 |
| `浮点绝对值(x)` | `fabs(x)` | 浮点数绝对值 |
| `浮点取模(a, b)` | `fmod(a, b)` | 浮点数取模 |
| `余数(a, b)` | `remainder(a, b)` | IEEE 余数 |
| `符号复制(a, b)` | `copysign(a, b)` | a 的绝对值 + b 的符号 |
| `乘加(a, b, c)` | `fma(a, b, c)` | a×b + c |
| `阶乘(n)` | `factorial(n)` | n 的阶乘 |
| `符号(n)` | `sign(n)` | 返回 -1, 0, 或 1 |
| `角度转弧度(d)` | `deg2rad(d)` | 度 → 弧度 |
| `弧度转角度(r)` | `rad2deg(r)` | 弧度 → 度 |
| `圆周率()` | `pi()` | 返回 π (3.14159...) |
| `自然常数()` | `e()` | 返回 e (2.71828...) |

### 1.6 统计函数

| 函数 | 说明 |
|------|------|
| `mean(arr)` | 平均值 |
| `median(arr)` | 中位数 |
| `variance(arr)` | 方差 |
| `stddev(arr)` | 标准差 |
| `mode(arr)` | 众数 |
| `percentile(arr, p)` | 第 p 百分位数 |
| `correlation(a, b)` | 相关系数 |
| `covariance(a, b)` | 协方差 |
| `linearRegression(x, y)` | 线性回归 |

### 1.7 随机数

| 函数 | 别名 | 说明 |
|------|------|------|
| `随机值(min, max)` | `random(min, max)` | [min, max] 区间随机整数 |
| `随机浮点()` | `randomFloat()` | [0.0, 1.0) 随机浮点数 |
| `随机正态(mean, std)` | `randomNormal(mean, std)` | 正态分布随机数 |
| `随机种子(seed)` | `randomSeed(seed)` | 设置随机种子 |
| `洗牌(arr)` | `shuffle(arr)` | 随机打乱数组 |
| `均匀随机(min, max)` | `randomUniformInt(min, max)` | 均匀分布整数 |
| `指数随机(lambda)` | `randomExponential(lambda)` | 指数分布 |
| `伯努利随机(p)` | `randomBernoulli(p)` | 伯努利试验 |
| `泊松随机(lambda)` | `randomPoisson(lambda)` | 泊松分布 |

### 示例

```cp
变量 r = 随机值(1, 100);
变量 pi = 圆周率();
变量 s = 平方根(16);        // s = 4
变量 f = 阶乘(5);           // f = 120
变量 x = 正弦(圆周率()/2);  // x ≈ 1.0
```

---

## 2. 字符串模块

### 2.1 基本操作

| 函数 | 中文别名 | 说明 |
|------|---------|------|
| `长度(s)` | `strlen(s)` | 返回字符串长度（字符数） |
| `子串(s, start, len)` | `substr(s, start, len)` | 提取子串 |
| `连接(a, b)` | `concat(a, b)` | 拼接两个字符串 |
| `查找(s, pat)` | `find(s, pat)` | 返回 pat 首次出现的位置，未找到返回 -1 |
| `替换(s, from, to)` | `replace(s, from, to)` | 替换（单次）第一个匹配子串 |
| `分割(s, sep)` | `split(s, sep)` | 按分隔符分割，返回数组 |
| `修剪(s)` | `trim(s)` | 去除首尾空白字符 |
| `小写(s)` | `lower(s)` | 转为小写 |
| `大写(s)` | `upper(s)` | 转为大写 |
| `以...开头(s, prefix)` | `startsWith(s, prefix)` | 检查前缀 |
| `以...结尾(s, suffix)` | `endsWith(s, suffix)` | 检查后缀 |
| `包含(s, sub)` | `contains(s, sub)` | 检查是否包含子串 |
| `重复(s, n)` | `repeat(s, n)` | 重复 n 次 |
| `反转字符串(s)` | `reverse(s)` | 反转字符串 |
| `计数(s, sub)` | `count(s, sub)` | 统计子串出现次数 |

### 2.2 大小写转换

| 函数 | 说明 |
|------|------|
| `大小写翻转(s)` | 翻转每个字符的大小写 |
| `首字母大写(s)` | 首字母大写，其余小写 |
| `单词首字母大写(s)` | 每个单词首字母大写 |

### 2.3 编码转换

| 函数 | 别名 | 说明 |
|------|------|------|
| `转整数(s)` | `parseInt(s)` | 字符串 → 整数 |
| `转浮点(s)` | `parseFloat(s)` | 字符串 → 浮点数 |
| `转字符串(v)` | `toString(v)` | 任意值 → 字符串 |
| `字符码(c)` | `charCodeAt(s, i)` | 获取第 i 个字符的编码 |
| `码转字符(n)` | `fromCharCode(n)` | 编码 → 字符 |
| `格式化(fmt, ...)` | `format(fmt, ...)` | 格式化字符串（{} 占位符） |

### 示例

```cp
变量 s = "  Hello World  ";
变量 t = 修剪(s);              // "Hello World"
变量 idx = 查找(s, "World");   // 7
变量 parts = 分割("a,b,c", ",");  // ["a", "b", "c"]
变量 upper = 大写("hello");    // "HELLO"
```

---

## 3. 数组模块

### 3.1 基本操作

| 函数 | 中文别名 | 说明 |
|------|---------|------|
| `数组长(arr)` | `arrlen(arr)` | 返回数组长度 |
| `长度(arr)` | `len(arr)` | 通用长度（同 arrlen） |
| `追加(arr, val)` | `push(arr, val)` | 在末尾追加元素 |
| `弹出(arr)` | `pop(arr)` | 移除并返回末尾元素 |
| `插入(arr, idx, val)` | `insert(arr, idx, val)` | 在指定位置插入 |
| `移除(arr, idx)` | `remove(arr, idx)` | 移除指定位置的元素 |
| `切片(arr, start, end)` | `slice(arr, start, end)` | 提取 [start, end) 子数组 |
| `反转(arr)` | `reverse(arr)` | 反转数组，返回新数组 |
| `排序(arr)` | `sort(arr)` | 排序，返回新数组 |
| `取索引(arr, val)` | `indexOf(arr, val)` | 查找值首次出现的索引 |
| `取最后索引(arr, val)` | `lastIndexOf(arr, val)` | 查找值最后出现的索引 |
| `填充(arr, val, start, end)` | `fill(arr, val, start, end)` | 填充指定范围 |

### 3.2 高阶函数

| 函数 | 说明 |
|------|------|
| `映射(arr, fn)` / `map(arr, fn)` | 对每个元素应用 fn，返回新数组 |
| `过滤(arr, fn)` / `filter(arr, fn)` | 保留 fn 返回真的元素 |
| `累积(arr, fn, init)` / `reduce(arr, fn, init)` | 从左到右累积 |
| `所有(arr, fn)` / `every(arr, fn)` | 所有元素满足条件？ |
| `任意(arr, fn)` / `some(arr, fn)` | 任一元素满足条件？ |
| `查找元素(arr, fn)` / `find(arr, fn)` | 找到第一个满足条件的元素 |
| `展平(arr)` / `flatten(arr)` | 展平一层嵌套数组 |

### 3.3 集合操作

| 函数 | 说明 |
|------|------|
| `并集(a, b)` / `union(a, b)` | 两个数组的并集 |
| `交集(a, b)` / `intersection(a, b)` | 两个数组的交集 |
| `差集(a, b)` / `difference(a, b)` | a 中有但 b 中没有的元素 |
| `去重(arr)` / `unique(arr)` | 去除重复元素 |

### 示例

```cp
变量 arr = [3, 1, 2];
追加(arr, 4);                    // [3, 1, 2, 4]
变量 sorted = 排序(arr);         // [1, 2, 3, 4]
变量 doubled = 映射(arr, (x) => x * 2);  // [3, 1, 2, 4] × 2
变量 sub = 切片(arr, 1, 3);      // [1, 2]
```

---

## 4. 表模块

CP 语言的"表"是一种灵活的键值对数据结构，类似于字典/哈希表。

### 4.1 表操作

| 函数 | 中文别名 | 说明 |
|------|---------|------|
| `表创建(keys?, vals?)` | `tableCreate(keys?, vals?)` | 创建新表 |
| `表取(t, key)` | `tableGet(t, key)` | 获取键对应的值 |
| `表设(t, key, val)` | `tableSet(t, key, val)` | 设置键值对 |
| `表长(t)` | `tableLen(t)` | 表的键数量 |
| `表键(t)` | `tableKeys(t)` | 返回所有键的数组 |
| `表值(t)` | `tableValues(t)` | 返回所有值的数组 |
| `表含有(t, key)` | `tableHas(t, key)` | 检查键是否存在 |
| `表删除(t, key)` | `tableDelete(t, key)` | 删除键 |
| `表清空(t)` | `tableClear(t)` | 清空所有键值对 |
| `表遍历(t, fn)` | `tableForEach(t, fn)` | 对每个键值对调用 fn(key, val) |

### 示例

```cp
变量 t = 表创建(["名字", "年龄"], ["小明", 12]);
表设(t, "分数", 95);
变量 name = 表取(t, "名字");       // "小明"
变量 age = 表取(t, "年龄");        // 12
变量 count = 表长(t);              // 3
```

---

## 5. IO 模块

### 5.1 控制台输出

| 函数 | 中文别名 | 说明 |
|------|---------|------|
| `打印(...)` | `print(...)` | 打印参数（空格分隔） |
| `打印行(...)` | `println(...)` | 打印参数并换行 |
| `格式化打印(fmt, ...)` | `printf(fmt, ...)` | 格式化打印（{} 占位符） |
| `清屏()` | `clearScreen()` | 清空控制台 |

### 5.2 控制台输入

| 函数 | 中文别名 | 说明 |
|------|---------|------|
| `输入()` | `input()` | 读取一行用户输入 |
| `输入提示(prompt)` | `inputPrompt(prompt)` | 显示提示并读取输入 |
| `读取密码(prompt)` | `readPassword(prompt)` | 无回显读取密码 |

### 5.3 文件读写

| 函数 | 中文别名 | 说明 |
|------|---------|------|
| `读取文件(path)` | `readFile(path)` | 读取文件全部内容 |
| `写入文件(path, content)` | `writeFile(path, content)` | 写入内容到文件 |
| `追加文件(path, content)` | `appendFile(path, content)` | 追加内容到文件 |
| `读取行(path)` | `readLines(path)` | 读取文件所有行，返回数组 |
| `写入行(path, lines)` | `writeLines(path, lines)` | 写入行数组到文件 |
| `读取二进制(path)` | `readBinary(path)` | 二进制读取 |
| `写入二进制(path, data)` | `writeBinary(path, data)` | 二进制写入 |

### 示例

```cp
打印("你好", "世界");           // 你好 世界
变量 name = 输入("你的名字: ");
写入文件("test.txt", "Hello CP!");
变量 content = 读取文件("test.txt");
```

---

## 6. 文件系统模块

| 函数 | 中文别名 | 说明 |
|------|---------|------|
| `文件存在(path)` | `fileExists(path)` | 检查文件/目录是否存在 |
| `是文件(path)` | `isFile(path)` | 是否为普通文件 |
| `是目录(path)` | `isDir(path)` | 是否为目录 |
| `文件大小(path)` | `fileSize(path)` | 返回文件大小（字节） |
| `删除文件(path)` | `deleteFile(path)` | 删除文件 |
| `删除目录(path)` | `deleteDir(path)` | 删除目录（递归） |
| `创建目录(path)` | `createDir(path)` | 创建目录 |
| `重命名(old, new)` | `rename(old, new)` | 重命名文件/目录 |
| `移动文件(src, dst)` | `moveFile(src, dst)` | 移动文件 |
| `复制文件(src, dst)` | `copyFile(src, dst)` | 复制文件 |
| `列出目录(path)` | `listDir(path)` | 列出目录内容，返回数组 |
| `当前目录()` | `currentDir()` | 获取当前工作目录 |
| `改变目录(path)` | `changeDir(path)` | 改变工作目录 |
| `临时目录()` | `tempDir()` | 获取临时目录路径 |
| `主目录()` | `homeDir()` | 获取用户主目录 |
| `路径连接(...)` | `pathJoin(...)` | 拼接路径段 |
| `绝对路径(path)` | `absolutePath(path)` | 获取绝对路径 |
| `文件名(path)` | `fileName(path)` | 提取文件名 |
| `扩展名(path)` | `fileExt(path)` | 提取扩展名 |
| `父目录(path)` | `parentDir(path)` | 提取父目录路径 |
| `文件修改时间(path)` | `fileModTime(path)` | 文件最后修改时间 |

### 示例

```cp
如果 (文件存在("config.json")) {
    变量 content = 读取文件("config.json");
}
列出目录("./src");     // 返回文件列表
```

---

## 7. 时间与系统模块

### 7.1 时间

| 函数 | 中文别名 | 说明 |
|------|---------|------|
| `当前时间()` | `currentTime()` | Unix 时间戳（秒） |
| `当前毫秒()` | `currentMillis()` | Unix 时间戳（毫秒） |
| `当前微秒()` | `currentMicros()` | Unix 时间戳（微秒） |
| `睡眠(ms)` | `sleep(ms)` | 暂停执行指定毫秒 |
| `计时器()` | `timer()` | 高精度计时器（秒） |
| `格式化时间(ts, fmt)` | `formatTime(ts, fmt)` | 格式化时间戳 |
| `解析时间(str, fmt)` | `parseTime(str, fmt)` | 解析时间字符串 |
| `持续时间(n)` | `duration(n)` | 创建持续时间对象 |

### 7.2 系统

| 函数 | 中文别名 | 说明 |
|------|---------|------|
| `系统名称()` | `systemName()` | 操作系统名称 |
| `系统版本()` | `systemVersion()` | 操作系统版本 |
| `处理器数量()` | `cpuCount()` | CPU 核心数 |
| `内存总量()` | `totalMemory()` | 总内存（字节） |
| `可用内存()` | `freeMemory()` | 可用内存（字节） |
| `环境变量(name)` | `getEnv(name)` | 获取环境变量 |
| `设置环境变量(name, val)` | `setEnv(name, val)` | 设置环境变量 |
| `执行命令(cmd)` | `exec(cmd)` | 执行系统命令 |
| `程序参数()` | `programArgs()` | 获取程序启动参数 |
| `程序退出(code)` | `exit(code)` | 退出程序 |
| `进程ID()` | `processId()` | 当前进程 PID |
| `平台()` | `platform()` | 返回 "windows" / "linux" / "macos" |

### 示例

```cp
变量 now = 当前时间();
打印("当前时间戳: " + 转字符串(now));
睡眠(1000);   // 等待 1 秒
变量 cpu = 处理器数量();
```

---

## 8. 容器模块

### 8.1 集合 (Set)

| 函数 | 说明 |
|------|------|
| `集合创建()` / `setCreate()` | 创建空集合 |
| `集合添加(s, val)` / `setAdd(s, val)` | 添加元素 |
| `集合移除(s, val)` / `setRemove(s, val)` | 移除元素 |
| `集合含有(s, val)` / `setHas(s, val)` | 检查是否包含 |
| `集合大小(s)` / `setSize(s)` | 集合大小 |
| `集合清空(s)` / `setClear(s)` | 清空集合 |
| `集合转数组(s)` / `setToArray(s)` | 转为数组 |

### 8.2 有序集合

| 函数 | 说明 |
|------|------|
| `orderedSetCreate()` | 创建有序集合 |

### 8.3 栈 (Stack)

| 函数 | 说明 |
|------|------|
| `栈创建()` / `stackCreate()` | 创建空栈 |
| `入栈(s, val)` / `stackPush(s, val)` | 压入栈顶 |
| `出栈(s)` / `stackPop(s)` | 弹出栈顶 |
| `栈顶(s)` / `stackPeek(s)` | 查看栈顶（不弹出） |
| `栈空(s)` / `stackEmpty(s)` | 栈是否为空 |
| `栈大小(s)` / `stackSize(s)` | 栈大小 |

### 8.4 队列 (Queue)

| 函数 | 说明 |
|------|------|
| `队列创建()` / `queueCreate()` | 创建空队列 |
| `入队(q, val)` / `queuePush(q, val)` | 入队（队尾） |
| `出队(q)` / `queuePop(q)` | 出队（队首） |
| `队首(q)` / `queueFront(q)` | 查看队首 |
| `队空(q)` / `queueEmpty(q)` | 队列是否为空 |
| `队大小(q)` / `queueSize(q)` | 队列大小 |

### 8.5 双端队列 (Deque)

| 函数 | 说明 |
|------|------|
| `dequeCreate()` | 创建双端队列 |
| `dequePushFront(d, v)` | 前端插入 |
| `dequePushBack(d, v)` | 后端插入 |
| `dequePopFront(d)` | 前端弹出 |
| `dequePopBack(d)` | 后端弹出 |

### 8.6 优先队列

| 函数 | 说明 |
|------|------|
| `优先队列创建()` / `pqCreate()` | 创建优先队列 |
| `pqPush(pq, val, prio)` | 插入带优先级的元素 |
| `pqPop(pq)` | 弹出最高优先级元素 |
| `pqPeek(pq)` | 查看最高优先级元素 |

### 8.7 链表

| 函数 | 说明 |
|------|------|
| `链表创建()` / `linkedListCreate()` | 创建双向链表 |
| `单链表创建()` / `slistCreate()` | 创建单向链表 |
| `链表添加(ll, val)` / `llAdd(ll, val)` | 尾部添加 |
| `链表删除(ll, idx)` / `llRemove(ll, idx)` | 删除指定位置元素 |
| `链表大小(ll)` / `llSize(ll)` | 链表大小 |

### 8.8 多重集/多重映射

| 函数 | 说明 |
|------|------|
| `multisetCreate()` | 创建多重集（允许重复元素） |
| `multimapCreate()` | 创建多重映射（允许重复键） |

### 示例

```cp
变量 s = 栈创建();
入栈(s, 1); 入栈(s, 2); 入栈(s, 3);
变量 top = 出栈(s);      // 3

变量 q = 队列创建();
入队(q, "A"); 入队(q, "B");
变量 front = 出队(q);    // "A"
```

---

## 9. 算法与位运算模块

### 9.1 排序与查找

| 函数 | 说明 |
|------|------|
| `稳定排序(arr)` / `stableSort(arr)` | 稳定排序 |
| `二分查找(arr, val)` / `binarySearch(arr, val)` | 二分查找 |
| `分区(arr, pred)` / `partition(arr, pred)` | 按谓词分区 |
| `stablePartition(arr, pred)` | 稳定分区 |
| `归并(a, b)` / `merge(a, b)` | 归并两个已排序数组 |
| `全排列(arr)` / `nextPermutation(arr)` | 下一个排列 |
| `prevPermutation(arr)` | 上一个排列 |
| `isSorted(arr)` | 检查是否已排序 |

### 9.2 位运算

| 函数 | 中文别名 | 说明 |
|------|---------|------|
| `位与(a, b)` | `bitAnd(a, b)` | a & b |
| `位或(a, b)` | `bitOr(a, b)` | a \| b |
| `位异或(a, b)` | `bitXor(a, b)` | a ^ b |
| `位非(a)` | `bitNot(a)` | ~a |
| `左移(a, n)` | `bitLeftShift(a, n)` | a << n |
| `右移(a, n)` | `bitRightShift(a, n)` | a >> n |
| `位计数(n)` | `bitCount(n)` | 二进制中 1 的个数 |
| `位测试(n, pos)` | `bitTest(n, pos)` | 测试第 pos 位 |

### 9.3 位集 (Bitset)

| 函数 | 别名 | 说明 |
|------|------|------|
| `位集置位(bs, pos)` | `bitsetSet(bs, pos)` | 设置位 |
| `位集清零(bs, pos)` | `bitsetClear(bs, pos)` | 清除位 |
| `位集翻转位(bs, pos)` | `bitsetToggle(bs, pos)` | 翻转位 |
| `位集测位(bs, pos)` | `bitsetTest(bs, pos)` | 测试位 |
| `位集计数(bs)` | `bitsetCount(bs)` | 计数置位数 |
| `位集全置(bs)` | `bitsetAll(bs)` | 所有位是否都为 1 |
| `位集任一(bs)` | `bitsetAny(bs)` | 是否有位为 1 |
| `位集全零(bs)` | `bitsetNone(bs)` | 所有位是否都为 0 |
| `位集翻转(bs)` | `bitsetFlip(bs)` | 翻转所有位 |
| `位集转字符串(bs)` | `bitsetToString(bs)` | 转为二进制字符串 |
| `字符串转位集(s)` | `bitsetFromString(s)` | 从二进制字符串创建 |

### 示例

```cp
变量 arr = [3, 1, 4, 1, 5];
变量 sorted = 排序(arr);           // [1, 1, 3, 4, 5]
变量 idx = 二分查找(sorted, 4);    // 2

变量 result = 位与(6, 3);          // 2
```

---

## 10. 网络模块

### 10.1 TCP

| 函数 | 中文别名 | 说明 |
|------|---------|------|
| `TCP服务器(port)` / `tcpServer(port)` | 创建 TCP 服务器 |
| `TCP连接(host, port)` / `tcpConnect(host, port)` | 创建 TCP 客户端连接 |
| `TCP接受(server)` / `tcpAccept(server)` | 接受客户端连接 |
| `TCP发送(conn, data)` / `tcpSend(conn, data)` | 发送数据 |
| `TCP接收(conn, size)` / `tcpRecv(conn, size)` | 接收数据 |
| `TCP关闭(conn)` / `tcpClose(conn)` | 关闭连接 |
| `TCP回显服务器(port)` / `tcpEchoServer(port)` | 简易回显服务器 |

### 10.2 UDP

| 函数 | 说明 |
|------|------|
| `udpSocket()` | 创建 UDP 套接字 |
| `udpSend(sock, host, port, data)` | 发送 UDP 数据 |
| `udpRecv(sock, size)` | 接收 UDP 数据 |
| `udpClose(sock)` | 关闭 UDP 套接字 |

### 10.3 HTTP

| 函数 | 中文别名 | 说明 |
|------|---------|------|
| `HTTP请求(method, url, headers?, body?)` | `httpRequest(...)` | 发送 HTTP 请求 |
| `HTTP获取(url)` | `httpGet(url)` | GET 请求 |
| `HTTP发布(url, body)` | `httpPost(url, body)` | POST 请求 |
| `HTTP服务器(port)` | `httpServer(port)` | 创建 HTTP 服务器 |
| `HTTP下载(url, path)` | `httpDownload(url, path)` | 下载文件 |

### 10.4 WebSocket

| 函数 | 说明 |
|------|------|
| `websocketConnect(url)` | 连接 WebSocket |
| `websocketSend(ws, msg)` | 发送消息 |
| `websocketRecv(ws)` | 接收消息 |
| `websocketClose(ws)` | 关闭连接 |

### 10.5 DNS

| 函数 | 说明 |
|------|------|
| `dns解析(host)` / `dnsResolve(host)` | DNS 解析，返回 IP |
| `dns反查(ip)` / `dnsReverse(ip)` | 反向 DNS 查询 |

### 示例

```cp
变量 resp = HTTP获取("https://api.example.com/data");
变量 client = TCP连接("localhost", 8080);
TCP发送(client, "Hello Server");
变量 reply = TCP接收(client, 1024);
TCP关闭(client);
```

---

## 11. 正则表达式模块

| 函数 | 中文别名 | 说明 |
|------|---------|------|
| `正则匹配(str, pat)` | `regexMatch(str, pat)` | 检查整体是否匹配 |
| `正则搜索(str, pat)` | `regexSearch(str, pat)` | 搜索第一个匹配 |
| `正则替换(str, pat, repl)` | `regexReplace(str, pat, repl)` | 替换所有匹配 |
| `正则分割(str, pat)` | `regexSplit(str, pat)` | 按正则分割 |
| `正则全找(str, pat)` | `regexFindAll(str, pat)` | 找到所有匹配 |

### 示例

```cp
变量 matched = 正则匹配("hello123", "[a-z]+[0-9]+");   // 真
变量 parts = 正则分割("a,b;c,d", "[,;]");               // ["a", "b", "c", "d"]
变量 result = 正则替换("hello world", "world", "CP");   // "hello CP"
```

---

## 12. 编码与加密模块

### 12.1 编码

| 函数 | 中文别名 | 说明 |
|------|---------|------|
| `Base64编码(data)` | `base64Encode(data)` | Base64 编码 |
| `Base64解码(str)` | `base64Decode(str)` | Base64 解码 |
| `URL编码(str)` | `urlEncode(str)` | URL 编码 |
| `URL解码(str)` | `urlDecode(str)` | URL 解码 |
| `JSON解析(str)` | `jsonParse(str)` | 解析 JSON 字符串 |
| `转JSON(val)` | `toJSON(val)` | 值转 JSON 字符串 |
| `CSV解析(str)` | `csvParse(str)` | 解析 CSV |
| `CSV写入(data)` | `csvWrite(data)` | 生成 CSV |

### 12.2 哈希

| 函数 | 中文别名 | 说明 |
|------|---------|------|
| `MD5(str)` | `md5(str)` | MD5 哈希 |
| `SHA1(str)` | `sha1(str)` | SHA-1 哈希 |
| `SHA256(str)` | `sha256(str)` | SHA-256 哈希 |
| `HMAC_MD5(key, msg)` | `hmacMd5(key, msg)` | HMAC-MD5 |
| `HMAC_SHA256(key, msg)` | `hmacSha256(key, msg)` | HMAC-SHA256 |
| `UUID()` | `uuid()` | 生成 UUID v4 |

### 12.3 加密

| 函数 | 说明 |
|------|------|
| `AES加密(data, key)` / `aesEncrypt(data, key)` | AES 加密 |
| `AES解密(data, key)` / `aesDecrypt(data, key)` | AES 解密 |
| `压缩(data)` / `compress(data)` | 数据压缩 (zlib) |
| `解压(data)` / `decompress(data)` | 数据解压 |

### 示例

```cp
变量 hash = SHA256("hello");
变量 encoded = Base64编码("Hello CP!");
变量 json = 转JSON([1, 2, 3]);     // "[1,2,3]"
变量 obj = JSON解析('{"a":1}');
变量 uid = UUID();                 // "550e8400-e29b-..."
```

---

## 13. 变体与工具模块

### 13.1 Optional

| 函数 | 说明 |
|------|------|
| `optionalOf(val)` | 创建有值的 Optional |
| `optionalEmpty()` | 创建空的 Optional |
| `optionalIsPresent(opt)` | 是否有值 |
| `optionalGet(opt)` | 获取值 |
| `optionalOrElse(opt, default)` | 获取值或默认值 |

### 13.2 Variant

| 函数 | 说明 |
|------|------|
| `variantCreate(tag, ...)` | 创建变体 |
| `variantTag(v)` | 获取变体标签 |
| `variantGet(v, field)` | 获取变体字段 |

### 13.3 Tuple

| 函数 | 说明 |
|------|------|
| `tupleCreate(...)` | 创建元组 |
| `tupleGet(t, idx)` | 获取第 idx 个元素 |
| `tupleSize(t)` | 元组大小 |

### 13.4 Any 类型

| 函数 | 说明 |
|------|------|
| `anyOf(val)` | 包装任意值 |
| `anyGet(a)` | 获取原始值 |

### 13.5 Pair / Complex

| 函数 | 说明 |
|------|------|
| `对(a, b)` / `pair(a, b)` | 创建键值对 |
| `复数(r, i)` / `complex(r, i)` | 创建复数 |
| `实部(c)` / `realPart(c)` | 复数实部 |
| `虚部(c)` / `imagPart(c)` | 复数虚部 |

### 13.6 数值极限

| 函数 | 说明 |
|------|------|
| `最大整数()` / `intMax()` | Int64 最大值 |
| `最小整数()` / `intMin()` | Int64 最小值 |
| `最大浮点()` / `floatMax()` | Float64 最大值 |
| `最小浮点()` / `floatMin()` | Float64 最小值 |

### 13.7 反射

| 函数 | 中文别名 | 说明 |
|------|---------|------|
| `类型(v)` | `typeof(v)` | 返回值的类型字符串 |
| `是类型(v, t)` | `isType(v, t)` | 检查类型 |
| `类型名(v)` | `typeName(v)` | 获取类的类型名 |
| `字段名(obj)` | `fieldNames(obj)` | 获取对象的字段名列表 |
| `方法名(obj)` | `methodNames(obj)` | 获取对象的方法名列表 |

### 示例

```cp
变量 t = 类型(42);                    // "int"
变量 t2 = 类型("hello");              // "string"
变量 t3 = 类型([1, 2]);               // "array"
```

---

## 14. 并发模块

### 14.1 线程

| 函数 | 中文别名 | 说明 |
|------|---------|------|
| `创建线程(fn)` | `threadCreate(fn)` | 创建并启动线程 |
| `等待线程(t)` | `threadJoin(t)` | 等待线程结束 |
| `分离线程(t)` | `threadDetach(t)` | 分离线程 |
| `线程ID()` | `threadId()` | 当前线程 ID |
| `让出()` | `yield()` | 让出 CPU 时间片 |

### 14.2 互斥与同步

| 函数 | 中文别名 | 说明 |
|------|---------|------|
| `互斥创建()` | `mutexCreate()` | 创建互斥锁 |
| `加锁(m)` | `mutexLock(m)` | 上锁 |
| `解锁(m)` | `mutexUnlock(m)` | 解锁 |
| `尝试加锁(m)` | `mutexTryLock(m)` | 尝试上锁 |
| `条件变量创建()` | `condCreate()` | 创建条件变量 |
| `等待条件(cv, m)` | `condWait(cv, m)` | 等待条件 |
| `通知一个(cv)` | `condSignal(cv)` | 唤醒一个等待线程 |
| `通知所有(cv)` | `condBroadcast(cv)` | 唤醒所有等待线程 |
| `信号量创建(count)` | `semCreate(count)` | 创建信号量 |
| `信号量P(s)` | `semWait(s)` | P 操作（等待） |
| `信号量V(s)` | `semPost(s)` | V 操作（释放） |
| `读写锁创建()` | `rwLockCreate()` | 创建读写锁 |
| `读锁(rw)` | `rwLockRead(rw)` | 获取读锁 |
| `写锁(rw)` | `rwLockWrite(rw)` | 获取写锁 |
| `解锁读写(rw)` | `rwLockUnlock(rw)` | 释放读写锁 |

### 14.3 Channel

| 函数 | 中文别名 | 说明 |
|------|---------|------|
| `通道创建(cap?)` | `channelCreate(cap?)` | 创建通道 |
| `通道发送(ch, val)` | `channelSend(ch, val)` | 发送数据 |
| `通道接收(ch)` | `channelRecv(ch)` | 接收数据 |
| `通道尝试接收(ch)` | `channelTryRecv(ch)` | 非阻塞接收 |
| `通道选择(channels, timeout?)` | `channelSelect(channels, timeout?)` | 多通道选择 |
| `通道关闭(ch)` | `channelClose(ch)` | 关闭通道 |
| `通道已关闭(ch)` | `channelClosed(ch)` | 检查是否已关闭 |

### 14.4 Future / Promise

| 函数 | 说明 |
|------|------|
| `futureCreate(fn)` | 创建 Future |
| `futureGet(f, timeout?)` | 获取结果 |
| `futureReady(f)` | 是否已完成 |

### 示例

```cp
变量 ch = 通道创建(10);
通道发送(ch, "Hello");
变量 msg = 通道接收(ch);

变量 m = 互斥创建();
加锁(m);
// ... 临界区 ...
解锁(m);
```

---

## 15. 数据库模块

### 15.1 SQLite

| 函数 | 中文别名 | 说明 |
|------|---------|------|
| `SQLite打开(path)` | `sqliteOpen(path)` | 打开/创建数据库 |
| `SQLite执行(db, sql)` | `sqliteExec(db, sql)` | 执行 SQL |
| `SQLite查询(db, sql)` | `sqliteQuery(db, sql)` | 查询并返回结果 |
| `SQLite关闭(db)` | `sqliteClose(db)` | 关闭数据库 |
| `SQLite准备(db, sql)` | `sqlitePrepare(db, sql)` | 预编译 SQL |
| `SQLite步进(stmt)` | `sqliteStep(stmt)` | 步进查询 |
| `SQLite列计数(stmt)` | `sqliteColumnCount(stmt)` | 结果列数 |
| `SQLite列名(stmt, i)` | `sqliteColumnName(stmt, i)` | 第 i 列列名 |
| `SQLite列值(stmt, i)` | `sqliteColumnValue(stmt, i)` | 第 i 列值 |
| `SQLite完成(stmt)` | `sqliteFinalize(stmt)` | 释放预编译语句 |

### 15.2 MySQL

| 函数 | 中文别名 | 说明 |
|------|---------|------|
| `MySQL连接(host, user, pass, db, port)` | `mysqlConnect(...)` | 连接 MySQL |
| `MySQL查询(conn, sql)` | `mysqlQuery(conn, sql)` | 执行查询 |
| `MySQL执行(conn, sql)` | `mysqlExec(conn, sql)` | 执行非查询 SQL |
| `MySQL关闭(conn)` | `mysqlClose(conn)` | 关闭连接 |
| `MySQL错误(conn)` | `mysqlError(conn)` | 获取错误信息 |

### 15.3 PostgreSQL

| 函数 | 中文别名 | 说明 |
|------|---------|------|
| `PG连接(host, user, pass, db, port)` | `pgConnect(...)` | 连接 PostgreSQL |
| `PG查询(conn, sql)` | `pgQuery(conn, sql)` | 执行查询 |
| `PG执行(conn, sql)` | `pgExec(conn, sql)` | 执行非查询 SQL |
| `PG关闭(conn)` | `pgClose(conn)` | 关闭连接 |
| `PG错误(conn)` | `pgError(conn)` | 获取错误信息 |

### 15.4 Redis

| 函数 | 中文别名 | 说明 |
|------|---------|------|
| `Redis连接(host, port, pass?)` | `redisConnect(...)` | 连接 Redis |
| `Redis获取(conn, key)` | `redisGet(conn, key)` | GET |
| `Redis设置(conn, key, val)` | `redisSet(conn, key, val)` | SET |
| `Redis删除(conn, key)` | `redisDel(conn, key)` | DEL |
| `Redis过期(conn, key, sec)` | `redisExpire(conn, key, sec)` | EXPIRE |
| `Redis关闭(conn)` | `redisClose(conn)` | 关闭连接 |

### 示例

```cp
变量 db = SQLite打开("test.db");
变量 results = SQLite查询(db, "SELECT * FROM users");
SQLite关闭(db);

变量 mysql = MySQL连接("localhost", "root", "pass", "test", 3306);
变量 rows = MySQL查询(mysql, "SELECT * FROM products");
MySQL关闭(mysql);
```

---

## 16. 图形模块

CP 语言通过 Raylib 绑定提供 2D/3D 图形能力，通过 ImGui 提供 UI 控件。

### 16.1 窗口管理

| 函数 | 中文别名 | 说明 |
|------|---------|------|
| `初始化窗口(w, h, title)` | `initWindow(w, h, title)` | 创建窗口 |
| `窗口应关闭()` | `windowShouldClose()` | 窗口是否应关闭 |
| `关闭窗口()` | `closeWindow()` | 关闭窗口 |
| `设置目标帧率(fps)` | `setTargetFPS(fps)` | 设置目标帧率 |

### 16.2 绘图

| 函数 | 中文别名 | 说明 |
|------|---------|------|
| `开始绘图()` | `beginDrawing()` | 开始一帧 |
| `结束绘图()` | `endDrawing()` | 结束一帧 |
| `清空背景(color)` | `clearBackground(color)` | 清屏 |

### 16.3 形状

| 函数 | 中文别名 | 说明 |
|------|---------|------|
| `绘制矩形(x, y, w, h, color)` | `drawRectangle(x, y, w, h, color)` | 矩形 |
| `绘制圆形(cx, cy, r, color)` | `drawCircle(cx, cy, r, color)` | 圆 |
| `绘制直线(x1, y1, x2, y2, color)` | `drawLine(x1, y1, x2, y2, color)` | 直线 |

### 16.4 文本

| 函数 | 中文别名 | 说明 |
|------|---------|------|
| `绘制文本(text, x, y, size, color)` | `drawText(text, x, y, size, color)` | 绘制文本 |
| `绘制帧率(x, y)` | `drawFPS(x, y)` | 显示 FPS |

### 16.5 输入

| 函数 | 中文别名 | 说明 |
|------|---------|------|
| `键盘按下(key)` | `isKeyPressed(key)` | 按键是否按下 |
| `鼠标X()` | `getMouseX()` | 鼠标 X 坐标 |
| `鼠标Y()` | `getMouseY()` | 鼠标 Y 坐标 |

### 16.6 颜色常量

`乳白` `浅灰` `深灰` `红` `绿` `蓝` `深绿` `黄` `橙` `紫` `粉` `黑` `白`

### 示例

```cp
初始化窗口(800, 600, "我的游戏");
设置目标帧率(60);

当 (窗口应关闭() == 假) {
    开始绘图();
    清空背景(乳白);
    绘制矩形(100, 100, 200, 150, 蓝);
    绘制文本("Hello CP!", 10, 10, 20, 深灰);
    结束绘图();
}
关闭窗口();
```

---

## 函数快速索引

### 最常用的 20 个函数

1. `打印(...)` — 输出到控制台
2. `输入()` — 读取用户输入
3. `长度(x)` — 获取长度
4. `转字符串(v)` — 转换为字符串
5. `转整数(s)` — 转换为整数
6. `如果(条件, ...)` — 条件判断（语句）
7. `当(条件, ...)` — 循环（语句）
8. `函数 名称(...)` — 定义函数（语句）
9. `追加(arr, v)` — 数组追加
10. `弹出(arr)` — 数组弹出
11. `表创建(keys?, vals?)` — 创建表
12. `表取(t, k)` — 表取值
13. `表设(t, k, v)` — 表设值
14. `排序(arr)` — 排序
15. `包含(s, sub)` — 字符串包含
16. `替换(s, from, to)` — 字符串替换
17. `随机值(min, max)` — 随机数
18. `绝对值(n)` — 绝对值
19. `类型(v)` — 获取类型
20. `读取文件(path)` — 读取文件

---

> 📝 本文档基于 CP 语言 v0.1.0-beta。随着标准库的扩展将持续更新。
> 如需帮助，请访问 https://gitee.com/cplang/cplang
