const { spawn } = require('child_process');
const path = require('path');

function getCplangHome() {
    return process.env.CPLANG_HOME || path.join(require('os').homedir(), 'cplang');
}

function getCplangCompiler() {
    const home = getCplangHome();
    const exe = process.platform === 'win32' ? 'cplang.exe' : 'cplang';
    // 依次搜索：CPLANG_HOME/bin、CPLANG_HOME/build_msvc/bin、CPLANG_HOME/build、PATH
    const candidates = [
        path.join(home, 'bin', exe),                    // NSIS 安装标准路径
        path.join(home, 'build_verify', 'bin', exe),    // CMake Ninja 构建路径（最新）
        path.join(home, 'build_msvc', 'bin', exe),      // 旧 MSVC 构建路径
        path.join(home, 'build', exe),                  // Linux 构建路径
    ];
    for (const p of candidates) {
        try { if (require('fs').statSync(p).isFile()) return p; } catch (_) {}
    }
    // 最后尝试 PATH
    return exe;
}

function findCompilerNearFile(filePath) {
    const exe = process.platform === 'win32' ? 'cplang.exe' : 'cplang';
    // 从文件所在目录向上搜索 build_msvc/bin/cplang.exe
    let dir = path.dirname(filePath);
    while (true) {
        const p = path.join(dir, 'build_msvc', 'bin', exe);
        try { if (require('fs').statSync(p).isFile()) return p; } catch (_) {}
        const parent = path.dirname(dir);
        if (parent === dir) break;
        dir = parent;
    }
    // 兜底：旧的查找逻辑
    return getCplangCompiler();
}

function activate(context) {
    const vscode = require('vscode');

    // ---- LSP Client (Node.js) ----
    const isWin = process.platform === 'win32';
    const nodeCmd = isWin ? 'node' : 'node';
    const serverScript = path.join(__dirname, 'cplsp.js');
    const projectRoot = getCplangHome();

    console.log(`[cplsp] starting: ${nodeCmd} ${serverScript}`);

    const server = spawn(nodeCmd, [serverScript], {
        cwd: projectRoot,
        stdio: ['pipe', 'pipe', 'pipe'],
        env: Object.assign({}, process.env, { CPLANG_HOME: projectRoot })
    });

    server.stderr.on('data', d => {
        console.error('[cplsp]', d.toString());
    });

    server.on('error', err => {
        console.error('[cplsp] start failed:', err.message);
    });

    server.on('exit', (code, signal) => {
        console.log(`[cplsp] exit: code=${code} signal=${signal}`);
    });

    // LSP 客户端（可选依赖，不阻塞插件启动）
    try {
        const { LanguageClient } = require('vscode-languageclient/node');
        const client = new LanguageClient(
            'cplsp',
            'CP Language Server',
            () => Promise.resolve({ writer: server.stdin, reader: server.stdout }),
            {
                documentSelector: [{ scheme: 'file', language: 'cp' }],
                synchronize: { configurationSection: 'cp' }
            }
        );
        client.start();
    } catch (e) {
        console.log('[cp] LSP 客户端未加载（不影响 cp.run 命令）');
    }

    // ---- Helper: get current .cp file ----
    function getActiveCpFile() {
        const editor = vscode.window.activeTextEditor;
        if (!editor) { vscode.window.showErrorMessage('Please open a .cp file first'); return null; }
        if (!editor.document.fileName.endsWith('.cp')) {
            vscode.window.showErrorMessage('Current file is not a .cp file'); return null;
        }
        return editor.document.fileName;
    }

    // ---- Command: run .cp file ----
    const runCommand = vscode.commands.registerCommand('cp.run', () => {
        const filePath = getActiveCpFile();
        if (!filePath) return;
        const compiler = findCompilerNearFile(filePath);
        const terminal = vscode.window.createTerminal({ name: 'CP Run', hideFromUser: false });
        terminal.show();
        terminal.sendText(`& "${compiler}" -c "${filePath}"`);
    });

    // ---- Command: compile .cp file (check only) ----
    const buildCommand = vscode.commands.registerCommand('cp.build', () => {
        const filePath = getActiveCpFile();
        if (!filePath) return;
        const compiler = getCplangCompiler();
        const terminal = vscode.window.createTerminal({ name: 'CP Build', hideFromUser: false });
        terminal.show();
        terminal.sendText(`"${compiler}" -p "${filePath}"`);
    });

    // ---- Command: SFX 打包为 .exe ----
    const sfxCommand = vscode.commands.registerCommand('cp.sfx', () => {
        const filePath = getActiveCpFile();
        if (!filePath) return;
        const compiler = getCplangCompiler();
        const outPath = filePath.replace(/\.cp$/i, '.exe');
        const terminal = vscode.window.createTerminal({ name: 'CP SFX Pack', hideFromUser: false });
        terminal.show();
        terminal.sendText(`& "${compiler}" -k "${filePath}" -o "${outPath}"`);
    });

    // ---- Command: 构建 Linux 可执行文件 ----
    const linuxCommand = vscode.commands.registerCommand('cp.linux', () => {
        const filePath = getActiveCpFile();
        if (!filePath) return;
        const projectRoot = path.dirname(path.dirname(path.dirname(__dirname)));
        const terminal = vscode.window.createTerminal({ name: 'CP Linux Build', hideFromUser: false });
        terminal.show();
        const buildScript = path.join(projectRoot, 'build_linux.sh').replace(/\\/g, '/');
        const wslPath = '/mnt/' + buildScript[0].toLowerCase() + buildScript.slice(2);
        terminal.sendText(`wsl chmod +x ${wslPath} && wsl ${wslPath} Release cli`);
    });

    // ---- Command: 构建 Android APK ----
    const androidCommand = vscode.commands.registerCommand('cp.android', () => {
        const filePath = getActiveCpFile();
        if (!filePath) return;
        const projectRoot = path.dirname(path.dirname(path.dirname(__dirname)));
        const terminal = vscode.window.createTerminal({ name: 'CP Android Build', hideFromUser: false });
        terminal.show();
        const buildScript = path.join(projectRoot, 'build_linux.sh').replace(/\\/g, '/');
        const wslPath = '/mnt/' + buildScript[0].toLowerCase() + buildScript.slice(2);
        terminal.sendText(`wsl "ANDROID_NDK=\${ANDROID_NDK:-\$HOME/Android/Sdk/ndk-bundle} ${wslPath} android"`);
    });

    // ---- Command: show environment info ----
    const infoCommand = vscode.commands.registerCommand('cp.info', () => {
        const home = getCplangHome();
        const compiler = getCplangCompiler();
        const exists = require('fs').existsSync(compiler);
        const dir = __dirname;
        const exe = process.platform === 'win32' ? 'cplang.exe' : 'cplang';
        const candidates = [
            path.join(home, 'bin', exe),
            path.join(home, 'build_msvc', 'bin', exe),
            path.join(home, 'build', exe),
        ];
        vscode.window.showInformationMessage(
            `CPLANG_HOME = ${home}\n__dirname = ${dir}\ncompiler = ${compiler}\nexists = ${exists}\n\n候选路径:\n${candidates.join('\n')}`
        );
    });

    context.subscriptions.push(runCommand, buildCommand, sfxCommand, linuxCommand, androidCommand, infoCommand);

    // ---- Cleanup ----
    context.subscriptions.push({
        dispose: () => {
            server.kill();
            client.stop();
        }
    });
}

function deactivate() {}

module.exports = { activate, deactivate };