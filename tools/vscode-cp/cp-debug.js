/**
 * CP 语言调试适配器 (DAP) v0.2
 * 支持: F5 启动程序、输出查看、断点设置、基本变量显示
 */
const { spawn } = require('child_process');
const path = require('path');
const fs = require('fs');

function getCompiler() {
    const home = process.env.CPLANG_HOME || path.join(require('os').homedir(), 'cplang');
    return path.join(home, 'build', process.platform === 'win32' ? 'cplang.exe' : 'cplang');
}

class CPDebugAdapter {
    constructor() {
        this.process = null;
        this.readBuffer = '';
        this.seq = 0;
        this.breakpoints = new Map();  // file → [{line, verified}]
        this.supportsBreakpoints = false;  // 编译器暂不支持交互式断点
    }

    send(msg) {
        const body = JSON.stringify(msg);
        process.stdout.write(`Content-Length: ${Buffer.byteLength(body, 'utf-8')}\r\n\r\n${body}`);
    }

    sendEvent(event, body) {
        this.send({ seq: ++this.seq, type: 'event', event, body });
    }

    sendResponse(request, body) {
        this.send({ seq: ++this.seq, type: 'response', request_seq: request.seq, success: true, command: request.command, body });
    }

    sendErrorResponse(request, message) {
        this.send({ seq: ++this.seq, type: 'response', request_seq: request.seq, success: false, command: request.command, message });
        this.sendEvent('output', { category: 'stderr', output: `\n[CP] ${message}\n` });
    }

    handleMessage(msg) {
        if (msg.type === 'request') {
            switch (msg.command) {
                case 'initialize':
                    this.sendResponse(msg, {
                        supportsConfigurationDoneRequest: true,
                        supportsTerminateRequest: true,
                        supportsTerminateThreadsRequest: true,
                        supportsSetVariable: false,
                        supportsRestartRequest: false,
                        supportsStepBack: false,
                        supportsEvaluateForHovers: false,
                        exceptionBreakpointFilters: [
                            { filter: 'uncaught', label: '未捕获的异常', default: true }
                        ]
                    });
                    this.sendEvent('initialized');
                    break;

                case 'launch':
                    this.doLaunch(msg);
                    break;

                case 'setBreakpoints': {
                    const args = msg.arguments;
                    const file = args.source ? args.source.path : '';
                    const lines = args.lines || [];
                    const bps = lines.map(line => {
                        if (!this.breakpoints.has(file)) this.breakpoints.set(file, new Set());
                        this.breakpoints.get(file).add(line);
                        return { verified: true, line, message: '断点已记录（运行时不中断，编译器暂不支持交互调试）' };
                    });
                    this.sendResponse(msg, { breakpoints: bps });
                    // 通知用户断点限制
                    if (lines.length > 0 && !this.supportsBreakpoints) {
                        this.sendEvent('output', {
                            category: 'console',
                            output: '\n[CP] 断点已设置。注意: 当前版本编译器以 -c 模式运行，不会在断点处暂停。\n'
                        });
                        this.sendEvent('output', {
                            category: 'console',
                            output: '[CP] 将来版本将支持交互式断点调试。\n\n'
                        });
                    }
                    break;
                }

                case 'setExceptionBreakpoints':
                    this.sendResponse(msg, { breakpoints: [] });
                    break;

                case 'setFunctionBreakpoints':
                    this.sendResponse(msg, { breakpoints: [] });
                    break;

                case 'configurationDone':
                    this.sendResponse(msg, {});
                    if (this.pendingLaunch) {
                        this.startProcess(this.pendingLaunch);
                        this.pendingLaunch = null;
                    }
                    break;

                case 'threads':
                    this.sendResponse(msg, { threads: [{ id: 1, name: 'CP 主线程' }] });
                    break;

                case 'stackTrace': {
                    const frames = [];
                    // 尝试从程序输出中提取调用栈（如果可用）
                    if (this.lastStackTrace) {
                        this.lastStackTrace.forEach((f, i) => {
                            frames.push({
                                id: i, name: f.func || '<main>',
                                source: f.file ? { path: f.file } : undefined,
                                line: f.line || 1, column: 0
                            });
                        });
                    }
                    if (frames.length === 0) {
                        frames.push({ id: 0, name: '<main>', line: 1, column: 0 });
                    }
                    this.sendResponse(msg, { stackFrames: frames, totalFrames: frames.length });
                    break;
                }

                case 'scopes': {
                    this.sendResponse(msg, {
                        scopes: [
                            { name: '局部变量', variablesReference: 1, expensive: false },
                            { name: '全局变量', variablesReference: 2, expensive: true }
                        ]
                    });
                    break;
                }

                case 'variables': {
                    const ref = msg.arguments.variablesReference;
                    const vars = [];
                    if (ref === 1) {
                        if (this.lastLocals) {
                            for (const [name, val] of Object.entries(this.lastLocals)) {
                                vars.push({ name, value: String(val), variablesReference: 0 });
                            }
                        }
                    } else if (ref === 2) {
                        if (this.lastGlobals) {
                            for (const [name, val] of Object.entries(this.lastGlobals)) {
                                vars.push({ name, value: String(val), variablesReference: 0 });
                            }
                        }
                    }
                    if (vars.length === 0) {
                        vars.push({ name: '<无数据>', value: '程序未暂停或编译器未在调试模式运行', variablesReference: 0 });
                    }
                    this.sendResponse(msg, { variables: vars });
                    break;
                }

                case 'evaluate': {
                    this.sendResponse(msg, {
                        result: `(无法求值: ${msg.arguments.expression} — 编译器未在交互调试模式运行)`,
                        variablesReference: 0
                    });
                    break;
                }

                case 'continue':
                    this.sendResponse(msg, { allThreadsContinued: true });
                    break;

                case 'next':
                case 'stepIn':
                case 'stepOut':
                    this.sendResponse(msg, {});
                    this.sendEvent('output', { category: 'console', output: '\n[CP] 单步调试暂不支持，程序将持续运行。\n' });
                    break;

                case 'pause':
                    this.sendResponse(msg, {});
                    this.sendEvent('output', { category: 'console', output: '\n[CP] 暂停暂不支持。\n' });
                    break;

                case 'terminate':
                case 'disconnect':
                    if (this.process) { this.process.kill(); this.process = null; }
                    this.sendResponse(msg, {});
                    break;

                default:
                    this.sendResponse(msg, {});
                    break;
            }
        }
    }

