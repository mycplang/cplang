const { spawn } = require('child_process');
const path = require('path');

function getCplangHome() {
    return process.env.CPLANG_HOME || path.join(require('os').homedir(), 'cplang');
}

function getCplangCompiler() {
    const home = getCplangHome();
    const exe = process.platform === 'win32' ? 'cplang.exe' : 'cplang';
    return path.join(home, 'build', exe);
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
        const compiler = getCplangCompiler();
        const terminal = vscode.window.createTerminal({ name: 'CP Run', hideFromUser: false });
        terminal.show();
        terminal.sendText(`"${compiler}" -c "${filePath}"`);
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

    // ---- Command: show environment info ----
    const infoCommand = vscode.commands.registerCommand('cp.info', () => {
        const home = getCplangHome();
        const compiler = getCplangCompiler();
        const exists = require('fs').existsSync(compiler);
        vscode.window.showInformationMessage(
            `CPLANG_HOME = ${home}\ncompiler = ${compiler}\n${exists ? 'OK: compiler found' : 'ERROR: compiler not found'}`
        );
    });

    context.subscriptions.push(runCommand, buildCommand, infoCommand);

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