const { spawn } = require('child_process');
const path = require('path');

function getCplangHome() {
    return process.env.CPLANG_HOME || path.join(require('os').homedir(), 'cplang');
}

function getCplangCompiler() {
    const home = getCplangHome();
    const exe = process.platform === 'win32' ? 'cplang.exe' : 'cplang';
    // 渚濇鎼滅储锛欳PLANG_HOME/bin銆丆PLANG_HOME/build_msvc/bin銆丆PLANG_HOME/build銆丳ATH
    const candidates = [
        path.join(home, 'bin', exe),
        path.join(home, 'build_new', 'bin', exe),       // NMake MSVC build (latest)                    // NSIS 瀹夎鏍囧噯璺緞
        path.join(home, 'build_verify', 'bin', exe),    // CMake Ninja 鏋勫缓璺緞锛堟渶鏂帮級
        path.join(home, 'build_msvc', 'bin', exe),      // 鏃?MSVC 鏋勫缓璺緞
        path.join(home, 'build', exe),                  // Linux 鏋勫缓璺緞
    ];
    for (const p of candidates) {
    // Hard fallback: try C:\CPLANG directly
    for (const bd of ['build_new', 'build_verify', 'build_msvc']) {
        try { const fb = path.join('C:\\CPLANG', bd, 'bin', exe); if (require('fs').statSync(fb).isFile()) return fb; } catch (_) {}
    }
        try { if (require('fs').statSync(p).isFile()) return p; } catch (_) {}
    }
    // 鏈€鍚庡皾璇?PATH
    return exe;
}

function findCompilerNearFile(filePath) {
    const exe = process.platform === 'win32' ? 'cplang.exe' : 'cplang';
    // search from file directory upward
    let dir = path.dirname(filePath);
    while (true) {
        let p = path.join(dir, 'build_new', 'bin', exe);
        try { if (require('fs').statSync(p).isFile()) return p; } catch (_) {}
        p = path.join(dir, 'build_verify', 'bin', exe);
        try { if (require('fs').statSync(p).isFile()) return p; } catch (_) {}
        p = path.join(dir, 'build_msvc', 'bin', exe);
        try { if (require('fs').statSync(p).isFile()) return p; } catch (_) {}
        const parent = path.dirname(dir);
        if (parent === dir) break;
        dir = parent;
    }
    return getCplangCompiler();
    // Hard fallback: try C:\CPLANG directly
    for (const bd of ['build_new', 'build_verify', 'build_msvc']) {
        try { const fb = path.join('C:\\CPLANG', bd, 'bin', exe); if (require('fs').statSync(fb).isFile()) return fb; } catch (_) {}
    }
}

function activate(context) {
    const vscode = require('vscode');

    // ---- LSP Client (原生 cplsp.exe) ----
    // 优先使用插件内打包的二进制，回退到构建目录
    const isWin = process.platform === 'win32';
    const platformDir = isWin ? 'win32' : (process.platform === 'linux' ? 'linux' : 'darwin');
    const exeExt = isWin ? '.exe' : '';
    const serverExe = 'cplsp' + exeExt;
    const projectRoot = getCplangHome();

    // 搜索顺序: 插件内置 → 构建目录 → CPLANG_HOME/build_*
    const candidates = [
        path.join(__dirname, 'bin', platformDir, serverExe),   // 插件内置（平台专用）
        path.join(__dirname, 'bin', serverExe),                 // 插件内置（通用）
        path.join(__dirname, '..', '..', 'build_new', 'bin', serverExe),
        path.join(getCplangHome(), 'build_new', 'bin', serverExe),
        path.join(getCplangHome(), 'build_verify', 'bin', serverExe),
        path.join(getCplangHome(), 'build', serverExe),
    ];

    let serverPath = null;
    for (const p of candidates) {
        try { if (require('fs').statSync(p).isFile()) { serverPath = p; break; } } catch (_) {}
    }

    let server = null;
    let client = null;

    if (serverPath) {
        console.log(`[cplsp] starting: ${serverPath}`);

        server = spawn(serverPath, [], {
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

        // LSP 客户端（依赖 server）
        try {
            const { LanguageClient } = require('vscode-languageclient/node');
            client = new LanguageClient(
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
    } else {
        console.log('[cplsp] cplsp.exe not found, LSP disabled (F5 and other commands still work)');
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

    // ---- Command: SFX 鑷В鍘嬫墦鍖呬负 .exe (Ctrl+F7) ----
    const buildCommand = vscode.commands.registerCommand('cp.build', () => {
        const filePath = getActiveCpFile();
        if (!filePath) return;
        const compiler = findCompilerNearFile(filePath);
        const outPath = filePath.replace(/\.cp$/i, '.exe');
        const terminal = vscode.window.createTerminal({ name: 'CP SFX Pack', hideFromUser: false });
        terminal.show();
        terminal.sendText(`& "${compiler}" -k "${filePath}" -o "${outPath}"`);
    });

    // ---- Command: 鏋勫缓 Linux 鍙墽琛屾枃浠?----
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

    // ---- Command: 鏋勫缓 Android APK ----
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

    // ---- Command: 鍚姩璋冭瘯鍣?----
    const debugCommand = vscode.commands.registerCommand('cp.debug', () => {
        const vscode_ = require('vscode');
        vscode_.commands.executeCommand('workbench.action.debug.start');
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
        path.join(home, 'build_new', 'bin', exe),       // NMake MSVC build (latest)
            path.join(home, 'build_msvc', 'bin', exe),
            path.join(home, 'build', exe),
        ];
        vscode.window.showInformationMessage(
            `CPLANG_HOME = ${home}\n__dirname = ${dir}\ncompiler = ${compiler}\nexists = ${exists}\n\n鍊欓€夎矾寰?\n${candidates.join('\n')}`
        );
    });

    context.subscriptions.push(runCommand, buildCommand, linuxCommand, androidCommand, debugCommand, infoCommand);

    // ---- Cleanup ----
    context.subscriptions.push({
        dispose: () => {
            if (server) server.kill();
            if (client) client.stop();
        }
    });
}

function deactivate() {}

module.exports = { activate, deactivate };