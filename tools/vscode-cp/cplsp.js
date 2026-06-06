/**
 * CP Language Server (LSP)
 * stdio JSON-RPC 2.0, 零外部依赖
 *
 * 功能:
 *   - textDocument/didOpen → 编译诊断
 *   - textDocument/didChange → 增量诊断
 *   - textDocument/completion → 代码补全
 *   - textDocument/hover → 悬停提示
 *   - textDocument/definition → 跳转到定义
 */

const fs = require('fs');
const path = require('path');
const { spawn } = require('child_process');

// ─── 配置 ───────────────────────────────────────────────
const CPLANG_HOME = process.env.CPLANG_HOME ||
    path.join(require('os').homedir(), 'cplang');
const COMPILER = path.join(CPLANG_HOME, 'build',
    process.platform === 'win32' ? 'cplang.exe' : 'cplang');

// 文档缓存: uri → { text, version }
const docs = new Map();

// ─── LSP 消息收发 ───────────────────────────────────────
let buf = '';
process.stdin.on('data', chunk => {
    buf += chunk.toString();
    while (true) {
        const m = buf.match(/^Content-Length: (\d+)\r\n\r\n/);
        if (!m) break;
        const len = parseInt(m[1], 10);
        const hdr = m[0];
        if (buf.length < hdr.length + len) break;
        const body = buf.slice(hdr.length, hdr.length + len);
        buf = buf.slice(hdr.length + len);
        try { handle(JSON.parse(body)); } catch (e) { /* ignore */ }
    }
});

function send(msg) {
    const body = JSON.stringify(msg);
    process.stdout.write(`Content-Length: ${Buffer.byteLength(body)}\r\n\r\n${body}`);
}

