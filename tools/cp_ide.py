#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""CP IDE v2.5.0 - VS Code 风格界面 + 活动栏 + 命令面板 + 智能补全 + 括号匹配增强"""

import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import os, subprocess, re, sys, threading, json, urllib.request, shutil

IDE_VERSION = "2.4.1"
IDE_VERSION_URL = "https://raw.githubusercontent.com/cplang/cplang/main/tools/version.json"
IDE_DOWNLOAD_BASE = "https://github.com/cplang/cplang/releases/download/latest"

CP_KEYWORDS = sorted([
    # 函数/变量
    "函数","function","fn","异步","async","变量","var","常量","const",
    "设","let","可变","mutable",
    # 控制流
    "如果","if","否则","else","否则如果","当","while","循环","for",
    "遍历","foreach","为","do","跳出","break","继续","continue",
    "返回","return",
    # 选择/匹配
    "选择","switch","情况","case","其他","default",
    "匹配","match",
    # 异常
    "尝试","try","捕获","catch","抛出","throw","最终","finally",
    # 推迟
    "推迟","defer",
    # OOP
    "类","class","结构体","struct","枚举","enum","接口","interface",
    "实现","extends","新建","new","这个","this","继承","super",
    # 访问修饰符
    "公有","public","私有","private","保护","protected",
    "静态","static","虚拟","virtual","重写","override","抽象","abstract",
    # 模块
    "导入","import","包名","package",
    # 等待
    "等待","await",
    # 布尔/空
    "真","true","假","false","空","nil","null",
    # 类型工具
    "类型","typeof","是","is",
], key=len, reverse=True)

CP_TYPES = sorted([
    "整数","int","布尔","bool","浮点","float","文本","string",
    "变体","字节","数组","表","void","var","auto",
    "i8","i16","i32","i64","u8","u16","u32","u64",
    "f32","f64","char","byte",
], key=len, reverse=True)

KW_RE = re.compile("|".join(rf"\b{re.escape(k)}\b" for k in CP_KEYWORDS))
TYPE_RE = re.compile("|".join(rf"\b{re.escape(t)}\b" for t in CP_TYPES))

# ── 主题 ──
THEMES = [
    {"name":"暗黑","bg":"#1e1e1e","fg":"#d4d4d4","ln":"#858585","sel":"#264f78",
     "keyword":"#569cd6","type":"#4ec9b0","string":"#ce9178","comment":"#6a9955","number":"#b5cea8",
     "tab_bg":"#2d2d2d","tab_act":"#3c3c3c","output_bg":"#1e1e1e","line_bg":"#1e1e1e","cursor":"#aeafad",
     "sb_bg":"#007acc","sb_fg":"white"},
    {"name":"亮白","bg":"#ffffff","fg":"#333333","ln":"#cccccc","sel":"#add6ff",
     "keyword":"#0000ff","type":"#267f99","string":"#a31515","comment":"#008000","number":"#098658",
     "tab_bg":"#ececec","tab_act":"#ffffff","output_bg":"#f8f8f8","line_bg":"#f0f0f0","cursor":"#000000",
     "sb_bg":"#e0e0e0","sb_fg":"#333333"},
    {"name":"森林","bg":"#1e2e1e","fg":"#c8dcc8","ln":"#5a7a5a","sel":"#2d4f2d",
     "keyword":"#7ecf7e","type":"#4ec9b0","string":"#ce9178","comment":"#6a9955","number":"#b5cea8",
     "tab_bg":"#2d3d2d","tab_act":"#3d5d3d","output_bg":"#1e2e1e","line_bg":"#1e2e1e","cursor":"#aeafad",
     "sb_bg":"#2d5a2d","sb_fg":"#c8dcc8"},
    {"name":"海洋","bg":"#0d1b2a","fg":"#c8d8e8","ln":"#4a6a8a","sel":"#1b3d5c",
     "keyword":"#66b0ff","type":"#4ecfb0","string":"#dba87a","comment":"#5a8a7a","number":"#8ac8a8",
     "tab_bg":"#1a2a3a","tab_act":"#2a4a6a","output_bg":"#0d1b2a","line_bg":"#0d1b2a","cursor":"#aeafad",
     "sb_bg":"#1a3a5a","sb_fg":"#c8d8e8"},
]

# 所有内置函数名（用于补全 — 同步自 VS Code 插件 v0.4.0）
CP_BUILTINS = [
    # 数学
    "绝对值","平方根","幂","向下取整","向上取整","四舍五入",
    "正弦","余弦","正切","反正弦","反余弦","反正切","圆周率","自然常数",
    "随机值","阶乘","均值","中位数","标准差","abs","sqrt","pow",
    "floor","ceil","round","sin","cos","tan","random","pi","e",
    # IO
    "打印","print","println","输入","input","读取文件","写入文件","文件存在",
    "readFile","writeFile","fileExists",
    # 字符串
    "长度","len","子串","substr","连接","concat","查找","find","替换","replace",
    "分割","split","修剪","trim","小写","lower","大写","upper",
    "包含","contains","格式化","format","strlen",
    # 转换
    "转字符串","toString","转整数","parseInt","转浮点","parseFloat",
    "JSON解析","jsonParse","转JSON","jsonStringify",
    # 数组
    "追加","push","弹出","pop","插入","insert","移除","remove","切片","slice",
    "排序","sort","反转","reverse","映射","map","过滤","filter","累积","reduce",
    "去重","unique","展平","flatten","打包","zip","头出","shift","头插","unshift",
    # 表
    "表创建","table","表设","tableSet","表取","tableGet","表有","has",
    "表长","tblen","表键","keys","表值","values","表删","tableDel",
    "表清空","tableClear","表合并","tableMerge","表转数组","tableToArray",
    # 类型
    "是空","isNil","是布尔","isBool","是整数","isInt","是浮点","isFloat",
    "是字符串","isString","是数组","isArray","是函数","isFunction","取类型","typeOf",
    # 时间/系统
    "时间戳","now","延时","sleep","计时器","tick","平台","platform",
    "程序退出","exit","环境变量","getEnv","当前目录","cwd","执行命令","exec",
    "进程ID","pid","CPU核数","cpuCount",
    # 容器
    "栈创建","stackCreate","入栈","stackPush","出栈","stackPop",
    "队列创建","queueCreate","入队","queuePush","出队","queuePop",
    "集合新建","setNew","集合添加","setAdd","集合包含","setHas",
    # 并发
    "通道创建","channelCreate","通道发送","channelSend","通道接收","channelRecv",
    "互斥创建","mutexCreate","互斥加锁","mutexLock","互斥解锁","mutexUnlock",
    "线程创建","threadCreate","线程等待","threadJoin","异步执行","futureGo",
    # 网络
    "HTTP获取","httpGet","HTTP提交","httpPost","HTTP下载","httpDownload",
    "TCP连接","tcpConnect","TCP发送","tcpSend","TCP接收","tcpRecv","TCP关闭","tcpClose",
    "TCP监听","tcpListen",
    # 加密
    "MD5","md5","SHA256","sha256","Base64编码","base64Encode","Base64解码","base64Decode",
    "UUID","uuid4","AES加密","aesEncrypt","AES解密","aesDecrypt",
    # 数据库
    "数据库打开","sqliteOpen","数据库查询","sqliteQuery","数据库执行","sqliteExec",
    "数据库关闭","sqliteClose","数据库错误","sqliteErrMsg",
    "MySQL连接","mysqlConnect","MySQL查询","mysqlQuery","MySQL关闭","mysqlClose",
    "Redis连接","redisConnect","Redis获取","redisGet","Redis设置","redisSet",
    # 图形
    "初始化窗口","设置目标帧率","窗口应关闭","开始绘图","结束绘图","关闭窗口",
    "清空背景","绘制文本","绘制矩形","绘制圆形","绘制帧率","键盘按下",
    # ImGui
    "igInit","igBegin","igEnd","igButton","igText","igInputText","igSameLine",
    "igSeparator","igBeginMenuBar","igEndMenuBar","igBeginMenu","igEndMenu",
    "igMenuItem","igBeginChild","igEndChild","igBeginTabBar","igEndTabBar",
    "igBeginTabItem","igEndTabItem","igInputTextMultiline",
    "igTreeNode","igTreePop","igCollapsingHeader",
    "igIsItemClicked","igIsItemHovered","igIsKeyDown","igIsKeyPressed",
    "igPushStyleColor","igPopStyleColor",
    # 算法
    "稳定排序","二分查找","全排列","位与","bitAnd","位或","bitOr","位异或","bitXor",
    # 正则
    "正则匹配","regexMatch","正则搜索","regexSearch","正则替换","regexReplace",
    "正则分割","regexSplit",
]

class Tab:
    def __init__(self, name="未命名.cp", path=None, content=""):
        self.name = name; self.path = path
        self.content = content; self.modified = False


class FindReplaceBar:
    def __init__(self, parent, text_widget):
        self.text = text_widget; self.frame = tk.Frame(parent, bg="#252526")
        self.visible = False; self.term = tk.StringVar()
        self.repl = tk.StringVar(); self.matches = []; self.cur = -1

        r1 = tk.Frame(self.frame, bg="#252526"); r1.pack(fill=tk.X, padx=6, pady=4)
        tk.Label(r1, text="🔍 查找:", bg="#252526", fg="#d4d4d4",
                font=("Segoe UI", 9)).pack(side=tk.LEFT)
        e1 = tk.Entry(r1, textvariable=self.term, width=28,
                     bg="#3c3c3c", fg="white", relief=tk.FLAT, bd=0,
                     insertbackground="white", font=("Segoe UI", 9))
        e1.pack(side=tk.LEFT, padx=4, ipady=2)
        e1.bind("<Return>", lambda e: self.find(1))
        e1.bind("<Shift-Return>", lambda e: self.find(-1))
        def mk_small_btn(parent, text, cmd):
            btn = tk.Button(parent, text=text, command=cmd, bg="#3c3c3c", fg="#d4d4d4",
                           relief=tk.FLAT, bd=0, padx=6, pady=1, cursor="hand2",
                           activebackground="#505050", activeforeground="white",
                           font=("Segoe UI", 9))
            btn.pack(side=tk.LEFT, padx=1)
            btn.bind("<Enter>", lambda e: btn.config(bg="#4a4a4a"))
            btn.bind("<Leave>", lambda e: btn.config(bg="#3c3c3c"))
            return btn
        mk_small_btn(r1, "▼", lambda: self.find(1))
        mk_small_btn(r1, "▲", lambda: self.find(-1))
        self.label = tk.Label(r1, text="", bg="#252526", fg="#ce9178",
                             font=("Segoe UI", 9)); self.label.pack(side=tk.LEFT, padx=6)
        mk_small_btn(r1, "✕", self.hide).pack(side=tk.RIGHT)

        r2 = tk.Frame(self.frame, bg="#252526"); r2.pack(fill=tk.X, padx=6, pady=(0,4))
        tk.Label(r2, text="✏ 替换:", bg="#252526", fg="#d4d4d4",
                font=("Segoe UI", 9)).pack(side=tk.LEFT)
        tk.Entry(r2, textvariable=self.repl, width=28,
                bg="#3c3c3c", fg="white", relief=tk.FLAT, bd=0,
                insertbackground="white", font=("Segoe UI", 9)).pack(side=tk.LEFT, padx=4, ipady=2)
        mk_small_btn(r2, "替换", self.replace_one)
        mk_small_btn(r2, "全部", self.replace_all)

    def show(self):
        if not self.visible:
            self.frame.pack(fill=tk.X, before=self.text); self.visible = True
        self.term.set("")
        try:
            s = self.text.selection_get()
            if s: self.term.set(s)
        except: pass
        self.find(1)

    def hide(self):
        if self.visible:
            self.frame.pack_forget(); self.visible = False
            self.text.tag_remove("fm", "1.0", tk.END)

    def find(self, direction=1):
        self.text.tag_remove("fm", "1.0", tk.END)
        t = self.term.get()
        if not t: return
        c = self.text.get("1.0", tk.END)
        self.matches = []; pos = 0
        while True:
            i = c.find(t, pos)
            if i < 0: break
            self.matches.append(i); pos = i + len(t)
        if not self.matches:
            self.label.config(text="未找到"); return
        
        cur = self.text.index(tk.INSERT)
        cp = self.text.count("1.0", cur)[0] if cur != "1.0" else 0
        
        if direction > 0:
            self.cur = 0
            for i, p in enumerate(self.matches):
                if p >= cp: self.cur = i; break
        else:
            self.cur = len(self.matches) - 1
            for i, p in enumerate(reversed(self.matches)):
                if p <= cp: self.cur = len(self.matches)-1-i; break
        
        p = self.matches[self.cur]
        line = c[:p].count("\n") + 1; col = p - c[:p].rfind("\n") - 1
        self.text.see(f"{line}.0")
        self.text.tag_remove("sel", "1.0", tk.END)
        self.text.tag_add("sel", f"{line}.{col}", f"{line}.{col+len(t)}")
        self.text.mark_set(tk.INSERT, f"{line}.{col+len(t)}")
        self.label.config(text=f"{self.cur+1}/{len(self.matches)}")
        
        for p in self.matches:
            line = c[:p].count("\n") + 1; col = p - c[:p].rfind("\n") - 1
            self.text.tag_add("fm", f"{line}.{col}", f"{line}.{col+len(t)}")
        self.text.tag_configure("fm", background="#6a3e00")

    def replace_one(self):
        if self.cur < 0: self.find(1); return
        t = self.term.get(); r = self.repl.get()
        p = self.matches[self.cur]
        c = self.text.get("1.0", tk.END)
        line = c[:p].count("\n") + 1; col = p - c[:p].rfind("\n") - 1
        self.text.delete(f"{line}.{col}", f"{line}.{col+len(t)}")
        self.text.insert(f"{line}.{col}", r)
        d = len(r) - len(t)
        self.matches = [m + d if m > p else m for m in self.matches]
        self.find(1)

    def replace_all(self):
        t = self.term.get(); r = self.repl.get()
        if not t: return
        c = self.text.get("1.0", tk.END)
        n = c.replace(t, r)
        self.text.delete("1.0", tk.END)
        self.text.insert("1.0", n.rstrip("\n"))
        self.matches = []; self.cur = -1; self.label.config(text="")

