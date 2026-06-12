/**
 * CP Language Server (LSP) v0.4
 * stdio JSON-RPC 2.0, 零外部依赖
 *
 * 功能:
 *   - textDocument/didOpen → 编译诊断
 *   - textDocument/didChange → 增量诊断
 *   - textDocument/completion → 代码补全
 *   - textDocument/hover → 悬停提示
 *   - textDocument/definition → 跳转到定义
 *   - textDocument/references → 查找引用
 *   - textDocument/rename → 重命名
 *   - textDocument/documentSymbol → 文档符号
 *   - textDocument/signatureHelp → 签名帮助
 *   - textDocument/formatting → 代码格式化
 */

const fs = require('fs');
const path = require('path');
const { spawn } = require('child_process');

// ─── 配置 ───────────────────────────────────────────────
const CPLANG_HOME = process.env.CPLANG_HOME ||
    path.join(require('os').homedir(), 'cplang');
const COMPILER = (() => {
    const exe = process.platform === 'win32' ? 'cplang.exe' : 'cplang';
    const candidates = [
        path.join(CPLANG_HOME, 'bin', exe),             // NSIS 安装标准路径
        path.resolve(__dirname, '../..', 'build_verify', 'bin', exe),
        path.resolve(__dirname, '../..', 'build_msvc', 'bin', exe),
        path.join(CPLANG_HOME, 'build_verify', 'bin', exe),
        path.join(CPLANG_HOME, 'build_msvc', 'bin', exe),
        path.join(CPLANG_HOME, 'build', exe),
    ];
    for (const p of candidates) {
        try { if (require('fs').statSync(p).isFile()) return p; } catch (_) {}
    }
    return candidates[0];
})();
const FORMATTER = path.join(__dirname, '..', 'cpfmt.py');

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

