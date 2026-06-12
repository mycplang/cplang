导入 "@cp/graphics";  // 图形模块 (Raylib 2D/3D + ImGui)
// ═══════════════════════════════════════════════════════
//  CP Desktop v1.0 — 桌面Shell (替代传统桌面环境)
//  桌面+任务栏+开始菜单+窗口管理+文件浏览
// ═══════════════════════════════════════════════════════

// ── 状态 ──
变量 g_desktop_width = 1280;
变量 g_desktop_height = 720;
变量 g_taskbar_height = 36;
变量 g_bg_color = 1712430;

// 窗口管理
变量 g_windows = [];  // [{title, x, y, w, h, open, type}]
变量 g_active_window = 0;
变量 g_show_start_menu = 假;
变量 g_show_context_menu = 假;
变量 g_context_x = 0;
变量 g_context_y = 0;

// 桌面图标
变量 g_desktop_items = [
    "📁 文件管理",
    "📝 文本编辑", 
    "💻 终端",
    "⚙️ 设置",
    "🧮 计算器",
    "📦 应用商店"
];

// ── 桌面 ──
函数 渲染桌面() {
    // 背景
    igPushStyleColor(0, g_bg_color);  // ImGuiCol_WindowBg
    igSetNextWindowPos(0, 0, 0);
    igSetNextWindowSize(g_desktop_width, g_desktop_height - g_taskbar_height, 0);
    igBegin("桌面", 0, 263);  // NoTitleBar|NoResize|NoMove|NoScrollbar|NoBringToFrontOnFocus
    
    // 桌面图标
    x = 20; y = 20;
    i = 0;
    当 (i < 长度(g_desktop_items)) {
        igSetCursorPos(x, y);
        如果 (igButton(g_desktop_items[i], 64, 64)) {
            启动应用(i);
        }
        y = y + 80;
        如果 (y > g_desktop_height - 120) {
            y = 20;
            x = x + 80;
        }
        i = i + 1;
    }
    
    // 右键菜单
    如果 (g_show_context_menu) {
        igOpenPopup("桌面菜单", 0);
        如果 (igBeginPopup("桌面菜单", 0)) {
            如果 (igMenuItem("刷新", "", 假, 真)) {
                g_show_context_menu = 假;
            }
            如果 (igMenuItem("新建文件", "", 假, 真)) {
                追加(g_windows, ["新建文件.cp", 100, 100, 600, 400, 真, "editor"]);
            }
            如果 (igMenuItem("打开终端", "", 假, 真)) {
                追加(g_windows, ["终端", 150, 150, 700, 450, 真, "terminal"]);
            }
            igSeparator();
            如果 (igMenuItem("设置", "", 假, 真)) {
                追加(g_windows, ["设置", 300, 200, 500, 400, 真, "settings"]);
            }
            igEndPopup();
        }
    }
    
    igEnd();
    igPopStyleColor(1);
}

// ── 窗口渲染 ──
函数 渲染窗口(win) {
    title = win[0]; x = win[1]; y = win[2]; w = win[3]; h = win[4];
    open = win[5]; win_type = win[6];
    
    如果 (!open) { 返回; }
    
    igSetNextWindowPos(x, y, 0);
    igSetNextWindowSize(w, h, 0);
    
    flags = 8;  // ImGuiWindowFlags_NoCollapse
    closed = 0;
    visible = igBegin(title, 0, flags);
    
    如果 (visible) {
        如果 (win_type == "editor") {
            渲染编辑器(win);
        }
        如果 (win_type == "filemgr") {
            渲染文件管理(win);
        }
        如果 (win_type == "terminal") {
            渲染终端(win);
        }
        如果 (win_type == "settings") {
            渲染设置(win);
        }
        如果 (win_type == "calc") {
            渲染计算器(win);
        }
    }
    
    igEnd();
    
    如果 (!visible || closed) {
        win[5] = 假;  // 标记关闭
    }
}

// ── 编辑器 ──
变量 g_edit_buf = "";
变量 g_edit_file = "";
变量 g_edit_lines = [""];

函数 渲染编辑器(win) {
    如果 (g_edit_buf == "") {
        g_edit_buf = "// CP 语言\n打印(\"你好，世界！\");\n";
    }
    igInputTextMultiline("##editor", g_edit_buf, 8192, 0, 0, 0);
}

// ── 文件管理 ──
变量 g_fm_path = "/home/cplang";
变量 g_fm_files = [];

