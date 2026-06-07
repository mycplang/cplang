const { app, BrowserWindow, ipcMain, dialog, Menu, shell, protocol, net } = require('electron');
const path = require('path');
const fs = require('fs');
const { spawn, execSync, exec } = require('child_process');
const iconv = require('iconv-lite');
const https = require('https');
const os = require('os');

let mainWin;

// ── 会话存储路径 ──
const sessionPath = path.join(app.getPath('userData'), 'session.json');

// ── 查找编译器 ──
function findCompiler() {
  // 1. 优先使用 extraResources 中的捆绑版（electron-builder 打包后）
  try {
    const resPath = process.resourcesPath;
    if (resPath) {
      const p = path.join(resPath, 'cplang.exe');
      if (fs.existsSync(p)) return p;
    }
  } catch (_) {}
  // 2. 开发/调试模式：IDE 目录下的捆绑版
  const ideDir = __dirname;
  const bundled = path.join(ideDir, 'cplang.exe');
  if (fs.existsSync(bundled)) return bundled;
  // 3. 优先使用支持 AOT 的编译器
  const aotDirs = [
    path.join('C:', 'cplang', 'build_cmake_new', 'bin', 'Release', 'cplang.exe'),  // 35MB 带 LLVM
    path.join('C:', 'cplang', 'build_llvm', 'bin', 'Release', 'cplang.exe'),       // 27MB 带 LLVM
  ];
  for (const dir of aotDirs) {
    if (fs.existsSync(dir)) return dir;
  }
  // 4. CPLANG_HOME 环境变量
  const home = process.env.CPLANG_HOME || path.join(require('os').homedir(), 'cplang');
  const b1 = path.join(home, 'build', 'cplang.exe');
  if (fs.existsSync(b1)) return b1;
  const b2 = path.join(home, 'build_llvm', 'bin', 'Release', 'cplang.exe');
  if (fs.existsSync(b2)) return b2;
  return null;
}

// ── 查找 cppack.exe 打包器 ──
function findCppack() {
  try {
    const resPath = process.resourcesPath;
    if (resPath) {
      const p = path.join(resPath, 'cppack.exe');
      if (fs.existsSync(p)) return p;
    }
  } catch (_) {}
  const ideDir = __dirname;
  const p = path.join(ideDir, 'cppack.exe');
  if (fs.existsSync(p)) return p;
  return null;
}

function createWindow() {
  mainWin = new BrowserWindow({
    width: 1300,
    height: 850,
    minWidth: 800,
    minHeight: 500,
    title: 'CP IDE - Electron',
    icon: path.join(__dirname, 'cp_icon.ico'),
    backgroundColor: '#1e1e1e',
    frame: false,                  // 无框窗口，自定义标题栏+窗口控制按钮（VS Code 风格）
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: false,
    },
    show: true,
    autoHideMenuBar: true,       // 隐藏原生菜单栏，使用自定义 HTML 菜单
  });

  mainWin.loadFile('index.html');
  // 开发阶段可打开 DevTools（默认关闭）
  // mainWin.webContents.openDevTools();

  // 菜单定义：渲染进程通过 IPC 获取
  const menuDef = [
    {
      label: '文件',
      items: [
        { label: '新建文件', accelerator: 'Ctrl+N', cmd: 'new' },
        { label: '从模板新建', accelerator: 'Ctrl+Shift+N', cmd: 'new-file-template' },
        { label: '打开文件', accelerator: 'Ctrl+O', cmd: 'open' },
        { type: 'separator' },
        { label: '保存', accelerator: 'Ctrl+S', cmd: 'save' },
        { label: '另存为...', accelerator: 'Ctrl+Shift+S', cmd: 'save-as' },
        { type: 'separator' },
        { label: '关闭标签', accelerator: 'Ctrl+W', cmd: 'close-tab' },
      ],
    },
    {
      label: '编辑',
      items: [
        { role: 'undo' },
        { role: 'redo' },
        { type: 'separator' },
        { label: '撤销关闭标签', accelerator: 'Ctrl+Shift+T', cmd: 'undo-close' },
        { type: 'separator' },
        { role: 'cut' },
        { role: 'copy' },
        { role: 'paste' },
      ],
    },
    {
      label: '运行',
      items: [
        { label: '编译运行 (VM)', accelerator: 'F5', cmd: 'run' },
        { label: '启动调试', accelerator: 'Ctrl+F5', cmd: 'debug' },
        { type: 'separator' },
        { label: '仅编译检查', accelerator: 'Ctrl+F7', cmd: 'build' },
        { label: '编译为 EXE (AOT)', accelerator: 'Ctrl+F9', cmd: 'build-exe' },
        { type: 'separator' },
        { label: '中断运行', accelerator: 'Shift+F5', cmd: 'stop' },
      ],
    },
    {
      label: '视图',
      items: [
        { label: '命令面板', accelerator: 'Ctrl+Shift+P', cmd: 'palette' },
        { label: '快速打开', accelerator: 'Ctrl+P', cmd: 'quick-open' },
        { type: 'separator' },
        { label: '切换侧边栏', accelerator: 'Ctrl+B', cmd: 'toggle-sidebar' },
        { label: '切换输出面板', accelerator: 'Ctrl+`', cmd: 'toggle-output' },
        { type: 'separator' },
        { label: '设置', accelerator: 'Ctrl+,', cmd: 'settings' },
      ],
    },
    {
      label: '帮助',
      items: [
        { label: '关于 CP IDE', cmd: 'about' },
        { type: 'separator' },
        { label: '快捷键查看', accelerator: 'Ctrl+Shift+K', cmd: 'show-shortcuts' },
        { type: 'separator' },
        { label: '检查编译器更新', cmd: 'check-update' },
      ],
    },
  ];
  ipcMain.handle('get-menu-def', () => menuDef);

  // ── IPC: 窗口控制（frame: false 自定义按钮） ──
  ipcMain.handle('win-minimize', () => mainWin?.minimize());
  ipcMain.handle('win-maximize', () => {
    if (mainWin?.isMaximized()) mainWin.unmaximize(); else mainWin?.maximize();
  });
  ipcMain.handle('win-close', () => mainWin?.close());
  ipcMain.handle('win-is-maximized', () => mainWin?.isMaximized());
}

// ── IPC: 文件操作 ──

// ── 编码检测 ──

// 检测 BOM
function detectBOM(buf) {
  if (buf.length < 2) return null;
  if (buf[0] === 0xEF && buf[1] === 0xBB && buf[2] === 0xBF) return 'utf-8-sig';
  if (buf[0] === 0xFF && buf[1] === 0xFE) return 'utf-16le';
  if (buf[0] === 0xFE && buf[1] === 0xFF) return 'utf-16be';
  return null;
}

// 检测是否为有效的 UTF-8（没有非法序列）
function isValidUTF8(buf) {
  let i = 0;
  while (i < buf.length) {
    const b = buf[i];
    if (b < 0x80) { i += 1; }
    else if (b >= 0xC2 && b <= 0xDF) { if (i + 1 >= buf.length || (buf[i+1] & 0xC0) !== 0x80) return false; i += 2; }
    else if (b >= 0xE0 && b <= 0xEF) { if (i + 2 >= buf.length || (buf[i+1] & 0xC0) !== 0x80 || (buf[i+2] & 0xC0) !== 0x80) return false; i += 3; }
    else if (b >= 0xF0 && b <= 0xF4) { if (i + 3 >= buf.length || (buf[i+1] & 0xC0) !== 0x80 || (buf[i+2] & 0xC0) !== 0x80 || (buf[i+3] & 0xC0) !== 0x80) return false; i += 4; }
    else return false; // 非法首字节
  }
  return true;
}

