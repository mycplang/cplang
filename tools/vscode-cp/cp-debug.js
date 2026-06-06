/**
 * CP 语言调试适配器 (DAP)
 * 支持 F5 启动程序、显示输出、终止进程
 * 暂不支持断点/单步/变量查看
 *
 * 程序输出显示在 VSCode 的 DEBUG CONSOLE (调试控制台) 面板中
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
        this.sendEvent('output', { category: 'stderr', output: `\n[CP 错误] ${message}\n` });
    }

    handleMessage(msg) {
        if (msg.type === 'request') {
            switch (msg.command) {
                case 'initialize':
                    this.sendResponse(msg, {
                        supportsConfigurationDoneRequest: true,
                        supportsTerminateRequest: true,
                        supportsTerminateThreadsRequest: true
                    });
                    this.sendEvent('initialized');
                    break;

                case 'launch':
                    this.doLaunch(msg);
                    break;

                case 'setBreakpoints':
                case 'setExceptionBreakpoints':
                    this.sendResponse(msg, { breakpoints: [] });
                    break;

                case 'configurationDone':
                    this.sendResponse(msg, {});
                    if (this.pendingLaunch) {
                        this.startProcess(this.pendingLaunch);
                        this.pendingLaunch = null;
                    }
                    break;

                case 'terminate':
                case 'disconnect':
                    if (this.process) {
                        this.process.kill();
                        this.process = null;
                    }
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
            this.sendErrorResponse(msg, '未指定程序文件 (program is empty)');
            return;
        }

        this.sendEvent('output', {
            category: 'console',
            output: `\n═══════════════════════════════════════════\n`
        });
        this.sendEvent('output', {
            category: 'console',
            output: `  CP 调试适配器已启动\n`
        });
        this.sendEvent('output', {
            category: 'console',
            output: `  文件: ${program}\n`
        });

        // Check compiler
        const compiler = getCompiler();
        this.sendEvent('output', {
            category: 'console',
            output: `  编译器: ${compiler}\n`
        });
        this.sendEvent('output', {
            category: 'console',
            output: `  CPLANG_HOME: ${process.env.CPLANG_HOME || '(未设置)'}\n`
        });

        if (!fs.existsSync(compiler)) {
            this.sendErrorResponse(msg, `编译器未找到: ${compiler}\n请检查 CPLANG_HOME 环境变量是否设置正确`);
            return;
        }

        this.launchConfig = { program, compiler };

        if (args.noDebug) {
            this.sendResponse(msg, {});
            this.startProcess(this.launchConfig);
        } else {
            this.sendResponse(msg, {});
            this.pendingLaunch = this.launchConfig;
        }
    }

    startProcess(config) {
        this.sendEvent('output', {
            category: 'console',
            output: `  状态: 编译运行中...\n`
        });
        this.sendEvent('output', {
            category: 'console',
            output: `═══════════════════════════════════════════\n\n`
        });

        this.process = spawn(config.compiler, ['-c', config.program], {
            cwd: path.dirname(config.program),
            env: Object.assign({}, process.env)
        });

        this.process.stdout.on('data', (data) => {
            this.sendEvent('output', { category: 'stdout', output: data.toString() });
        });

        this.process.stderr.on('data', (data) => {
            this.sendEvent('output', { category: 'stderr', output: data.toString() });
        });

        this.process.on('exit', (code) => {
            this.sendEvent('output', {
                category: 'console',
                output: `\n═══════════════════════════════════════════\n`
            });
            this.sendEvent('output', {
                category: 'console',
                output: `  CP 进程已退出，退出码: ${code}\n`
            });
            if (code === 0) {
                this.sendEvent('output', {
                    category: 'console',
                    output: `  运行成功 ✓\n`
                });
            } else {
                this.sendEvent('output', {
                    category: 'console',
                    output: `  运行失败 ✗\n`
                });
            }
            this.sendEvent('output', {
                category: 'console',
                output: `═══════════════════════════════════════════\n\n`
            });
            this.process = null;
            this.sendEvent('terminated', { restart: false });
        });

        this.process.on('error', (err) => {
            this.sendEvent('output', {
                category: 'console',
                output: `\n[CP] 启动进程失败: ${err.message}\n`
            });
            this.sendEvent('terminated', { restart: false });
        });
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
            try {
                this.handleMessage(JSON.parse(body));
            } catch (e) {
                // ignore parse errors
            }
        }
    }
}

const adapter = new CPDebugAdapter();
process.stdin.on('data', (data) => adapter.onData(data.toString()));
process.stdin.on('close', () => process.exit(0));
