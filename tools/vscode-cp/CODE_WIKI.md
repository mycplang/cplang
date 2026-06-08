# CP Language VSCode 扩展 - Code Wiki

## 目录

1. [项目概述](#项目概述)
2. [项目结构](#项目结构)
3. [核心模块说明](#核心模块说明)
4. [关键类与函数](#关键类与函数)
5. [配置与依赖](#配置与依赖)
6. [运行与开发](#运行与开发)
7. [架构设计](#架构设计)
8. [贡献指南](#贡献指南)

---

## 项目概述

### 项目简介

**CP Language Support** 是为中文编程语言 CP 提供完整 IDE 支持的 VSCode 扩展。该扩展提供了语法高亮、代码补全、跳转定义、引用查找、重命名、格式化、悬停提示、语义诊断以及调试运行等丰富功能。

- **版本**: 0.4.0
- **发布者**: cplang
- **许可证**: MIT
- **仓库**: https://github.com/cplang/cplang

### 主要特性

- ✅ 语法高亮（支持中文关键字和英文别名）
- ✅ 智能代码补全
- ✅ 跳转到定义
- ✅ 查找引用
- ✅ 重命名重构
- ✅ 代码格式化
- ✅ 悬停提示
- ✅ 实时诊断
- ✅ 文档符号浏览
- ✅ 签名帮助
- ✅ 调试支持
- ✅ 快捷运行与编译

---

## 项目结构

```
vscode-cp/
├── icons/                     # 语言图标
│   ├── cp-dark.svg           # 深色主题图标
│   └── cp-light.svg          # 浅色主题图标
├── syntaxes/                  # 语法高亮定义
│   └── cp.tmLanguage.json    # TextMate 语法规则
├── extension.js               # VSCode 扩展主入口
├── cplsp.js                   # CP 语言服务器 (LSP)
├── cp-debug.js                # 调试适配器 (DAP)
├── language-configuration.json # 语言配置文件
├── snippets.json              # 代码片段
├── package.json               # 扩展配置与依赖
└── package-lock.json          # 依赖锁文件
```

### 核心文件说明

| 文件 | 描述 |
|------|------|
| [package.json](file:///c:/CPLANG/tools/vscode-cp/package.json) | 扩展配置清单，定义命令、语法、调试器等 |
| [extension.js](file:///c:/CPLANG/tools/vscode-cp/extension.js) | VSCode 扩展激活入口，注册命令和 LSP 客户端 |
| [cplsp.js](file:///c:/CPLANG/tools/vscode-cp/cplsp.js) | 语言服务器，实现 LSP 协议的各种功能 |
| [cp-debug.js](file:///c:/CPLANG/tools/vscode-cp/cp-debug.js) | 调试适配器，实现 DAP 协议 |
| [language-configuration.json](file:///c:/CPLANG/tools/vscode-cp/language-configuration.json) | 语言配置（注释、括号、缩进等） |
| [syntaxes/cp.tmLanguage.json](file:///c:/CPLANG/tools/vscode-cp/syntaxes/cp.tmLanguage.json) | TextMate 语法高亮规则 |
| [snippets.json](file:///c:/CPLANG/tools/vscode-cp/snippets.json) | 预定义的代码片段 |

---

## 核心模块说明

### 1. 扩展主入口 (extension.js)

**职责**:
- 激活 VSCode 扩展
- 启动语言服务器进程
- 注册编辑器命令
- 管理语言客户端生命周期

**关键功能**:

```javascript
// 获取 CPLANG_HOME 路径
function getCplangHome()

// 获取编译器路径
function getCplangCompiler()

// 扩展激活函数
function activate(context)

// 获取活动的 CP 文件
function getActiveCpFile()
```

**注册的命令**:

| 命令 ID | 快捷键 | 描述 |
|---------|--------|------|
| `cp.run` | Ctrl+Shift+R | 运行当前 CP 文件 |
| `cp.build` | Ctrl+F7 | 编译检查当前 CP 文件 |
| `cp.info` | - | 显示环境信息 |

---

### 2. 语言服务器 (cplsp.js)

**职责**:
- 实现 LSP (Language Server Protocol)
- 提供代码补全、诊断、跳转等功能
- 与编译器集成提供语义分析

**支持的 LSP 能力**:

| 能力 | 说明 |
|------|------|
| textDocumentSync | 文档打开/关闭/变更同步 |
| completionProvider | 代码补全（触发器：.） |
| hoverProvider | 悬停提示 |
| definitionProvider | 跳转到定义 |
| referencesProvider | 查找引用 |
| renameProvider | 重命名 |
| documentSymbolProvider | 文档符号 |
| signatureHelpProvider | 签名帮助（触发器：(） |
| documentFormattingProvider | 代码格式化 |

**核心数据结构**:

```javascript
// 关键字列表（含中英文）
const KEYWORDS = [...]

// 内置函数列表
const BUILTINS = [...]

// 悬停提示文档
const HOVER_DOCS = {}

// 打开的文档缓存
const docs = new Map()
```

**主要处理函数**:

- [send()](file:///c:/CPLANG/tools/vscode-cp/cplsp.js#L47-L49): 发送 LSP 响应
- [runDiagnostics()](file:///c:/CPLANG/tools/vscode-cp/cplsp.js#L52-L82): 运行编译器诊断
- [handle()](file:///c:/CPLANG/tools/vscode-cp/cplsp.js#L336-L539): 处理 LSP 请求
- [formatCode()](file:///c:/CPLANG/tools/vscode-cp/cplsp.js#L578-L597): 代码格式化

---

### 3. 调试适配器 (cp-debug.js)

**职责**:
- 实现 DAP (Debug Adapter Protocol)
- 启动和管理调试进程
- 处理断点、变量查看等调试操作

**主要类**:

```javascript
class CPDebugAdapter {
    // 调试适配器主类
    constructor()
    send(msg)
    sendEvent(event, body)
    sendResponse(request, body)
    sendErrorResponse(request, message)
    handleMessage(msg)
    doLaunch(msg)
    startProcess(config)
    parseDebugOutput(text)
    onData(data)
}
```

**支持的调试功能**:

- 启动程序 (launch)
- 输出重定向
- 断点设置（当前仅记录，不暂停）
- 线程信息
- 堆栈跟踪
- 变量查看
- 程序终止

---

### 4. 语言配置 (language-configuration.json)

**配置项**:

- **注释**: 单行 `//`，多行 `/* */`
- **括号**: `{ }`, `[ ]`, `( )`
- **自动闭合**: 括号和引号自动闭合
- **环绕**: 括号和引号环绕
- **缩进规则**: 基于关键字和大括号的自动缩进
- **单词模式**: 支持中文字符作为标识符

---

### 5. 语法高亮 (syntaxes/cp.tmLanguage.json)

**语法规则**:

| 规则名 | 匹配内容 |
|--------|----------|
| comments | 单行和多行注释 |
| strings | 双引号字符串及转义字符 |
| numbers | 十六进制、二进制、八进制、十进制、浮点数 |
| keywords | 中英文控制关键字 |
| types | 中英文类型名称 |
| builtins | 内置函数 |
| function-decl | 函数声明 |
| operators | 运算符 |

---

### 6. 代码片段 (snippets.json)

**预定义片段**:

| 片段名 | 前缀 | 描述 |
|--------|------|------|
| 函数定义 | 函数, function, fn | 定义新函数 |
| 异步函数 | 异步, async | 定义异步函数 |
| 变量声明 | 变量, var | 声明变量 |
| 常量声明 | 常量, const | 声明常量 |
| let 绑定 | 设, let | let 绑定 |
| 如果条件 | 如果, if | if 语句 |
| 如果否则 | ifelse | if-else 语句 |
| 当循环 | 当, while | while 循环 |
| 循环 | 循环, for | for 循环 |
| 遍历 | 遍历, foreach | 遍历循环 |
| 选择语句 | 选择, switch | switch 语句 |
| 匹配表达式 | 匹配, match | match 表达式 |
| 类声明 | 类, class | 定义类 |
| 结构体 | 结构体, struct | 定义结构体 |
| 枚举 | 枚举, enum | 定义枚举 |
| 接口 | 接口, interface | 定义接口 |
| 尝试捕获 | 尝试, try | try-catch |
| 抛出异常 | 抛出, throw | 抛出异常 |
| 推迟执行 | 推迟, defer | defer 语句 |
| 导入模块 | 导入, import | 导入模块 |
| 打印输出 | 打印, print | 打印输出 |
| 返回语句 | 返回, return | 返回语句 |
| 主函数 | main | main 函数 |

---

## 关键类与函数

### extension.js

#### [activate(context)](file:///c:/CPLANG/tools/vscode-cp/extension.js#L14-L103)

扩展激活函数，VSCode 启动时调用。

**参数**:
- `context`: VSCode 扩展上下文

**功能**:
1. 启动语言服务器进程
2. 创建并启动语言客户端
3. 注册三个命令
4. 注册清理资源的 dispose 回调

---

### cplsp.js

#### [runDiagnostics(uri, text)](file:///c:/CPLANG/tools/vscode-cp/cplsp.js#L52-L82)

运行编译器诊断并发送结果。

**参数**:
- `uri`: 文档 URI
- `text`: 文档内容

**流程**:
1. 将内容写入临时文件
2. 调用编译器进行检查
3. 解析编译器错误输出
4. 发送诊断结果
5. 清理临时文件

---

#### [handle(msg)](file:///c:/CPLANG/tools/vscode-cp/cplsp.js#L336-L539)

LSP 请求主处理函数。

**参数**:
- `msg`: LSP 消息对象

**处理的请求**:
- `initialize`: 初始化服务器能力
- `textDocument/didOpen`: 文档打开
- `textDocument/didChange`: 文档变更
- `textDocument/completion`: 代码补全
- `textDocument/hover`: 悬停提示
- `textDocument/definition`: 跳转定义
- `textDocument/references`: 查找引用
- `textDocument/rename`: 重命名
- `textDocument/documentSymbol`: 文档符号
- `textDocument/signatureHelp`: 签名帮助
- `textDocument/formatting`: 代码格式化

---

#### [formatCode(text)](file:///c:/CPLANG/tools/vscode-cp/cplsp.js#L578-L597)

内置代码格式化函数。

**参数**:
- `text`: 原始代码

**返回**:
- 格式化后的代码

**规则**:
- 使用 tab 缩进
- 遇到 `{` 增加缩进
- 遇到 `}` 减少缩进
- 保留空行

---

### cp-debug.js

#### [class CPDebugAdapter](file:///c:/CPLANG/tools/vscode-cp/cp-debug.js#L14-L276)

调试适配器主类。

**属性**:
- `process`: 子进程引用
- `breakpoints`: 断点映射
- `seq`: 消息序列计数器

**主要方法**:

| 方法 | 描述 |
|------|------|
| `send()` | 发送 DAP 消息 |
| `sendEvent()` | 发送事件 |
| `sendResponse()` | 发送响应 |
| `handleMessage()` | 处理请求 |
| `doLaunch()` | 处理启动请求 |
| `startProcess()` | 启动编译器进程 |

---

## 配置与依赖

### package.json 配置

#### 激活事件

```json
"activationEvents": [
    "onLanguage:cp",
    "onCommand:cp.run",
    "onCommand:cp.build",
    "onCommand:cp.info"
]
```

#### 贡献点

| 类型 | 内容 |
|------|------|
| languages | CP 语言注册（.cp 文件） |
| grammars | TextMate 语法高亮 |
| snippets | 代码片段 |
| commands | 三个编辑器命令 |
| keybindings | 快捷键绑定 |
| menus | 菜单栏和右键菜单 |
| debuggers | CP 调试器 |

#### 依赖

```json
"dependencies": {
    "vscode-languageclient": "^8.1.0"
}
```

### 环境变量

- **CPLANG_HOME**: CP 语言安装目录（默认：`~/cplang`）
- 编译器路径: `$CPLANG_HOME/build/cplang` (Windows: `cplang.exe`)

---

## 运行与开发

### 安装扩展

方式一：从 VSIX 安装
```bash
# 打包扩展
vsce package

# 在 VSCode 中:
# Ctrl+Shift+P → Extensions: Install from VSIX...
```

方式二：开发模式
```bash
# 在 vscode-cp 目录下
code .

# 按 F5 启动扩展开发主机
```

### 开发流程

1. 修改相关文件
2. 按 F5 启动扩展开发主机
3. 在新窗口测试功能
4. 查看调试控制台输出

### 编译打包

```bash
# 需要先安装 vsce
npm install -g @vscode/vsce

# 打包
vsce package

# 生成: cp-language-0.4.0.vsix
```

---

## 架构设计

### 整体架构图

```
┌─────────────────────────────────────────────────────────┐
│                     VSCode 编辑器                        │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │  语法高亮    │  │  编辑器命令  │  │  调试面板    │  │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘  │
└─────────┼─────────────────┼──────────────────┼──────────┘
          │                 │                  │
          ▼                 ▼                  ▼
┌─────────────────────────────────────────────────────────┐
│                  extension.js (LSP 客户端)               │
│  ┌───────────────────────────────────────────────────┐  │
│  │  LanguageClient (vscode-languageclient)           │  │
│  └───────────────────────────────────────────────────┘  │
└─────────────────┬───────────────────────────────────────┘
                  │
         ┌────────┴────────┐
         │                 │
         ▼                 ▼
┌─────────────────┐  ┌──────────────────┐
│   cplsp.js      │  │   cp-debug.js    │
│  (LSP 服务器)   │  │  (DAP 适配器)    │
└────────┬────────┘  └────────┬─────────┘
         │                    │
         └────────┬───────────┘
                  ▼
         ┌──────────────────┐
         │   CP 编译器      │
         │  (cplang.exe)   │
         └──────────────────┘
```

### 通信协议

#### LSP (Language Server Protocol)

扩展通过 stdio 与语言服务器通信，消息格式:
```
Content-Length: <length>\r\n\r\n
<JSON body>
```

#### DAP (Debug Adapter Protocol)

调试适配器使用相同的协议格式与 VSCode 通信。

### 工作流程

#### 1. 语法高亮流程

```
用户打开 .cp 文件
    ↓
VSCode 加载 cp.tmLanguage.json
    ↓
TextMate 引擎解析并高亮
```

#### 2. 诊断流程

```
文档变更
    ↓
extension.js 通知 cplsp.js
    ↓
cplsp.js 保存临时文件
    ↓
调用编译器检查
    ↓
解析错误输出
    ↓
发送诊断到 VSCode
    ↓
编辑器显示错误波浪线
```

#### 3. 代码补全流程

```
用户输入触发字符 (如 .)
    ↓
VSCode 发送 completion 请求
    ↓
cplsp.js 查询 KEYWORDS + BUILTINS
    ↓
提取文档内定义的标识符
    ↓
返回补全列表
    ↓
用户选择插入
```

---

## 贡献指南

### 添加新的代码片段

编辑 [snippets.json](file:///c:/CPLANG/tools/vscode-cp/snippets.json)，遵循格式:

```json
"片段名称": {
    "prefix": ["触发词1", "触发词2"],
    "body": ["代码行1", "代码行2"],
    "description": "描述"
}
```

### 扩展语法高亮

编辑 [syntaxes/cp.tmLanguage.json](file:///c:/CPLANG/tools/vscode-cp/syntaxes/cp.tmLanguage.json)，添加新的匹配规则。

### 添加内置函数补全

编辑 [cplsp.js](file:///c:/CPLANG/tools/vscode-cp/cplsp.js) 中的 `BUILTINS` 数组。

### 测试变更

1. 在 VSCode 中打开项目
2. 按 F5 启动扩展开发主机
3. 在新窗口创建 test.cp 文件
4. 测试功能

---

## 附录

### 支持的关键字（中英文对照）

| 中文 | 英文 | 描述 |
|------|------|------|
| 函数 | function, fn | 函数定义 |
| 变量 | var | 变量声明 |
| 常量 | const | 常量声明 |
| 设 | let | let 绑定 |
| 可变 | mutable | 可变变量 |
| 返回 | return | 返回语句 |
| 如果 | if | 条件判断 |
| 否则 | else | 否则分支 |
| 当 | while | while 循环 |
| 循环 | for | for 循环 |
| 遍历 | foreach | 遍历循环 |
| 为 | do | do-while |
| 跳出 | break | 跳出循环 |
| 继续 | continue | 继续循环 |
| 选择 | switch | switch 语句 |
| 情况 | case | case 分支 |
| 其他 | default | default 分支 |
| 匹配 | match | match 表达式 |
| 尝试 | try | try 块 |
| 捕获 | catch | catch 块 |
| 抛出 | throw | 抛出异常 |
| 最终 | finally | finally 块 |
| 推迟 | defer | defer 语句 |
| 类 | class | 类定义 |
| 结构体 | struct | 结构体 |
| 枚举 | enum | 枚举 |
| 接口 | interface | 接口 |
| 继承 | extends | 继承 |
| 这个 | this | 当前实例 |
| 新建 | new | 实例化 |
| 公有 | public | 公开 |
| 私有 | private | 私有 |
| 保护 | protected | 保护 |
| 静态 | static | 静态 |
| 虚拟 | virtual | 虚方法 |
| 重写 | override | 重写 |
| 抽象 | abstract | 抽象 |
| 导入 | import | 导入 |
| 包名 | package | 包声明 |
| 等待 | await | await |
| 异步 | async | async |
| 真 | true | true |
| 假 | false | false |
| 空 | null, nil | null |

---

### 内置函数分类

#### 数学函数
- `绝对值`, `平方根`, `幂`, `向下取整`, `向上取整`, `四舍五入`
- `正弦`, `余弦`, `正切`, `反正弦`, `反余弦`, `反正切`
- `圆周率`, `自然常数`, `随机值`, `阶乘`, `均值`, `中位数`, `标准差`

#### 输入输出
- `打印`, `println`, `输入`, `读取文件`, `写入文件`, `文件存在`

#### 字符串操作
- `长度`, `子串`, `连接`, `查找`, `替换`, `分割`, `修剪`, `小写`, `大写`, `包含`, `格式化`

#### 类型转换
- `转字符串`, `转整数`, `转浮点`, `JSON解析`, `转JSON`

#### 数组操作
- `追加`, `弹出`, `插入`, `删除`, `切片`, `排序`, `反转`, `映射`, `过滤`, `累积`, `去重`, `展平`, `打包`, `头出`, `头插`

#### 表操作
- `表创建`, `表取`, `表设`, `表长`, `表键`, `表值`, `表有`, `表删`, `表清空`, `表合并`, `表转数组`

#### 类型检查
- `类型`, `是整数`, `是字符串`, `是数组`, `是函数`, `是空`

#### 时间与系统
- `时间戳`, `延时`, `计时器`, `平台`, `程序退出`, `环境变量`, `当前目录`, `进程ID`

#### 并发
- `通道创建`, `通道发送`, `通道接收`, `互斥创建`, `互斥加锁`, `互斥解锁`, `线程创建`, `线程等待`, `异步执行`

#### 网络
- `HTTP获取`, `HTTP提交`, `TCP连接`, `TCP发送`, `TCP接收`

#### 加密
- `MD5`, `SHA256`, `Base64编码`, `Base64解码`, `UUID`

#### 数据库
- `数据库打开`, `数据库查询`, `数据库执行`, `数据库关闭`, `Redis连接`, `Redis获取`, `Redis设置`

#### 图形
- `初始化窗口`, `开始绘图`, `结束绘图`, `绘制文本`, `绘制矩形`

---

*文档版本: 1.0*  
*最后更新: 2026-06-07*