// 自动检测编码并解码
function decodeFile(buf, preferredEncoding) {
  // 如果有指定编码，直接使用
  if (preferredEncoding && preferredEncoding !== 'auto') {
    if (preferredEncoding === 'utf-8-sig') {
      if (buf[0] === 0xEF && buf[1] === 0xBB && buf[2] === 0xBF) buf = buf.slice(3);
      return { content: iconv.decode(buf, 'utf-8'), encoding: 'utf-8-sig' };
    }
    return { content: iconv.decode(buf, preferredEncoding), encoding: preferredEncoding };
  }

  // 自动检测
  const bom = detectBOM(buf);
  if (bom) {
    let skip = 0;
    if (bom === 'utf-8-sig') skip = 3;
    else if (bom === 'utf-16le') skip = 2;
    else if (bom === 'utf-16be') skip = 2;
    const content = iconv.decode(buf.slice(skip), bom === 'utf-8-sig' ? 'utf-8' : bom);
    if (content) return { content, encoding: bom };
  }

  // 尝试 UTF-8
  if (isValidUTF8(buf)) {
    return { content: iconv.decode(buf, 'utf-8'), encoding: 'utf-8' };
  }

  // 回退到 GBK
  return { content: iconv.decode(buf, 'gbk'), encoding: 'gbk' };
}

ipcMain.handle('read-file', async (e, filePath, preferredEncoding) => {
  try {
    const buf = fs.readFileSync(filePath);
    const result = decodeFile(buf, preferredEncoding || 'auto');
    return { ok: true, content: result.content, encoding: result.encoding };
  } catch (err) {
    return { ok: false, error: err.message };
  }
});

ipcMain.handle('write-file', async (e, filePath, content, encoding) => {
  try {
    if (encoding && encoding !== 'utf-8') {
      const buf = iconv.encode(content, encoding === 'utf-8-sig' ? 'utf-8' : encoding);
      if (encoding === 'utf-8-sig') {
        // 加 BOM
        const bom = Buffer.from([0xEF, 0xBB, 0xBF]);
        fs.writeFileSync(filePath, Buffer.concat([bom, buf]));
      } else {
        fs.writeFileSync(filePath, buf);
      }
    } else {
      fs.writeFileSync(filePath, content, 'utf-8');
    }
    return { ok: true };
  } catch (err) {
    return { ok: false, error: err.message };
  }
});

ipcMain.handle('list-dir', async (e, dirPath) => {
  try {
    const items = fs.readdirSync(dirPath, { withFileTypes: true });
    const dirs = items.filter(d => d.isDirectory() && !d.name.startsWith('.')).map(d => d.name).sort();
    const files = items.filter(d => d.isFile()).map(d => d.name).sort();
    return { ok: true, dirs, files, current: dirPath };
  } catch (err) {
    return { ok: false, error: err.message };
  }
});

ipcMain.handle('open-file-dialog', async (e, preferredEncoding) => {
  const r = await dialog.showOpenDialog(mainWin, {
    filters: [{ name: 'CP 源文件', extensions: ['cp'] }, { name: '所有文件', extensions: ['*'] }],
    properties: ['openFile'],
  });
  if (r.canceled) return { ok: false };
  const p = r.filePaths[0];
  const buf = fs.readFileSync(p);
  const result = decodeFile(buf, preferredEncoding || 'auto');
  return { ok: true, path: p, name: path.basename(p), content: result.content, encoding: result.encoding };
});

ipcMain.handle('save-file-dialog', async (e, defaultName) => {
  const r = await dialog.showSaveDialog(mainWin, {
    defaultPath: defaultName,
    filters: [{ name: 'CP 源文件', extensions: ['cp'] }, { name: '所有文件', extensions: ['*'] }],
  });
  if (r.canceled) return { ok: false };
  return { ok: true, path: r.filePath };
});

ipcMain.handle('open-folder-dialog', async () => {
  const r = await dialog.showOpenDialog(mainWin, {
    properties: ['openDirectory'],
  });
  if (r.canceled || !r.filePaths.length) return { ok: false };
  return { ok: true, path: r.filePaths[0] };
});

ipcMain.handle('get-compiler', () => {
  const compiler = findCompiler();
  return { path: compiler, exists: compiler !== null };
});

// ── IPC: 编译器更新 ──

// 获取编译器版本信息
ipcMain.handle('get-compiler-version', async () => {
  const compiler = findCompiler();
  if (!compiler) return { ok: false, version: '未找到' };
  try {
    const out = execSync(`"${compiler}"`, { encoding: 'utf-8', timeout: 5000 });
    const m = out.match(/CP语言编译器\s*v?([\d.]+)/);
    const version = m ? m[1] : '未知';
    return { ok: true, version, path: compiler, size: fs.statSync(compiler).size };
  } catch (e) {
    return { ok: true, version: '未知', path: compiler, size: fs.statSync(compiler).size };
  }
});

// 检查 GitHub 上的最新版本
ipcMain.handle('check-compiler-update', async () => {
  return new Promise((resolve) => {
    const url = 'https://raw.githubusercontent.com/cplang/cplang/main/tools/version.json';
    https.get(url, { timeout: 10000, headers: { 'User-Agent': 'CPIDE' } }, (res) => {
      if (res.statusCode !== 200) {
        resolve({ ok: false, error: `HTTP ${res.statusCode}` });
        return;
      }
      let data = '';
      res.on('data', chunk => data += chunk);
      res.on('end', () => {
        try {
          const info = JSON.parse(data);
          resolve({
            ok: true,
            latestVersion: info.version || '',
            downloadUrl: info.download_url || '',
            releaseNotes: info.release_notes || '',
            compilerUrl: info.compiler_url || '',
          });
        } catch (e) {
          resolve({ ok: false, error: '解析版本信息失败' });
        }
      });
    }).on('error', (e) => {
      resolve({ ok: false, error: e.message });
    });
  });
});

// 下载最新编译器
ipcMain.handle('download-compiler', async (e, downloadUrl) => {
  const compilerPath = findCompiler() || path.join(__dirname, 'cplang.exe');
  return new Promise((resolve) => {
    const url = downloadUrl || 'https://github.com/cplang/cplang/releases/download/latest/cplang-win64.exe';
    const tempFile = compilerPath + '.download';

    // 发送进度到渲染进程
    const sendProgress = (pct) => {
      if (mainWin && !mainWin.isDestroyed()) {
        mainWin.webContents.send('download-progress', pct);
      }
    };

    sendProgress(0);
    const file = fs.createWriteStream(tempFile);
    https.get(url, { timeout: 60000, headers: { 'User-Agent': 'CPIDE' } }, (res) => {
      if (res.statusCode === 302 || res.statusCode === 301) {
        // 重定向
        https.get(res.headers.location, { timeout: 60000, headers: { 'User-Agent': 'CPIDE' } }, (res2) => {
          const total = parseInt(res2.headers['content-length'] || '0', 10);
          let downloaded = 0;
          res2.on('data', (chunk) => {
            downloaded += chunk.length;
            if (total > 0) sendProgress(Math.round((downloaded / total) * 100));
          });
          res2.pipe(file);
          file.on('finish', () => {
            file.close();
            sendProgress(100);
            try {
              // 备份旧版本
              if (fs.existsSync(compilerPath)) {
                fs.renameSync(compilerPath, compilerPath + '.bak');
              }
              fs.renameSync(tempFile, compilerPath);
              resolve({ ok: true });
            } catch (e) {
              resolve({ ok: false, error: e.message });
            }
          });
        }).on('error', (e) => resolve({ ok: false, error: e.message }));
        return;
      }
      const total = parseInt(res.headers['content-length'] || '0', 10);
      let downloaded = 0;
      res.on('data', (chunk) => {
        downloaded += chunk.length;
        if (total > 0) sendProgress(Math.round((downloaded / total) * 100));
      });
      res.pipe(file);
      file.on('finish', () => {
        file.close();
        sendProgress(100);
        try {
          if (fs.existsSync(compilerPath)) {
            fs.renameSync(compilerPath, compilerPath + '.bak');
          }
          fs.renameSync(tempFile, compilerPath);
          resolve({ ok: true });
        } catch (e) {
          resolve({ ok: false, error: e.message });
        }
      });
    }).on('error', (e) => {
      sendProgress(-1);
      resolve({ ok: false, error: e.message });
    });
  });
});