函数 渲染文件管理(win) {
    g_fm_path = igInputText("路径", g_fm_path, 256, 0);
    
    如果 (igButton("📂 打开", 0, 0)) {
        g_fm_files = 目录列表(g_fm_path);
    }
    
    如果 (igButton("⬆ 上级", 0, 0)) {
        g_fm_path = 路径目录(g_fm_path);
        g_fm_files = 目录列表(g_fm_path);
    }
    
    igSeparator();
    
    如果 (g_fm_files == nil) { g_fm_files = 目录列表(g_fm_path); }
    
    i = 0;
    当 (i < 长度(g_fm_files)) {
        f = g_fm_files[i];
        如果 (f != "." && f != "..") {
            如果 (igButton(f, 0, 0)) {
                full = g_fm_path + "/" + f;
                如果 (文件存在(full)) {
                    // 是文件→打开编辑器
                    g_edit_file = full;
                    g_edit_buf = 读取文件(full);
                    追加(g_windows, [f, 100, 100, 600, 400, 真, "editor"]);
                }
            }
        }
        i = i + 1;
    }
}

// ── 终端 ──
变量 g_term_buf = "";
变量 g_term_fd = 0;
变量 g_term_pid = 0;
变量 g_term_input = "";

函数 渲染终端(win) {
    如果 (g_term_fd == 0) {
        // 启动 shell
        g_term_fd = 终端打开();
        如果 (g_term_fd > 0) {
            g_term_buf = "CP Desktop 终端 v1.0\n$ ";
        }
    }
    
    // 终端输出
    igInputTextMultiline("##termout", g_term_buf, 16384, 0, 0, 32);
    
    // 输入行
    
    如果 (igInputText("##termin", g_term_input, 256, 32, 0)) {
        如果 (字符串包含(g_term_input, "\n")) {
            cmd = g_term_input;
            g_term_buf = g_term_buf + cmd;
            如果 (g_term_fd > 0) {
                终端写入(g_term_fd, cmd);
                延时(100);
                out = 终端读取(g_term_fd);
                g_term_buf = g_term_buf + out;
            }
            g_term_input = "";
            g_term_buf = g_term_buf + "$ ";
        }
    }
    
}

// ── 设置 ──
函数 渲染设置(win) {
    igText("主题颜色:");
    如果 (igColorEdit3("背景", 0, 0)) { }
    如果 (igButton("暗色", 0, 0)) { g_bg_color = 1712430; }
    igSameLine(0, 8);
    如果 (igButton("深蓝", 0, 0)) { g_bg_color = 855319; }
    igSameLine(0, 8);
    如果 (igButton("经典", 0, 0)) { g_bg_color = 2832940; }
    
    igSeparator();
    igText("桌面快捷键:");
    igText("  Win      — 开始菜单");
    igText("  Win+E   — 文件管理");
    igText("  Win+T   — 终端");
    igText("  Win+R   — 运行");
    igText("  Alt+F4  — 关闭窗口");
    
    igSeparator();
    igText("系统信息:");
    igText("  CP Desktop v1.0");
    igText("  stdlib: 348 functions");
    igText("  github.com/mycplang/cplang");
}

// ── 计算器 ──
变量 g_calc_input = "";
变量 g_calc_result = "";

函数 渲染计算器(win) {
    g_calc_input = igInputText("##calcinput", g_calc_input, 64, 0);
    如果 (igButton("  C  ", 0, 0)) { g_calc_input = ""; g_calc_result = ""; }
    
    buttons = ["7","8","9","/","4","5","6","*","1","2","3","-","0",".","=","+"];
    i = 0;
    当 (i < 16) {
        如果 (igButton(buttons[i], 40, 40)) {
            btn = buttons[i];
            如果 (btn == "=") {
                如果 (字符串包含(g_calc_input, "+")) {
                    parts = 字符串分割(g_calc_input, "+");
                    g_calc_result = 整数转字符串(转整数(parts[0]) + 转整数(parts[1]));
                }
                如果 (字符串包含(g_calc_input, "-")) {
                    parts = 字符串分割(g_calc_input, "-");
                    g_calc_result = 整数转字符串(转整数(parts[0]) - 转整数(parts[1]));
                }
                如果 (字符串包含(g_calc_input, "*")) {
                    parts = 字符串分割(g_calc_input, "*");
                    g_calc_result = 整数转字符串(转整数(parts[0]) * 转整数(parts[1]));
                }
                如果 (字符串包含(g_calc_input, "/")) {
                    parts = 字符串分割(g_calc_input, "/");
                    g_calc_result = 整数转字符串(转整数(parts[0]) / 转整数(parts[1]));
                }
            } 否则 {
                g_calc_input = g_calc_input + btn;
            }
        }
        如果 (i % 4 != 3) { igSameLine(0, 4); }
        i = i + 1;
    }
    
    如果 (g_calc_result != "") {
        igText("= " + g_calc_result);
    }
}

