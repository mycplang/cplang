# CP语言 Bug 跟踪

## P0 - 数据库模块编译错误
**状态**: 待修复
**文件**: `src/stdlib/stdlib_db.cpp`
**影响**: test_mysql.cp 和 test_pg.cp 失败 (2/24 回归测试)

### 问题
`stdlib_db.cpp` 在本次会话中被 PowerShell 批量替换操作破坏了换行符，导致：
1. 多处注释和代码合并到同一行
2. 字符串文字被截断（C2001: 常量中有换行符）
3. `MysqlField` struct 定义在注释中

### 修复步骤
1. 从版本控制恢复 `stdlib_db.cpp` 原始文件
2. 重新应用命名修复：
   - 成员变量名加 `mysql_` 前缀（`init` → `mysql_init` 等, 共 21 个）
   - 所有 `api.xxx` 引用加 `mysql_` 前缀（约 50 处）
   - PG 部分：成员加 `PQ` 前缀（`connectdb` → `PQconnectdb` 等, 共 12 个）
3. 验证 `L` 宏参数与成员变量名匹配
4. 在 `stdlib.cpp` 中取消注释 `#include "stdlib_db.cpp"`
5. 移除空 stub 函数
6. 构建验证

### 数据库连接信息
> ⚠️ **安全提示**: 数据库凭证已从版本控制中移除。
> 本地开发请使用环境变量或 `.env` 文件配置：
> - `CP_DB_HOST` — 数据库服务器地址
> - `CP_MYSQL_USER` / `CP_MYSQL_PASS` / `CP_MYSQL_DB` — MySQL 连接信息（端口 3306）
> - `CP_PG_USER` / `CP_PG_PASS` / `CP_PG_DB` — PostgreSQL 连接信息（端口 5432）

### 相关测试文件
- `tests/cp/test_mysql.cp` - MySQL 集成测试（使用中文函数名如 MySQL连接、MySQL查询）
- `tests/cp/test_pg.cp` - PostgreSQL 集成测试（使用中文函数名如 PG连接、PG查询）

---

## P2 - JIT writef 函数未实现
**状态**: 影响线程 A，待解决
**文件**: `src/jit/jit_runtime.cpp`, `src/codegen/llvm_codegen.cpp`

### 问题
writef（格式化写文件）函数在 JIT 模式下未实现。

---

## P2 - AOT 数组计算偏差
**状态**: 预存问题
**文件**: `src/codegen/llvm_codegen.cpp`

### 问题
`-a` (AOT) 模式编译的 hello.cp 运行时数组计算结果有偏差（如平均分显示不正确），但不崩溃。

---

## P3 - REPL 崩溃
**状态**: 待修复
**触发**: `cplang.exe -r`

### 问题
`VM::current()` 在 REPL 模式返回 null，导致 0xC0000005 崩溃。

---

## 本次会话修改的文件清单

### 修改（需保留）
| 文件 | 修改内容 |
|------|----------|
| `src/codegen/llvm_codegen.cpp` | AOT/JIT 桥接函数选择; 中文函数映射 |
| `src/jit/jit_runtime.cpp` | extractString 解码器; g_jitVM; jit_len/jit_toString |
| `src/jit/jit_dispatch.cpp` | jit_setVM() 注册 |
| `src/jit/orc_jit.cpp` | jit_len/jit_toString 符号注册 |
| `include/jit/jit_runtime.hpp` | extractString/g_jitVM 声明 |
| `src/optimizer/escape_analyzer.cpp` | VERBOSE→if 守卫 |
| `src/optimizer/function_inliner.cpp` | VERBOSE→if 守卫 |
| `src/optimizer/loop_unroller.cpp` | VERBOSE→if 守卫 |
| `src/optimizer/tail_recursion_optimizer.cpp` | VERBOSE→if 守卫 |
| `src/optimizer/optimizer.cpp` | VERBOSE→if 守卫 |
| `src/codegen/bytecode_optimizer.cpp` | VERBOSE→if 守卫 |
| `src/jit/jit_compiler.cpp` | VERBOSE→if 守卫 |
| `src/stdlib/stdlib_imgui.cpp` | 移除重复 NO_FONT_AWESOME |

### 清理
- `escape_analyzer.cpp`: 移除重复 `#include "core/verbose.hpp"`

### 被破坏（需恢复）
- `src/stdlib/stdlib_db.cpp` — PowerShell 批量替换破坏

### 已恢复安全状态
- `src/stdlib/stdlib.cpp` — 数据库模块已注释，使用空 stub

---

## 测试结果

| 模式 | 目标 | 结果 |
|------|------|------|
| `-c` | hello.cp | ✅ 完美（7 行输出正确） |
| `-j` | hello.cp | ✅ 完美（7 行输出正确） |
| `-a` | hello.cp | ✅ 不崩溃（数组计算偏差，预存） |
| `-c` | 回归测试 | ✅ 22/24（test_mysql.cp, test_pg.cp 因数据库模块禁用失败） |
| `-r` | REPL | ❌ 崩溃 (VM::current 为 null) |

---

## JIT 修复总结

### 问题 1: NaN-boxed 字符串输出 `<object>`
- **根因**: `decodeNanBoxedString` 把 vtable 指针当作 typeTag 检查，误判
- **修复**: `extractString()` 通过 `(first8 >> 40) == 0x7F` 检测 vtable 区分 VM 对象和 LLVM char* 常量

### 问题 2: `jit_call_native` 崩溃
- **根因**: `VM::current()` 的 thread_local 变量在 JIT 调用链中为 null
- **修复**: 全局 `g_jitVM` + `jit_setVM()`，JIT 入口前设置

### 问题 3: 中文标准库函数在 JIT 模式下不可用
- **修复**: `长度` → `jit_len`, `转字符串` → `jit_toString` 的 sanitized name 映射
- `jit_len` 支持字符串和表的长度计算
- `jit_toString` 支持整数转字符串

### 问题 4: AOT LNK2019 未解析符号
- **根因**: `skipNativeCallRemoval_` 标志未用于选择桥接函数
- **修复**: AOT 用 `aot_call_native`，JIT 用 `jit_call_native`

### 问题 5: VERBOSE 宏误用
- **根因**: `VERBOSE(x)` 只接受单语句，后续代码无条件执行
- **修复**: 8 个文件改用 `if (!verboseEnabled()) return;`