// 从 Git 克隆/更新源码并构建
ipcMain.handle('update-compiler-from-git', async (e, action) => {
  // action: 'clone' | 'pull'
  const homeDir = process.env.CPLANG_HOME || path.join(os.homedir(), 'cplang');
  const repoDir = homeDir;
  const buildDir = path.join(repoDir, 'build');

  try {
    if (action === 'clone' || !fs.existsSync(repoDir)) {
      // 克隆仓库
      const msg = `git clone https://github.com/cplang/cplang.git "${repoDir}"`;
      execSync(msg, { timeout: 120000, stdio: 'pipe', encoding: 'utf-8' });
    } else if (action === 'pull') {
      // 拉取最新
      execSync(`git -C "${repoDir}" pull`, { timeout: 60000, stdio: 'pipe', encoding: 'utf-8' });
    }

    // 构建
    if (fs.existsSync(path.join(repoDir, 'CMakeLists.txt'))) {
      // CMake 构建
      fs.mkdirSync(buildDir, { recursive: true });
      execSync(`cd "${buildDir}" && cmake .. -DCMAKE_BUILD_TYPE=Release`, { timeout: 60000, stdio: 'pipe', encoding: 'utf-8' });
      execSync(`cd "${buildDir}" && cmake --build . --config Release`, { timeout: 300000, stdio: 'pipe', encoding: 'utf-8' });

      const builtExe = path.join(buildDir, 'Release', 'cplang.exe');
      if (fs.existsSync(builtExe)) {
        const target = path.join(__dirname, 'cplang.exe');
        if (fs.existsSync(target)) fs.renameSync(target, target + '.bak');
        fs.copyFileSync(builtExe, target);
        return { ok: true, from: 'cmake' };
      }
    }

    return { ok: false, error: '构建产物未找到' };
  } catch (e) {
    return { ok: false, error: e.message || String(e) };
  }
});

// ── IPC: 文件系统管理 ──

ipcMain.handle('create-file', async (e, parentDir, name) => {
  try {
    const fp = path.join(parentDir, name);
    if (fs.existsSync(fp)) return { ok: false, error: '文件已存在' };
    fs.writeFileSync(fp, '', 'utf-8');
    return { ok: true };
  } catch (err) {
    return { ok: false, error: err.message };
  }
});

ipcMain.handle('create-dir', async (e, parentDir, name) => {
  try {
    const dp = path.join(parentDir, name);
    if (fs.existsSync(dp)) return { ok: false, error: '目录已存在' };
    fs.mkdirSync(dp, { recursive: true });
    return { ok: true };
  } catch (err) {
    return { ok: false, error: err.message };
  }
});

ipcMain.handle('rename-item', async (e, oldPath, newName) => {
  try {
    const dir = path.dirname(oldPath);
    const newPath = path.join(dir, newName);
    if (fs.existsSync(newPath)) return { ok: false, error: '目标已存在' };
    fs.renameSync(oldPath, newPath);
    return { ok: true, newPath };
  } catch (err) {
    return { ok: false, error: err.message };
  }
});

ipcMain.handle('delete-item', async (e, targetPath) => {
  try {
    const stat = fs.statSync(targetPath);
    if (stat.isDirectory()) {
      fs.rmSync(targetPath, { recursive: true, force: true });
    } else {
      fs.unlinkSync(targetPath);
    }
    return { ok: true };
  } catch (err) {
    return { ok: false, error: err.message };
  }
});

// ── IPC: 全文搜索 ──

ipcMain.handle('search-files', async (e, rootDir, query, options) => {
  const results = [];
  const caseSensitive = options?.caseSensitive || false;
  const maxResults = options?.maxResults || 500;

  // 跳过不需要搜索的目录
  function shouldSkipDir(name) {
    return name.startsWith('.') ||
      ['node_modules', 'bin', 'obj', 'build', 'release', '__pycache__'].includes(name);
  }

  // 判断是否为文本文件（根据扩展名）
  function isTextFile(name) {
    const ext = path.extname(name).toLowerCase();
    const textExts = new Set([
      '.cp', '.txt', '.md', '.json', '.js', '.ts', '.html', '.css', '.py',
      '.java', '.c', '.cpp', '.h', '.hpp', '.rs', '.go', '.rb', '.php',
      '.xml', '.yaml', '.yml', '.toml', '.ini', '.cfg', '.conf',
      '.bat', '.sh', '.ps1', '.lua', '.sql', '.svelte', '.vue', '.jsx', '.tsx', '.dart',
      '.cmake', '.makefile', '.gnumakefile', '.log', '.csv', '.tsv',
    ]);
    return textExts.has(ext) || name === 'Makefile' || name === 'CMakeLists.txt';
  }

  let aborted = false;

  function walkDir(dir) {
    if (aborted) return;
    try {
      const items = fs.readdirSync(dir, { withFileTypes: true });
      for (const item of items) {
        if (aborted) return;
        const fullPath = path.join(dir, item.name);
        try {
          if (item.isDirectory()) {
            if (!shouldSkipDir(item.name)) {
              walkDir(fullPath);
            }
          } else if (item.isFile() && isTextFile(item.name)) {
            if (results.length >= maxResults) { aborted = true; return; }
            const buf = fs.readFileSync(fullPath);
            const { content } = decodeFile(buf, 'auto');
            const lines = content.split('\n');
            const searchLine = caseSensitive ? query : query.toLowerCase();
            for (let i = 0; i < lines.length; i++) {
              if (results.length >= maxResults) { aborted = true; return; }
              const lineText = lines[i];
              const matchIn = caseSensitive ? lineText : lineText.toLowerCase();
              const idx = matchIn.indexOf(searchLine);
              if (idx !== -1) {
                results.push({
                  file: fullPath,
                  fileName: item.name,
                  lineNumber: i + 1,
                  column: idx + 1,
                  content: lineText.substring(0, 200), // 截断过长行
                  relativePath: path.relative(rootDir, fullPath).replace(/\\/g, '/'),
                });
              }
            }
          }
        } catch (_) {
          // 跳过无法访问的文件/目录
        }
      }
    } catch (_) {
      // 跳过无法访问的目录
    }
  }

  walkDir(rootDir);
  return { ok: true, results, total: results.length, truncated: aborted };
});

