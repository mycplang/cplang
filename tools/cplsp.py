#!/usr/bin/env python3
"""CP Language Server Protocol — 代码补全/跳转/诊断/悬停"""

import sys, json, os, re, subprocess, platform
from pathlib import Path

# 跨平台路径
IS_WIN = platform.system() == "Windows"
# 优先使用环境变量 CPLANG_HOME，其次默认路径
_CPLANG_HOME = os.environ.get("CPLANG_HOME", str(Path.home() / "cplang"))
CP_COMPILER = os.path.join(_CPLANG_HOME, "build", "cplang.exe" if IS_WIN else "cplang")
CP_KEYWORDS = ["函数","function","变量","var","常量","const","返回","return",
    "如果","if","否则","else","当","while","循环","for","遍历","forEach",
    "跳出","break","继续","continue","选择","switch","情况","case","其他","default",
    "类","class","接口","interface","枚举","enum","结构体","struct",
    "新建","new","公有","public","私有","private","保护","protected",
    "真","true","假","false","空","null","nil",
    "导入","import","包名","package","类型","typedef",
    "抛出","throw","尝试","try","捕获","catch",
    "推迟","defer","信任","trust","为","as","删除","delete","是","is","不是","!is"]
def log(msg):
    """跨平台日志"""
    log_dir = os.environ.get("TEMP", "/tmp") if IS_WIN else "/tmp"
    log_path = os.path.join(log_dir, "cplsp.log")
    try:
        with open(log_path, "a", encoding="utf-8") as f:
            f.write(f"{msg}\n")
    except Exception:
        pass  # 静默处理日志错误

def load_builtins_from_grammar():
    """从 TextMate 语法文件加载内置函数列表（自动同步 666 个中英文内置名）"""
    grammar_path = Path(__file__).parent / "vscode-cp" / "syntaxes" / "cp.tmLanguage.json"
    try:
        with open(grammar_path, "r", encoding="utf-8-sig") as f:
            grammar = json.load(f)
        match_str = grammar["repository"]["builtins"]["match"]
        m = re.search(r'\\b\((.+)\)\\b', match_str)
        if m:
            return m.group(1).split("|")
    except Exception as e:
        log(f"load_builtins_from_grammar: {e}")
    return ["打印","print","println","长度","len","size","类型","typeOf","typeof",
        "tick","tock","sleep","now","随机","random","rand",
        "正弦","sin","余弦","cos","平方根","sqrt","绝对值","abs","幂","pow",
        "向上取整","ceil","向下取整","floor","四舍五入","round",
        "最大值","max","最小值","min","自然对数","log","圆周率","pi","自然常数","e",
        "字符串分割","strSplit","字符串拼接","strConcat","字符串查找","strFind",
        "转字符串","toString","parseInt","parseFloat",
        "读取文件","readFile","写入文件","writeFile","文件存在","fileExists",
        "JSON解析","jsonParse","转JSON","jsonStringify",
        "MD5","md5","SHA256","sha256","CRC32","crc32",
        "AES加密","aesEncrypt","AES解密","aesDecrypt",
        "Base64编码","base64Encode","Base64解码","base64Decode",
        "push","pop","shift","unshift",
        "排序","sort","映射","map","过滤","filter","遍历","forEach",
        "通道创建","channel","通道发送","channelSend","通道选择","channelSelect",
        "格式化","format","输入","input","断言","assert","退出","exit",
        "初始化窗口","setTargetFPS","窗口应关闭","关闭窗口","开始绘图","结束绘图",
        "清空背景","绘制矩形","绘制文本","绘制圆形","绘制线条",
        "键盘按下","鼠标位置","屏幕宽度","屏幕高度",
        "表取","表设","表删","表有","表键","表值","表长","表创建",
        "追加","插入","弹出","替换","修剪","包含","查找",
        "红","绿","蓝","黑","白","黄","紫","灰","金","亮绿","深灰","乳白"]

CP_BUILTINS = load_builtins_from_grammar()

def read_message():
    """Read JSON-RPC message from stdin"""
    headers = {}
    while True:
        line = sys.stdin.readline().strip()
        if not line:
            break
        if ":" in line:
            k, v = line.split(":", 1)
            headers[k.strip().lower()] = v.strip()
    length = int(headers.get("content-length", 0))
    if length > 0:
        content = sys.stdin.read(length)
        return json.loads(content)
    return None