// ─── 编译器交互 ─────────────────────────────────────────
function runDiagnostics(uri, text) {
    const filePath = uri.startsWith('file://') ? decodeURI(uri.slice(7)) : uri;
    const tmpFile = path.join(require('os').tmpdir(), `_cplsp_${Date.now()}.cp`);

    fs.writeFileSync(tmpFile, text, 'utf-8');

    const proc = spawn(COMPILER, ['-p', tmpFile], { cwd: CPLANG_HOME });
    let stderr = '';
    proc.stderr.on('data', d => stderr += d.toString());
    proc.on('close', code => {
        const diagnostics = [];
        if (code !== 0 || stderr) {
            // 解析编译器错误: 第N行第M列: 消息 (当前: 'xxx')
            const lineRe = /第(\d+)行第(\d+)列:\s*(.*?)(?:\s*\(当前:\s*'[^']*'\))?/g;
            let m;
            while ((m = lineRe.exec(stderr)) !== null) {
                diagnostics.push({
                    range: {
                        start: { line: parseInt(m[1]) - 1, character: parseInt(m[2]) - 1 },
                        end:   { line: parseInt(m[1]) - 1, character: parseInt(m[2]) + 5 }
                    },
                    severity: 1, // Error
                    source: 'cplang',
                    message: m[3].trim()
                });
            }
        }
        // 也检测 stdout 中的错误
        if (stderr.includes('语法分析成功') || stderr.includes('success')) {
            // 无错误，保持 diagnostics 为空
        }
        send({
            jsonrpc: '2.0',
            method: 'textDocument/publishDiagnostics',
            params: { uri, diagnostics }
        });
        try { fs.unlinkSync(tmpFile); } catch (_) {}
    });
}

// ─── 补全数据 ───────────────────────────────────────────
const KEYWORDS = [
    { label: '函数', detail: '函数定义', insertText: '函数 ${1:name}(${2:params}) {\n\t$0\n}' },
    { label: 'function', detail: '函数定义 (英文)', insertText: 'function ${1:name}(${2:params}) {\n\t$0\n}' },
    { label: 'fn', detail: '函数定义 (简写)', insertText: 'fn ${1:name}(${2:params}) {\n\t$0\n}' },
    { label: '变量', detail: '声明变量', insertText: '变量 ${1:name} = ${2:value};' },
    { label: 'var', detail: '声明变量 (英文)', insertText: 'var ${1:name} = ${2:value};' },
    { label: '常量', detail: '声明常量', insertText: '常量 ${1:name} = ${2:value};' },
    { label: 'const', detail: '声明常量 (英文)', insertText: 'const ${1:name} = ${2:value};' },
    { label: '返回', detail: '返回值' },
    { label: 'return', detail: '返回值 (英文)' },
    { label: '如果', detail: '条件判断', insertText: '如果 (${1:condition}) {\n\t$0\n}' },
    { label: 'if', detail: '条件判断 (英文)', insertText: 'if (${1:condition}) {\n\t$0\n}' },
    { label: '否则', detail: '否则分支', insertText: '否则 {\n\t$0\n}' },
    { label: 'else', detail: '否则分支 (英文)' },
    { label: '当', detail: '当循环', insertText: '当 (${1:condition}) {\n\t$0\n}' },
    { label: 'while', detail: '当循环 (英文)', insertText: 'while (${1:condition}) {\n\t$0\n}' },
    { label: '循环', detail: '计数循环', insertText: '循环 (${1:init}; ${2:cond}; ${3:inc}) {\n\t$0\n}' },
    { label: 'for', detail: '计数循环 (英文)', insertText: 'for (${1:init}; ${2:cond}; ${3:inc}) {\n\t$0\n}' },
    { label: '遍历', detail: '遍历容器', insertText: '遍历 (${1:item}) {\n\t$0\n}' },
    { label: 'foreach', detail: '遍历容器 (英文)', insertText: 'foreach (${1:item}) {\n\t$0\n}' },
    { label: '选择', detail: '选择语句', insertText: '选择 (${1:expr}) {\n\t情况 ${2:val}: {\n\t\t$0\n\t}\n}' },
    { label: 'switch', detail: '选择语句 (英文)', insertText: 'switch (${1:expr}) {\n\tcase ${2:val}: {\n\t\t$0\n\t}\n}' },
    { label: '情况', detail: '选择分支' },
    { label: 'case', detail: '选择分支 (英文)' },
    { label: '其他', detail: '默认分支' },
    { label: 'default', detail: '默认分支 (英文)' },
    { label: '类', detail: '定义类', insertText: '类 ${1:name} {\n\t$0\n}' },
    { label: 'class', detail: '定义类 (英文)', insertText: 'class ${1:name} {\n\t$0\n}' },
    { label: '结构体', detail: '定义结构体', insertText: '结构体 ${1:name} {\n\t$0\n}' },
    { label: 'struct', detail: '定义结构体 (英文)', insertText: 'struct ${1:name} {\n\t$0\n}' },
    { label: '导入', detail: '导入模块', insertText: '导入 "${1:module}";' },
    { label: 'import', detail: '导入模块 (英文)' },
    { label: '真', detail: '布尔值 true' },
    { label: 'true', detail: '布尔值 true' },
    { label: '假', detail: '布尔值 false' },
    { label: 'false', detail: '布尔值 false' },
    { label: '空', detail: '空值 null' },
    { label: 'null', detail: '空值 null' },
    { label: 'nil', detail: '空值 nil' },
    { label: 'main', detail: '程序入口', insertText: '函数 main() {\n\t$0\n}' },
];

const BUILTINS = [
    { label: '打印', detail: '打印输出到控制台' },
    { label: 'print', detail: '打印输出' },
    { label: 'println', detail: '打印并换行' },
    { label: '长度', detail: '获取字符串/数组长度' },
    { label: 'len', detail: '获取长度' },
    { label: '转字符串', detail: '转为字符串' },
    { label: 'toString', detail: '转为字符串 (英文)' },
    { label: '输入', detail: '从控制台读取输入' },
    { label: 'input', detail: '读取输入' },
    { label: '断言', detail: '断言检查' },
    { label: 'assert', detail: '断言' },
    { label: '退出', detail: '退出程序' },
    { label: 'exit', detail: '退出程序' },
    { label: '随机', detail: '生成随机数' },
    { label: 'random', detail: '随机数' },
    { label: 'rand', detail: '随机数 (简写)' },
    { label: 'tick', detail: '获取当前毫秒数' },
    { label: 'tock', detail: '从上次 tick 经过的毫秒数' },
    { label: 'sleep', detail: '休眠指定毫秒' },
    { label: '现在', detail: '获取当前时间戳' },
    { label: '类型', detail: '获取值的类型' },
    { label: 'typeof', detail: '获取类型 (英文)' },
    { label: '表取', detail: '从表/字典中取值' },
    { label: '表设', detail: '设置表/字典中的值' },
    { label: '表有', detail: '检查键是否存在' },
    { label: '表删', detail: '删除表中的键' },
    { label: '整数', detail: '整数类型' },
    { label: '布尔', detail: '布尔类型' },
    { label: '浮点', detail: '浮点数类型' },
    { label: '文本', detail: '字符串类型' },
    { label: 'string', detail: '字符串类型' },
    { label: 'void', detail: '无返回值类型' },
    { label: 'int', detail: '整数类型 (英文)' },
    { label: 'bool', detail: '布尔类型 (英文)' },
    { label: 'f32', detail: '32位浮点' },
    { label: 'f64', detail: '64位浮点' },
];

const HOVER_DOCS = {};
for (const k of [...KEYWORDS, ...BUILTINS]) {
    HOVER_DOCS[k.label] = k.detail || '';
}

// ─── 请求处理 ───────────────────────────────────────────
function handle(msg) {
    const { id, method, params } = msg;

    switch (method) {

        // ── 初始化 ──
        case 'initialize':
            return send({
                jsonrpc: '2.0', id,
                result: {
                    capabilities: {
                        textDocumentSync: {
                            openClose: true,
                            change: 1 // Full
                        },
                        completionProvider: {
                            triggerCharacters: ['.'],
                            resolveProvider: false
                        },
                        hoverProvider: true,
                        definitionProvider: true
                    },
                    serverInfo: { name: 'cplsp.js', version: '0.1.0' }
                }
            });

        case 'initialized':
            return;

        // ── 关闭 ──
        case 'shutdown':
            return send({ jsonrpc: '2.0', id, result: null });

        case 'exit':
            return process.exit(0);

        // ── 文档同步 ──
        case 'textDocument/didOpen': {
            const { uri, text, version } = params.textDocument;
            docs.set(uri, { text, version });
            runDiagnostics(uri, text);
            return;
        }

        case 'textDocument/didChange': {
            const { uri, version } = params.textDocument;
            const change = params.contentChanges[0];
            docs.set(uri, { text: change.text, version });
            runDiagnostics(uri, change.text);
            return;
        }

        case 'textDocument/didClose': {
            docs.delete(params.textDocument.uri);
            return;
        }

        // ── 代码补全 ──
        case 'textDocument/completion': {
            const items = [...KEYWORDS, ...BUILTINS].map(k => ({
                label: k.label,
                kind: k.label.match(/^[A-Za-z]/) ? 14 : 14, // Property
                detail: k.detail,
                insertText: k.insertText || k.label,
                insertTextFormat: k.insertText ? 2 : 1 // Snippet or PlainText
            }));
            // 添加文档中已定义的标识符
            const doc = docs.get(params.textDocument.uri);
            if (doc) {
                const idents = new Set();
                const re = /(?:函数|function|fn|变量|var|常量|const)\s+([A-Za-z_\u4e00-\u9fff][A-Za-z0-9_\u4e00-\u9fff]*)/g;
                let m;
                while ((m = re.exec(doc.text)) !== null) {
                    if (!idents.has(m[1])) {
                        idents.add(m[1]);
                        items.push({
                            label: m[1],
                            kind: 6, // Function
                            detail: '文档内定义'
                        });
                    }
                }
            }
            return send({
                jsonrpc: '2.0', id,
                result: { isIncomplete: false, items }
            });
        }

        // ── 悬停提示 ──
        case 'textDocument/hover': {
            const doc = docs.get(params.textDocument.uri);
            if (!doc) return send({ jsonrpc: '2.0', id, result: null });

            const pos = params.position;
            const lines = doc.text.split('\n');
            const line = lines[pos.line] || '';
            // 提取当前标识符
            const wordRe = /[A-Za-z_\u4e00-\u9fff][A-Za-z0-9_\u4e00-\u9fff]*/g;
            let wm, word = '';
            while ((wm = wordRe.exec(line)) !== null) {
                if (wm.index <= pos.character && wm.index + wm[0].length >= pos.character) {
                    word = wm[0];
                    break;
                }
            }

            if (word && HOVER_DOCS[word]) {
                return send({
                    jsonrpc: '2.0', id,
                    result: {
                        contents: { kind: 'markdown', value: `**${word}** — ${HOVER_DOCS[word]}` }
                    }
                });
            }
            return send({ jsonrpc: '2.0', id, result: null });
        }

        // ── 跳转到定义 ──
        case 'textDocument/definition': {
            const doc = docs.get(params.textDocument.uri);
            if (!doc) return send({ jsonrpc: '2.0', id, result: null });

            const pos = params.position;
            const lines = doc.text.split('\n');
            const line = lines[pos.line] || '';
            const wordRe = /[A-Za-z_\u4e00-\u9fff][A-Za-z0-9_\u4e00-\u9fff]*/g;
            let wm, word = '';
            while ((wm = wordRe.exec(line)) !== null) {
                if (wm.index <= pos.character && wm.index + wm[0].length >= pos.character) {
                    word = wm[0];
                    break;
                }
            }

            if (word) {
                // 在文档中搜索定义位置
                const defRe = new RegExp(
                    `(?:函数|function|fn|变量|var|常量|const)\\s+${word.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')}`);
                for (let i = 0; i < lines.length; i++) {
                    if (defRe.test(lines[i])) {
                        const col = lines[i].indexOf(word);
                        return send({
                            jsonrpc: '2.0', id,
                            result: {
                                uri: params.textDocument.uri,
                                range: {
                                    start: { line: i, character: Math.max(0, col) },
                                    end:   { line: i, character: col + word.length }
                                }
                            }
                        });
                    }
                }
            }
            return send({ jsonrpc: '2.0', id, result: null });
        }

        default:
            // 未知方法忽略
            if (id) send({ jsonrpc: '2.0', id, error: { code: -32601, message: 'not implemented' } });
    }
}

// 启动完成通知
console.error('[cplsp] 语言服务器已启动');