// ── IPC: 在文件中替换 ──
ipcMain.handle('replace-in-files', async (e, rootDir, query, replacement, options) => {
  const caseSensitive = options?.caseSensitive || false;
  let replaceCount = 0;
  let fileCount = 0;
  const errors = [];

  function shouldSkipDir(name) {
    return name.startsWith('.') ||
      ['node_modules', 'bin', 'obj', 'build', 'release', '__pycache__'].includes(name);
  }

  function isTextFile(name) {
    const ext = path.extname(name).toLowerCase();
    const textExts = new Set([
      '.cp', '.txt', '.md', '.json', '.js', '.ts', '.html', '.css', '.py',
      '.java', '.c', '.cpp', '.h', '.hpp', '.rs', '.go', '.rb', '.php',
      '.xml', '.yaml', '.yml', '.toml', '.ini', '.cfg', '.conf',
      '.bat', '.sh', '.ps1', '.lua', '.sql', '.svelte', '.vue', '.jsx', '.tsx', '.dart',
    ]);
    return textExts.has(ext) || name === 'Makefile' || name === 'CMakeLists.txt';
  }

  function walkDir(dir) {
    try {
      const items = fs.readdirSync(dir, { withFileTypes: true });
      for (const item of items) {
        const fullPath = path.join(dir, item.name);
        try {
          if (item.isDirectory()) {
            if (!shouldSkipDir(item.name)) {
              walkDir(fullPath);
            }
          } else if (item.isFile() && isTextFile(item.name)) {
            const buf = fs.readFileSync(fullPath);
            const { content } = decodeFile(buf, 'auto');
            let newContent;
            if (caseSensitive) {
              const regex = new RegExp(query.replace(/[.*+?^${}()|[\]\\]/g, '\\$&'), 'g');
              newContent = content.replace(regex, replacement);
            } else {
              const regex = new RegExp(query.replace(/[.*+?^${}()|[\]\\]/g, '\\$&'), 'gi');
              newContent = content.replace(regex, replacement);
            }
            if (newContent !== content) {
              const replaced = (content.match(caseSensitive ? new RegExp(query.replace(/[.*+?^${}()|[\]\\]/g, '\\$&'), 'g') : new RegExp(query.replace(/[.*+?^${}()|[\]\\]/g, '\\$&'), 'gi')) || []).length;
              replaceCount += replaced;
              fileCount++;
              fs.writeFileSync(fullPath, newContent, 'utf-8');
            }
          }
        } catch (_) {
          errors.push(fullPath);
        }
      }
    } catch (_) {
      errors.push(dir);
    }
  }

  walkDir(rootDir);
  return { ok: true, replaceCount, fileCount, errors: errors.length > 0 ? errors : undefined };
});

// ── IPC: 设置 ──

const settingsPath = path.join(__dirname, 'settings.json');

// ── Monaco Editor 本地协议 ──
// 将 monaco:// 请求映射到本地 node_modules/monaco-editor/ 目录
// 注意：路径可能以 vs/ 开头（AMD loader 模块请求）或 min-maps/ 开头（源映射）
// 文件实际在 min/vs/ 或 min-maps/ 等子目录中
const MONACO_BASE = path.join(__dirname, 'node_modules', 'monaco-editor');