// ── 开始菜单 ──
函数 渲染开始菜单() {
    如果 (!g_show_start_menu) { 返回; }
    
    igSetNextWindowPos(0, g_desktop_height - g_taskbar_height - 320, 0);
    igSetNextWindowSize(280, 320, 0);
    
    如果 (igBegin("开始菜单", 0, 264)) {  // NoResize|NoMove|NoTitleBar|NoScrollbar
        menu_items = [
            ["📁 文件管理", "filemgr"],
            ["📝 文本编辑", "editor"],
            ["💻 终端", "terminal"],
            ["⚙️ 设置", "settings"],
            ["🧮 计算器", "calc"],
            ["📦 应用商店", "store"]
        ];
        
        i = 0;
        当 (i < 长度(menu_items)) {
            item = menu_items[i];
            如果 (igButton(item[0], 0, 0)) {
                启动应用(item[1]);
                g_show_start_menu = 假;
            }
            i = i + 1;
        }
        
        igSeparator();
        如果 (igButton("关机 / 退出", 0, 0)) {
            // 退出桌面
            g_running = 假;
        }
    }
    igEnd();
}

// ── 任务栏 ──
变量 g_clock = "";

函数 渲染任务栏() {
    taskbar_y = g_desktop_height - g_taskbar_height;
    
    igSetNextWindowPos(0, taskbar_y, 0);
    igSetNextWindowSize(g_desktop_width, g_taskbar_height, 0);
    
    igBegin("任务栏", 0, 519);  // NoTitleBar|NoResize|NoMove|NoScrollbar|NoBringToFrontOnFocus
    
    // 开始按钮
    如果 (igButton("🅲 开始", 0, 0)) {
        g_show_start_menu = !g_show_start_menu;
    }
    
    igSameLine(0, 8);
    igSeparator();
    igSameLine(0, 8);
    
    // 已打开窗口标签
    i = 0;
    当 (i < 长度(g_windows)) {
        win = g_windows[i];
        如果 (win[5]) {  // open
            如果 (igButton(win[0], 0, 0)) {
                g_active_window = i;
            }
            igSameLine(0, 4);
        }
        i = i + 1;
    }
    
    // 时钟 - 右侧
    igSameLine(g_desktop_width - 80, 0);
    如果 (g_clock == "") {
        g_clock = 子串(procSystem("date +%H:%M"), 0, 5);
    }
    igText(g_clock);
    
    igEnd();
}

// ── 应用启动器 ──
函数 启动应用(type) {
    如果 (type == "filemgr") {
        追加(g_windows, ["文件管理", 50, 50, 700, 500, 真, "filemgr"]);
        g_fm_files = 目录列表(g_fm_path);
    }
    如果 (type == "editor") {
        追加(g_windows, ["文本编辑", 100, 100, 600, 400, 真, "editor"]);
    }
    如果 (type == "terminal") {
        追加(g_windows, ["终端", 150, 150, 700, 450, 真, "terminal"]);
    }
    如果 (type == "settings") {
        追加(g_windows, ["设置", 300, 200, 500, 400, 真, "settings"]);
    }
    如果 (type == "calc") {
        追加(g_windows, ["计算器", 400, 250, 240, 320, 真, "calc"]);
    }
    如果 (type == "store") {
        追加(g_windows, ["应用商店", 200, 100, 600, 450, 真, "settings"]);
    }
}

// ═══════════════════════════════════════════════════════
//  主循环
// ═══════════════════════════════════════════════════════

变量 g_running = 真;

// ── 初始化窗口和界面 ──
初始化窗口(g_desktop_width, g_desktop_height, "CP Desktop v1.0");
igInit();
设置目标帧率(60);

// 默认打开文件管理
追加(g_windows, ["文件管理", 50, 50, 700, 500, 真, "filemgr"]);
g_fm_files = 目录列表(g_fm_path);

当 (g_running) {
    // 获取显示大小
    如果 (1 != 0) {
        // 使用默认 1280x720
    }
    
    // 渲染桌面
    渲染桌面();
    
    // 渲染所有窗口
    i = 0;
    当 (i < 长度(g_windows)) {
        渲染窗口(g_windows[i]);
        i = i + 1;
    }
    
    // 渲染开始菜单
    渲染开始菜单();
    
    // 渲染任务栏
    渲染任务栏();
    
    // 清理关闭的窗口
    new_wins = [];
    i = 0;
    当 (i < 长度(g_windows)) {
        如果 (g_windows[i][5]) {  // open
            追加(new_wins, g_windows[i]);
        }
        i = i + 1;
    }
    g_windows = new_wins;
}