def send_message(msg):
    """Send JSON-RPC response to stdout"""
    body = json.dumps(msg, ensure_ascii=False)
    sys.stdout.write(f"Content-Length: {len(body.encode('utf-8'))}\r\n\r\n{body}")
    sys.stdout.flush()

class CPLanguageServer:
    def __init__(self):
        self.documents = {}  # uri → text
        self.diagnostics = {}  # uri → [diagnostics]
        self.functions = {}  # uri → [{name, line, params}]
        self.keywords = CP_KEYWORDS
        self.builtins = CP_BUILTINS
        self.server_capabilities = {
            "textDocumentSync": 1,  # full sync
            "completionProvider": {"triggerCharacters": [".", " ", "(", "（"]},
            "hoverProvider": True,
            "definitionProvider": True,
            "documentSymbolProvider": True,
            "signatureHelpProvider": {"triggerCharacters": ["(", "（"]},
        }

    def handle_message(self, msg):
        method = msg.get("method", "")
        msg_id = msg.get("id")
        params = msg.get("params", {})

        if method == "initialize":
            return self.initialize(params, msg_id)
        elif method == "initialized":
            return None
        elif method == "textDocument/didOpen":
            self.did_open(params)
            return None
        elif method == "textDocument/didChange":
            self.did_change(params)
            return None
        elif method == "textDocument/didClose":
            self.did_close(params)
            return None
        elif method == "textDocument/completion":
            return self.completion(params, msg_id)
        elif method == "textDocument/hover":
            return self.hover(params, msg_id)
        elif method == "textDocument/definition":
            return self.definition(params, msg_id)
        elif method == "textDocument/documentSymbol":
            return self.document_symbol(params, msg_id)
        elif method == "textDocument/signatureHelp":
            return self.signature_help(params, msg_id)
        elif method == "shutdown":
            return {"jsonrpc": "2.0", "id": msg_id, "result": None}

    def initialize(self, params, msg_id):
        return {"jsonrpc": "2.0", "id": msg_id, "result": {"capabilities": self.server_capabilities,
            "serverInfo": {"name": "cplsp", "version": "0.1"}}}

    def get_uri(self, params):
        return params.get("textDocument", {}).get("uri", "")

    def did_open(self, params):
        uri = self.get_uri(params)
        text = params.get("textDocument", {}).get("text", "")
        self.documents[uri] = text
        self.analyze(uri, text)
        self.publish_diagnostics(uri)

    def did_change(self, params):
        uri = self.get_uri(params)
        changes = params.get("contentChanges", [])
        if changes:
            self.documents[uri] = changes[-1].get("text", "")
        self.analyze(uri, self.documents.get(uri, ""))
        self.publish_diagnostics(uri)

    def did_close(self, params):
        uri = self.get_uri(params)
        self.documents.pop(uri, None)
        self.diagnostics.pop(uri, None)
        self.publish_diagnostics(uri)

    def analyze(self, uri, text):
        """分析源码：提取函数定义位置 + 编译诊断"""
        funcs = []
        lines = text.split("\n")
        for i, line in enumerate(lines):
            m = re.search(r'(?:函数|function|fn)\s+(\w+)', line)
            if m:
                funcs.append({"name": m.group(1), "line": i, "params": self.extract_params(line)})
        self.functions[uri] = funcs

        # 尝试编译并获得诊断（静默模式）
        diags = []
        try:
            result = subprocess.run(
                [CP_COMPILER, "-c", "-"],
                input=text.encode("utf-8"),
                capture_output=True,
                timeout=5,
                cwd=os.path.dirname(CP_COMPILER) if os.path.exists(CP_COMPILER) else None
            )
            if result.returncode != 0:
                stderr = result.stderr.decode("utf-8", errors="replace")
                # 解析错误输出，提取行号和消息
                for line_text in stderr.split("\n"):
                    line_text = line_text.strip()
                    if not line_text:
                        continue
                    # 尝试匹配 "第X行第Y列: 消息" 格式
                    m = re.search(r'第(\d+)行(?:第(\d+)列)?:\s*(.+)', line_text)
                    if m:
                        diag_line = int(m.group(1)) - 1  # LSP 使用 0-based
                        diag_col = int(m.group(2)) if m.group(2) else 0
                        msg = m.group(3)
                        diags.append({
                            "range": {
                                "start": {"line": diag_line, "character": diag_col},
                                "end": {"line": diag_line, "character": diag_col + 1}
                            },
                            "severity": 1,  # Error
                            "message": msg,
                            "source": "cplang"
                        })
                    else:
                        # 无法解析行号的错误，附加到最后一行
                        diags.append({
                            "range": {
                                "start": {"line": max(len(lines) - 1, 0), "character": 0},
                                "end": {"line": max(len(lines) - 1, 0), "character": 1}
                            },
                            "severity": 1,
                            "message": line_text,
                            "source": "cplang"
                        })
        except (FileNotFoundError, subprocess.TimeoutExpired) as e:
            # 编译器不可用或超时，静默忽略
            pass

        self.diagnostics[uri] = diags

    def extract_params(self, line):
        m = re.search(r'\((.*?)\)', line)
        if m:
            return [p.strip() for p in m.group(1).split(",") if p.strip()]
        return []

    def publish_diagnostics(self, uri):
        """发送诊断信息"""
        diags = self.diagnostics.get(uri, [])
        send_message({"jsonrpc": "2.0", "method": "textDocument/publishDiagnostics",
            "params": {"uri": uri, "diagnostics": diags}})

    def path_from_uri(self, uri):
        """将 file:// URI 转换为本地路径（跨平台）"""
        if uri.startswith("file:///"):
            # Windows: file:///C:/path → C:/path
            return uri[8:]
        return uri.replace("file://", "")

    def completion(self, params, msg_id):
        uri = self.get_uri(params)
        position = params.get("position", {})
        line = position.get("line", 0)
        text = self.documents.get(uri, "")
        lines = text.split("\n")
        if line < len(lines):
            current_line = lines[line]
        else:
            current_line = ""

        items = []

        # CP keywords
        for kw in self.keywords:
            items.append({"label": kw, "kind": 14, "detail": "CP关键字", "sortText": "0"+kw})

        # Builtins
        for bi in self.builtins:
            items.append({"label": bi, "kind": 3, "detail": "内置函数", "sortText": "1"+bi})

        # Functions in current file
        for func in self.functions.get(uri, []):
            items.append({"label": func["name"], "kind": 3, "detail": "函数",
                "documentation": f"参数: {', '.join(func['params'])}", "sortText": "2"+func["name"]})

        # Variables in current scope
        for var in self.find_variables(text, line):
            items.append({"label": var, "kind": 6, "detail": "变量", "sortText": "3"+var})

        return {"jsonrpc": "2.0", "id": msg_id, "result": items}

    def find_variables(self, text, line):
        """简单作用域分析：提取当前位置可见的变量名"""
        vars = []
        lines = text.split("\n")
        for i in range(min(line + 1, len(lines))):
            # x = ... pattern (通用赋值)
            for m in re.finditer(r'(\w+)\s*=', lines[i]):
                name = m.group(1)
                if name not in CP_KEYWORDS and name not in self.builtins:
                    if name not in vars:
                        vars.append(name)
            # 变量/常量/var/const x = ... pattern
            for m in re.finditer(r'(?:变量|常量|var|const)\s+(\w+)', lines[i]):
                name = m.group(1)
                if name not in vars:
                    vars.append(name)
        return vars

    def hover(self, params, msg_id):
        uri = self.get_uri(params)
        position = params.get("position", {})
        line = position.get("line", 0)
        char = position.get("character", 0)
        text = self.documents.get(uri, "")
        lines = text.split("\n")
        if line >= len(lines):
            return {"jsonrpc": "2.0", "id": msg_id, "result": None}

        current_line = lines[line]
        word = self.word_at(current_line, char)

        # Check keywords
        if word in self.keywords:
            return {"jsonrpc": "2.0", "id": msg_id, "result": {"contents": f"**{word}** — CP语言关键字"}}

        # Check builtins
        if word in self.builtins:
            return {"jsonrpc": "2.0", "id": msg_id, "result": {"contents": f"**{word}** — CP内置函数"}}

        # Check functions
        for func in self.functions.get(uri, []):
            if func["name"] == word:
                params_str = ", ".join(func["params"])
                return {"jsonrpc": "2.0", "id": msg_id, "result": {
                    "contents": {"language": "cp", "value": f"函数 {word}({params_str})"}}}

        return {"jsonrpc": "2.0", "id": msg_id, "result": None}

    def word_at(self, line, char):
        if not line or char >= len(line):
            return ""
        start, end = char, char
        while start > 0 and line[start-1].isalnum():
            start -= 1
        while end < len(line) and line[end].isalnum():
            end += 1
        return line[start:end]

    def definition(self, params, msg_id):
        uri = self.get_uri(params)
        position = params.get("position", {})
        line = position.get("line", 0)
        char = position.get("character", 0)
        text = self.documents.get(uri, "")
        lines = text.split("\n")
        if line >= len(lines):
            return {"jsonrpc": "2.0", "id": msg_id, "result": None}

        word = self.word_at(lines[line], char)

        # Search for function definition
        for func in self.functions.get(uri, []):
            if func["name"] == word:
                return {"jsonrpc": "2.0", "id": msg_id, "result": {
                    "uri": uri, "range": {
                        "start": {"line": func["line"], "character": 0},
                        "end": {"line": func["line"], "character": len(func["name"]) + 3}}}}
        return {"jsonrpc": "2.0", "id": msg_id, "result": None}

    # ── 常用内置函数签名 ──
    BUILTIN_SIGNATURES = {
        "print": ["内容"], "println": ["内容"], "打印": ["内容"],
        "input": ["提示(可选)"], "输入": ["提示(可选)"],
        "len": ["数组/字符串"], "length": ["数组/字符串"], "长度": ["数组/字符串"],
        "size": ["数组/表"],
        "abs": ["数值"], "绝对值": ["数值"],
        "sqrt": ["数值"], "平方根": ["数值"],
        "pow": ["底数", "指数"], "幂": ["底数", "指数"],
        "sin": ["弧度"], "正弦": ["弧度"],
        "cos": ["弧度"], "余弦": ["弧度"],
        "tan": ["弧度"], "正切": ["弧度"],
        "max": ["值1", "值2", "..."], "最大值": ["值1", "值2", "..."],
        "min": ["值1", "值2", "..."], "最小值": ["值1", "值2", "..."],
        "floor": ["数值"], "向下取整": ["数值"],
        "ceil": ["数值"], "向上取整": ["数值"],
        "round": ["数值"], "四舍五入": ["数值"],
        "random": ["最小值(可选)", "最大值(可选)"], "随机": ["最小值(可选)", "最大值(可选)"],
        "log": ["数值"], "自然对数": ["数值"],
        "exp": ["数值"], "自然指数": ["数值"],
        "pi": [], "圆周率": [],
        "e": [], "自然常数": [],
        "toString": ["值"], "转字符串": ["值"],
        "parseInt": ["字符串"], "parseFloat": ["字符串"],
        "转整数": ["字符串"], "转浮点": ["字符串"],
        "split": ["字符串", "分隔符"], "分割": ["字符串", "分隔符"],
        "join": ["数组", "分隔符"],
        "trim": ["字符串"], "修剪": ["字符串"],
        "contains": ["字符串/数组", "子串/元素"], "包含": ["字符串/数组", "子串/元素"],
        "find": ["字符串", "目标"], "查找": ["字符串", "目标"],
        "substr": ["字符串", "起始", "长度(可选)"], "子串": ["字符串", "起始", "长度(可选)"],
        "replace": ["字符串", "旧", "新"], "替换": ["字符串", "旧", "新"],
        "push": ["数组", "元素"], "追加": ["数组", "元素"],
        "pop": ["数组"], "弹出": ["数组"],
        "insert": ["数组", "索引", "元素"], "插入": ["数组", "索引", "元素"],
        "sort": ["数组"], "排序": ["数组"],
        "map": ["数组", "函数"], "映射": ["数组", "函数"],
        "filter": ["数组", "函数"], "过滤": ["数组", "函数"],
        "keys": ["表"], "values": ["表"],
        "表取": ["表", "键"], "表设": ["表", "键", "值"],
        "表有": ["表", "键"], "表删": ["表", "键"],
        "表键": ["表"], "表值": ["表"], "表长": ["表"], "表创建": ["初始键值对(可选)"],
        "jsonParse": ["字符串"], "转JSON": ["值"],
        "jsonStringify": ["值"], "JSON解析": ["字符串"],
        "读取文件": ["路径"], "readFile": ["路径"],
        "写入文件": ["路径", "内容"], "writeFile": ["路径", "内容"],
        "文件存在": ["路径"], "fileExists": ["路径"],
        "创建目录": ["路径"], "mkdir": ["路径"],
        "目录列表": ["路径"], "listDir": ["路径"],
        "初始化窗口": ["宽度", "高度", "标题"],
        "设置目标帧率": ["帧率"],
        "窗口应关闭": [], "关闭窗口": [],
        "开始绘图": [], "结束绘图": [],
        "清空背景": ["颜色"],
        "绘制矩形": ["x", "y", "宽", "高", "颜色"],
        "绘制文本": ["文本", "x", "y", "字号", "颜色"],
        "绘制圆形": ["x", "y", "半径", "颜色"],
        "绘制线条": ["x1", "y1", "x2", "y2", "颜色"],
        "键盘按下": ["键码"], "鼠标位置": [],
        "屏幕宽度": [], "屏幕高度": [],
        "typeOf": ["值"], "类型": ["值"],
        "isNil": ["值"], "isEmpty": ["值"],
        "format": ["模板", "..."], "格式化": ["模板", "..."],
        "assert": ["条件", "消息(可选)"], "断言": ["条件", "消息(可选)"],
        "exit": ["代码(可选)"], "退出": ["代码(可选)"],
    }

    def signature_help(self, params, msg_id):
        """处理签名帮助请求"""
        uri = self.get_uri(params)
        position = params.get("position", {})
        line = position.get("line", 0)
        char = position.get("character", 0)
        text = self.documents.get(uri, "")
        lines = text.split("\n")
        if line >= len(lines):
            return {"jsonrpc": "2.0", "id": msg_id, "result": None}

        # 从当前位置往前找函数名（在 '(' 之前）
        current_line = lines[line]
        # 检查当前行在光标前的部分
        before_cursor = current_line[:char]

        # 找到最后一个 '(' 前面的函数名
        paren_pos = -1
        for pos in range(len(before_cursor) - 1, -1, -1):
            if before_cursor[pos] == '(' or before_cursor[pos] == '（':
                paren_pos = pos
                break

        if paren_pos < 0:
            return {"jsonrpc": "2.0", "id": msg_id, "result": None}

        # 提取 '(' 前面的单词
        func_start = paren_pos
        while func_start > 0 and before_cursor[func_start-1].isalnum():
            func_start -= 1
        func_name = before_cursor[func_start:paren_pos]

        if not func_name:
            return {"jsonrpc": "2.0", "id": msg_id, "result": None}

        # 计算当前参数索引（第几个参数）
        # 统计 '(' 和 ',' 在光标前的数量
        arg_count = 1  # 默认在第一个参数
        depth = 0
        for i in range(paren_pos + 1, char):
            c = current_line[i] if i < len(current_line) else ''
            if c == '(' or c == '（':
                depth += 1
            elif c == ')' or c == '）':
                depth -= 1
            elif (c == ',' or c == '，') and depth == 0:
                arg_count += 1

        # 查找用户定义函数
        signatures = []
        for func in self.functions.get(uri, []):
            if func["name"] == func_name:
                params_str = ", ".join(func["params"]) if func["params"] else ""
                signatures.append({
                    "label": f"{func_name}({params_str})",
                    "parameters": [{"label": p} for p in func["params"]]
                })

        # 查找内置函数
        if not signatures:
            sig_key = func_name
            # 尝试匹配别名
            if sig_key in self.BUILTIN_SIGNATURES:
                params_list = self.BUILTIN_SIGNATURES[sig_key]
                params_str = ", ".join(params_list)
                signatures.append({
                    "label": f"{func_name}({params_str})",
                    "parameters": [{"label": p} for p in params_list]
                })

        if not signatures:
            return {"jsonrpc": "2.0", "id": msg_id, "result": None}

        # 返回签名帮助
        active_param = min(arg_count - 1, len(signatures[0]["parameters"]) - 1) if signatures[0]["parameters"] else 0
        if active_param < 0:
            active_param = 0

        return {"jsonrpc": "2.0", "id": msg_id, "result": {
            "signatures": signatures,
            "activeSignature": 0,
            "activeParameter": active_param
        }}

    def document_symbol(self, params, msg_id):
        uri = self.get_uri(params)
        symbols = []
        for func in self.functions.get(uri, []):
            symbols.append({
                "name": func["name"],
                "kind": 12,  # Function
                "location": {"uri": uri, "range": {
                    "start": {"line": func["line"], "character": 0},
                    "end": {"line": func["line"], "character": len(func["name"]) + 3}}},
                "containerName": uri.split("/")[-1]
            })
        return {"jsonrpc": "2.0", "id": msg_id, "result": symbols}

def main():
    server = CPLanguageServer()
    log("CP LSP Server started")
    
    while True:
        try:
            msg = read_message()
            if msg is None:
                break
            result = server.handle_message(msg)
            if result is not None:
                send_message(result)
        except Exception as e:
            log(f"Error: {e}")
            import traceback
            log(traceback.format_exc())

if __name__ == "__main__":
    main()
