# CPLANG 中文编程语言 - 项目总览
## 📌 基本信息
- **版本**: v0.1.0-beta
- **定位**: 面向少儿编程入门的中文高级编程语言
- **许可证**: MIT
- **支持平台**: Windows / Linux
## 📂 目录结构说明
| 目录 | 用途 |
|------|------|
| `.github.bak` | GitHub Action配置备份 |
| `.vscode` | VSCode开发配置 |
| `artifacts` | 历史滚动文档（已归档） |
| `assets` | 静态资源（Logo、图标等） |
| `benchmarks` | 性能基准测试用例 |
| `build` | 编译构建临时目录 |
| `build_llvm` | LLVM相关构建目录 |
| `cmake` | CMake构建配置脚本 |
| `C_` | C语言兼容层相关代码 |
| `data` | 运行时数据目录 |
| `data_admin` | 管理后台数据目录 |
| `docs` | 项目文档（快速入门、功能参考等） |
| `examples` | 示例代码（教学示例、游戏Demo等） |
| `games` | 游戏示例项目 |
| `include` | 头文件目录（所有模块的头文件定义） |
| `lib` | 编译生成的库文件目录 |
| `memory` | 内存管理相关代码 |
| `packages` | 内置CPM包目录 |
| `registry` | 模块注册表相关代码 |
| `scripts` | 构建、安装、测试相关脚本 |
| `src` | 核心源码实现 |
| `tests` | 单元测试和集成测试用例 |
| `third_party` | 第三方依赖库（Raylib、SQLite、ImGui等） |
| `tools` | 辅助开发工具 |
| `web` | Web端相关代码 |
| `workspace_artifacts` | 工作区构建产物 |
| `www` | Web前端静态资源 |
| `_test_fs_dir` | 文件系统测试临时目录 |
| `_tools` | 内部工具脚本 |
## ✅ 当前功能状态
### 核心编译管线
✅ 词法分析器（支持UTF-8中文关键字）  
✅ 语法分析器（生成AST，已拆分为4个文件）  
✅ 语义分析器（类型检查、作用域分析）  
✅ 字节码生成器（16字节指令格式）  
✅ LLVM IR生成器  
✅ 全套优化器（常数折叠、死代码消除、函数内联、循环展开、尾递归优化、逃逸分析）  

### 双引擎架构
```
源码 → Compiler(共享前端: Lex→Parse→Semantic) → AST + 字节码
         ├── DevEngine(VM+JIT)  ── 开发/学习/脚本
         │    ├ DevMode::Bytecode → 字节码解释执行
         │    ├ DevMode::Hotspot  → 字节码+热点JIT(95x)
         │    └ DevMode::JitAll   → 全量JIT预编译
         └── AOTEngine(LLVM IR→.obj→.exe) ── 部署/发布
```
✅ 栈式字节码虚拟机  
✅ LLVM 18 ORC JIT编译器（热点函数加速最高95x）  
✅ 三色标记GC（支持循环引用检测）  
✅ JIT 字段已从 VMFunction 抽到外部 JITRegistry 映射表  
### 特性
✅ 中文原生语法（关键字、变量名、函数名全中文支持）  
✅ 隐式变量声明（无需var/let）  
✅ 渐进式类型系统（动态类型+可选类型标注）  
✅ 三色标记GC（自动内存管理）  
✅ 交互式REPL环境  
✅ 模块系统（本地导入/远程导入）  
### 标准库（373+原生函数）
✅ 基础：数学、字符串、数组、表、IO  
✅ 扩展：时间、系统、文件系统、网络、JSON、正则、加密、容器、算法、并发  
✅ 图形：Raylib 2D/3D、ImGui UI  
✅ 数据库：SQLite、MySQL、Redis  
## ⚡ 核心性能指标
| 场景 | 字节码 | JIT编译 | 加速比 |
|------|--------|---------|--------|
| 1亿次循环 | 超时 | 0.35s | 100x+ |
| 1000万次函数调用 | 5.4s | 0.057s | 95x |
| 类型化函数执行 | 6.8s | 0.10s | 65x |
## 🚀 快速使用
### Windows构建
```bash
# MSVC版本构建
build_msvc.bat
# 发布版本构建
build_release.bat
# 带Raylib图形支持构建
build_raylib.bat
```
### 运行方式
```bash
# 字节码执行
cplang -c 程序.cp
# 热点JIT加速
cplang -c --hotspot 程序.cp
# 全量JIT编译
cplang -j 程序.cp
# 交互式REPL
cplang -r
```
## 📝 当前测试状态
- 总测试用例：217
- 通过：204（94.0%）
- 11个教学示例全部通过
## ⚠️ 已知问题
1. printVal不处理Bool类型（输出`<1>`而非`真/假`）
2. 仅支持中文关键字，无英文别名
3. 部分调试输出待清理
## 📌 后续优先级TODO
1. 修复剩余13个测试用例
2. 完善IDE插件、调试器等工具链
3. 补充高级特性文档
4. 优化错误提示体验
5. 建设第三方包生态