// ─── 编译器诊断 ─────────────────────────────────────────
function runDiagnostics(uri, text) {
    const tmpFile = path.join(require('os').tmpdir(), `_cplsp_${Date.now()}.cp`);
    fs.writeFileSync(tmpFile, text, 'utf-8');
    const proc = spawn(COMPILER, ['-p', tmpFile], { cwd: CPLANG_HOME });
    let stderr = '';
    proc.stderr.on('data', d => stderr += d.toString());
    proc.on('close', code => {
        const diagnostics = [];
        if (code !== 0 || stderr) {
            const lineRe = /第(\d+)行第(\d+)列:\s*(.*?)(?:\s*\(当前:\s*'[^']*'\))?/g;
            let m;
            while ((m = lineRe.exec(stderr)) !== null) {
                diagnostics.push({
                    range: {
                        start: { line: parseInt(m[1]) - 1, character: parseInt(m[2]) - 1 },
                        end:   { line: parseInt(m[1]) - 1, character: parseInt(m[2]) + 5 }
                    },
                    severity: 1,
                    source: 'cplang',
                    message: m[3].trim()
                });
            }
        }
        send({
            jsonrpc: '2.0',
            method: 'textDocument/publishDiagnostics',
            params: { uri, diagnostics }
        });
        try { fs.unlinkSync(tmpFile); } catch (_) {}
    });
}

// ─── 补全数据（完整版） ──────────────────────────────────
const KEYWORDS = [
    // 函数/变量
    { label: '函数', detail: '定义函数', insertText: '函数 ${1:name}(${2:params}) {\n\t$0\n}' },
    { label: 'function', detail: '定义函数 (英文)', insertText: 'function ${1:name}(${2:params}) {\n\t$0\n}' },
    { label: '异步', detail: '定义异步函数', insertText: '异步 函数 ${1:name}(${2:params}) {\n\t$0\n}' },
    { label: 'async', detail: '定义异步函数 (英文)', insertText: 'async function ${1:name}(${2:params}) {\n\t$0\n}' },
    { label: '变量', detail: '声明变量', insertText: '变量 ${1:name} = ${2:value};' },
    { label: 'var', detail: '声明变量 (英文)' },
    { label: '常量', detail: '声明常量', insertText: '常量 ${1:name} = ${2:value};' },
    { label: 'const', detail: '声明常量 (英文)' },
    { label: '设', detail: 'let 绑定', insertText: '设 ${1:name} = ${2:value};' },
    { label: 'let', detail: 'let 绑定 (英文)' },
    { label: '可变', detail: 'mutable 变量', insertText: '可变 ${1:name} = ${2:value};' },
    { label: 'mutable', detail: 'mutable 变量 (英文)' },
    // 控制流
    { label: '返回', detail: '返回值' },
    { label: 'return', detail: '返回值 (英文)' },
    { label: '如果', detail: '条件判断', insertText: '如果 (${1:condition}) {\n\t$0\n}' },
    { label: 'if', detail: '条件判断 (英文)', insertText: 'if (${1:condition}) {\n\t$0\n}' },
    { label: '否则', detail: '否则分支', insertText: '否则 {\n\t$0\n}' },
    { label: 'else', detail: '否则分支 (英文)' },
    { label: '否则如果', detail: 'else if 分支', insertText: '否则 如果 (${1:condition}) {\n\t$0\n}' },
    { label: '当', detail: 'while 循环', insertText: '当 (${1:condition}) {\n\t$0\n}' },
    { label: 'while', detail: 'while 循环 (英文)', insertText: 'while (${1:condition}) {\n\t$0\n}' },
    { label: '循环', detail: 'for 循环', insertText: '循环 (${1:init}; ${2:cond}; ${3:inc}) {\n\t$0\n}' },
    { label: 'for', detail: 'for 循环 (英文)', insertText: 'for (${1:init}; ${2:cond}; ${3:inc}) {\n\t$0\n}' },
    { label: '遍历', detail: '遍历容器', insertText: '遍历 (${1:item} : ${2:container}) {\n\t$0\n}' },
    { label: 'foreach', detail: '遍历容器 (英文)' },
    { label: '为', detail: 'do-while 循环', insertText: '为 {\n\t$0\n} 当 (${1:condition});' },
    { label: 'do', detail: 'do-while 循环 (英文)' },
    { label: '跳出', detail: '跳出循环' },
    { label: 'break', detail: '跳出循环 (英文)' },
    { label: '继续', detail: '继续下一次循环' },
    { label: 'continue', detail: '继续循环 (英文)' },
    // 选择/匹配
    { label: '选择', detail: 'switch 语句', insertText: '选择 (${1:expr}) {\n\t情况 ${2:val}: {\n\t\t$0\n\t\t跳出;\n\t}\n\t其他: {}\n}' },
    { label: 'switch', detail: 'switch 语句 (英文)' },
    { label: '情况', detail: 'case 分支' },
    { label: 'case', detail: 'case 分支 (英文)' },
    { label: '其他', detail: 'default 分支' },
    { label: 'default', detail: 'default 分支 (英文)' },
    { label: '匹配', detail: 'match 表达式', insertText: '匹配 (${1:expr}) {\n\t${2:pattern} => ${3:result};\n\t其他 => ${4:default};\n}' },
    { label: 'match', detail: 'match 表达式 (英文)' },
    // 异常处理
    { label: '尝试', detail: 'try 块', insertText: '尝试 {\n\t$0\n} 捕获 (${1:e}) {\n\t\n}' },
    { label: 'try', detail: 'try 块 (英文)' },
    { label: '捕获', detail: 'catch 块' },
    { label: 'catch', detail: 'catch 块 (英文)' },
    { label: '抛出', detail: '抛出异常', insertText: '抛出 "${1:message}";' },
    { label: 'throw', detail: '抛出异常 (英文)' },
    { label: '最终', detail: 'finally 块', insertText: '最终 {\n\t$0\n}' },
    { label: 'finally', detail: 'finally 块 (英文)' },
    // 推迟
    { label: '推迟', detail: 'defer 延迟执行', insertText: '推迟 ${1:expression};' },
    { label: 'defer', detail: 'defer 延迟执行 (英文)' },
    // OOP
    { label: '类', detail: '定义类', insertText: '类 ${1:name} {\n\t$0\n}' },
    { label: 'class', detail: '定义类 (英文)' },
    { label: '结构体', detail: '定义结构体', insertText: '结构体 ${1:name} {\n\t${2:field1}, ${3:field2}\n}' },
    { label: 'struct', detail: '定义结构体 (英文)' },
    { label: '枚举', detail: '定义枚举', insertText: '枚举 ${1:name} {\n\t${2:variant1},\n\t${3:variant2}\n}' },
    { label: 'enum', detail: '定义枚举 (英文)' },
    { label: '接口', detail: '定义接口', insertText: '接口 ${1:name} {\n\t${2:method}(${3:params});\n}' },
    { label: 'interface', detail: '定义接口 (英文)' },
    { label: '实现', detail: '继承/实现', insertText: '实现 ${1:BaseClass}' },
    { label: 'extends', detail: '继承 (英文)' },
    { label: '这个', detail: 'this / 当前实例' },
    { label: 'this', detail: 'this (英文)' },
    { label: '继承', detail: 'super / 父类引用' },
    { label: 'super', detail: 'super (英文)' },
    { label: '新建', detail: 'new 实例化' },
    { label: 'new', detail: 'new (英文)' },
    // 访问修饰符
    { label: '公有', detail: 'public 访问修饰符' },
    { label: 'public', detail: 'public (英文)' },
    { label: '私有', detail: 'private 访问修饰符' },
    { label: 'private', detail: 'private (英文)' },
    { label: '保护', detail: 'protected 访问修饰符' },
    { label: 'protected', detail: 'protected (英文)' },
    { label: '静态', detail: 'static 修饰符' },
    { label: 'static', detail: 'static (英文)' },
    { label: '虚拟', detail: 'virtual 方法' },
    { label: 'virtual', detail: 'virtual (英文)' },
    { label: '重写', detail: 'override 方法' },
    { label: 'override', detail: 'override (英文)' },
    { label: '抽象', detail: 'abstract 修饰符' },
    { label: 'abstract', detail: 'abstract (英文)' },
    // 模块
    { label: '导入', detail: '导入模块', insertText: '导入 "${1:module}";' },
    { label: 'import', detail: '导入模块 (英文)' },
    { label: '包名', detail: '声明包名', insertText: '包名 ${1:name};' },
    { label: 'package', detail: '声明包名 (英文)' },
    // 等待
    { label: '等待', detail: 'await 等待异步操作' },
    { label: 'await', detail: 'await (英文)' },
    // 布尔/空
    { label: '真', detail: '布尔值 true' },
    { label: 'true', detail: '布尔值 true (英文)' },
    { label: '假', detail: '布尔值 false' },
    { label: 'false', detail: '布尔值 false (英文)' },
    { label: '空', detail: '空值 null/nil' },
    { label: 'null', detail: '空值 (英文)' },
    { label: 'nil', detail: '空值 nil' },
    // 入口
    { label: 'main', detail: '程序入口', insertText: '函数 main() {\n\t$0\n}' },
];

const BUILTINS = [
    // 数学
    { label: '绝对值', detail: 'abs(n) — 绝对值' },
    { label: '平方根', detail: 'sqrt(n) — 平方根' },
    { label: '幂', detail: 'pow(a,b) — a的b次方' },
    { label: '向下取整', detail: 'floor(n) — 向下取整' },
    { label: '向上取整', detail: 'ceil(n) — 向上取整' },
    { label: '四舍五入', detail: 'round(n) — 四舍五入' },
    { label: '正弦', detail: 'sin(x) — 正弦' },
    { label: '余弦', detail: 'cos(x) — 余弦' },
    { label: '正切', detail: 'tan(x) — 正切' },
    { label: '反正弦', detail: 'asin(x) — 反正弦' },
    { label: '反余弦', detail: 'acos(x) — 反余弦' },
    { label: '反正切', detail: 'atan(x) — 反正切' },
    { label: '圆周率', detail: 'pi() — 圆周率 π' },
    { label: '自然常数', detail: 'e() — 自然常数 e' },
    { label: '随机值', detail: 'random(min,max) — 随机整数' },
    { label: '阶乘', detail: 'factorial(n) — 阶乘' },
    { label: '均值', detail: 'mean(arr) — 平均值' },
    { label: '中位数', detail: 'median(arr) — 中位数' },
    { label: '标准差', detail: 'stddev(arr) — 标准差' },
    // IO
    { label: '打印', detail: 'print(...) — 输出到控制台' },
    { label: 'print', detail: 'print(...) — 输出到控制台' },
    { label: 'println', detail: 'println(...) — 输出并换行' },
    { label: '输入', detail: 'input() — 读取用户输入' },
    { label: '读取文件', detail: 'readFile(path) — 读取文件' },
    { label: '写入文件', detail: 'writeFile(path,data) — 写入文件' },
    { label: '文件存在', detail: 'fileExists(path) — 文件是否存在' },
    // 字符串
    { label: '长度', detail: 'len(s) — 字符串/数组长度' },
    { label: '子串', detail: 'substr(s,start,len) — 提取子串' },
    { label: '连接', detail: 'concat(a,b) — 拼接字符串' },
    { label: '查找', detail: 'find(s,pat) — 查找子串位置' },
    { label: '替换', detail: 'replace(s,from,to) — 替换子串' },
    { label: '分割', detail: 'split(s,sep) — 分割字符串' },
    { label: '修剪', detail: 'trim(s) — 去除首尾空白' },
    { label: '小写', detail: 'lower(s) — 转为小写' },
    { label: '大写', detail: 'upper(s) — 转为大写' },
    { label: '包含', detail: 'contains(s,sub) — 是否包含子串' },
    { label: '格式化', detail: 'format(fmt,...) — 格式化字符串' },
    // 转换
    { label: '转字符串', detail: 'toString(v) — 转为字符串' },
    { label: '转整数', detail: 'parseInt(s) — 字符串转整数' },
    { label: '转浮点', detail: 'parseFloat(s) — 字符串转浮点' },
    { label: 'JSON解析', detail: 'jsonParse(s) — JSON→值' },
    { label: '转JSON', detail: 'jsonStringify(v) — 值→JSON' },
    // 数组
    { label: '追加', detail: 'push(arr,val) — 数组尾部追加' },
    { label: '弹出', detail: 'pop(arr) — 弹出尾部元素' },
    { label: '插入', detail: 'insert(arr,idx,val) — 指定位置插入' },
    { label: '删除', detail: 'remove(arr,idx) — 删除指定位置' },
    { label: '切片', detail: 'slice(arr,start,end) — 提取子数组' },
    { label: '排序', detail: 'sort(arr) — 数组排序' },
    { label: '反转', detail: 'reverse(arr) — 反转数组' },
    { label: '映射', detail: 'map(arr,fn) — 对每个元素应用函数' },
    { label: '过滤', detail: 'filter(arr,fn) — 过滤元素' },
    { label: '累积', detail: 'reduce(arr,fn,init?) — 累积计算' },
    { label: '去重', detail: 'unique(arr) — 数组去重' },
    { label: '展平', detail: 'flatten(arr) — 展平嵌套数组' },
    { label: '打包', detail: 'zip(a,b) — 两个数组打包为对数组' },
    { label: '头出', detail: 'shift(arr) — 弹出第一个元素' },
    { label: '头插', detail: 'unshift(arr,val) — 在开头插入' },
    // 表
    { label: '表创建', detail: 'tableCreate(keys?,vals?) — 创建表' },
    { label: '表取', detail: 'tableGet(t,key) — 表中取值' },
    { label: '表设', detail: 'tableSet(t,key,val) — 设置表键值' },
    { label: '表长', detail: 'tblen(t) — 表键数量' },
    { label: '表键', detail: 'keys(t) — 表的所有键' },
    { label: '表值', detail: 'values(t) — 表的所有值' },
    { label: '表有', detail: 'has(t,key) — 键是否存在' },
    { label: '表删', detail: 'tableDel(t,key) — 删除键' },
    { label: '表清空', detail: 'tableClear(t) — 清空表' },
    { label: '表合并', detail: 'tableMerge(a,b) — 合并表' },
    { label: '表转数组', detail: 'tableToArray(t) — 表→数组' },
    // 类型
    { label: '类型', detail: 'typeof(v) — 值的类型名' },
    { label: '是整数', detail: 'isInt(v) — 是否为整数' },
    { label: '是字符串', detail: 'isString(v) — 是否为字符串' },
    { label: '是数组', detail: 'isArray(v) — 是否为数组' },
    { label: '是函数', detail: 'isFunction(v) — 是否为函数' },
    { label: '是空', detail: 'isNil(v) — 是否为 nil' },
    // 时间/系统
    { label: '时间戳', detail: 'now() — 当前时间戳(ms)' },
    { label: '延时', detail: 'sleep(ms) — 暂停指定毫秒' },
    { label: '计时器', detail: 'tick() — 高精度计时器' },
    { label: '平台', detail: 'platform() — 操作系统名称' },
    { label: '程序退出', detail: 'exit(code) — 退出程序' },
    { label: '环境变量', detail: 'getEnv(name) — 获取环境变量' },
    { label: '当前目录', detail: 'cwd() — 当前工作目录' },
    { label: '进程ID', detail: 'pid() — 当前进程ID' },
    // 并发
    { label: '通道创建', detail: 'channelCreate(cap?) — 创建通道' },
    { label: '通道发送', detail: 'channelSend(ch,val) — 发送数据' },
    { label: '通道接收', detail: 'channelRecv(ch) — 接收数据' },
    { label: '互斥创建', detail: 'mutexCreate() — 创建互斥锁' },
    { label: '互斥加锁', detail: 'mutexLock(m) — 上锁' },
    { label: '互斥解锁', detail: 'mutexUnlock(m) — 解锁' },
    { label: '线程创建', detail: 'threadCreate(fn) — 创建线程' },
    { label: '线程等待', detail: 'threadJoin(t) — 等待线程' },
    { label: '异步执行', detail: 'futureGo(fn) — 异步执行' },
    // 网络
    { label: 'HTTP获取', detail: 'httpGet(url) — HTTP GET 请求' },
    { label: 'HTTP提交', detail: 'httpPost(url,body) — HTTP POST 请求' },
    { label: 'TCP连接', detail: 'tcpConnect(host,port) — TCP 连接' },
    { label: 'TCP发送', detail: 'tcpSend(conn,data) — TCP 发送' },
    { label: 'TCP接收', detail: 'tcpRecv(conn,size) — TCP 接收' },
    // 加密
    { label: 'MD5', detail: 'md5(s) — MD5 哈希' },
    { label: 'SHA256', detail: 'sha256(s) — SHA-256 哈希' },
    { label: 'Base64编码', detail: 'base64Encode(s) — Base64 编码' },
    { label: 'Base64解码', detail: 'base64Decode(s) — Base64 解码' },
    { label: 'UUID', detail: 'uuid4() — 生成 UUID v4' },
    // 数据库
    { label: '数据库打开', detail: 'sqliteOpen(path) — 打开 SQLite' },
    { label: '数据库查询', detail: 'sqliteQuery(db,sql) — 查询' },
    { label: '数据库执行', detail: 'sqliteExec(db,sql) — 执行 SQL' },
    { label: '数据库关闭', detail: 'sqliteClose(db) — 关闭数据库' },
    { label: 'Redis连接', detail: 'redisConnect(host,port) — 连接 Redis' },
    { label: 'Redis获取', detail: 'redisGet(conn,key) — GET' },
    { label: 'Redis设置', detail: 'redisSet(conn,key,val) — SET' },
    // 图形
    { label: '初始化窗口', detail: 'initWindow(w,h,title) — 创建窗口' },
    { label: '开始绘图', detail: 'beginDrawing() — 开始一帧' },
    { label: '结束绘图', detail: 'endDrawing() — 结束一帧' },
    { label: '绘制文本', detail: 'drawText(t,x,y,sz,c) — 绘制文字' },
    { label: '绘制矩形', detail: 'drawRect(x,y,w,h,c) — 绘制矩形' },
    // 类型
    { label: '整数', detail: '整数类型' }, { label: 'int', detail: '整数类型' },
    { label: '布尔', detail: '布尔类型' }, { label: 'bool', detail: '布尔类型' },
    { label: '浮点', detail: '浮点数类型' }, { label: 'float', detail: '浮点数类型' },
    { label: '文本', detail: '字符串类型' }, { label: 'string', detail: '字符串类型' },
    { label: 'void', detail: '无返回值类型' },
    { label: 'i8', detail: '8位整数' }, { label: 'i16', detail: '16位整数' },
    { label: 'i32', detail: '32位整数' }, { label: 'i64', detail: '64位整数' },
    { label: 'f32', detail: '32位浮点' }, { label: 'f64', detail: '64位浮点' },
    { label: '初始化窗口',      detail: '创建 raylib 窗口' },
    { label: 'initWindow',       detail: 'create raylib window' },
    { label: '窗口应关闭',       detail: '检查窗口是否应关闭' },
    { label: 'windowShouldClose',detail: 'check if window should close' },
    { label: '关闭窗口',         detail: '关闭 raylib 窗口' },
    { label: 'closeWindow',      detail: 'close raylib window' },
    { label: '设置目标帧率',     detail: '设置目标帧率' },
    { label: 'setTargetFPS',     detail: 'set target FPS' },
    { label: '开始绘制',         detail: '开始帧绘制' },
    { label: 'beginDrawing',     detail: 'begin frame drawing' },
    { label: '结束绘制',         detail: '结束帧绘制' },
    { label: 'endDrawing',       detail: 'end frame drawing' },
    { label: '清空背景',         detail: '清空窗口背景' },
    { label: 'clearBackground',  detail: 'clear window background' },
    { label: '绘制文本',         detail: '在窗口上绘制文字' },
    { label: 'drawText',         detail: 'draw text on window' },
    { label: '绘制矩形',         detail: '绘制实心矩形' },
    { label: 'drawRectangle',    detail: 'draw filled rectangle' },
    { label: '绘制圆形',         detail: '绘制实心圆' },
    { label: 'drawCircle',       detail: 'draw filled circle' },
    { label: '绘制三角形',       detail: '绘制实心三角形' },
    { label: 'drawTriangle',     detail: 'draw filled triangle' },
    { label: '绘制线条',         detail: '绘制线段' },
    { label: 'drawLine',         detail: 'draw line segment' },
    { label: '键盘按下',         detail: '检测按键按下' },
    { label: 'keyPressed',       detail: 'check if key is pressed' },
    { label: '获取帧率',         detail: '获取当前帧率' },
    { label: 'getFPS',           detail: 'get current FPS' },
    { label: '颜色',             detail: '创建颜色值' },
    { label: 'color',            detail: 'create color' },
    { label: 'hexToRgb',         detail: '十六进制转 RGB 表' },
    { label: '控制台颜色',       detail: '设置控制台文字颜色' },
    { label: 'conColor',         detail: 'set console text color' },
    { label: '控制台重置',       detail: '重置控制台颜色' },
    { label: 'conReset',         detail: 'reset console color' },
    { label: '控制台清屏',       detail: '清空控制台' },
    { label: 'conClear',         detail: 'clear console' },
]

const HOVER_DOCS = {};
for (const k of [...KEYWORDS, ...BUILTINS]) {
    HOVER_DOCS[k.label] = k.detail || '';
}

// ─── LSP 请求处理 ───────────────────────────────────────
function handle(msg) {
    const { id, method, params } = msg;

    switch (method) {

        case 'initialize':
            return send({
                jsonrpc: '2.0', id,
                result: {
                    capabilities: {
                        textDocumentSync: { openClose: true, change: 1 },
                        completionProvider: { triggerCharacters: ['.'], resolveProvider: false },
                        hoverProvider: true,
                        definitionProvider: true,
                        referencesProvider: true,
                        renameProvider: true,
                        documentSymbolProvider: true,
                        signatureHelpProvider: { triggerCharacters: ['('] },
                        documentFormattingProvider: true
                    },
                    serverInfo: { name: 'cplsp.js', version: '0.4.0' }
                }
            });

        case 'initialized': return;
        case 'shutdown': return send({ jsonrpc: '2.0', id, result: null });
        case 'exit': return process.exit(0);

        // ── 文档同步 ──
        case 'textDocument/didOpen':
            docs.set(params.textDocument.uri, { text: params.textDocument.text, version: params.textDocument.version });
            runDiagnostics(params.textDocument.uri, params.textDocument.text);
            return;

        case 'textDocument/didChange':
            docs.set(params.textDocument.uri, { text: params.contentChanges[0].text, version: params.textDocument.version });
            runDiagnostics(params.textDocument.uri, params.contentChanges[0].text);
            return;

        case 'textDocument/didClose':
            docs.delete(params.textDocument.uri);
            return;

        // ── 代码补全 ──
        case 'textDocument/completion': {
            const items = [...KEYWORDS, ...BUILTINS].map(k => ({
                label: k.label,
                kind: 14,
                detail: k.detail,
                insertText: k.insertText || k.label,
                insertTextFormat: k.insertText ? 2 : 1
            }));
            const doc = docs.get(params.textDocument.uri);
            if (doc) {
                const idents = new Set();
                const re = /(?:函数|function|fn|变量|var|常量|const|结构体|struct|枚举|enum|类|class|接口|interface)\s+([\w一-鿿]+)/g;
                let m;
                while ((m = re.exec(doc.text)) !== null) {
                    if (!idents.has(m[1])) {
                        idents.add(m[1]);
                        items.push({ label: m[1], kind: 6, detail: '文档内定义' });
                    }
                }
            }
            return send({ jsonrpc: '2.0', id, result: { isIncomplete: false, items } });
        }

        // ── 悬停提示 ──
        case 'textDocument/hover': {
            const doc = docs.get(params.textDocument.uri);
            if (!doc) return send({ jsonrpc: '2.0', id, result: null });
            const word = getWordAt(doc.text, params.position);
            if (word && HOVER_DOCS[word]) {
                return send({ jsonrpc: '2.0', id, result: { contents: { kind: 'markdown', value: `**${word}** — ${HOVER_DOCS[word]}` } } });
            }
            return send({ jsonrpc: '2.0', id, result: null });
        }

        // ── 跳转到定义 ──
        case 'textDocument/definition': {
            const doc = docs.get(params.textDocument.uri);
            if (!doc) return send({ jsonrpc: '2.0', id, result: null });
            const word = getWordAt(doc.text, params.position);
            if (word) {
                const loc = findDefinition(doc.text, word);
                if (loc) return send({ jsonrpc: '2.0', id, result: { uri: params.textDocument.uri, range: loc } });
            }
            return send({ jsonrpc: '2.0', id, result: null });
        }

        // ── 查找引用 ──
        case 'textDocument/references': {
            const doc = docs.get(params.textDocument.uri);
            if (!doc) return send({ jsonrpc: '2.0', id, result: [] });
            const word = getWordAt(doc.text, params.position);
            if (!word) return send({ jsonrpc: '2.0', id, result: [] });
            const refs = [];
            const lines = doc.text.split('\n');
            const re = new RegExp(`\\b${escapeRegex(word)}\\b`, 'g');
            for (let i = 0; i < lines.length; i++) {
                let m;
                while ((m = re.exec(lines[i])) !== null) {
                    refs.push({
                        uri: params.textDocument.uri,
                        range: { start: { line: i, character: m.index }, end: { line: i, character: m.index + word.length } }
                    });
                }
            }
            return send({ jsonrpc: '2.0', id, result: refs });
        }

        // ── 重命名 ──
        case 'textDocument/rename': {
            const doc = docs.get(params.textDocument.uri);
            if (!doc) return send({ jsonrpc: '2.0', id, result: null });
            const word = getWordAt(doc.text, params.position);
            if (!word) return send({ jsonrpc: '2.0', id, result: null });
            const newName = params.newName;
            const edits = [];
            const lines = doc.text.split('\n');
            const re = new RegExp(`\\b${escapeRegex(word)}\\b`, 'g');
            for (let i = 0; i < lines.length; i++) {
                let m;
                while ((m = re.exec(lines[i])) !== null) {
                    edits.push({
                        range: { start: { line: i, character: m.index }, end: { line: i, character: m.index + word.length } },
                        newText: newName
                    });
                }
            }
            return send({ jsonrpc: '2.0', id, result: { changes: { [params.textDocument.uri]: edits } } });
        }

        // ── 文档符号 ──
        case 'textDocument/documentSymbol': {
            const doc = docs.get(params.textDocument.uri);
            if (!doc) return send({ jsonrpc: '2.0', id, result: [] });
            const symbols = [];
            const lines = doc.text.split('\n');
            const re = /(?:函数|function|fn|类|class|结构体|struct|枚举|enum|接口|interface)\s+([\w一-鿿]+)/g;
            for (let i = 0; i < lines.length; i++) {
                let m;
                while ((m = re.exec(lines[i])) !== null) {
                    const kindMap = {
                        '函数': 12, 'function': 12, 'fn': 12,
                        '类': 5, 'class': 5,
                        '结构体': 23, 'struct': 23,
                        '枚举': 10, 'enum': 10,
                        '接口': 11, 'interface': 11
                    };
                    const kw = m[1] === lines[i].match(/(?:函数|function|fn|类|class|结构体|struct|枚举|enum|接口|interface)/)[0];
                    symbols.push({
                        name: m[1],
                        kind: kindMap[kw] || 12,
                        location: {
                            uri: params.textDocument.uri,
                            range: { start: { line: i, character: m.index }, end: { line: i, character: m.index + m[0].length } }
                        }
                    });
                }
            }
            return send({ jsonrpc: '2.0', id, result: symbols });
        }

        // ── 签名帮助 ──
        case 'textDocument/signatureHelp': {
            const doc = docs.get(params.textDocument.uri);
            if (!doc) return send({ jsonrpc: '2.0', id, result: null });
            const word = getWordBeforeParen(doc.text, params.position);
            if (word && HOVER_DOCS[word]) {
                return send({
                    jsonrpc: '2.0', id,
                    result: {
                        signatures: [{ label: `${word}(...)`, documentation: HOVER_DOCS[word] }],
                        activeSignature: 0, activeParameter: 0
                    }
                });
            }
            return send({ jsonrpc: '2.0', id, result: null });
        }

        // ── 格式化 ──
        case 'textDocument/formatting': {
            const doc = docs.get(params.textDocument.uri);
            if (!doc) return send({ jsonrpc: '2.0', id, result: [] });
            // 使用内置简单格式化（无需外部依赖）
            const formatted = formatCode(doc.text);
            const lines = doc.text.split('\n');
            return send({
                jsonrpc: '2.0', id,
                result: [{
                    range: {
                        start: { line: 0, character: 0 },
                        end: { line: lines.length - 1, character: lines[lines.length - 1].length }
                    },
                    newText: formatted
                }]
            });
        }

        default:
            if (id) send({ jsonrpc: '2.0', id, error: { code: -32601, message: 'not implemented' } });
    }
}

// ─── 辅助函数 ───────────────────────────────────────────
function getWordAt(text, pos) {
    const lines = text.split('\n');
    const line = lines[pos.line] || '';
    const re = /[\w一-鿿]+/g;
    let m;
    while ((m = re.exec(line)) !== null) {
        if (m.index <= pos.character && m.index + m[0].length >= pos.character) return m[0];
    }
    return '';
}

function getWordBeforeParen(text, pos) {
    const lines = text.split('\n');
    const line = lines[pos.line] || '';
    const before = line.substring(0, pos.character);
    const m = before.match(/([\w一-鿿]+)\s*$/);
    return m ? m[1] : '';
}

function findDefinition(text, word) {
    const lines = text.split('\n');
    const defRe = new RegExp(`(?:函数|function|fn|变量|var|常量|const|类|class|结构体|struct|枚举|enum|接口|interface)\\s+${escapeRegex(word)}`);
    for (let i = 0; i < lines.length; i++) {
        if (defRe.test(lines[i])) {
            const col = lines[i].indexOf(word);
            return { start: { line: i, character: Math.max(0, col) }, end: { line: i, character: col + word.length } };
        }
    }
    return null;
}

function escapeRegex(s) {
    return s.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
}

// ─── 内置代码格式化 ──────────────────────────────────────
function formatCode(text) {
    const lines = text.split('\n');
    const result = [];
    let indent = 0;
    for (let line of lines) {
        let trimmed = line.trim();
        if (!trimmed) { result.push(''); continue; }
        // 减少缩进：} 结尾的行
        if (trimmed.startsWith('}') || trimmed.startsWith('）')) {
            indent = Math.max(0, indent - 1);
        }
        // 添加缩进
        result.push('\t'.repeat(indent) + trimmed);
        // 增加缩进：{ 结尾的行
        if (trimmed.endsWith('{') || trimmed.endsWith('）')) {
            indent++;
        }
    }
    return result.join('\n') + '\n';
}

console.error('[cplsp] CP 语言服务器 v0.4 已启动');