function registerMonacoProtocol() {
  try {
    protocol.handle('monaco', (request) => {
      const url = new URL(request.url);
      // monaco://vs/loader.js → host='vs', pathname='/loader.js' → 目标 min/vs/loader.js
      // monaco:///min-maps/vs/loader.js.map → host='', pathname='/min-maps/vs/loader.js.map'
      let relativePath = (url.host || '') + url.pathname;
      if (relativePath.startsWith('/')) relativePath = relativePath.slice(1);
      // 尝试多个可能路径（AMD loader 可能以不同方式解析路径）
      const stripped = relativePath.replace(/^vs\//, '');
      const candidates = [
        path.join(MONACO_BASE, 'min', relativePath),           // min/vs/...
        path.join(MONACO_BASE, relativePath),                  // 直接 monaco-editor/...
        path.join(MONACO_BASE, 'min', stripped),               // min/xxx (去掉 vs/ 前缀)
        path.join(MONACO_BASE, stripped),                      // monaco-editor/xxx (去掉 vs/ 前缀)
      ];
      let lastErr = null;
      for (const fp of candidates) {
        try {
          const content = fs.readFileSync(fp);
          const ext = path.extname(fp).toLowerCase();
          const mimeMap = {
            '.js': 'application/javascript',
            '.css': 'text/css',
            '.html': 'text/html',
            '.ttf': 'font/ttf',
            '.woff': 'font/woff',
            '.woff2': 'font/woff2',
            '.png': 'image/png',
            '.svg': 'image/svg+xml',
            '.json': 'application/json',
            '.map': 'application/json',
          };
          const mime = mimeMap[ext] || 'application/octet-stream';
          return new Response(content, {
            status: 200,
            headers: { 'Content-Type': mime, 'Access-Control-Allow-Origin': '*' },
          });
        } catch (e) { lastErr = e; }
      }
      console.error('❌ Monaco 协议读取失败:', relativePath, lastErr?.message);
      return new Response('Not Found', { status: 404 });
    });
    console.log('✅ Monaco 本地协议已注册: monaco://');
  } catch (e) {
    console.warn('⚠️ Monaco 协议注册失败，将回退到 CDN:', e.message);
  }
}

function loadSettings() {
  try {
    if (fs.existsSync(settingsPath)) {
      const raw = fs.readFileSync(settingsPath, 'utf-8');
      return JSON.parse(raw);
    }
  } catch (_) { /* 忽略损坏文件 */ }
  return {}; // 返回空对象，使用默认值
}

ipcMain.handle('get-settings', async () => {
  return loadSettings();
});

ipcMain.handle('save-settings', async (e, newSettings) => {
  try {
    const current = loadSettings();
    const merged = { ...current, ...newSettings };
    fs.writeFileSync(settingsPath, JSON.stringify(merged, null, 2), 'utf-8');
    return { ok: true };
  } catch (err) {
    return { ok: false, error: err.message };
  }
});

// ── IPC: 编译 ──

let currentRunProcess = null; // 跟踪当前运行进程，用于中断

// 解析编译器错误输出中的行号/列号
// 格式: 第<行>行第<列>列: <消息>
function parseErrors(output) {
  const errors = [];
  const regex = /第(\d+)行第(\d+)列:\s*(.+)/g;
  let match;
  while ((match = regex.exec(output)) !== null) {
    errors.push({
      line: parseInt(match[1], 10),
      column: parseInt(match[2], 10),
      message: match[3].trim(),
    });
  }
  return errors;
}

ipcMain.handle('compile-run', async (e, filePath) => {
  const compiler = findCompiler();
  if (!compiler) return { ok: false, output: '❌ 编译器未找到', errors: [] };
  try {
    // 使用 spawn 以便后续可中断
    const proc = spawn(`"${compiler}"`, ['-c', `"${filePath}"`], {
      shell: true,
      timeout: 30000,
      env: process.env,
    });
    currentRunProcess = proc;

    let stdout = '';
    let stderr = '';

    proc.stdout.on('data', (data) => { stdout += data.toString(); });
    proc.stderr.on('data', (data) => { stderr += data.toString(); });

    const exitCode = await new Promise((resolve) => {
      proc.on('close', (code) => resolve(code));
      proc.on('error', (err) => { stderr += err.message; resolve(1); });
    });

    currentRunProcess = null;
    const out = stdout + stderr;
    const errors = parseErrors(out);
    return { ok: exitCode === 0, output: out || '(无输出)', exitCode, errors };
  } catch (err) {
    currentRunProcess = null;
    const out = (err.stdout || '') + (err.stderr || '');
    const errors = parseErrors(out);
    return { ok: false, output: out || err.message, exitCode: err.status || 1, errors };
  }
});

ipcMain.handle('stop-run', async () => {
  if (currentRunProcess) {
    try {
      currentRunProcess.kill('SIGTERM');
      // Windows 上 kill 树
      if (process.platform === 'win32') {
        execSync(`taskkill /PID ${currentRunProcess.pid} /T /F`, { stdio: 'ignore' });
      }
      currentRunProcess = null;
      return { ok: true };
    } catch (e) {
      return { ok: false, error: e.message };
    }
  }
  return { ok: false, error: '没有正在运行的进程' };
});

ipcMain.handle('compile-check', async (e, filePath) => {
  const compiler = findCompiler();
  if (!compiler) return { ok: false, output: '❌ 编译器未找到', errors: [] };
  try {
    const stdout = execSync(`"${compiler}" -p "${filePath}"`, {
      encoding: 'utf-8',
      timeout: 30000,
      env: process.env,
    });
    return { ok: true, output: stdout || '✅ 语法检查通过', exitCode: 0, errors: [] };
  } catch (err) {
    const out = (err.stdout || '') + (err.stderr || '');
    const errors = parseErrors(out);
    return { ok: false, output: out || err.message, exitCode: err.status || 1, errors };
  }
});

// ── IPC: AOT 编译为原生可执行文件 ──
ipcMain.handle('compile-aot', async (e, filePath) => {
  const compiler = findCompiler();
  if (!compiler) return { ok: false, output: '❌ 编译器未找到', errors: [] };
  try {
    const outPath = filePath.replace(/\.cp$/i, '') + '.exe';
    // 设置环境变量 LIB 以包含 CP 标准库，让链接器能解析 stdlib/raylib 符号
    const env = Object.assign({}, process.env);
    const libDirs = [
      'C:\\cplang\\build_cmake_new\\Release',
      'C:\\cplang\\third_party\\raylib\\build_release\\raylib\\Release',
    ];
    const existingLib = env.LIB || '';
    env.LIB = libDirs.join(';') + (existingLib ? ';' + existingLib : '');
    const stdout = execSync(`"${compiler}" -a -o "${outPath}" "${filePath}"`, {
      encoding: 'utf-8',
      timeout: 120000,
      env: env,
    });
    return { ok: true, output: stdout || `✅ 编译成功: ${outPath}`, exitCode: 0, errors: [], exePath: outPath };
  } catch (err) {
    const out = (err.stdout || '') + (err.stderr || '');
    const errors = parseErrors(out);
    return { ok: false, output: out || err.message, exitCode: err.status || 1, errors };
  }
});

// ── IPC: 打包为独立 exe（嵌入运行时） ──
ipcMain.handle('package-exe', async (e, filePath) => {
  try {
    const compiler = findCompiler();
    if (!compiler) return { ok: false, output: '❌ 编译器未找到' };
    const cppackPath = findCppack();
    if (!cppackPath) return { ok: false, output: '❌ 打包器未找到 (cppack.exe)' };

    // 查找 Visual Studio 的 vcvarsall.bat
    const pf86 = process.env['ProgramFiles(x86)'] || 'C:\\Program Files (x86)';
    const pf = process.env.ProgramFiles || 'C:\\Program Files';
    const vsSearch = [
      ['2022', 'Community'], ['2022', 'Professional'], ['2022', 'Enterprise'],
      ['2019', 'Community'], ['2019', 'Professional'], ['2019', 'Enterprise'],
    ];
    let vcvars = '';
    for (const [ver, ed] of vsSearch) {
      for (const base of [pf86, pf]) {
        const p = path.join(base, 'Microsoft Visual Studio', ver, ed, 'VC', 'Auxiliary', 'Build', 'vcvarsall.bat');
        if (fs.existsSync(p)) { vcvars = p; break; }
      }
      if (vcvars) break;
    }

    const outPath = filePath.replace(/\.cp$/i, '') + '_独立.exe';

    // 构建命令：在同一 cmd.exe 中先加载 VS 环境，再运行 cppack
    let cmd;
    if (vcvars) {
      cmd = `"${vcvars}" x64 >nul 2>&1 && "${cppackPath}" "${compiler}" "${filePath}" "${outPath}"`;
    } else {
      cmd = `"${cppackPath}" "${compiler}" "${filePath}" "${outPath}"`;
    }

    // 执行并捕获原始 Buffer，解决中文编码问题
    const buf = execSync(cmd, { timeout: 180000, shell: true, maxBuffer: 100 * 1024 * 1024, encoding: 'buffer' });
    let output = '';
    try {
      output = iconv.decode(buf, 'utf-8');
      if (output.indexOf('�') >= 0) output = iconv.decode(buf, 'gbk');
    } catch (_) {
      try { output = iconv.decode(buf, 'gbk'); } catch (_2) { output = buf.toString('utf-8'); }
    }
    return { ok: true, output: output || `✅ 打包成功: ${outPath}`, exePath: outPath };
  } catch (err) {
    // 捕获 stderr/stdout 并正确解码
    let fullBuf = Buffer.alloc(0);
    if (err.stdout) fullBuf = Buffer.concat([fullBuf, typeof err.stdout === 'string' ? Buffer.from(err.stdout) : err.stdout]);
    if (err.stderr) fullBuf = Buffer.concat([fullBuf, typeof err.stderr === 'string' ? Buffer.from(err.stderr) : err.stderr]);
    let output = '';
    try {
      output = iconv.decode(fullBuf, 'utf-8');
      if (output.indexOf('�') >= 0) output = iconv.decode(fullBuf, 'gbk');
    } catch (_) {
      try { output = iconv.decode(fullBuf, 'gbk'); } catch (_2) { output = fullBuf.toString('utf-8'); }
    }
    return { ok: false, output: output || err.message };
  }
});

// ── IPC: 调试器（TCP 调试服务器） ──

const tcpNet = require('net');  // 注意: 不与 Electron 的 net 冲突

let debugProcess = null;
let debugSocket = null;
let debugOutput = '';
let debugLineBuffer = '';
let debugSessionId = 0;

function sendDebugCommand(cmd) {
  if (!debugSocket || debugSocket.destroyed) return;
  debugSocket.write(JSON.stringify(cmd) + '\n');
}

ipcMain.handle('debug-start', async (e, filePath, breakpoints, sessionId) => {
  const compiler = findCompiler();
  if (!compiler) return { ok: false, output: '❌ 编译器未找到' };

  // 停止已有调试会话
  if (debugProcess) { try { debugProcess.kill(); } catch (_) {} debugProcess = null; }
  if (debugSocket) { try { debugSocket.destroy(); } catch (_) {} debugSocket = null; }

  debugOutput = '';
  debugLineBuffer = '';
  debugSessionId = sessionId || Date.now();

  const debugPort = 4711 + (debugSessionId % 1000); // 避免端口冲突

  try {
    // 启动编译器（调试服务器模式）
    const proc = spawn(`"${compiler}"`, ['--debug-server', String(debugPort), '-c', `"${filePath}"`], {
      shell: true,
      env: process.env,
    });
    debugProcess = proc;
    currentRunProcess = proc;

    proc.stdout.on('data', (data) => {
      const text = data.toString();
      debugOutput += text;
      try { mainWin.webContents.send('debug-output', text); } catch (_) {}
    });

    proc.stderr.on('data', (data) => {
      const text = data.toString();
      debugOutput += text;
      try { mainWin.webContents.send('debug-output', text); } catch (_) {}
    });

    proc.on('exit', (code) => {
      debugProcess = null;
      if (debugSocket) { try { debugSocket.destroy(); } catch (_) {} debugSocket = null; }
      const errors = parseErrors(debugOutput);
      try {
        mainWin.webContents.send('debug-end', {
          exitCode: code, output: debugOutput, errors, _sessionId: debugSessionId,
        });
      } catch (_) {}
    });

    proc.on('error', (err) => {
      debugProcess = null;
      if (debugSocket) { try { debugSocket.destroy(); } catch (_) {} debugSocket = null; }
      try {
        mainWin.webContents.send('debug-end', {
          exitCode: 1, output: debugOutput + `\n错误: ${err.message}`,
          errors: [{ message: err.message, line: 0, column: 0 }], _sessionId: debugSessionId,
        });
      } catch (_) {}
    });

    // 连接调试服务器（等待编译器启动）
    return new Promise((resolve) => {
      let attempts = 0;
      const tryConnect = () => {
        attempts++;
        if (attempts > 30) {
          resolve({ ok: false, output: '❌ 调试服务器连接超时' });
          return;
        }
        const sock = new tcpNet.Socket();
        sock.connect(debugPort, '127.0.0.1', () => {
          debugSocket = sock;
          // 发送断点
          if (breakpoints && breakpoints.length > 0) {
            const lines = breakpoints.map(b => b.line).filter(l => l > 0);
            const file = breakpoints[0].file || filePath;
            sendDebugCommand({ cmd: 'setBreakpoints', file, lines });
          }
          // 启动事件监听
          sock.on('data', (data) => {
            const text = data.toString();
            debugLineBuffer += text;
            while (true) {
              const nl = debugLineBuffer.indexOf('\n');
              if (nl < 0) break;
              const line = debugLineBuffer.slice(0, nl).trim();
              debugLineBuffer = debugLineBuffer.slice(nl + 1);
              if (!line) continue;
              try {
                const msg = JSON.parse(line);
                if (msg.type === 'paused') {
                  try { mainWin.webContents.send('debug-paused', msg); } catch (_) {}
                } else if (msg.type === 'stack') {
                  try { mainWin.webContents.send('debug-stack', msg); } catch (_) {}
                } else if (msg.type === 'variables') {
                  try { mainWin.webContents.send('debug-variables', msg); } catch (_) {}
                } else if (msg.type === 'connected') {
                  resolve({ ok: true, message: '调试会话已启动，已连接到 VM' });
                }
              } catch (_) {}
            }
          });
          sock.on('close', () => { debugSocket = null; });
          sock.on('error', () => { debugSocket = null; });
        });
        sock.on('error', () => {
          setTimeout(tryConnect, 300);
        });
      };
      setTimeout(tryConnect, 500); // 等待编译器启动
    });
  } catch (err) {
    debugProcess = null;
    return { ok: false, output: err.message };
  }
});

ipcMain.handle('debug-stop', async () => {
  if (debugSocket) { sendDebugCommand({ cmd: 'continue' }); try { debugSocket.destroy(); } catch (_) {} debugSocket = null; }
  if (debugProcess) {
    try { debugProcess.kill('SIGTERM'); if (process.platform === 'win32') { execSync(`taskkill /PID ${debugProcess.pid} /T /F`, { stdio: 'ignore' }); } } catch (_) {}
    debugProcess = null; currentRunProcess = null;
  }
  return { ok: true };
});

ipcMain.handle('debug-continue', async () => {
  if (debugSocket) { sendDebugCommand({ cmd: 'continue' }); return { ok: true }; }
  return { ok: false, message: '没有活动的调试会话' };
});

ipcMain.handle('debug-step-over', async () => {
  if (debugSocket) { sendDebugCommand({ cmd: 'stepOver' }); return { ok: true }; }
  return { ok: false, message: '没有活动的调试会话' };
});

ipcMain.handle('debug-step-into', async () => {
  if (debugSocket) { sendDebugCommand({ cmd: 'stepInto' }); return { ok: true }; }
  return { ok: false, message: '没有活动的调试会话' };
});

ipcMain.handle('debug-step-out', async () => {
  if (debugSocket) { sendDebugCommand({ cmd: 'stepOut' }); return { ok: true }; }
  return { ok: false, message: '没有活动的调试会话' };
});

ipcMain.handle('debug-get-stack', async () => {
  if (debugSocket) { sendDebugCommand({ cmd: 'getStack' }); return { ok: true }; }
  return { ok: false };
});

ipcMain.handle('debug-get-vars', async () => {
  if (debugSocket) { sendDebugCommand({ cmd: 'getVars' }); return { ok: true }; }
  return { ok: false };
});

// ── IPC: REPL 交互式执行（真·增量模式） ──

let replProcess = null;      // cplang -r 持久进程
let replPending = null;      // 等待 REPL 输出的 resolve 函数
let replBuffer = '';         // 累积输出缓冲区
let replHistory = [];        // 当前会话历史

function startReplProcess() {
  if (replProcess) return true;
  const compiler = findCompiler();
  if (!compiler) return false;

  replProcess = spawn(`"${compiler}"`, ['-r'], {
    shell: true,
    stdio: ['pipe', 'pipe', 'pipe'],
    env: process.env,
  });

  replProcess.stdout.on('data', (data) => {
    const text = data.toString();
    replBuffer += text;
    // 检查是否有等待的请求
    if (replPending) {
      const { resolve, collectCb } = replPending;
      if (collectCb) collectCb(text);
      // 检测 REPL 提示符 (> 或 请输入) 表示一轮输入完成
      if (text.includes('> ') || text.includes('>>>') || text.includes('请输入')) {
        const out = replBuffer;
        replBuffer = '';
        replPending = null;
        resolve({ ok: true, output: cleanReplOutput(out) });
      }
    }
  });

  replProcess.stderr.on('data', (data) => {
    replBuffer += data.toString();
  });

  replProcess.on('exit', () => {
    replProcess = null;
    if (replPending) {
      const { resolve } = replPending;
      replPending = null;
      const out = replBuffer; replBuffer = '';
      resolve({ ok: true, output: cleanReplOutput(out) + '\n[REPL 进程已退出]' });
    }
  });

  replProcess.on('error', (err) => {
    replProcess = null;
    if (replPending) {
      const { resolve } = replPending;
      replPending = null;
      resolve({ ok: false, output: `REPL 错误: ${err.message}` });
    }
  });

  return true;
}

function cleanReplOutput(raw) {
  return raw
    .split('\n')
    .filter(l => !/^cplang|^CP语言|版本|请输入|^> /i.test(l.trim()))
    .join('\n')
    .trim();
}

ipcMain.handle('repl-exec', async (e, code) => {
  if (!startReplProcess()) {
    return { ok: false, output: '❌ 编译器未找到' };
  }

  return new Promise((resolve) => {
    const timeout = setTimeout(() => {
      if (replPending) {
        const out = replBuffer; replBuffer = '';
        replPending = null;
        resolve({ ok: true, output: cleanReplOutput(out) || '(超时)' });
      }
    }, 15000);

    replPending = {
      resolve: (result) => {
        clearTimeout(timeout);
        if (result.ok) {
          replHistory.push({ code, output: result.output || '(无输出)' });
        }
        resolve(result);
      },
      collectCb: null,
    };

    // 发送代码到 REPL
    try {
      replProcess.stdin.write(code + '\n');
    } catch (err) {
      clearTimeout(timeout);
      replPending = null;
      resolve({ ok: false, output: `写入错误: ${err.message}` });
    }
  });
});

ipcMain.handle('repl-clear', async () => {
  replHistory = [];
  // 重启 REPL 进程以获得干净状态
  if (replProcess) {
    try { replProcess.kill(); } catch (_) {}
    replProcess = null;
    replBuffer = '';
    replPending = null;
  }
  return { ok: true };
});

ipcMain.handle('repl-history', async () => {
  return { ok: true, history: replHistory.slice(-50) };
});

// ── 终端集成 ──

let terminalProcess = null;

ipcMain.handle('terminal-start', async (e, cwd) => {
  try {
    // 如果已有终端进程，先终止
    if (terminalProcess) {
      terminalProcess.kill();
      terminalProcess = null;
    }

    const shell = process.platform === 'win32' ? 'cmd.exe' : '/bin/bash';
    const shellArgs = process.platform === 'win32' ? [] : ['-i'];

    terminalProcess = spawn(shell, shellArgs, {
      cwd: cwd || __dirname,
      stdio: ['pipe', 'pipe', 'pipe'],
      env: process.env,
      windowsHide: false,
    });

    terminalProcess.stdout.on('data', (data) => {
      try { mainWin.webContents.send('terminal-data', data.toString()); } catch (_) {}
    });

    terminalProcess.stderr.on('data', (data) => {
      try { mainWin.webContents.send('terminal-data', data.toString()); } catch (_) {}
    });

    terminalProcess.on('exit', (code) => {
      try { mainWin.webContents.send('terminal-exit', code); } catch (_) {}
      terminalProcess = null;
    });

    terminalProcess.on('error', (err) => {
      try { mainWin.webContents.send('terminal-data', `\r\n\x1b[31m[终端错误] ${err.message}\x1b[0m\r\n`); } catch (_) {}
    });

    return { ok: true };
  } catch (err) {
    return { ok: false, error: err.message };
  }
});

ipcMain.handle('terminal-write', async (e, data) => {
  try {
    if (terminalProcess && terminalProcess.stdin.writable) {
      terminalProcess.stdin.write(data);
      return { ok: true };
    }
    return { ok: false, error: '终端未运行' };
  } catch (err) {
    return { ok: false, error: err.message };
  }
});

ipcMain.handle('terminal-resize', async (e, cols, rows) => {
  // 实际 PTY 调整大小需要 node-pty，此处暂不实现
  return { ok: true };
});

ipcMain.handle('terminal-kill', async () => {
  try {
    if (terminalProcess) {
      terminalProcess.kill();
      terminalProcess = null;
      return { ok: true };
    }
    return { ok: false, error: '终端未运行' };
  } catch (err) {
    return { ok: false, error: err.message };
  }
});

// ── Git 集成 ──

function findGit() {
  const paths = [
    'C:\\Program Files\\Git\\bin\\git.exe',
    'C:\\Program Files (x86)\\Git\\bin\\git.exe',
    path.join(process.env.LOCALAPPDATA || '', 'Programs\\Git\\bin\\git.exe'),
    path.join(process.env.USERPROFILE || '', 'AppData\\Local\\Programs\\Git\\bin\\git.exe'),
  ];
  for (const p of paths) {
    if (fs.existsSync(p)) return p;
  }
  // 尝试从 PATH 查找
  try {
    const which = execSync('where git.exe', { encoding: 'utf-8', stdio: ['pipe', 'pipe', 'ignore'] }).trim();
    if (which) return which.split('\n')[0].trim();
  } catch (_) {}
  return null;
}

function gitExec(args, cwd) {
  return new Promise((resolve) => {
    const gitPath = findGit();
    if (!gitPath) return resolve({ ok: false, error: 'Git 未安装' });
    exec(`"${gitPath}" ${args}`, { cwd: cwd || __dirname, encoding: 'utf-8', maxBuffer: 10 * 1024 * 1024 },
      (err, stdout, stderr) => {
        if (err) return resolve({ ok: false, error: stderr || err.message, output: stdout });
        resolve({ ok: true, output: stdout.trim() });
      }
    );
  });
}

ipcMain.handle('git-check', async () => {
  const gitPath = findGit();
  if (!gitPath) return { ok: false, installed: false, error: 'Git 未安装' };
  try {
    const ver = execSync(`"${gitPath}" --version`, { encoding: 'utf-8' }).trim();
    return { ok: true, installed: true, path: gitPath, version: ver };
  } catch (_) {
    return { ok: false, installed: false, error: 'Git 不可用' };
  }
});

ipcMain.handle('git-status', async (e, repoPath) => {
  const r = await gitExec('status --porcelain -u', repoPath || __dirname);
  if (!r.ok) return r;
  const files = [];
  r.output.split('\n').forEach(line => {
    if (!line.trim()) return;
    const status = line.substring(0, 2).trim();
    const filePath = line.substring(3).trim();
    let state = 'unchanged';
    if (status === 'M' || status === 'M ') state = 'modified';
    else if (status === 'A' || status === 'A ') state = 'added';
    else if (status === 'D' || status === 'D ') state = 'deleted';
    else if (status === 'R' || status === 'R ') state = 'renamed';
    else if (status === 'C' || status === 'C ') state = 'copied';
    else if (status === '??') state = 'untracked';
    else if (status[1] === 'M') state = 'staged-modified';
    else if (status[1] === 'A') state = 'staged-added';
    else if (status[1] === 'D') state = 'staged-deleted';
    files.push({ path: filePath, state, raw: line.substring(0, 2) });
  });
  return { ok: true, files, gitAvailable: true };
});

ipcMain.handle('git-add', async (e, repoPath, filePath) => {
  return await gitExec(`add "${filePath}"`, repoPath || __dirname);
});

ipcMain.handle('git-commit', async (e, repoPath, message) => {
  return await gitExec(`commit -m "${message.replace(/"/g, '\\"')}"`, repoPath || __dirname);
});

ipcMain.handle('git-diff', async (e, repoPath, filePath) => {
  return await gitExec(`diff "${filePath}"`, repoPath || __dirname);
});

ipcMain.handle('git-log', async (e, repoPath, count) => {
  return await gitExec(`log --oneline -${count || 10}`, repoPath || __dirname);
});

ipcMain.handle('git-branch', async (e, repoPath) => {
  const r = await gitExec('branch', repoPath || __dirname);
  if (!r.ok) return r;
  const branches = r.output.split('\n').map(b => {
    const name = b.replace(/^\*?\s*/, '');
    return { name, current: b.startsWith('*') };
  });
  return { ok: true, branches, current: branches.find(b => b.current)?.name || '' };
});

// ── IDE 自动更新 ──

const IDE_VERSION = '1.1.1-beta';

ipcMain.handle('check-ide-update', async () => {
  return new Promise((resolve) => {
    const req = https.get('https://api.github.com/repos/cplang/cplang/releases/latest', {
      headers: { 'User-Agent': 'CP-IDE/' + IDE_VERSION, 'Accept': 'application/json' },
      timeout: 10000,
    }, (res) => {
      let body = '';
      res.on('data', d => body += d);
      res.on('end', () => {
        try {
          const data = JSON.parse(body);
          if (data.tag_name) {
            const latest = data.tag_name.replace(/^v/, '');
            const hasUpdate = compareVersions(latest, IDE_VERSION) > 0;
            resolve({
              ok: true,
              hasUpdate,
              currentVersion: IDE_VERSION,
              latestVersion: latest,
              releaseUrl: data.html_url || '',
              downloadUrl: data.assets?.[0]?.browser_download_url || '',
              releaseNotes: data.body ? data.body.substring(0, 500) : '',
            });
          } else {
            resolve({ ok: true, hasUpdate: false, currentVersion: IDE_VERSION, latestVersion: IDE_VERSION });
          }
        } catch (e) {
          resolve({ ok: false, error: '解析失败: ' + e.message });
        }
      });
    });
    req.on('error', (e) => resolve({ ok: false, error: '网络错误: ' + e.message }));
    req.on('timeout', () => { req.destroy(); resolve({ ok: false, error: '请求超时' }); });
  });
});

function compareVersions(a, b) {
  const pa = a.split('.').map(Number);
  const pb = b.split('.').map(Number);
  for (let i = 0; i < Math.max(pa.length, pb.length); i++) {
    const va = pa[i] || 0;
    const vb = pb[i] || 0;
    if (va > vb) return 1;
    if (va < vb) return -1;
  }
  return 0;
}

ipcMain.handle('download-ide-update', async (e, url) => {
  return new Promise((resolve) => {
    if (!url) return resolve({ ok: false, error: '下载链接为空' });
    const downloadPath = path.join(app.getPath('downloads'), 'CP-IDE-更新.exe');
    const file = fs.createWriteStream(downloadPath);
    const req = https.get(url, {
      headers: { 'User-Agent': 'CP-IDE/' + IDE_VERSION },
      timeout: 30000,
    }, (res) => {
      const total = parseInt(res.headers['content-length'] || '0');
      let downloaded = 0;
      res.on('data', (chunk) => {
        downloaded += chunk.length;
        const pct = total > 0 ? Math.round(downloaded / total * 100) : -1;
        try { mainWin.webContents.send('download-progress', pct); } catch (_) {}
      });
      res.pipe(file);
      file.on('finish', () => {
        file.close();
        resolve({ ok: true, path: downloadPath });
      });
    });
    req.on('error', (e) => { file.close(); fs.unlinkSync(downloadPath); resolve({ ok: false, error: e.message }); });
    req.on('timeout', () => { req.destroy(); file.close(); resolve({ ok: false, error: '下载超时' }); });
  });
});

ipcMain.handle('install-ide-update', async (e, installerPath) => {
  try {
    require('child_process').exec(`"${installerPath}"`, { detached: true });
    app.quit();
    return { ok: true };
  } catch (err) {
    return { ok: false, error: err.message };
  }
});

// ── 会话（Session）持久化 ──

ipcMain.handle('save-session', async (e, sessionData) => {
  try {
    fs.writeFileSync(sessionPath, JSON.stringify(sessionData, null, 2), 'utf-8');
    return { ok: true };
  } catch (err) {
    return { ok: false, error: err.message };
  }
});

ipcMain.handle('load-session', async () => {
  try {
    if (!fs.existsSync(sessionPath)) return { ok: true, data: null };
    const raw = fs.readFileSync(sessionPath, 'utf-8');
    const data = JSON.parse(raw);
    return { ok: true, data };
  } catch (err) {
    return { ok: true, data: null }; // 静默失败
  }
});

// ── 示例管理 ──
const EXAMPLES_DIR = path.join(__dirname, '..', '..', 'examples');

ipcMain.handle('list-examples', async () => {
  try {
    const result = [];
    function scanDir(dir, category) {
      if (!fs.existsSync(dir)) return;
      const items = fs.readdirSync(dir, { withFileTypes: true });
      for (const item of items) {
        if (item.name.endsWith('.cp') || item.name.endsWith('.md')) {
          const fullPath = path.join(dir, item.name);
          result.push({
            name: item.name,
            path: fullPath,
            category: category || '基础',
            size: fs.statSync(fullPath).size,
          });
        } else if (item.isDirectory() && !item.name.startsWith('.')) {
          const subCat = category ? `${category}/${item.name}` : item.name;
          scanDir(path.join(dir, item.name), subCat);
        }
      }
    }
    scanDir(EXAMPLES_DIR, null);
    return { ok: true, examples: result };
  } catch (e) {
    return { ok: false, error: e.message, examples: [] };
  }
});

ipcMain.handle('read-example', async (e, filePath) => {
  try {
    const content = fs.readFileSync(filePath, 'utf-8');
    return { ok: true, content, name: path.basename(filePath), path: filePath };
  } catch (err) {
    return { ok: false, error: err.message };
  }
});

// ── LSP 集成 ──

let lspProcess = null;
let lspBuffer = '';
let lspSeq = 0;
const lspPending = new Map(); // id → { resolve, reject }

function startLSP() {
  const serverScript = path.join(__dirname, '..', 'vscode-cp', 'cplsp.js');
  if (!fs.existsSync(serverScript)) {
    console.warn('[LSP] cplsp.js not found at', serverScript);
    return;
  }
  try {
    lspProcess = spawn('node', [serverScript], {
      cwd: path.join(__dirname, '..', '..'),
      stdio: ['pipe', 'pipe', 'pipe'],
      env: Object.assign({}, process.env, {
        CPLANG_HOME: process.env.CPLANG_HOME || path.join(os.homedir(), 'cplang'),
      }),
    });

    lspProcess.stdout.on('data', (data) => {
      lspBuffer += data.toString();
      while (true) {
        const m = lspBuffer.match(/^Content-Length: (\d+)\r\n\r\n/);
        if (!m) break;
        const len = parseInt(m[1], 10);
        const hdrEnd = lspBuffer.indexOf('\r\n\r\n') + 4;
        if (lspBuffer.length < hdrEnd + len) break;
        const body = lspBuffer.slice(hdrEnd, hdrEnd + len);
        lspBuffer = lspBuffer.slice(hdrEnd + len);
        try {
          const msg = JSON.parse(body);
          processLSPMessage(msg);
        } catch (_) {}
      }
    });

    lspProcess.stderr.on('data', (d) => console.error('[LSP stderr]', d.toString().trim()));
    lspProcess.on('exit', (c) => { console.log(`[LSP] exited: ${c}`); lspProcess = null; });
    lspProcess.on('error', (e) => { console.error('[LSP] error:', e.message); lspProcess = null; });

    // 发送 initialize 请求
    sendLSP({ jsonrpc: '2.0', id: nextLspId(), method: 'initialize', params: {
      processId: process.pid,
      capabilities: {},
      rootUri: null,
    }});
    sendLSP({ jsonrpc: '2.0', method: 'initialized', params: {} });
    console.log('[LSP] 语言服务器已启动');
  } catch (e) {
    console.error('[LSP] 启动失败:', e.message);
  }
}

function nextLspId() { return ++lspSeq; }

function sendLSP(msg) {
  if (!lspProcess || !lspProcess.stdin.writable) return;
  const body = JSON.stringify(msg);
  lspProcess.stdin.write(`Content-Length: ${Buffer.byteLength(body)}\r\n\r\n${body}`);
}

function processLSPMessage(msg) {
  // 处理响应
  if (msg.id && lspPending.has(msg.id)) {
    const { resolve } = lspPending.get(msg.id);
    lspPending.delete(msg.id);
    resolve(msg.result);
    return;
  }
  // 处理通知：publishDiagnostics → 转发到渲染进程
  if (msg.method === 'textDocument/publishDiagnostics') {
    try { mainWin?.webContents.send('lsp-diagnostics', msg.params); } catch (_) {}
  }
}

// IPC: 渲染进程请求 LSP 诊断
ipcMain.handle('lsp-didOpen', async (e, uri, text) => {
  if (!lspProcess) return;
  sendLSP({ jsonrpc: '2.0', method: 'textDocument/didOpen', params: {
    textDocument: { uri, languageId: 'cp', version: 1, text }
  }});
});

ipcMain.handle('lsp-didChange', async (e, uri, text) => {
  if (!lspProcess) return;
  sendLSP({ jsonrpc: '2.0', method: 'textDocument/didChange', params: {
    textDocument: { uri, version: Date.now() },
    contentChanges: [{ text }]
  }});
});

ipcMain.handle('lsp-completion', async (e, uri, line, col) => {
  if (!lspProcess) return [];
  return new Promise((resolve) => {
    const id = nextLspId();
    const timeout = setTimeout(() => { lspPending.delete(id); resolve([]); }, 3000);
    lspPending.set(id, { resolve: (r) => { clearTimeout(timeout); resolve(r?.items || r || []); } });
    sendLSP({ jsonrpc: '2.0', id, method: 'textDocument/completion', params: {
      textDocument: { uri }, position: { line, character: col }
    }});
  });
});

ipcMain.handle('lsp-hover', async (e, uri, line, col) => {
  if (!lspProcess) return null;
  return new Promise((resolve) => {
    const id = nextLspId();
    const timeout = setTimeout(() => { lspPending.delete(id); resolve(null); }, 2000);
    lspPending.set(id, { resolve: (r) => { clearTimeout(timeout); resolve(r); } });
    sendLSP({ jsonrpc: '2.0', id, method: 'textDocument/hover', params: {
      textDocument: { uri }, position: { line, character: col }
    }});
  });
});

// ── 应用启动 ──

app.whenReady().then(() => {
  registerMonacoProtocol();
  createWindow();
  startLSP();  // 启动语言服务器


  // 窗口关闭时自动保存会话并清理终端/LSP
  mainWin.on('close', () => {
    mainWin.webContents.send('cmd', 'save-session');
    if (terminalProcess) { try { terminalProcess.kill(); } catch (_) {} terminalProcess = null; }
    if (lspProcess) {
      sendLSP({ jsonrpc: '2.0', method: 'shutdown', params: {} });
      sendLSP({ jsonrpc: '2.0', method: 'exit', params: {} });
      setTimeout(() => { try { lspProcess.kill(); } catch (_) {} }, 500);
    }
  });

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit();
});