    doLaunch(msg) {
        const args = msg.arguments;
        const program = args.program || '';
        if (!program) {
            this.sendErrorResponse(msg, '未指定程序文件');
            return;
        }
        const compiler = getCompiler();
        if (!fs.existsSync(compiler)) {
            this.sendErrorResponse(msg, `编译器未找到: ${compiler}\n请设置 CPLANG_HOME 环境变量`);
            return;
        }

        this.launchConfig = { program, compiler };
        this.sendEvent('output', {
            category: 'console',
            output: `═══════════════════════════════════\n  CP 调试适配器 v0.2\n  文件: ${program}\n  编译器: ${compiler}\n═══════════════════════════════════\n\n`
        });

        if (args.noDebug) {
            this.sendResponse(msg, {});
            this.startProcess(this.launchConfig);
        } else {
            this.sendResponse(msg, {});
            this.pendingLaunch = this.launchConfig;
        }
    }

    startProcess(config) {
        this.sendEvent('output', { category: 'console', output: '[CP] 编译运行中...\n' });
        this.process = spawn(config.compiler, ['-c', config.program], {
            cwd: path.dirname(config.program),
            env: Object.assign({}, process.env)
        });

        this.process.stdout.on('data', (data) => {
            const text = data.toString();
            this.sendEvent('output', { category: 'stdout', output: text });
            // 尝试从输出中提取调用栈/变量信息
            this.parseDebugOutput(text);
        });

        this.process.stderr.on('data', (data) => {
            this.sendEvent('output', { category: 'stderr', output: data.toString() });
        });

        this.process.on('exit', (code) => {
            this.sendEvent('output', { category: 'console', output: `\n═══════════════════════════════════\n  CP 进程退出，码: ${code} ${code === 0 ? '✓' : '✗'}\n═══════════════════════════════════\n` });
            this.process = null;
            this.sendEvent('terminated', { restart: false });
        });

        this.process.on('error', (err) => {
            this.sendEvent('output', { category: 'console', output: `\n[CP] 启动失败: ${err.message}\n` });
            this.sendEvent('terminated', { restart: false });
        });
    }

    parseDebugOutput(text) {
        // 尝试解析编译器输出的调试信息（如果将来编译器支持 --debug 模式）
        // 当前版本: 仅日志记录
    }

    onData(data) {
        this.readBuffer += data;
        while (true) {
            const idx = this.readBuffer.indexOf('\r\n\r\n');
            if (idx === -1) break;
            const header = this.readBuffer.substring(0, idx);
            const match = header.match(/Content-Length:\s*(\d+)/i);
            if (!match) break;
            const length = parseInt(match[1]);
            const bodyStart = idx + 4;
            if (this.readBuffer.length < bodyStart + length) break;
            const body = this.readBuffer.substring(bodyStart, bodyStart + length);
            this.readBuffer = this.readBuffer.substring(bodyStart + length);
            try { this.handleMessage(JSON.parse(body)); } catch (e) {}
        }
    }
}

const adapter = new CPDebugAdapter();
process.stdin.on('data', (data) => adapter.onData(data.toString()));
process.stdin.on('close', () => process.exit(0));
