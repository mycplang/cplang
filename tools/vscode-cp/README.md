# CP Language Support — VSCode 扩展

## 功能

- 🎨 **语法高亮** — 完整支持中文关键字和英文别名
- 🔍 **代码补全** — 关键字、内置函数、变量、函数名
- 🧭 **跳转定义** — 函数定义位置跳转
- 💡 **悬停提示** — 关键字和函数说明
- ⚠️ **实时诊断** — 编译错误即时显示

## 安装

1. 在 VSCode 中按 `Ctrl+Shift+P`
2. 选择 `Extensions: Install from VSIX...`
3. 选择 `cp-language-0.3.0.vsix`

或手动复制到 `~/.vscode/extensions/cp-language/`

## 构建

```bash
# 需要 Node.js + vsce
npm install -g @vscode/vsce
cd tools/vscode-cp
vsce package
# 生成 cp-language-0.3.0.vsix
```

## 开发

- `syntaxes/cp.tmLanguage.json` — TextMate 语法规则（语法高亮）
- `snippets.json` — 代码片段
- `extension.js` — LSP 客户端（连接 cplsp.py）
- `language-configuration.json` — 语言配置
- `../cplsp.py` — 语言服务器（代码补全、诊断）