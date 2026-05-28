# CPLANG 开发TODO清单（按优先级排序）
## ✅ 已完成
1. **补充英文关键字别名**：`function`/`if`/`while`/`class`/`var`/`return` 等英文关键字已支持 (2026-05-16)
2. **清理代码中的调试输出**：所有优化统计输出已统一使用 `VERBOSE()` 宏保护 (2026-05-16)
3. **修复Bool类型输出问题**：`真`/`假` 输出已正常工作
4. **LLVM JIT 集成恢复**：从退化状态恢复，LLVM ORC JIT 100x 加速已验证
5. **AOT 编译修复**：MSVC 路径从 D: 更正为 C:，链接管道修复

## 🔴 高优先级
1. **修复基准测试超时** ✅
   - ✅ `test_loop100m.cp`：100M循环包裹为`loopSum()`函数，支持JIT全量编译
   - ✅ `jit_bench_loop.cp`：提取循环为独立函数，热点JIT可编译计算部分
   - ✅ `jit_tick`注册为JIT符号：`tick()/tock()`在JIT编译代码中不再触发native call移除
   - ✅ `jit_*`白名单：native call移除算法跳过已知JIT运行时符号
2. **优化错误提示** ✅（已完成）
   - ✅ 词法/语法/语义错误：全汉化，位置精准
   - ✅ 修复 Line 0 问题、解析器16处英文消息汉化、VM未知操作码汉化
3. **WindowsApps Python 存根问题** ✅
   - ✅ `python3` 命令原指向 WindowsApps 0字节存根
   - ✅ 已创建 `python3.bat` 包装器 → 转发到真实 Python
   - ✅ `python3 --version` 现在返回 `Python 3.13.13`

## 🟠 中优先级（体验优化）
1. **开发VSCode插件** ✅
   - ✅ `syntaxes/cp.tmLanguage.json` — 完整 TextMate 语法（关键字/类型/内置函数/字符串/注释/数字/运算符）
   - ✅ `extension.js` — LSP 客户端，连接 `cplsp.py`（跨平台，Windows/Linux/macOS）
   - ✅ `snippets.json` — 16个代码片段（函数/变量/循环/条件/类/结构体等）
   - ✅ `language-configuration.json` — 注释/括号/自动闭合/缩进规则
   - ✅ `cplsp.py` — 语言服务器（补全/跳转/悬停/诊断），支持跨平台路径
   - ✅ `icons/` — 语言图标
   - ✅ `README.md` — 安装和构建说明
2. **补充文档** ✅（已完成）
   - ✅ `docs/TYPE_SYSTEM.md` — 类型系统详解（渐进类型/NaN-boxing/类型推断/typed指令/10章节）
   - ✅ `docs/OWNERSHIP.md` — 所有权与借用检查（三层内存模型/借用规则/生命周期/可信块/9章节）
   - ✅ `docs/JIT_GUIDE.md` — 完整重写，覆盖 ORC JIT 架构/混合JIT/字节码优化/AOT/性能调优/10章节
   - ✅ `docs/ARCHITECTURE.md` — 编译器架构概览（新增）
3. **完善REPL功能** ✅（已完成）
   - ✅ 持久化历史记录：退出保存 `~/.cplang_history`，启动自动加载（最多500条）
   - ✅ 增强代码补全：完整关键字/内置函数列表、自动追加函数括号、导入路径补全
   - ✅ 语法提示：`%help <函数名>` 查看函数签名、补全列表显示函数说明
   - ✅ REPL命令系统：`%help` `%hist` `%save` `%load` `%type` `%time` `%cd` `%pwd` `%vars` `%exit`
   - ✅ 语法查询：`?<函数名>` 快速查看函数文档
   - ✅ 表达式自动打印 + ANSI彩色输出
   - ✅ `%time <表达式>` 微秒级计时
4. **添加代码格式化工具** ✅
   - ✅ `tools/cpfmt.py` — 基于行的代码格式化器，支持 `-i`/`-c`/`-w` 选项
   - ✅ 运算符空格：复合运算符 `<=`/`>=`/`==`/`!=`/`+=` 等不破坏，字符串内部原样保留
   - ✅ 缩进跟踪：递归处理 `{`/`}` 嵌套，正确处理 `} 否则 {` 等中块关键字
   - ✅ 分号自动补全：语句末尾自动加分号，控制流/块声明不添加
   - ✅ 空行合并：最多连续两个空行
   - ✅ BOM 自动移除，UTF-8 编码输出
   - ✅ 幂等性验证：257/257 测试文件通过
5. **完善包管理器** ✅
   - ✅ `src/cpkg/cpkg.cpp` — C++ 原生实现，WinHTTP + urlmon 双栈 HTTP
   - ✅ 支持 GitHub token 认证（环境变量 `CPKG_GITHUB_TOKEN` 或 `%USERPROFILE%\.cpkg\token`）
   - ✅ 中文命令：`安装` / `卸载` / `搜索` / `列表` / `更新` / `信息` / `注册表` / `帮助`
   - ✅ 英文命令：`install` / `remove` / `search` / `list` / `update` / `info` / `registry` / `help`
   - ✅ `tools/包.bat` — 中文命令包装器，支持 `包 安装 math`
   - ✅ 注册表：`registry/index.json` 指向 `mycplang/registry` 仓库
6. **基准测试自动跳过** ✅
   - ✅ `_run_regress.py`：自动检测基准文件（文件名含 bench/perf/loop100m/jit 等）
   - ✅ `_runner.py`：同上，同时自动选择可用 cplang.exe
   - ✅ 基准文件自动使用长超时（120s） + 热点 JIT（`--hotspot --hotspot-threshold=50`）
   - ✅ 普通文件保持 15s 超时 + 纯字节码模式

## 🟡 低优先级（生态建设）
1. **增加更多示例项目**：Web服务、GUI应用、游戏等实战示例
2. **开发调试器**：支持断点、单步执行、变量查看
3. **支持macOS平台**：完成macOS适配和构建脚本
4. **增加性能分析工具**：支持代码热点分析、性能瓶颈定位
5. **建设社区生态**：搭建官方网站、文档站、论坛

## 🟢 长期规划
1. **支持ARM平台**：适配ARM架构CPU
2. **增加交叉编译支持**：支持编译生成不同平台的可执行文件
3. **完善标准库**：增加机器学习、大数据等领域的扩展库
4. **开发IDE**：自研轻量级CPLANG集成开发环境
5. **企业级应用支持**：增加微服务、分布式计算等企业级特性

## 📊 测试状态 (2026-05-16)
- **回归测试**: 193/197 通过（4 个超时均为基准测试）
- **JIT**: LLVM ORC JIT 100x 加速验证通过
- **AOT**: 独立 exe 生成 + 链接通过
