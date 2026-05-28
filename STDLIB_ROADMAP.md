# CP语言标准库路线图

## 当前状态（2026-05-06）

| 指标 | 数值 |
|------|------|
| 回归测试 | 97/97 ✅ |
| 函数总数 | ~810 |
| 中文别名 | ~775 |
| VM TAG | 32 个 |
| stdlib.cpp | ~7350 行（含 #include 的独立 .cpp 文件） |
| 独立实现文件 | 15 个 |
| Git commits | 8+ |
| 构建系统 | MSVC cl.exe, /std:c++17 /EHsc |

### 四大语言特性 ✅
| 特性 | 状态 |
|------|------|
| 异常处理 try/catch/throw | ✅ OP_THROW/PUSHANDLER/POPHANDLER |
| 函数对象 / λ 谓词 | ✅ callFunction/callDepth_ |
| 迭代器协议（8种容器） | ✅ 5核心+6扩展 |
| 多线程（10模块 39函数） | ✅ thread/mutex/cond/semaphore/atomic/barrier/future/channel/rwlock/tls |

---

## 模块清单（全部完成）

### 容器全覆盖（26 模块）✅
数学、数学常量、数值极限、复数、对组、字符串、字符串搜索、数组、表、位运算、位集、栈、队列、双端队列、优先队列、链表、单向链表、集合、多重集合、映射、多重映射、无序集合、无序多重集合、无序映射、无序多重映射、堆操作

### 算法（2 模块）✅
算法基础、算法扩展、更多算法（stableSort/partition/anyOf/allOf等18个）、算法补全

### 字符串工具（12 模块）✅
正则、编码(Base64/Hex/URL)、格式化、SHA256/HMAC-SHA256/MD5/HMAC-MD5/CRC32、SHA-512/HMAC-SHA256/Base32、编辑距离、UUID v4、CSV读/写、颜色转换(RGB/Hex/HSL)、Glob匹配、大小写、工具集

### 并发（10 模块）✅
Thread/Mutex/Condition/Semaphore/Atomic/Barrier/Future/Channel/RWLock/TLS

### I/O 与系统（8 模块）✅
基础IO、文件操作(含seek/tell/eof/流式读/目录遍历/临时文件)、时间/日期时间、系统、进程(含管道)、JSON、环境变量、网络(HTTP/TCP/UDP/DNS)

### 加密与压缩 ✅
AES-128/256-CBC、randomBytes(BCryptGenRandom)、compress/gzip(zlib via miniz)、fileWatcher(RLE)

### 字符集 ✅
UTF-8 ↔ GBK/Big5/Shift-JIS、编码检测、有效性验证、宽字符转换

### 数据库 ✅
SQLite 绑定（sqlite3.c 集成）

### WebSocket ✅
客户端连接、握手、收发

### 新批次 P3 ✅
CSV写入、增强日志(logDebug/Info/Warn/Error + 级别过滤 + 文件输出)、不区分大小写比较

### 其他 ✅
统计、矩阵/向量、随机数、工具集、类型检查、Optional、Variant、Any、Tuple、Result、Functional、Span、Charconv、Memory、Box/RC、SourceLocation

---

## 剩余缺口（低优先级）

| 项目 | 说明 |
|------|------|
| YAML 解析 | 可用 JSON 替代，非必需 |
| TOML 解析 | 同上 |
| 信号处理 | Windows 平台复杂度高 |
| REPL 交互 | 方便调试 |
| 包管理器 | 大工程 |

## 已知问题

| 问题 | 严重度 | 状态 |
|------|--------|------|
| ~~MD5 输出错误~~ | ~~中~~ | ✅ 已修复（重复注册覆盖） |
| ~~OP_IMPORT 未 Slot 化~~ | ~~低~~ | ✅ 已用常量池查找修复 |
| C4244 警告 (Int64→Int32) | 低 | ⚠️ 剩余少数位置，需逐步改 |

---

## 下一步方向建议

1. **性能优化** — JIT/LLVM、常量折叠增强、逃逸分析完善
2. **工具链** — REPL、调试器增强、Language Server Protocol
3. **生态** — 包管理器、第三方库绑定
4. **语言特性** — 模式匹配、类型注解、async/await
5. **实战项目** — 用 CP 写实际应用暴露痛点
