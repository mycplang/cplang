# CP 标准库 vs C++ STL 差距分析

## 当前状态
559 函数 · 519 中文别名 · 64 个注册模块 · 91/91 测试全绿

---

## 按 C++ 头文件分类对标

### ✅ 已覆盖（无需补充）
| C++ | CP 对应 | 覆盖度 |
|-----|---------|--------|
| `<cmath>` | registerMath + registerMathMore | 95% |
| `<complex>` | registerComplex | 100% |
| `<algorithm>` | registerAlgorithms + registerAlgoExt | 85% |
| `<numeric>` | innerProduct/partialSum/adjacentDiff/iota | 80% |
| `<bit>` | registerBitwise | 100% |
| `<random>` | registerRandom (7分布) | 80% |
| `<limits>` | registerNumericLimits | 95% |
| `<string>` | registerString + StringMore + StringCase + StringSearch | 90% |
| `<regex>` | registerRegex | 90% |
| `<chrono>` | registerTime + registerTimeMore | 80% |
| `<filesystem>` | registerFile + registerFileMore + registerPath | 75% |
| `<thread>` | registerThreading (28函数) | 90% |
| `<mutex>` | mutexCreate/Lock/Unlock/TryLock + rwLock + cond | 85% |
| `<atomic>` | registerThreading.atomicInt系列 | 80% |
| `<future>` | futureGo/IsReady/Get | 70% |
| `<semaphore>` | semCreate/Post/TryWait | 90% |
| `<barrier>` | barrierCreate/Wait | 100% |
| `<deque>` `<list>` `<forward_list>` `<stack>` `<queue>` | 全部到位 | 95% |
| `<set>` `<map>` `<unordered_set>` `<unordered_map>` | 含 multiset/multimap 变体 | 95% |
| `<bitset>` | registerBitset | 95% |
| `<optional>` | registerOptional | 90% |
| `<variant>` | registerVariant | 80% |
| `<any>` | registerAny | 80% |
| `<tuple>` | registerTuple (弱化版) | 50% ⚠️ |
| `<utility>` | registerPair + registerUtils | 60% |
| `<cstdio>` `<io>` | registerIO + registerConsole | 70% |
| JSON | registerJSON | 90% |
| SQLite | registerSqlite | 90% |
| WebSocket | registerWebSocket | 80% |
| Crypto | registerCrypto + registerEncoding | 60% ⚠️ |

---

### 🔴 P0 — 严重缺失（影响测试/核心可用性）

| 缺失项 | C++ 对应 | 影响 |
|--------|---------|------|
| **`<ordered_map>`** | std::map (红黑树) | 有序键值对完全缺失，只有 unordered |
| **`stableSort`** | std::stable_sort | 在 registerAlgorithms 里缺失 |
| **`tuple` 增强** | std::tuple | 只有 tupMake/Get/Size/Cat/Slice，缺 tie/swap/结构化绑定 |
| **`charconv` 全功能** | from_chars/to_chars | 只有 intToHex，缺浮点↔字符串精确转换 |
| **`<source_location>`** | std::source_location | 3 个 function 都是 stub，无法获取实际行号 |

---

### 🟡 P1 — 功能缺口（影响对标完整度）

| 缺失项 | 说明 |
|--------|------|
| **`${}` 字符串内插** | C++ std::format (Python f-string) 替代方案 |
| **阶乘/tgamma/lgamma** | 数学特殊函数 (erf, erfc, tgamma, lgamma) |
| **`partition` 系列** | partition/stable_partition/partition_point (算法) |
| **`any_of/all_of/none_of`** | 谓词逻辑 (算法) |
| **`accumulate`/`reduce`** | 折叠归约 (区别于 innerProduct) |
| **`find_if`/`find_if_not`** | 条件查找 (配合 lambda) |
| **`generate`/`generate_n`** | 生成序列 |
| **`for_each`/`transform`** | 函数式遍历/映射 |
| **`call_once`** | 线程安全一次性初始化 |
| **`jthread`** | C++20 auto-join 线程 |
| **`span` 全功能** | 目前只有 spanNew/Len/Get，缺 subspan/first/last/迭代 |
| **`channel` 增强** | select/multiplex，目前单通道等待 |
| **`result/monad`** | Result类型缺 map/flatMap (and_then/or_else) |

---

### 🟢 P2 — 锦上添花（可选，非关键）

| 缺失项 | 说明 |
|--------|------|
| **SHA-512 / SHA-3** | 现代哈希标准 |
| **AES 加密** | 对称加密 |
| **HMAC-SHA256** | 消息认证 (目前只有 HMAC-MD5) |
| **Base32 编码** | 目前只有 Base64/Hex/URL |
| **文件二进制读写** | 当前 readFile/writeFile 是文本模式 |
| **文件 seek** | 随机访问读 |
| **递归目录遍历** | 目前 dirList 只列一层 |
| **临时文件/目录** | tmpfile/tmpnam |
| **信号处理** | SIGINT 等 |
| **locale/Unicode** | 目前 UTF-8 only |
| **duration 类型系统** | 目前直接用毫秒，缺纳秒/秒转换函数 (secToMs 存在但分散) |
| **XML/SVG/LS 解析** | HTML 解析 |
| **YAML/TOML** | 配置文件格式 |
| **MIME/邮件** | 邮件/附件编码 |
| **日志系统** | spdlog 式结构化日志 |

---

### 📊 覆盖率总评

| 大类 | 覆盖度 | 评级 |
|------|--------|------|
| 容器 | 95% | A |
| 算法 | 85% | B+ |
| 数学 | 90% | A- |
| 字符串 | 90% | A- |
| I/O 与文件系统 | 75% | B |
| 线程与并发 | 85% | B+ |
| 时间 | 80% | B |
| 网络 | 75% | B |
| 加密与编码 | 60% | C+ |
| 序列化 | 85% | B+ |
| 类型工具 | 75% | B |
| **总体** | **~82%** | **B+** |

---

### 🎯 建议优先实施（3 项 P0 + 6 项 P1）

1. **`orderedMap` 系列** — mapNew/Insert/Find/LowerBound 等（用 std::map 包装）
2. **数学特殊函数** — tgamma/lgamma/erf/erfc（4个函数，引用 cmath 即可）
3. **`stableSort`** + 缺失算法 — stableSort, partition, anyOf, allOf, noneOf, accumulate, findIf, generate
4. **`span` 全功能** — subspan, first, last, 迭代器支持
5. **`charconv` 浮点** — floatToStr/strToFloat（精确往返转换）
6. **`result` 单子操作** — map/flatMap/orElse（提升函数式体验）
7. **`tuple` 完善** — tupleTie, tupleSwap（引用绑定）
8. **文件二进制读写** — readFileBinary/writeFileBinary
9. **HMAC-SHA256** — 标准消息认证