class CPIDE:
    def __init__(self, root):
        self.root = root
        self.root.title("CP IDE v2.0")
        self.root.geometry("1300x850")
        # 优先使用内置编译器
        self.ide_dir = os.path.dirname(os.path.abspath(__file__))
        self.bundled_compiler = os.path.join(self.ide_dir, "cplang.exe")
        self.cplang_home = os.environ.get("CPLANG_HOME") or os.path.join(os.path.expanduser("~"), "cplang")
        self.compiler = self.bundled_compiler
        if not os.path.exists(self.compiler):
            self.compiler = os.path.join(self.cplang_home, "build", "cplang.exe")
        if not os.path.exists(self.compiler):
            alt = os.path.join(self.cplang_home, "build_llvm", "bin", "Release", "cplang.exe")
            if os.path.exists(alt): self.compiler = alt
        self.compiler_ok = os.path.exists(self.compiler)
        self.tabs = [Tab()]; self.cur_tab = 0
        self.theme_idx = 0; self.recent_files = []
        self.in_completion = False
        self.font_size = 12; self.wrap_mode = False
        self.error_marks = []; self.file_encoding = "utf-8"
        self.setup_style(); self.setup_menu(); self.setup_ui()
        self.setup_statusbar(); self.setup_shortcuts()
        self.log("╔══════════════════════════════════════╗", "info")
        self.log(f"║   CP IDE v{IDE_VERSION} — CP语言开发环境       ║", "info")
        self.log("╚══════════════════════════════════════╝", "info")
        self.log(f"🔧 编译器: {'✅ 内置' if os.path.exists(self.bundled_compiler) else '系统'} | {self.compiler}", "info")
        self.log(f"📂 CPLANG_HOME: {self.cplang_home}", "")
        self.log(f"💡 F5 运行 | Ctrl+F 查找 | Ctrl+S 保存", "")
        self.refresh_tree(); self.update_tabs(); self.load_tab()
        self.start_file_watcher()
        # 后台检查更新
        threading.Thread(target=self.check_update, daemon=True).start()

    def setup_style(self):
        self.style = ttk.Style(); self.style.theme_use("clam")
        # ── 全局 ttk 样式 ──
        # 滚动条
        self.style.configure("Vertical.TScrollbar", background="#3c3c3c", troughcolor="#1e1e1e",
                             arrowcolor="#858585", bordercolor="#1e1e1e", relief=tk.FLAT)
        self.style.configure("Horizontal.TScrollbar", background="#3c3c3c", troughcolor="#1e1e1e",
                             arrowcolor="#858585", bordercolor="#1e1e1e", relief=tk.FLAT)
        # Treeview
        self.style.configure("Treeview", background="#2d2d2d", foreground="#d4d4d4",
                             fieldbackground="#2d2d2d", borderwidth=0, font=("Segoe UI", 9))
        self.style.map("Treeview", background=[("selected", "#264f78")],
                       foreground=[("selected", "white")])
        self.style.configure("Treeview.Item", background="#2d2d2d", foreground="#d4d4d4")
        # ttk 按钮
        self.style.configure("TButton", background="#3c3c3c", foreground="white",
                             borderwidth=1, focusthickness=0, font=("Segoe UI", 9))
        self.style.map("TButton", background=[("active", "#505050"), ("pressed", "#2d2d2d")])
        # ttk 标签
        self.style.configure("TLabel", background="#1e1e1e", foreground="#d4d4d4")
        self.style.configure("TFrame", background="#1e1e1e")
        # ttk 条目 (Entry)
        self.style.configure("TEntry", fieldbackground="#3c3c3c", foreground="white",
                             borderwidth=1, font=("Segoe UI", 9))
        self.apply_theme()

    def apply_theme(self):
        """应用当前主题到编辑器"""
        t = THEMES[self.theme_idx % len(THEMES)]
        if hasattr(self, 'text') and self.text:
            self.text.config(bg=t["bg"], fg=t["fg"], insertbackground=t["cursor"])
        if hasattr(self, 'ln') and self.ln:
            self.ln.config(bg=t["line_bg"])
        if hasattr(self, 'out') and self.out:
            self.out.config(bg=t["output_bg"])
        # 重设高亮标签颜色
        if hasattr(self, 'text') and self.text:
            self.text.tag_configure("keyword", foreground=t["keyword"])
            self.text.tag_configure("type", foreground=t["type"])
            self.text.tag_configure("string", foreground=t["string"])
            self.text.tag_configure("comment", foreground=t["comment"])
            self.text.tag_configure("number", foreground=t["number"])
        # 状态栏主题
        if hasattr(self, 'sl') and self.sl:
            self.sl.master.config(bg=t["sb_bg"])
            self.sl.config(bg=t["sb_bg"], fg=t["sb_fg"])
            self.sr.config(bg=t["sb_bg"], fg=t["sb_fg"])
            self.enc_label.config(bg=t["sb_bg"], fg=t["sb_fg"])
        self.theme_name = t["name"]

    def cycle_theme(self):
        """切换主题"""
        self.theme_idx = (self.theme_idx + 1) % len(THEMES)
        self.apply_theme()
        self.highlight()
        self.log(f"主题: {THEMES[self.theme_idx]['name']}", "info")

    def setup_menu(self):
        m = tk.Menu(self.root); self.root.config(menu=m)
        fm = tk.Menu(m, tearoff=0)
        m.add_cascade(label="文件", menu=fm)
        fm.add_command(label="新建 Ctrl+N", command=self.new_file)
        fm.add_command(label="打开 Ctrl+O", command=self.open_file)
        fm.add_command(label="保存 Ctrl+S", command=self.save_file)
        fm.add_command(label="另存为 Ctrl+Shift+S", command=self.save_as)
        fm.add_separator()
        # 最近文件
        self.recent_menu = tk.Menu(fm, tearoff=0)
        fm.add_cascade(label="最近文件", menu=self.recent_menu)
        fm.add_separator()
        fm.add_command(label="关闭标签 Ctrl+W", command=self.close_tab)
        fm.add_command(label="退出", command=self.root.quit)
        em = tk.Menu(m, tearoff=0)
        m.add_cascade(label="编辑", menu=em)
        em.add_command(label="撤销 Ctrl+Z", command=lambda: self.text.edit_undo())
        em.add_command(label="重做 Ctrl+Y", command=lambda: self.text.edit_redo())
        em.add_separator()
        em.add_command(label="查找替换 Ctrl+F", command=self.show_find)
        em.add_command(label="转到行 Ctrl+G", command=self.goto_line)
        em.add_separator()
        em.add_command(label="插入代码片段 Ctrl+I", command=self.insert_snippet)
        rm = tk.Menu(m, tearoff=0)
        m.add_cascade(label="运行", menu=rm)
        rm.add_command(label="编译运行 F5", command=self.run_file)
        rm.add_command(label="仅编译 Ctrl+F7", command=self.build_file)
        vm = tk.Menu(m, tearoff=0)
        m.add_cascade(label="视图", menu=vm)
        vm.add_command(label="命令面板 Ctrl+Shift+P", command=self.command_palette)
        vm.add_separator()
        vm.add_command(label="切换侧边栏 Ctrl+B", command=self.toggle_sidebar)
        vm.add_command(label="切换输出面板 Ctrl+J", command=self.toggle_output)
        vm.add_command(label="切换活动栏 Ctrl+Shift+B", command=self.toggle_activity_bar)
        vm.add_separator()
        vm.add_command(label="切换主题", command=self.cycle_theme)
        vm.add_command(label="自动换行", command=self.toggle_wrap)
        vm.add_command(label="字体放大  Ctrl++", command=lambda: self.zoom_in())
        vm.add_command(label="字体缩小  Ctrl+-", command=lambda: self.zoom_out())
        tm = tk.Menu(m, tearoff=0)
        m.add_cascade(label="工具", menu=tm)
        tm.add_command(label="命令面板 Ctrl+Shift+P", command=self.command_palette)
        tm.add_separator()
        tm.add_command(label="环境信息", command=self.env_info)
        tm.add_command(label="项目中查找 Ctrl+Shift+F", command=self.project_search)

    def setup_ui(self):
        self.main_h = tk.Frame(self.root, bg="#1e1e1e")
        self.main_h.pack(fill=tk.BOTH, expand=True)
        self.setup_activity_bar()
        self.pane = tk.PanedWindow(self.main_h, orient=tk.HORIZONTAL, sashrelief=tk.FLAT, sashwidth=4, bg="#2d2d2d")
        self.pane.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        self.setup_tree(); self.setup_editor()

    def setup_activity_bar(self):
        """VS Code 风格的活动栏（左侧图标栏）"""
        self.ab = tk.Frame(self.main_h, bg="#333333", width=48)
        self.ab.pack(side=tk.LEFT, fill=tk.Y)
        self.ab.pack_propagate(False)
        self.active_view = "explorer"
        self.ab_visible = True

        # 右侧分割线（活动栏与侧边栏的分界）
        tk.Frame(self.ab, bg="#474747", width=1).pack(side=tk.RIGHT, fill=tk.Y)

        # 图标按钮定义： (图标, 命令, 视图名)
        ab_items = [
            ("📁", self.toggle_sidebar, "explorer"),
            ("🔍", self.toggle_search, "search"),
            ("📋", self.toggle_outline_view, "outline"),
            ("🔀", self.show_git, "git"),
        ]
        self.ab_views = {}
        for icon, cmd, view in ab_items:
            btn = tk.Button(self.ab, text=icon, command=cmd, bg="#333333", fg="#cccccc",
                           relief=tk.FLAT, bd=0, padx=0, pady=10, cursor="hand2",
                           activebackground="#3c3c3c", activeforeground="white",
                           font=("Segoe UI", 14))
            btn.pack(fill=tk.X, padx=4, pady=1)
            btn.bind("<Enter>", lambda e, b=btn: b.config(bg="#3c3c3c"))
            btn.bind("<Leave>", lambda e, b=btn, v=view: b.config(bg="#3c3c3c" if self.active_view == v else "#333333"))
            self.ab_views[view] = btn

        # 底部设置按钮
        sep = tk.Frame(self.ab, bg="#474747", height=1)
        sep.pack(side=tk.BOTTOM, fill=tk.X, padx=8, pady=4)
        set_btn = tk.Button(self.ab, text="⚙", bg="#333333", fg="#cccccc",
                           relief=tk.FLAT, bd=0, padx=0, pady=10, cursor="hand2",
                           activebackground="#3c3c3c", activeforeground="white",
                           font=("Segoe UI", 14))
        set_btn.pack(side=tk.BOTTOM, fill=tk.X, padx=4, pady=1)
        set_btn.bind("<Enter>", lambda e: set_btn.config(bg="#3c3c3c"))
        set_btn.bind("<Leave>", lambda e: set_btn.config(bg="#333333"))

    def toggle_sidebar(self):
        """切换文件树侧边栏显示"""
        self.set_active_view("explorer")
        self.sidebar_visible = getattr(self, 'sidebar_visible', True)
        children = self.pane.panes()
        if children:
            w = self.pane.nametowidget(children[0])
            if self.sidebar_visible:
                self.sidebar_width = w.winfo_width()
                self.pane.sash_place(0, 0, 0)
                self.sidebar_visible = False
            else:
                sw = getattr(self, 'sidebar_width', 250)
                if sw < 50: sw = 250
                self.pane.sash_place(0, sw, 0)
                self.sidebar_visible = True

    def toggle_search(self):
        """打开搜索面板"""
        self.set_active_view("search")
        self.project_search()

    def toggle_outline_view(self):
        """切换大纲面板"""
        self.set_active_view("outline")
        self.toggle_outline()

    def show_git(self):
        """Git 功能（占位）"""
        self.set_active_view("git")
        self.log("🔀 Git 集成: 请在终端中执行 git 命令", "info")

    def set_active_view(self, view):
        """高亮当前活动视图图标"""
        self.active_view = view
        for v, btn in self.ab_views.items():
            btn.config(bg="#3c3c3c" if v == view else "#333333")

    def toggle_output(self):
        """切换输出面板显示"""
        self.output_visible = getattr(self, 'output_visible', True)
        if hasattr(self, 'right_pw'):
            if self.output_visible:
                # 记住当前高度
                children = self.right_pw.panes()
                if children and len(children) > 1:
                    h = self.right_pw.sash_coord(0)[1] if self.right_pw.sash_coord(0) else 0
                    if h > 10:
                        self.output_height = self.right_pw.winfo_height() - h
                self.right_pw.sash_place(0, 0, self.right_pw.winfo_height() - 30)
                self.output_visible = False
            else:
                oh = getattr(self, 'output_height', 200)
                h = self.right_pw.winfo_height() - oh
                if h < 50: h = self.right_pw.winfo_height() - 200
                self.right_pw.sash_place(0, 0, h)
                self.output_visible = True

    def toggle_activity_bar(self):
        """切换活动栏显示"""
        self.ab_visible = getattr(self, 'ab_visible', True)
        if self.ab_visible:
            self.ab.pack_forget()
            self.ab_visible = False
        else:
            self.ab.pack(side=tk.LEFT, fill=tk.Y, before=self.pane)
            self.ab_visible = True

    def setup_tree(self):
        left = tk.Frame(self.pane, bg="#252526")
        self.pane.add(left, width=250, minsize=120)
        h = tk.Frame(left, bg="#252526"); h.pack(fill=tk.X, padx=8, pady=(6,2))
        tk.Label(h, text="📁 文件浏览器", bg="#252526", fg="#d4d4d4",
                font=("Segoe UI", 9, "bold")).pack(side=tk.LEFT)
        def mk_tree_btn(parent, text, cmd):
            b = tk.Button(parent, text=text, command=cmd, bg="#3c3c3c", fg="#d4d4d4",
                         relief=tk.FLAT, bd=0, padx=4, pady=1, cursor="hand2",
                         activebackground="#505050", activeforeground="white",
                         font=("Segoe UI", 8))
            b.pack(side=tk.RIGHT, padx=1)
            b.bind("<Enter>", lambda e: b.config(bg="#4a4a4a"))
            b.bind("<Leave>", lambda e: b.config(bg="#3c3c3c"))
            return b
        mk_tree_btn(h, "↻", self.refresh_tree)
        mk_tree_btn(h, "↑", self.go_up)
        self.dir_lbl = tk.Label(left, text=".", bg="#252526", fg="#569cd6",
                               font=("Segoe UI", 8), anchor=tk.W)
        self.dir_lbl.pack(fill=tk.X, padx=8, pady=2)
        q = tk.Frame(left, bg="#252526"); q.pack(fill=tk.X, padx=8, pady=2)
        for n, p in [("项目","."), ("示例","examples"), ("根",os.path.dirname(self.cplang_home))]:
            def make_jump(pp=p):
                b = tk.Button(q, text=n, bg="#2d2d2d", fg="#d4d4d4", relief=tk.FLAT, bd=0,
                             padx=6, pady=1, cursor="hand2",
                             activebackground="#3c3c3c", activeforeground="white",
                             font=("Segoe UI", 8),
                             command=lambda pp=pp: self.jump_dir(pp))
                b.pack(side=tk.LEFT, padx=1)
                b.bind("<Enter>", lambda e: b.config(bg="#3a3a3a"))
                b.bind("<Leave>", lambda e: b.config(bg="#2d2d2d"))
            make_jump()
        self.tree = ttk.Treeview(left, show="tree", selectmode="browse")
        self.tree.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        self.tree.bind("<Double-1>", self.on_tree_click)
        self.tree.bind("<Button-3>", self.on_tree_right)
        # 右侧分割线（侧边栏与编辑区的分界）
        tk.Frame(left, bg="#474747", width=1).pack(side=tk.RIGHT, fill=tk.Y)
        sc = ttk.Scrollbar(left, orient=tk.VERTICAL, command=self.tree.yview, style="Vertical.TScrollbar")
        sc.pack(side=tk.RIGHT, fill=tk.Y); self.tree.config(yscrollcommand=sc.set)
        self.tree_ctx = tk.Menu(self.tree, tearoff=0)
        self.tree_ctx.add_command(label="打开", command=self.tree_open)
        self.tree_ctx.add_command(label="新标签", command=self.tree_new_tab)

    def setup_editor(self):
        self.right_pw = tk.PanedWindow(self.pane, orient=tk.VERTICAL, sashrelief=tk.FLAT, sashwidth=4, bg="#2d2d2d")
        self.pane.add(self.right_pw, width=800)
        ef = tk.Frame(self.right_pw, bg="#1e1e1e"); self.right_pw.add(ef, height=550)
        self.tab_frame = tk.Frame(ef, bg="#252526", height=30)
        self.tab_frame.pack(fill=tk.X, padx=0, pady=(0,0))
        self.tab_frame.pack_propagate(False)
        # 工具栏边框
        tk.Frame(ef, bg="#474747", height=1).pack(fill=tk.X)
        tb = tk.Frame(ef, bg="#2d2d2d"); tb.pack(fill=tk.X, padx=0, pady=0)
        def mk_btn(parent, text, cmd, w=None):
            btn = tk.Button(parent, text=text, command=cmd, bg="#2d2d2d", fg="#d4d4d4",
                           relief=tk.FLAT, bd=0, padx=8, pady=4, cursor="hand2",
                           activebackground="#3c3c3c", activeforeground="white",
                           font=("Segoe UI", 9))
            if w: btn.config(width=w)
            btn.pack(side=tk.LEFT, padx=1)
            btn.bind("<Enter>", lambda e: btn.config(bg="#3a3a3a"))
            btn.bind("<Leave>", lambda e: btn.config(bg="#2d2d2d"))
            return btn
        mk_btn(tb, "📄", self.new_file, 3)
        mk_btn(tb, "📂", self.open_file, 3)
        mk_btn(tb, "💾", self.save_file, 3)
        # 分隔线
        sep = tk.Frame(tb, bg="#3a3a3a", width=1, height=20)
        sep.pack(side=tk.LEFT, padx=6)
        mk_btn(tb, "▶ Run", self.run_file)
        mk_btn(tb, "🔍 Find", self.show_find)
        mk_btn(tb, "📋 Outline", self.toggle_outline)
        sep2 = tk.Frame(tb, bg="#3a3a3a", width=1, height=20)
        sep2.pack(side=tk.LEFT, padx=6)
        mk_btn(tb, "▶ 编译", self.build_file)
        self.outline_visible = False
        # ── 面包屑导航（VS Code 风格路径栏）──
        self.bc = tk.Frame(ef, bg="#252526", height=22)
        self.bc.pack(fill=tk.X, padx=0, pady=(0,0))
        self.bc.pack_propagate(False)
        # 下边框
        tk.Frame(self.bc, bg="#474747", height=1).pack(side=tk.BOTTOM, fill=tk.X)
        self.bc_label = tk.Label(self.bc, text="", bg="#252526", fg="#888888",
                                font=("Segoe UI", 9), anchor=tk.W)
        self.bc_label.pack(side=tk.LEFT, padx=8)
        self.update_breadcrumbs()
        ec = tk.Frame(ef, bg="#1e1e1e"); ec.pack(fill=tk.BOTH, expand=True, padx=1, pady=1)
        self.find_bar = FindReplaceBar(ec, None)
        # 编辑器 + 大纲面板
        editor_pane = tk.PanedWindow(ec, orient=tk.HORIZONTAL, sashrelief=tk.FLAT, sashwidth=4, bg="#1e1e1e")
        editor_pane.pack(fill=tk.BOTH, expand=True)
        cf = tk.Frame(editor_pane, bg="#1e1e1e"); editor_pane.add(cf, width=700)
        # 大纲面板（右侧）
        self.outline_frame = tk.Frame(editor_pane, bg="#252526", width=0)
        editor_pane.add(self.outline_frame, width=0)
        tk.Label(self.outline_frame, text="📋 大纲", bg="#252526", fg="#d4d4d4",
                font=("Segoe UI", 9, "bold")).pack(pady=4)
        self.outline_tree = ttk.Treeview(self.outline_frame, show="tree", selectmode="browse", height=5)
        self.outline_tree.pack(fill=tk.BOTH, expand=True, padx=2, pady=2)
        self.outline_tree.bind("<Double-1>", self.on_outline_click)
        self.ln = tk.Canvas(cf, width=56, bg="#1e1e1e", highlightthickness=0)
        self.ln.pack(side=tk.LEFT, fill=tk.Y)
        self.text = tk.Text(cf, wrap=tk.NONE, font=("Consolas",self.font_size),
            bg="#1e1e1e", fg="#d4d4d4", insertbackground="#aeafad",
            relief=tk.FLAT, borderwidth=0, padx=10, pady=8, undo=True)
        self.text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        self.find_bar.text = self.text
        vs = ttk.Scrollbar(cf, orient=tk.VERTICAL, command=self.text.yview, style="Vertical.TScrollbar")
        vs.pack(side=tk.RIGHT, fill=tk.Y); self.text.config(yscrollcommand=vs.set)
        hs = ttk.Scrollbar(ef, orient=tk.HORIZONTAL, command=self.text.xview, style="Horizontal.TScrollbar")
        hs.pack(fill=tk.X, padx=3); self.text.config(xscrollcommand=hs.set)
        self.text.bind("<KeyRelease>", self.on_change)
        self.text.bind("<<Modified>>", self.on_change)
        self.text.bind("<MouseWheel>", self.on_mousewheel)
        self.text.bind("<Control-MouseWheel>", self.on_zoom)
        self.text.bind("<Button-1>", lambda e: self.root.after(10, self.update_status))
        self.text.bind("<Return>", self.on_enter_key)
        self.text.bind("<Tab>", self.on_tab_key)
        self.text.bind("<Key>", self.on_key_press, add="+")
        self.text.bind("<Shift-Tab>", self.on_shift_tab)
        self.text.bind("<ButtonRelease-1>", self.on_cursor_move)
        self.text.bind("<Escape>", lambda e: self.hide_completion())
        self.text.bind("<FocusOut>", self.on_focus_out)
        self.text.bind("<Control-slash>", self.toggle_comment)
        self.text.bind("<Alt-Up>", self.move_line_up)
        self.text.bind("<Alt-Down>", self.move_line_down)
        self.text.bind("<Control-d>", self.duplicate_line)
        # Ctrl+click 跳转定义
        self.text.tag_configure("definition", foreground="#4fc1ff", underline=True)
        self.text.bind("<Control-Button-1>", self.on_ctrl_click)
        self.text.bind("<Motion>", self.on_ctrl_motion)
        self.definitions = {}  # name -> line
        # 语法高亮标签（会在 apply_theme 中覆盖）
        self.text.tag_configure("keyword", foreground="#569cd6")
        self.text.tag_configure("type", foreground="#4ec9b0")
        self.text.tag_configure("string", foreground="#ce9178")
        self.text.tag_configure("comment", foreground="#6a9955")
        self.text.tag_configure("number", foreground="#b5cea8")
        self.text.tag_configure("bracket", foreground="#000000", background="#ffd700", font=("Consolas",self.font_size,"bold"))
        self.text.tag_configure("bracket_bg", background="#3a3520")
        self.text.tag_configure("complete_highlight", background="#264f78", foreground="white")
        self.text.tag_configure("error_squiggle", foreground="#f44747", underline=True)
        self.text.tag_configure("curline", background="#2a2d2a")
        self.ctx = tk.Menu(self.text, tearoff=0)
        self.ctx.add_command(label="撤销", command=lambda: self.text.edit_undo())
        self.ctx.add_command(label="重做", command=lambda: self.text.edit_redo())
        self.ctx.add_separator()
        self.ctx.add_command(label="剪切", command=lambda: self.text.event_generate("<<Cut>>"))
        self.ctx.add_command(label="复制", command=lambda: self.text.event_generate("<<Copy>>"))
        self.ctx.add_command(label="粘贴", command=lambda: self.text.event_generate("<<Paste>>"))
        self.ctx.add_separator()
        self.ctx.add_command(label="查找", command=self.show_find)
        self.ctx.add_command(label="转到行", command=self.goto_line)
        self.text.bind("<Button-3>", lambda e: self.ctx.post(e.x_root, e.y_root))
        
        # 输出面板顶部外框分割线（与编辑器区域的分界）
        of = tk.Frame(self.right_pw, bg="#1e1e1e"); self.right_pw.add(of, height=200)
        oh = tk.Frame(of, bg="#1e1e1e"); oh.pack(fill=tk.X, padx=6, pady=(4,2))
        # 输出面板上边框
        tk.Frame(oh, bg="#474747", height=1).pack(fill=tk.X, pady=(0,4))
        tk.Label(oh, text="📟 输出", bg="#1e1e1e", fg="#d4d4d4",
                font=("Segoe UI", 9, "bold")).pack(side=tk.LEFT)
        def mk_out_btn(parent, text, cmd):
            b = tk.Button(parent, text=text, command=cmd, bg="#3c3c3c", fg="#d4d4d4",
                         relief=tk.FLAT, bd=0, padx=8, pady=1, cursor="hand2",
                         activebackground="#505050", activeforeground="white",
                         font=("Segoe UI", 9))
            b.pack(side=tk.RIGHT, padx=1)
            b.bind("<Enter>", lambda e: b.config(bg="#4a4a4a"))
            b.bind("<Leave>", lambda e: b.config(bg="#3c3c3c"))
            return b
        mk_out_btn(oh, "清空", self.clear_out)
        self.out = tk.Text(of, wrap=tk.WORD, font=("Consolas",11),
            bg="#1e1e1e", fg="#cccccc", relief=tk.FLAT, borderwidth=0,
            height=8, state=tk.DISABLED, cursor="hand2")
        self.out.pack(fill=tk.BOTH, expand=True, padx=4, pady=2)
        os2 = ttk.Scrollbar(of, orient=tk.VERTICAL, command=self.out.yview, style="Vertical.TScrollbar")
        os2.pack(side=tk.RIGHT, fill=tk.Y); self.out.config(yscrollcommand=os2.set)
        self.out.tag_configure("info", foreground="#569cd6")
        self.out.tag_configure("ok", foreground="#4ec9b0")
        self.out.tag_configure("err", foreground="#f44747")
        self.out.tag_configure("warn", foreground="#ce9178")
        self.out.bind("<Button-1>", self.on_out_click)

    def setup_statusbar(self):
        """状态栏：深色背景 + 分段信息"""
        sf = tk.Frame(self.root, bg="#007acc", height=24)
        sf.pack(side=tk.BOTTOM, fill=tk.X)
        sf.pack_propagate(False)
        # 左段
        self.sl = tk.Label(sf, text="就绪", bg="#007acc", fg="white",
                           anchor=tk.W, font=("Segoe UI", 9))
        self.sl.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(8, 4))
        # 中段（分隔点）
        sep1 = tk.Label(sf, text="|", bg="#007acc", fg="#88ccff", font=("Segoe UI", 9))
        sep1.pack(side=tk.LEFT)
        # 右段（编码，点击可切换）
        self.encoding_cycle = ["utf-8", "gbk", "utf-8-sig", "utf-16"]
        self.enc_label = tk.Label(sf, text="UTF-8", bg="#007acc", fg="white",
                                  font=("Segoe UI", 9), cursor="hand2")
        self.enc_label.pack(side=tk.RIGHT, padx=(4, 8))
        self.enc_label.bind("<Button-1>", self.cycle_encoding)
        # 右段（行列）
        self.sr = tk.Label(sf, text="行 1, 列 1", bg="#007acc", fg="white",
                           anchor=tk.E, font=("Segoe UI", 9))
        self.sr.pack(side=tk.RIGHT, padx=4)

    def setup_shortcuts(self):
        self.root.bind("<Control-n>", lambda e: self.new_file())
        self.root.bind("<Control-o>", lambda e: self.open_file())
        self.root.bind("<Control-s>", lambda e: self.save_file())
        self.root.bind("<Control-Shift-S>", lambda e: self.save_as())
        self.root.bind("<Control-w>", lambda e: self.close_tab())
        self.root.bind("<Control-Tab>", lambda e: self.next_tab())
        self.root.bind("<Control-Shift-Key-Tab>", lambda e: self.prev_tab())
        self.root.bind("<F5>", lambda e: self.run_file())
        self.root.bind("<Control-F7>", lambda e: self.build_file())
        self.root.bind("<Control-f>", lambda e: self.show_find())
        self.root.bind("<Control-Shift-F>", lambda e: self.project_search())
        self.root.bind("<Control-Shift-P>", lambda e: self.command_palette())
        self.root.bind("<Control-g>", lambda e: self.goto_line())
        self.root.bind("<Control-equal>", lambda e: self.zoom_in())
        self.root.bind("<Control-plus>", lambda e: self.zoom_in())
        self.root.bind("<Control-minus>", lambda e: self.zoom_out())
        self.root.bind("<Control-i>", lambda e: self.insert_snippet())
        self.root.bind("<Control-d>", lambda e: self.duplicate_line())
        self.root.bind("<Alt-Up>", lambda e: self.move_line_up())
        self.root.bind("<Alt-Down>", lambda e: self.move_line_down())
        self.root.bind("<Control-b>", lambda e: self.toggle_sidebar())
        self.root.bind("<Control-j>", lambda e: self.toggle_output())
        self.root.bind("<Control-Shift-B>", lambda e: self.toggle_activity_bar())

    def tab(self):
        if 0 <= self.cur_tab < len(self.tabs): return self.tabs[self.cur_tab]
        return None

    def switch_tab(self, idx):
        if idx < 0 or idx >= len(self.tabs) or idx == self.cur_tab: return
        t = self.tab()
        if t: t.content = self.text.get("1.0", tk.END).rstrip("\n")
        self.cur_tab = idx; self.load_tab(); self.update_tabs(); self.update_title(); self.update_breadcrumbs()

    def load_tab(self):
        t = self.tab()
        if not t: return
        self.hide_completion()
        self.text.delete("1.0", tk.END); self.text.insert("1.0", t.content)
        self.text.edit_reset(); self.text.edit_modified(False)
        self.update_ln(); self.highlight(); self.update_breadcrumbs()

    def update_tabs(self):
        """更新标签栏（VS Code 风格，带关闭按钮）"""
        for w in self.tab_frame.winfo_children(): w.destroy()
        self.tab_frame.config(bg="#252526")
        for i, t in enumerate(self.tabs):
            act = (i == self.cur_tab)
            nm = t.name
            bg = "#3c3c3c" if act else "#2d2d2d"
            fg = "white" if act else "#999"
            # 标签容器
            tab = tk.Frame(self.tab_frame, bg=bg, height=28)
            tab.pack(side=tk.LEFT, padx=(0, 1))
            tab.pack_propagate(False)
            # 活跃标签底部蓝色强调线
            if act:
                accent = tk.Frame(tab, bg="#007acc", height=2)
                accent.pack(side=tk.BOTTOM, fill=tk.X)
            # 内部水平布局：名称 + 关闭按钮
            inner = tk.Frame(tab, bg=bg)
            inner.pack(fill=tk.BOTH, expand=True)
            # 修改指示符
            mod_ind = "● " if t.modified else "  "
            lbl = tk.Label(inner, text=mod_ind + nm, bg=bg, fg=fg,
                          font=("Segoe UI", 9), padx=6, pady=3, cursor="hand2")
            lbl.pack(side=tk.LEFT)
            # 关闭按钮 ×
            close_btn = tk.Label(inner, text="×", bg=bg, fg=fg,
                                font=("Segoe UI", 10), padx=4, pady=3, cursor="hand2")
            close_btn.pack(side=tk.RIGHT)
            # 点击事件
            lbl.bind("<Button-1>", lambda e, idx=i: self.switch_tab(idx))
            inner.bind("<Button-1>", lambda e, idx=i: self.switch_tab(idx))
            close_btn.bind("<Button-1>", lambda e, idx=i: self.close_tab(idx))
            lbl.bind("<Button-2>", lambda e, idx=i: self.close_tab(idx))
            lbl.bind("<Button-3>", lambda e, idx=i: self.tab_ctx(e, idx))
            inner.bind("<Button-3>", lambda e, idx=i: self.tab_ctx(e, idx))
            # 悬停效果：标签变亮，关闭按钮可见
            if not act:
                close_btn.config(text="")
                def on_enter(event, l=lbl, c=close_btn, b=bg):
                    l.config(bg="#353535")
                    c.config(bg="#353535", text="×", fg="#999")
                def on_leave(event, l=lbl, c=close_btn, b=bg):
                    l.config(bg=b)
                    c.config(bg=b, text="", fg=b)
                lbl.bind("<Enter>", on_enter)
                close_btn.bind("<Enter>", on_enter)
                lbl.bind("<Leave>", on_leave)
                close_btn.bind("<Leave>", on_leave)
        # "+" 新建标签按钮
        plus = tk.Frame(self.tab_frame, bg="#2d2d2d", height=28, width=30)
        plus.pack(side=tk.LEFT)
        plus.pack_propagate(False)
        plus_lbl = tk.Label(plus, text="+", bg="#2d2d2d", fg="#888",
                           font=("Segoe UI", 14, "bold"), cursor="hand2")
        plus_lbl.pack(fill=tk.BOTH, expand=True)
        plus_lbl.bind("<Button-1>", lambda e: self.new_file())
        plus_lbl.bind("<Enter>", lambda e: plus_lbl.config(fg="white"))
        plus_lbl.bind("<Leave>", lambda e: plus_lbl.config(fg="#888"))

    def tab_ctx(self, event, idx):
        m = tk.Menu(self.root, tearoff=0)
        m.add_command(label="关闭", command=lambda: self.close_tab(idx))
        m.add_command(label="关闭其他", command=lambda: self.close_others(idx))
        m.add_command(label="关闭全部", command=self.close_all)
        m.post(event.x_root, event.y_root)

    def new_file(self):
        self.tabs.append(Tab()); self.switch_tab(len(self.tabs)-1)

    def close_tab(self, idx=None):
        if idx is None: idx = self.cur_tab
        if len(self.tabs) <= 1:
            self.tabs[0] = Tab(); self.switch_tab(0); return
        t = self.tabs[idx]
        if t.modified:
            r = messagebox.askyesnocancel("未保存", f"{t.name} 已修改，保存?")
            if r is None: return
            if r:
                if t.path: self._save(t.path, t.content)
                else: self.save_as()
        self.tabs.pop(idx)
        if self.cur_tab >= len(self.tabs): self.cur_tab = len(self.tabs)-1
        self.switch_tab(self.cur_tab)

    def close_others(self, keep):
        self.tabs = [self.tabs[keep]]; self.cur_tab = 0; self.switch_tab(0)

    def close_all(self):
        self.tabs = [Tab()]; self.cur_tab = 0; self.switch_tab(0)

    def next_tab(self):
        self.switch_tab((self.cur_tab + 1) % len(self.tabs))

    def prev_tab(self):
        self.switch_tab((self.cur_tab - 1) % len(self.tabs))

    def refresh_tree(self):
        self.tree.delete(*self.tree.get_children())
        self.dir_lbl.config(text=self.current_dir if hasattr(self,'current_dir') else ".")
        if not hasattr(self, 'current_dir'): self.current_dir = os.getcwd()
        try: items = os.listdir(self.current_dir)
        except: return
        dirs = sorted([n for n in items if os.path.isdir(os.path.join(self.current_dir,n)) and not n.startswith(".")], key=str.lower)
        files = sorted([n for n in items if n.endswith(".cp")], key=str.lower)
        for d in dirs: self.tree.insert("","end", text=d+"/", values=(d,"dir"))
        for f in files: self.tree.insert("","end", text=f, values=(f,"file"))

    def go_up(self):
        p = os.path.dirname(self.current_dir)
        if p and p != self.current_dir: self.current_dir = p; self.refresh_tree()

    def jump_dir(self, path):
        if os.path.isdir(path): self.current_dir = path; self.refresh_tree()

    def on_tree_click(self, e): self.tree_open()

    def on_tree_right(self, e):
        item = self.tree.identify_row(e.y)
        if item: self.tree.selection_set(item); self.tree_ctx.post(e.x_root, e.y_root)

    def tree_open(self):
        sel = self.tree.selection()
        if not sel: return
        v = self.tree.item(sel[0], "values")
        if not v: return
        n, t = v; full = os.path.join(self.current_dir, n)
        if t == "dir": self.current_dir = full; self.refresh_tree()
        else: self.open_path(full, False)

    def tree_new_tab(self):
        sel = self.tree.selection()
        if not sel: return
        v = self.tree.item(sel[0], "values")
        if not v or v[1] != "file": return
        self.open_path(os.path.join(self.current_dir, v[0]), True)

    def open_file(self):
        p = filedialog.askopenfilename(title="打开 CP 文件", filetypes=[("CP 源文件","*.cp"),("所有文件","*.*")])
        if p: self.open_path(p)

    def open_path(self, path, new_tab=False):
        try:
            enc = self.detect_encoding(path)
            self.file_encoding = enc
            with open(path, "r", encoding=enc) as f: content = f.read()
        except Exception as e: messagebox.showerror("打开失败", str(e)); return
        for i, t in enumerate(self.tabs):
            if t.path and os.path.abspath(t.path) == os.path.abspath(path):
                self.switch_tab(i); return
        t = Tab(os.path.basename(path), path, content)
        if new_tab: self.tabs.append(t); self.switch_tab(len(self.tabs)-1)
        else: self.tabs[self.cur_tab] = t; self.load_tab(); self.update_tabs()
        # 记录最近文件
        if path in self.recent_files: self.recent_files.remove(path)
        self.recent_files.append(path)
        self.update_recent_menu()
        self.log(f"打开: {path}", "info")

    def save_file(self):
        t = self.tab()
        if not t: return
        c = self.text.get("1.0", tk.END).rstrip("\n")
        if t.path: self._save(t.path, c); t.modified = False; t.content = c
        else: self.save_as(); return
        self.update_tabs(); self.update_title()

    def save_as(self):
        p = filedialog.asksaveasfilename(title="保存", defaultextension=".cp",
            filetypes=[("CP 源文件","*.cp"),("所有文件","*.*")])
        if not p: return
        t = self.tab(); c = self.text.get("1.0", tk.END).rstrip("\n")
        self._save(p, c)
        if t: t.path = p; t.name = os.path.basename(p); t.modified = False; t.content = c
        self.update_tabs(); self.update_title()

    def _save(self, path, content):
        try:
            with open(path, "w", encoding="utf-8") as f: f.write(content)
            self.log(f"已保存: {path}", "ok")
        except Exception as e: messagebox.showerror("保存失败", str(e))

    def run_file(self):
        t = self.tab()
        if not t: return
        t.content = self.text.get("1.0", tk.END).rstrip("\n")
        if not t.path: self.save_as()
        if not t.path: return
        self.save_file()
        if not self.compiler_ok:
            self.log(f"编译器未找到: {self.compiler}", "err"); return
        self.clear_error_marks()
        self.log("="*55, ""); self.log(f">> 编译运行: {t.path}", "info")
        self.sl.config(text="编译中..."); self.root.update()
        try:
            r = subprocess.run([self.compiler, "-c", t.path],
                capture_output=True, encoding="utf-8", errors="replace",
                timeout=30, env=os.environ)
            o = r.stdout + ("\n"+r.stderr if r.stderr else "")
            for line in o.split("\n"):
                if not line.strip(): continue
                if any(x in line for x in ["错误","失败","error","Error"]): self.log(line, "err")
                elif any(x in line for x in ["成功","success"]): self.log(line, "ok")
                elif any(x in line for x in ["WARNING","警告"]): self.log(line, "warn")
                else: self.log(line, "")
            if r.returncode == 0: self.log("OK", "ok"); self.sl.config(text="OK")
            else:
                self.log(f"退出码: {r.returncode}", "err"); self.sl.config(text="FAIL")
                self.mark_errors_from_output(o)
        except subprocess.TimeoutExpired: self.log("超时", "err")
        except Exception as e: self.log(f"错误: {e}", "err")

    def build_file(self):
        t = self.tab()
        if not t or not t.path: self.save_as()
        if not t or not t.path: return
        self.save_file()
        if not self.compiler_ok: return
        self.log(f">> 编译检查: {t.path}", "info")
        try:
            r = subprocess.run([self.compiler, "-p", t.path],
                capture_output=True, encoding="utf-8", errors="replace", timeout=30)
            if r.returncode == 0: self.log("OK", "ok")
            else:
                o = (r.stdout or "") + ("\n"+r.stderr if r.stderr else "")
                for line in o.split("\n"):
                    if line.strip(): self.log(line, "err")
        except Exception as e: self.log(f"错误: {e}", "err")

    def show_find(self): self.find_bar.show()

    def on_out_click(self, e):
        try:
            idx = self.out.index(f"@{e.x},{e.y}")
            txt = self.out.get(f"{idx} linestart", f"{idx} lineend")
            m = re.search(r"(?:行|第|line|:)\s*(\d+)", txt, re.I)
            if not m: m = re.search(r"\((\d+)\)", txt)
            if m:
                n = int(m.group(1))
                self.text.see(f"{n}.0"); self.text.mark_set(tk.INSERT, f"{n}.0")
                self.text.focus_set(); self.update_status()
        except: pass

    def goto_line(self):
        d = tk.Toplevel(self.root); d.title("转到行"); d.geometry("280x110")
        d.transient(self.root); d.grab_set()
        ttk.Label(d, text="行号:").pack(pady=(15,5))
        e = ttk.Entry(d, font=("",11)); e.pack(); e.focus_set()
        def go():
            try:
                n = int(e.get()); self.text.mark_set(tk.INSERT, f"{n}.0")
                self.text.see(tk.INSERT); self.text.focus_set(); self.update_status(); d.destroy()
            except: pass
        e.bind("<Return>", lambda e: go())
        ttk.Button(d, text="转到", command=go).pack(pady=8)

    def env_info(self):
        t = self.tab()
        msg = f"""CP IDE v2.0
CPLANG_HOME = {self.cplang_home}
编译器 = {self.compiler}
存在 = {'OK' if self.compiler_ok else 'NO'}
Python = {sys.version.split()[0]}
文件 = {t.path if t and t.path else '(无)'}
标签 = {len(self.tabs)}"""
        messagebox.showinfo("环境信息", msg)

    def log(self, text, tag=""):
        self.out.config(state=tk.NORMAL)
        self.out.insert(tk.END, text+"\n", tag)
        self.out.see(tk.END); self.out.config(state=tk.DISABLED)

    def clear_out(self):
        self.out.config(state=tk.NORMAL); self.out.delete("1.0", tk.END)
        self.out.config(state=tk.DISABLED)

    def on_change(self, e=None):
        t = self.tab()
        if t: t.modified = True
        self.update_ln(); self.highlight(); self.update_tabs(); self.update_status()
        self.update_curline()
        self.highlight_brackets()
        # 刷新大纲
        if self.outline_visible:
            self.update_outline()
        # 自动补全（至少输入 2 个字符后自动弹出）
        if e and e.keysym not in ("Control_L","Control_R","Alt_L","Alt_R","Shift_L","Shift_R",
                                   "Up","Down","Left","Right","Home","End","Page_Up","Page_Down",
                                   "Escape","Return","Tab","BackSpace","Delete","slash",
                                   "Caps_Lock","Num_Lock","Scroll_Lock","Print","Pause",
                                   "F1","F2","F3","F4","F5","F6","F7","F8","F9","F10","F11","F12"):
            # 延迟显示补全（避免快速输入时闪烁）
            if hasattr(self, '_comp_after_id'):
                self.root.after_cancel(self._comp_after_id)
            self._comp_after_id = self.root.after(150, self._debounced_completion)
        else:
            self.hide_completion()
        if self.text.edit_modified(): self.text.edit_modified(False)

    def update_status(self):
        try:
            c = self.text.index(tk.INSERT); l, cl = c.split(".")
            t = self.tab()
            info = f"行 {l}, 列 {int(cl)+1}"
            if t and t.path:
                try:
                    sz = os.path.getsize(t.path)
                    if sz < 1024: info += f" | {sz}B"
                    else: info += f" | {sz/1024:.1f}KB"
                except: pass
                info += f" | {self.file_encoding if hasattr(self,'file_encoding') else 'utf-8'}"
            self.sr.config(text=info)
        except: pass

    def cycle_encoding(self, e=None):
        """点击编码标签切换编码"""
        try:
            idx = self.encoding_cycle.index(self.file_encoding)
            self.file_encoding = self.encoding_cycle[(idx + 1) % len(self.encoding_cycle)]
        except:
            self.file_encoding = self.encoding_cycle[0]
        self.enc_label.config(text=self.file_encoding.upper())
        self.log(f"编码: {self.file_encoding}", "info")

    def update_title(self):
        t = self.tab()
        if t and t.path: self.root.title(f"CP IDE - {t.name}")
        else: self.root.title("CP IDE v2.0")

    def update_breadcrumbs(self):
        """更新面包屑导航（文件路径）"""
        t = self.tab()
        if t and t.path:
            try:
                rel = os.path.relpath(t.path, self.current_dir if hasattr(self, 'current_dir') else os.path.dirname(t.path))
                parts = rel.replace("\\", "/").split("/")
                bc = " > ".join(parts)
                self.bc_label.config(text=f"📄  {bc}")
            except:
                self.bc_label.config(text=f"📄  {t.name}")
        else:
            self.bc_label.config(text="")

    def update_ln(self):
        """更新行号显示 + 缩进参考线 + 分隔线"""
        self.ln.delete("all")
        try: cnt = int(self.text.index("end-1c").split(".")[0])
        except: cnt = 1
        # 右侧分隔线（行号区与编辑区的分界）
        self.ln.create_line(50, 0, 50, cnt*20+10, fill="#2a2a2a", width=1)
        # 计算字符宽度（用于缩进参考线）
        char_w = 7
        try:
            font_m = tk.font.Font(font=self.text.cget("font"))
            char_w = font_m.measure(" ") or 7
        except: pass
        for i in range(1, cnt+1):
            # 当前行号高亮
            cur_line = self.text.index(tk.INSERT).split(".")[0] if hasattr(self, 'text') else "1"
            ln_fill = "#d4d4d4" if str(i) == cur_line else "#858585"
            self.ln.create_text(46, i*20-14, anchor=tk.E, text=str(i), fill=ln_fill, font=("Consolas",9))
            # 缩进参考线：在行号区左侧画竖线
            txt = self.text.get(f"{i}.0", f"{i}.end")
            indent = len(txt) - len(txt.lstrip())
            if indent > 0:
                level = indent // 4
                for lv in range(1, level + 1):
                    x = 4 + (lv - 1) * 2
                    if lv % 2 == 0:
                        self.ln.create_line(x, i*20-18, x, i*20-2, fill="#3a3a3a", width=1)
                    else:
                        self.ln.create_line(x, i*20-18, x, i*20-2, fill="#2a2a2a", width=1)

    def highlight(self):
        for tag in ("keyword","type","string","comment","number"):
            self.text.tag_remove(tag, "1.0", tk.END)
        c = self.text.get("1.0", tk.END)
        for m in re.finditer(r"//[^\n]*", c): self._tag(m, "comment")
        for m in re.finditer(r"/\*.*?\*/", c, re.DOTALL): self._tag(m, "comment")
        for m in re.finditer(r'"[^"\\]*(\\.[^"\\]*)*"', c): self._tag(m, "string")
        for m in re.finditer(r'\b[0-9]+(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?\b', c): self._tag(m, "number")
        for m in KW_RE.finditer(c): self._tag(m, "keyword")
        for m in TYPE_RE.finditer(c): self._tag(m, "type")

    def _tag(self, m, tag):
        try: self.text.tag_add(tag, f"1.0+{m.start()}c", f"1.0+{m.end()}c")
        except: pass

    # ═══════════════════════════ 括号匹配 ═══════════════════════════

    def highlight_brackets(self):
        """高亮匹配的括号（含行号区标记和括号内文本背景高亮）"""
        self.text.tag_remove("bracket", "1.0", tk.END)
        self.text.tag_remove("bracket_bg", "1.0", tk.END)
        try:
            pos = self.text.index(tk.INSERT)
            char = self.text.get(pos, f"{pos}+1c")
            # 也检查光标前的字符
            if char not in "()[]{}":
                prev_pos = f"{pos}-1c"
                char = self.text.get(prev_pos, pos)
                if char in "()[]{}":
                    pos = prev_pos
            pairs = {"(":")", "[":"]", "{":"}", ")":"(", "]":"[", "}":"{"}
            if char in pairs:
                self.text.tag_add("bracket", pos, f"{pos}+1c")
                # 查找匹配
                open_b = char in "([{"
                target = pairs[char]
                direction = 1 if open_b else -1
                depth = 1
                line, col = map(int, pos.split("."))
                content = self.text.get("1.0", tk.END)
                total_lines = content.count("\n") + 1
                l, c = line, col
                while 1 <= l <= total_lines:
                    line_text = self.text.get(f"{l}.0", f"{l}.end")
                    start_c = c + direction if l == line else (0 if direction > 0 else len(line_text))
                    if direction > 0:
                        search_range = line_text[start_c:]
                    else:
                        search_range = line_text[:start_c][::-1]
                    for i, ch in enumerate(search_range):
                        if ch == target:
                            depth -= 1
                            if depth == 0:
                                match_pos = start_c + i if direction > 0 else start_c - i - 1
                                self.text.tag_add("bracket", f"{l}.{match_pos}", f"{l}.{match_pos}+1c")
                                # 括号间背景高亮
                                if direction > 0:
                                    self.text.tag_add("bracket_bg", f"{pos}+1c", f"{l}.{match_pos}")
                                else:
                                    self.text.tag_add("bracket_bg", f"{l}.{match_pos}+1c", pos)
                                # 在行号区画指示线
                                self.ln.delete("bracket_line")
                                start_line = int(pos.split(".")[0])
                                end_line = l
                                step = 1 if start_line <= end_line else -1
                                for ln in range(start_line, end_line + step, step):
                                    self.ln.create_line(0, ln*20-10, 3, ln*20-10,
                                                       fill="#ffd700", width=2, tags="bracket_line")
                                return
                        elif ch == char:
                            depth += 1
                    l += direction; c = 0
        except: pass

    # ═══════════════════════════ 自动缩进 ═══════════════════════════

    def on_enter_key(self, e):
        """回车自动缩进"""
        try:
            if self.completion_visible():
                self.apply_completion(); return "break"
            cur = self.text.index(tk.INSERT)
            line_num = int(cur.split(".")[0])
            if line_num > 1:
                prev = self.text.get(f"{line_num-1}.0", f"{line_num-1}.end")
                indent = len(prev) - len(prev.lstrip())
                self.text.insert(tk.INSERT, "\n" + " " * indent)
                if prev.rstrip().endswith("{"):
                    self.text.insert(tk.INSERT, "    ")
            else:
                self.text.insert(tk.INSERT, "\n")
            return "break"
        except: return None

    def on_tab_key(self, e):
        """Tab 键：插入空格、缩进选择行或确认补全"""
        if self.completion_visible():
            self.apply_completion()
            return "break"
        # 多行选择缩进
        try:
            sel = self.text.tag_ranges("sel")
            if sel:
                start_line = int(self.text.index(sel[0]).split(".")[0])
                end_line = int(self.text.index(sel[1]).split(".")[0])
                if end_line > start_line:
                    for ln in range(start_line, end_line + 1):
                        self.text.insert(f"{ln}.0", "    ")
                    return "break"
        except:
            pass
        self.text.insert(tk.INSERT, "    ")
        return "break"

    # ═══════════════════════════ 自动补全括号 & 多行操作 ═══════════════════════════

    def on_key_press(self, e):
        """自动补全括号和引号"""
        char = e.char
        if not char or len(char) != 1:
            return None
        if e.state & 0x4 or e.state & 0x20000:
            return None
        pairs_close = {'(': ')', '[': ']', '{': '}'}
        if char in pairs_close:
            self.handle_open_bracket(char, pairs_close[char])
            return "break"
        if char in ('"', "'"):
            self.handle_quote(char)
            return "break"
        if char in ')]}':
            try:
                nxt = self.text.get(tk.INSERT, f"{tk.INSERT}+1c")
                if nxt == char:
                    self.text.mark_set(tk.INSERT, f"{tk.INSERT}+1c")
                    return "break"
            except:
                pass
        return None

    def handle_open_bracket(self, open_ch, close_ch):
        """处理开括号：包裹选中文本或插入对"""
        try:
            sel = self.text.tag_ranges("sel")
            if sel:
                start = self.text.index(sel[0])
                end = self.text.index(sel[1])
                self.text.insert(end, close_ch)
                self.text.insert(start, open_ch)
                self.text.tag_remove("sel", "1.0", tk.END)
                return
        except:
            pass
        self.text.insert(tk.INSERT, open_ch + close_ch)
        self.text.mark_set(tk.INSERT, f"{tk.INSERT}-1c")

    def handle_quote(self, ch):
        """处理引号：包裹选中文本或插入对"""
        try:
            sel = self.text.tag_ranges("sel")
            if sel:
                start = self.text.index(sel[0])
                end = self.text.index(sel[1])
                self.text.insert(end, ch)
                self.text.insert(start, ch)
                self.text.tag_remove("sel", "1.0", tk.END)
                return
        except:
            pass
        self.text.insert(tk.INSERT, ch + ch)
        self.text.mark_set(tk.INSERT, f"{tk.INSERT}-1c")

    def on_shift_tab(self, e=None):
        """Shift+Tab 取消缩进选中行"""
        try:
            sel = self.text.tag_ranges("sel")
            if sel:
                start_line = int(self.text.index(sel[0]).split(".")[0])
                end_line = int(self.text.index(sel[1]).split(".")[0])
                for ln in range(start_line, end_line + 1):
                    txt = self.text.get(f"{ln}.0", f"{ln}.end")
                    if txt.startswith("    "):
                        self.text.delete(f"{ln}.0", f"{ln}.0+4c")
                    elif txt.startswith("\t"):
                        self.text.delete(f"{ln}.0", f"{ln}.0+1c")
                return "break"
            cur = self.text.index(tk.INSERT)
            line = int(cur.split(".")[0])
            txt = self.text.get(f"{line}.0", f"{line}.end")
            if txt.startswith("    "):
                self.text.delete(f"{line}.0", f"{line}.0+4c")
            return "break"
        except:
            return None

    def on_cursor_move(self, e=None):
        """光标移动时更新行高亮和状态"""
        self.update_curline()
        self.update_status()

    def update_curline(self):
        """高亮当前行"""
        self.text.tag_remove("curline", "1.0", tk.END)
        try:
            cur = self.text.index(tk.INSERT)
            line = cur.split(".")[0]
            self.text.tag_add("curline", f"{line}.0", f"{line}.end+1c")
        except:
            pass

    # ═══════════════════════════ 代码补全 ═══════════════════════════

    ALL_COMPLETIONS = CP_KEYWORDS + CP_TYPES + CP_BUILTINS

    def get_word_before_cursor(self):
        """获取光标前的单词"""
        try:
            pos = self.text.index(tk.INSERT)
            line_text = self.text.get(f"{pos} linestart", pos)
            m = re.search(r'[一-鿿\w]+$', line_text)
            return m.group(0) if m else ""
        except: return ""

    def _debounced_completion(self):
        """延迟触发补全（避免快速输入时闪烁）"""
        prefix = self.get_word_before_cursor()
        if len(prefix) >= 2:
            self.show_completion()
        elif len(prefix) < 1:
            self.hide_completion()

    def completion_visible(self):
        return hasattr(self, 'comp_win') and self.comp_win and self.comp_win.winfo_exists()

    def show_completion(self):
        """显示自动补全弹窗（至少 2 个字符触发）"""
        prefix = self.get_word_before_cursor()
        if len(prefix) < 2:
            self.hide_completion(); return

        # 找出匹配的候选词
        matches = [w for w in self.ALL_COMPLETIONS if w.startswith(prefix) and w != prefix]
        matches.sort(key=lambda x: (x.startswith(prefix), x))
        matches = matches[:20]  # 最多20个

        if not matches:
            self.hide_completion(); return

        if not self.completion_visible():
            self.comp_win = tk.Toplevel(self.root)
            self.comp_win.overrideredirect(True)
            self.comp_win.attributes("-topmost", True)
            self.comp_list = tk.Listbox(self.comp_win, font=("Consolas",11),
                                         bg="#2d2d2d", fg="#d4d4d4",
                                         selectbackground="#264f78",
                                         selectforeground="white",
                                         relief=tk.FLAT, borderwidth=1,
                                         height=min(len(matches), 12), width=25)
            self.comp_list.pack()
            self.comp_list.bind("<Button-1>", lambda e: self.apply_completion())
            self.comp_list.bind("<Double-Button-1>", lambda e: self.apply_completion())

        # 更新候选项
        self.comp_list.delete(0, tk.END)
        for w in matches: self.comp_list.insert(tk.END, w)
        self.comp_list.config(height=min(len(matches), 12))

        # 定位到光标下方
        try:
            bbox = self.text.bbox(tk.INSERT)
            if bbox:
                x = self.text.winfo_rootx() + bbox[0]
                y = self.text.winfo_rooty() + bbox[1] + bbox[3] + 5
                # 确保不超出屏幕
                screen_w = self.root.winfo_screenwidth()
                screen_h = self.root.winfo_screenheight()
                win_w = 280
                win_h = min(len(matches), 12) * 25 + 10
                if x + win_w > screen_w: x = screen_w - win_w - 20
                if y + win_h > screen_h: y = screen_h - win_h - 20
                self.comp_win.geometry(f"+{int(x)}+{int(y)}")
        except: pass

    def hide_completion(self):
        if self.completion_visible():
            try: self.comp_win.destroy()
            except: pass
            self.comp_win = None

    def apply_completion(self):
        """选择补全项"""
        if not self.completion_visible(): return
        sel = self.comp_list.curselection()
        if not sel: return
        word = self.comp_list.get(sel[0])
        prefix = self.get_word_before_cursor()
        if prefix:
            pos = self.text.index(tk.INSERT)
            line, col = map(int, pos.split("."))
            start_col = col - len(prefix)
            self.text.delete(f"{line}.{start_col}", pos)
            self.text.insert(f"{line}.{start_col}", word)
        self.hide_completion()

    # ═══════════════════════════ 项目中查找 ═══════════════════════════

    def project_search(self):
        """在项目中搜索文本 (Ctrl+Shift+F)"""
        d = tk.Toplevel(self.root); d.title("项目中查找"); d.geometry("600x500")
        d.transient(self.root)
        tk.Label(d, text="搜索文本:", font=("",10)).pack(pady=(10,5))
        entry = ttk.Entry(d, font=("",11), width=50)
        entry.pack(padx=10, pady=5)
        entry.focus_set()
        result_frame = ttk.Frame(d); result_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)
        result_text = tk.Text(result_frame, font=("Consolas",10), bg="#1e1e1e", fg="#cccccc",
                              wrap=tk.NONE, relief=tk.FLAT, borderwidth=0)
        result_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        rs = ttk.Scrollbar(result_frame, orient=tk.VERTICAL, command=result_text.yview)
        rs.pack(side=tk.RIGHT, fill=tk.Y); result_text.config(yscrollcommand=rs.set)
        result_text.tag_configure("file", foreground="#569cd6", font=("Consolas",10,"bold"))
        result_text.tag_configure("match", foreground="#ce9178")

        def do_search():
            result_text.delete("1.0", tk.END)
            q = entry.get()
            if not q: return
            root_dir = getattr(self, 'current_dir', '.')
            count = 0
            for dirpath, dirnames, filenames in os.walk(root_dir):
                dirnames[:] = [d for d in dirnames if not d.startswith(".") and d != "__pycache__"]
                for f in filenames:
                    if not f.endswith(".cp"): continue
                    path = os.path.join(dirpath, f)
                    try:
                        with open(path, "r", encoding="utf-8", errors="replace") as fp:
                            for i, line in enumerate(fp, 1):
                                if q in line:
                                    rel = os.path.relpath(path, root_dir)
                                    result_text.insert(tk.END, f"{rel}:{i}", "file")
                                    result_text.insert(tk.END, f"  {line.strip()}\n", "match")
                                    count += 1
                    except: pass
            if count == 0:
                result_text.insert(tk.END, "未找到匹配")
            result_text.insert(tk.END, f"\n--- 共 {count} 处匹配 ---")

        entry.bind("<Return>", lambda e: do_search())
        btn_frame = ttk.Frame(d); btn_frame.pack(fill=tk.X, padx=10, pady=5)
        ttk.Button(btn_frame, text="搜索", command=do_search).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_frame, text="关闭", command=d.destroy).pack(side=tk.RIGHT, padx=2)

    # ═══════════════════════════ 命令面板 ═══════════════════════════

    def command_palette(self):
        """Ctrl+Shift+P 命令面板（VS Code 风格）"""
        d = tk.Toplevel(self.root); d.title("命令面板"); d.geometry("500x400")
        d.transient(self.root); d.grab_set()
        # 覆盖标题栏
        d.overrideredirect(True)
        d.configure(bg="#2d2d2d")
        # 搜索框
        entry = tk.Entry(d, bg="#3c3c3c", fg="white", relief=tk.FLAT, bd=0,
                        insertbackground="white", font=("Segoe UI", 12))
        entry.pack(fill=tk.X, padx=0, pady=0, ipady=8)
        entry.focus_set()
        # 在搜索框下方加一条分割线
        sep = tk.Frame(d, bg="#3c3c3c", height=1)
        sep.pack(fill=tk.X)
        # 命令列表
        lst = tk.Listbox(d, bg="#2d2d2d", fg="#d4d4d4", selectbackground="#264f78",
                        selectforeground="white", relief=tk.FLAT, bd=0,
                        font=("Segoe UI", 11))
        lst.pack(fill=tk.BOTH, expand=True)
        # 所有命令
        all_commands = [
            ("📄 新建文件", self.new_file),
            ("📂 打开文件", self.open_file),
            ("💾 保存", self.save_file),
            ("▶ 编译运行 (F5)", self.run_file),
            ("🔍 查找替换 (Ctrl+F)", self.show_find),
            ("📋 切换大纲", self.toggle_outline),
            ("🎨 切换主题", self.cycle_theme),
            ("🔎 项目中查找 (Ctrl+Shift+F)", self.project_search),
            ("↗ 转到行 (Ctrl+G)", self.goto_line),
            ("🔀 Git 状态", self.show_git),
            ("ℹ 环境信息", self.env_info),
        ]
        for cmd_name, _ in all_commands:
            lst.insert(tk.END, cmd_name)

        def filter_list(*args):
            q = entry.get().lower()
            lst.delete(0, tk.END)
            for cmd_name, _ in all_commands:
                if q in cmd_name.lower():
                    lst.insert(tk.END, cmd_name)

        def execute():
            sel = lst.curselection()
            if sel:
                idx = sel[0]
                # 查找对应命令（考虑过滤后的索引）
                q = entry.get().lower()
                filtered = [(n, c) for n, c in all_commands if q in n.lower()]
                if filtered:
                    _, cmd = filtered[idx]
                    cmd()
            d.destroy()

        entry.bind("<KeyRelease>", filter_list)
        entry.bind("<Return>", lambda e: execute())
        entry.bind("<Escape>", lambda e: d.destroy())
        lst.bind("<Double-1>", lambda e: execute())
        lst.bind("<Return>", lambda e: execute())

        # 定位到屏幕中央
        d.update_idletasks()
        x = self.root.winfo_x() + (self.root.winfo_width() - 500) // 2
        y = self.root.winfo_y() + 80
        d.geometry(f"+{x}+{y}")

    # ═══════════════════════════ 最近文件更新 ═══════════════════════════

    def update_recent_menu(self):
        """更新最近文件菜单"""
        self.recent_menu.delete(0, tk.END)
        for path in self.recent_files[-10:]:
            name = os.path.basename(path)
            self.recent_menu.add_command(label=name, command=lambda p=path: self.open_path(p))
        if not self.recent_files:
            self.recent_menu.add_command(label="(无)", state=tk.DISABLED)

    # ═══════════════════════════ 字体缩放 ═══════════════════════════

    def on_mousewheel(self, e):
        """普通滚轮：更新行号"""
        self.update_ln()

    def on_zoom(self, e):
        """Ctrl+滚轮：缩放字体"""
        delta = e.delta
        if delta > 0: self.font_size = min(28, self.font_size + 1)
        else: self.font_size = max(8, self.font_size - 1)
        self._apply_font_size()

    def zoom_in(self):
        self.font_size = min(28, self.font_size + 1)
        self._apply_font_size()

    def zoom_out(self):
        self.font_size = max(8, self.font_size - 1)
        self._apply_font_size()

    def _apply_font_size(self):
        """应用字体大小"""
        self.text.config(font=("Consolas", self.font_size))
        self.update_ln()
        self.sl.config(text=f"字体: {self.font_size}")

    # ═══════════════════════════ 自动保存 ═══════════════════════════

    def on_focus_out(self, e):
        """失去焦点时自动保存已命名的文件"""
        t = self.tab()
        if t and t.path and t.modified:
            c = self.text.get("1.0", tk.END).rstrip("\n")
            try:
                with open(t.path, "w", encoding="utf-8") as f: f.write(c)
                t.modified = False; t.content = c
                self.update_tabs()
            except: pass

    # ═══════════════════════════ 自动换行 ═══════════════════════════

    def toggle_wrap(self):
        """切换自动换行"""
        self.wrap_mode = not self.wrap_mode
        self.text.config(wrap=tk.WORD if self.wrap_mode else tk.NONE)
        self.log(f"换行: {'开' if self.wrap_mode else '关'}", "info")

    def toggle_outline(self):
        """切换大纲面板"""
        self.outline_visible = not self.outline_visible
        if self.outline_visible:
            self.outline_frame.config(width=180)
            self.update_outline()
        else:
            self.outline_frame.config(width=0)

    # ═══════════════════════════ 错误标记 ═══════════════════════════

    def clear_error_marks(self):
        """清除所有错误标记"""
        self.text.tag_remove("error_squiggle", "1.0", tk.END)
        self.error_marks = []

    def mark_errors_from_output(self, output):
        """从编译器输出解析错误并标记"""
        self.clear_error_marks()
        for line in output.split("\n"):
            # 匹配常见错误格式: "filename.cp:line:col: error message"
            m = re.search(r"(?:第?\s*(\d+)\s*行|:(\d+):|\((\d+)\))\s*(.*?)(?:错误|error)", line, re.I)
            if m:
                line_num = int(m.group(1) or m.group(2) or m.group(3) or 0)
                msg = m.group(4).strip()
                if line_num > 0 and line_num <= int(self.text.index("end-1c").split(".")[0]):
                    self.text.tag_add("error_squiggle", f"{line_num}.0", f"{line_num}.end")
                    self.error_marks.append((line_num, msg))

    # ═══════════════════════════ 代码片段 ═══════════════════════════

    SNIPPETS = {
        "fn": ("函数 名称() {\n    \n}", "函数定义"),
        "if": ("如果 (条件) {\n    \n}", "如果条件"),
        "else": ("否则 {\n    \n}", "否则分支"),
        "elif": ("否则 如果 (条件) {\n    \n}", "否则如果"),
        "while": ("当 (条件) {\n    \n}", "当循环"),
        "for": ("循环 (变量=0; 变量<10; 变量++) {\n    \n}", "循环"),
        "var": ("变量 名称 = 值;", "变量声明"),
        "const": ("常量 名称 = 值;", "常量声明"),
        "class": ("类 名称 {\n    \n}", "类定义"),
        "main": ("函数 主() {\n    \n}\n\n主();", "主函数"),
        "print": ("打印(\"\");", "打印输出"),
        "table": ("变量 t = 表();\n表设(t, \"键\", 值);\n表取(t, \"键\")", "表操作"),
        "ig": ("初始化窗口(800, 600, \"标题\");\nigInit();\n设置目标帧率(60);\n当 (!窗口应关闭()) {\n    开始绘制();\n    igBeginDraw();\n    igEndDraw();\n    结束绘制();\n}\nigShutdown();\n关闭窗口();", "ImGui 模板"),
    }

    def insert_snippet(self, name=None):
        """插入代码片段"""
        if name is None:
            self.show_snippet_picker()
            return
        snippet = self.SNIPPETS.get(name)
        if not snippet: return
        code, desc = snippet
        self.text.insert(tk.INSERT, code)
        self.log(f"插入片段: {desc}", "info")

    def show_snippet_picker(self):
        """显示片段选择器"""
        d = tk.Toplevel(self.root); d.title("代码片段"); d.geometry("350x400")
        d.transient(self.root); d.grab_set()
        tk.Label(d, text="选择代码片段:", font=("",10,"bold")).pack(pady=5)
        lst = tk.Listbox(d, font=("Consolas",10), bg="#2d2d2d", fg="#d4d4d4",
                         selectbackground="#264f78", selectforeground="white")
        lst.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)
        for key in sorted(self.SNIPPETS.keys()):
            code, desc = self.SNIPPETS[key]
            lst.insert(tk.END, f"{key:10s}  {desc}")
        def insert():
            sel = lst.curselection()
            if sel:
                key = list(sorted(self.SNIPPETS.keys()))[sel[0]]
                code, desc = self.SNIPPETS[key]
                self.text.insert(tk.INSERT, code)
                self.log(f"插入片段: {desc}", "info")
                d.destroy()
        lst.bind("<Double-1>", lambda e: insert())
        ttk.Button(d, text="插入", command=insert).pack(pady=5)

    # ═══════════════════════════ 编码检测 ═══════════════════════════

    def detect_encoding(self, path):
        """检测文件编码"""
        try:
            with open(path, "rb") as f:
                raw = f.read(4)
            if raw.startswith(b"\xef\xbb\xbf"): return "utf-8-sig"
            if raw.startswith(b"\xff\xfe") or raw.startswith(b"\xfe\xff"): return "utf-16"
            # 尝试 UTF-8
            try:
                with open(path, "r", encoding="utf-8") as f: f.read(); return "utf-8"
            except UnicodeDecodeError:
                try:
                    with open(path, "r", encoding="gbk") as f: f.read(); return "gbk"
                except: pass
            return "utf-8"
        except: return "utf-8"

    # ═══════════════════════════ 注释切换 ═══════════════════════════

    def toggle_comment(self, e=None):
        """Ctrl+/ 切换行注释"""
        try:
            sel = self.text.tag_ranges("sel")
            if sel:
                start_line = int(self.text.index(sel[0]).split(".")[0])
                end_line = int(self.text.index(sel[1]).split(".")[0])
            else:
                cur = self.text.index(tk.INSERT)
                start_line = end_line = int(cur.split(".")[0])

            all_commented = True
            for ln in range(start_line, end_line + 1):
                txt = self.text.get(f"{ln}.0", f"{ln}.0+2c")
                if txt != "//":
                    all_commented = False; break

            for ln in range(start_line, end_line + 1):
                line_text = self.text.get(f"{ln}.0", f"{ln}.end")
                if all_commented:
                    # 取消注释
                    if line_text.startswith("//"):
                        self.text.delete(f"{ln}.0", f"{ln}.0+2c")
                else:
                    # 添加注释
                    self.text.insert(f"{ln}.0", "//")
            return "break"
        except: return None

    # ═══════════════════════════ 行移动 ═══════════════════════════

    def get_line_text(self, line_num):
        return self.text.get(f"{line_num}.0", f"{line_num}.end")

    def move_line_up(self, e=None):
        """Alt+↑ 上移当前行"""
        try:
            cur = self.text.index(tk.INSERT)
            line = int(cur.split(".")[0])
            col = int(cur.split(".")[1])
            if line <= 1: return "break"
            txt_cur = self.get_line_text(line)
            txt_prev = self.get_line_text(line - 1)
            self.text.delete(f"{line-1}.0", f"{line-1}.end")
            self.text.insert(f"{line-1}.0", txt_cur)
            self.text.delete(f"{line}.0", f"{line}.end")
            self.text.insert(f"{line}.0", txt_prev)
            self.text.mark_set(tk.INSERT, f"{line-1}.{col}")
            return "break"
        except: return None

    def move_line_down(self, e=None):
        """Alt+↓ 下移当前行"""
        try:
            cur = self.text.index(tk.INSERT)
            line = int(cur.split(".")[0])
            col = int(cur.split(".")[1])
            total = int(self.text.index("end-1c").split(".")[0])
            if line >= total: return "break"
            txt_cur = self.get_line_text(line)
            txt_next = self.get_line_text(line + 1)
            self.text.delete(f"{line}.0", f"{line}.end")
            self.text.insert(f"{line}.0", txt_next)
            self.text.delete(f"{line+1}.0", f"{line+1}.end")
            self.text.insert(f"{line+1}.0", txt_cur)
            self.text.mark_set(tk.INSERT, f"{line+1}.{col}")
            return "break"
        except: return None

    def duplicate_line(self, e=None):
        """Ctrl+D 复制当前行"""
        try:
            cur = self.text.index(tk.INSERT)
            line = int(cur.split(".")[0])
            txt = self.get_line_text(line)
            self.text.insert(f"{line+1}.0", txt + "\n")
            return "break"
        except: return None

    # ═══════════════════════════ 文档大纲 ═══════════════════════════

    def generate_outline(self):
        """从当前内容提取函数/变量定义作为大纲"""
        content = self.text.get("1.0", tk.END)
        items = []
        # 匹配函数定义
        for m in re.finditer(r"(?:函数|func|fn)\s+(\w+)\s*\(", content):
            line = content[:m.start()].count("\n") + 1
            items.append(("🔵 函数", m.group(1), line))
        # 匹配变量定义
        for m in re.finditer(r"(?:变量|var|常量|const)\s+(\w+)\s*[=:=]", content):
            line = content[:m.start()].count("\n") + 1
            items.append(("🟢 变量", m.group(1), line))
        # 匹配类定义
        for m in re.finditer(r"(?:类|class|结构|struct|枚举|enum)\s+(\w+)", content):
            line = content[:m.start()].count("\n") + 1
            items.append(("🟣 类型", m.group(1), line))
        return items

    def update_outline(self):
        """刷新大纲树"""
        if not hasattr(self, 'outline_tree') or not self.outline_tree: return
        self.outline_tree.delete(*self.outline_tree.get_children())
        items = self.generate_outline()
        for cat, name, line in items:
            self.outline_tree.insert("", "end", text=f"{name}  :{line}", values=(line,))

    def on_outline_click(self, e):
        """点击大纲项跳转到对应行"""
        sel = self.outline_tree.selection()
        if not sel: return
        v = self.outline_tree.item(sel[0], "values")
        if v:
            try:
                ln = int(v[0])
                self.text.see(f"{ln}.0")
                self.text.mark_set(tk.INSERT, f"{ln}.0")
                self.text.focus_set()
            except: pass

    # ═══════════════════════════ 代码跳转 ═══════════════════════════

    def build_definitions(self):
        """构建函数/变量定义索引"""
        self.definitions = {}
        content = self.text.get("1.0", tk.END)
        for m in re.finditer(r"(?:函数|func|fn|class|类|结构|struct)\s+(\w+)", content):
            line = content[:m.start()].count("\n") + 1
            self.definitions[m.group(1)] = line

    def on_ctrl_motion(self, e):
        """Ctrl 悬停时高亮可点击的词"""
        self.text.tag_remove("definition", "1.0", tk.END)
        if e.state & 0x0004:  # Ctrl key pressed
            try:
                idx = self.text.index(f"@{e.x},{e.y}")
                word = self.text.get(f"{idx} wordstart", f"{idx} wordend")
                if word in self.definitions:
                    # 高亮所有出现
                    content = self.text.get("1.0", tk.END)
                    for m in re.finditer(rf"\b{re.escape(word)}\b", content):
                        self._tag(m, "definition")
            except: pass

    def on_ctrl_click(self, e):
        """Ctrl+点击跳转到定义"""
        if not (e.state & 0x0004): return  # 没按 Ctrl
        try:
            idx = self.text.index(f"@{e.x},{e.y}")
            word = self.text.get(f"{idx} wordstart", f"{idx} wordend")
            if word in self.definitions:
                ln = self.definitions[word]
                self.text.see(f"{ln}.0")
                self.text.mark_set(tk.INSERT, f"{ln}.0")
                self.text.focus_set()
                self.log(f"跳转到: {word} (第{ln}行)", "info")
        except: pass

    # ═══════════════════════════ 自动更新 ═══════════════════════════

    def check_update(self):
        """后台检查 GitHub 上的新版本"""
        import time
        time.sleep(3)  # 等 IDE 启动完成
        try:
            req = urllib.request.Request(IDE_VERSION_URL, headers={"User-Agent": "CPIDE"})
            with urllib.request.urlopen(req, timeout=10) as resp:
                data = json.loads(resp.read().decode("utf-8"))
            latest = data.get("version", "")
            if latest and latest > IDE_VERSION:
                notes = data.get("release_notes", "")
                def show():
                    if messagebox.askyesno("发现新版本",
                        f"新版本 v{latest} 可用 (当前 v{IDE_VERSION})\n\n"
                        f"更新说明:\n{notes}\n\n是否立即下载更新?"):
                        self.do_update(data)
                self.root.after(100, show)
            else:
                self.log(f"已是最新版本 v{IDE_VERSION}", "ok")
        except Exception as e:
            pass  # 网络不可用时不提示

    def do_update(self, data):
        """下载并更新 IDE"""
        self.log("正在下载更新...", "info")
        try:
            # 下载新版本
            url = data.get("download_url", "")
            if not url:
                # 从 update_url 推导
                base = os.path.dirname(data.get("update_url", ""))
                url = f"{base}/cp_ide.py"
            req = urllib.request.Request(url, headers={"User-Agent": "CPIDE"})
            with urllib.request.urlopen(req, timeout=30) as resp:
                new_code = resp.read().decode("utf-8")
            # 备份当前版本
            backup = os.path.join(self.ide_dir, f"cp_ide_v{IDE_VERSION}.py.bak")
            shutil.copy2(os.path.join(self.ide_dir, "cp_ide.py"), backup)
            # 写入新版本
            target = os.path.join(self.ide_dir, "cp_ide.py")
            with open(target, "w", encoding="utf-8") as f:
                f.write(new_code)
            self.log(f"更新完成！已备份旧版本到 {backup}", "ok")
            self.log("请重启 IDE 以应用更新", "info")
            if messagebox.askyesno("更新完成", "重启 IDE 以应用更新？"):
                python = sys.executable
                os.startfile(target)
                self.root.quit()
        except Exception as e:
            self.log(f"更新失败: {e}", "err")
            messagebox.showerror("更新失败", str(e))

    # ═══════════════════════════ 文件树自动刷新 ═══════════════════════════

    def start_file_watcher(self):
        """启动文件树自动刷新线程"""
        def watch():
            last_mtimes = {}
            while hasattr(self, 'root') and self.root:
                try:
                    dirs = [os.path.join(self.current_dir, d)
                           for d in os.listdir(self.current_dir)
                           if os.path.isdir(os.path.join(self.current_dir, d)) and not d.startswith(".")]
                    dirs.append(self.current_dir)
                    changed = False
                    for d in dirs:
                        try:
                            mtime = os.path.getmtime(d)
                            if d not in last_mtimes:
                                last_mtimes[d] = mtime
                            elif mtime != last_mtimes[d]:
                                last_mtimes[d] = mtime
                                changed = True
                        except: pass
                    if changed:
                        self.root.after(100, self.refresh_tree)
                except: pass
                import time; time.sleep(2)
        t = threading.Thread(target=watch, daemon=True)
        t.start()

    def start_lsp(self):
        try:
            lsp = os.path.join(os.path.dirname(os.path.abspath(__file__)), "cplsp.py")
            if not os.path.exists(lsp):
                lsp = os.path.join(self.cplang_home, "tools", "cplsp.py")
            if not os.path.exists(lsp): return
            python = sys.executable
            self.lsp = subprocess.Popen([python, lsp], stdin=subprocess.PIPE,
                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                cwd=self.cplang_home, env=os.environ)
            self.lsp_ok = True
            self.t_lsp = threading.Thread(target=self.lsp_read, daemon=True)
            self.t_lsp.start()
            self.lsp_send({"jsonrpc":"2.0","id":1,"method":"initialize",
                "params":{"processId":os.getpid(),
                "rootUri":f"file:///{self.cplang_home.replace(os.sep,'/')}",
                "capabilities":{}}})
            self.lsp_send({"jsonrpc":"2.0","method":"initialized","params":{}})
            self.log("LSP 已启动", "info")
        except Exception as e:
            self.log(f"LSP 失败: {e}", "warn")

    def lsp_send(self, msg):
        if not hasattr(self,'lsp') or not self.lsp or not self.lsp.stdin: return
        try:
            d = json.dumps(msg, ensure_ascii=False)
            self.lsp.stdin.write(f"Content-Length: {len(d.encode('utf-8'))}\r\n\r\n".encode())
            self.lsp.stdin.write(d.encode("utf-8")); self.lsp.stdin.flush()
        except: self.lsp_ok = False

    def lsp_read(self):
        buf = ""
        while hasattr(self,'lsp') and self.lsp and self.lsp.stdout:
            try:
                c = self.lsp.stdout.read(4096)
                if not c: break
                buf += c.decode("utf-8", errors="replace")
                while "\r\n\r\n" in buf:
                    i = buf.index("\r\n\r\n")
                    h = buf[:i]
                    m = re.search(r"Content-Length:\s*(\d+)", h)
                    if not m: buf = buf[i+4:]; continue
                    n = int(m.group(1)); bs = i+4
                    if len(buf) < bs + n: break
                    b = buf[bs:bs+n]; buf = buf[bs+n:]
                    try:
                        msg = json.loads(b)
                        if "method" in msg and msg["method"] == "textDocument/publishDiagnostics":
                            diag = msg.get("params",{}).get("diagnostics",[])
                            for d in diag:
                                ln = d.get("range",{}).get("start",{}).get("line",0)+1
                                self.log(f"  第{ln}行: {d.get('message','')}", "err" if d.get("severity",1)<=2 else "warn")
                    except: pass
            except: break


def main():
    root = tk.Tk()
    app = CPIDE(root)
    if len(sys.argv) > 1:
        p = os.path.abspath(sys.argv[1])
        if os.path.exists(p): root.after(200, lambda: app.open_path(p))
    root.mainloop()

if __name__ == "__main__":
    main()
