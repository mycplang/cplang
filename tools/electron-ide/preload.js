const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('cpAPI', {
  // 设置
  getSettings: () => ipcRenderer.invoke('get-settings'),
  saveSettings: (settings) => ipcRenderer.invoke('save-settings', settings),

  // 文件操作（支持编码参数）
  readFile: (path, encoding) => ipcRenderer.invoke('read-file', path, encoding),
  writeFile: (path, content, encoding) => ipcRenderer.invoke('write-file', path, content, encoding),
  listDir: (dir) => ipcRenderer.invoke('list-dir', dir),
  openFileDialog: (encoding) => ipcRenderer.invoke('open-file-dialog', encoding),
  openFolderDialog: () => ipcRenderer.invoke('open-folder-dialog'),
  saveFileDialog: (name) => ipcRenderer.invoke('save-file-dialog', name),

  // 编译器
  getCompiler: () => ipcRenderer.invoke('get-compiler'),
  getCompilerVersion: () => ipcRenderer.invoke('get-compiler-version'),
  compileRun: (path) => ipcRenderer.invoke('compile-run', path),
  compileCheck: (path) => ipcRenderer.invoke('compile-check', path),
  compileAot: (path) => ipcRenderer.invoke('compile-aot', path),
  packageExe: (path) => ipcRenderer.invoke('package-exe', path),
  stopRun: () => ipcRenderer.invoke('stop-run'),

  // 编译器更新
  checkCompilerUpdate: () => ipcRenderer.invoke('check-compiler-update'),
  downloadCompiler: (url) => ipcRenderer.invoke('download-compiler', url),
  updateCompilerFromGit: (action) => ipcRenderer.invoke('update-compiler-from-git', action),

  // 命令监听
  onCommand: (cb) => { ipcRenderer.on('cmd', (e, cmd) => cb(cmd)); },
  onDownloadProgress: (cb) => { ipcRenderer.on('download-progress', (e, pct) => cb(pct)); },

  // 文件系统工具
  pathSep: () => require('path').sep,
  dirname: (p) => require('path').dirname(p),
  basename: (p) => require('path').basename(p),
  join: (...args) => require('path').join(...args),
  homedir: () => require('os').homedir(),
  appDir: () => __dirname,

  // 文件系统管理
  createFile: (dir, name) => ipcRenderer.invoke('create-file', dir, name),
  createDir: (dir, name) => ipcRenderer.invoke('create-dir', dir, name),
  renameItem: (oldPath, newName) => ipcRenderer.invoke('rename-item', oldPath, newName),
  deleteItem: (targetPath) => ipcRenderer.invoke('delete-item', targetPath),

  // 全文搜索
  searchFiles: (dir, query, options) => ipcRenderer.invoke('search-files', dir, query, options),

  // REPL 交互执行
  replExec: (code) => ipcRenderer.invoke('repl-exec', code),
  replClear: () => ipcRenderer.invoke('repl-clear'),
  replHistory: () => ipcRenderer.invoke('repl-history'),

  // 会话恢复
  saveSession: (data) => ipcRenderer.invoke('save-session', data),
  loadSession: () => ipcRenderer.invoke('load-session'),

  // Git 集成
  gitCheck: () => ipcRenderer.invoke('git-check'),
  gitStatus: (repoPath) => ipcRenderer.invoke('git-status', repoPath),
  gitAdd: (repoPath, filePath) => ipcRenderer.invoke('git-add', repoPath, filePath),
  gitCommit: (repoPath, message) => ipcRenderer.invoke('git-commit', repoPath, message),
  gitDiff: (repoPath, filePath) => ipcRenderer.invoke('git-diff', repoPath, filePath),
  gitLog: (repoPath, count) => ipcRenderer.invoke('git-log', repoPath, count),
  gitBranch: (repoPath) => ipcRenderer.invoke('git-branch', repoPath),

  // 终端
  terminalStart: (cwd) => ipcRenderer.invoke('terminal-start', cwd),
  terminalWrite: (data) => ipcRenderer.invoke('terminal-write', data),
  terminalResize: (cols, rows) => ipcRenderer.invoke('terminal-resize', cols, rows),
  terminalKill: () => ipcRenderer.invoke('terminal-kill'),
  onTerminalData: (cb) => { ipcRenderer.on('terminal-data', (e, data) => cb(data)); },
  onTerminalExit: (cb) => { ipcRenderer.on('terminal-exit', (e, code) => cb(code)); },

  // 调试器
  debugStart: (filePath, breakpoints, sessionId) => ipcRenderer.invoke('debug-start', filePath, breakpoints, sessionId),
  debugStop: () => ipcRenderer.invoke('debug-stop'),
  debugContinue: () => ipcRenderer.invoke('debug-continue'),
  debugStepOver: () => ipcRenderer.invoke('debug-step-over'),
  debugStepInto: () => ipcRenderer.invoke('debug-step-into'),
  debugStepOut: () => ipcRenderer.invoke('debug-step-out'),
  onDebugOutput: (cb) => { ipcRenderer.on('debug-output', (e, data) => cb(data)); },
  onDebugEnd: (cb) => { ipcRenderer.on('debug-end', (e, data) => cb(data)); },

  // 示例管理
  listExamples: () => ipcRenderer.invoke('list-examples'),
  readExample: (filePath) => ipcRenderer.invoke('read-example', filePath),
  getExamplesDir: () => 'examples',

  // IDE 自动更新
  checkIdeUpdate: () => ipcRenderer.invoke('check-ide-update'),
  downloadIdeUpdate: (url) => ipcRenderer.invoke('download-ide-update', url),
  installIdeUpdate: (path) => ipcRenderer.invoke('install-ide-update', path),

  // 菜单栏
  getMenuDef: () => ipcRenderer.invoke('get-menu-def'),

  // 窗口控制
  winMinimize: () => ipcRenderer.invoke('win-minimize'),
  winMaximize: () => ipcRenderer.invoke('win-maximize'),
  winClose: () => ipcRenderer.invoke('win-close'),
  winIsMaximized: () => ipcRenderer.invoke('win-is-maximized'),

  // LSP 语言服务器
  lspDidOpen: (uri, text) => ipcRenderer.invoke('lsp-didOpen', uri, text),
  lspDidChange: (uri, text) => ipcRenderer.invoke('lsp-didChange', uri, text),
  lspCompletion: (uri, line, col) => ipcRenderer.invoke('lsp-completion', uri, line, col),
  lspHover: (uri, line, col) => ipcRenderer.invoke('lsp-hover', uri, line, col),
  onLspDiagnostics: (cb) => { ipcRenderer.on('lsp-diagnostics', (e, params) => cb(params)); },
});
