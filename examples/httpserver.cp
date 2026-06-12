导入 "@cp/net";  // 网络模块 (HTTP + JSON)
// ═══════════════════════════════════════════════════════════
// CP HTTP 服务器框架 — 纯 CP 语言实现
// ═══════════════════════════════════════════════════════════

// ── 全局路由表 ──
变量 路由表 = [];
变量 静态目录 = "./web";

// ── 工具函数 ──
函数 字符串包含(s, sub) {
    // 简单包含检测
    i = 0;
    while (i + 长度(sub) <= 长度(s)) {
        如果 (子串(s, i, 长度(sub)) == sub) { 返回 真; }
        i = i + 1;
    }
    返回 假;
}

函数 字符串分割(s, delim) {
    // 返回数组
    结果 = [];
    pos = 0; last = 0;
    while (pos < 长度(s)) {
        如果 (子串(s, pos, 长度(delim)) == delim) {
            追加(结果, 子串(s, last, pos - last));
            pos = pos + 长度(delim);
            last = pos;
        } 否则 {
            pos = pos + 1;
        }
    }
    追加(结果, 子串(s, last, pos - last));
    返回 结果;
}

函数 字符串替换(s, from, to) {
    结果 = ""; i = 0; fl = 长度(from);
    while (i < 长度(s)) {
        如果 (i + fl <= 长度(s) && 子串(s, i, fl) == from) {
            结果 = 结果 + to; i = i + fl;
        } 否则 {
            结果 = 结果 + 子串(s, i, 1); i = i + 1;
        }
    }
    返回 结果;
}

// ── 路由注册 ──
函数 路由(方法, 路径, 处理器) {
    条目 = {};
    表设(条目, "方法", 方法);
    表设(条目, "路径", 路径);
    表设(条目, "处理器", 处理器);
    追加(路由表, 条目);
}

函数 静态文件(目录) {
    静态目录 = 目录;
}

// ── MIME 类型 ──
函数 获取MIME(path) {
    如果 (字符串包含(path, ".html")) { 返回 "text/html; charset=utf-8"; }
    如果 (字符串包含(path, ".css"))  { 返回 "text/css; charset=utf-8"; }
    如果 (字符串包含(path, ".js"))   { 返回 "application/javascript"; }
    如果 (字符串包含(path, ".json")) { 返回 "application/json"; }
    如果 (字符串包含(path, ".png"))  { 返回 "image/png"; }
    如果 (字符串包含(path, ".svg"))  { 返回 "image/svg+xml"; }
    返回 "text/plain; charset=utf-8";
}

// ── HTTP 请求解析 ──
函数 解析请求(raw) {
    请求 = {};

    // 第一行: GET /path HTTP/1.1
    lines = 字符串分割(raw, "\n");
    如果 (表长(lines) == 0) { 返回 {}; }

    firstLine = lines[0];
    parts = 字符串分割(firstLine, " ");
    如果 (表长(parts) >= 2) {
        表设(请求, "方法", parts[0]);
        表设(请求, "路径", parts[1]);
    }
    
    // 解析头部
    i = 1; headers = {};
    while (i < 表长(lines)) {
        line = lines[i];
        // 空行 = body 开始
        如果 (line == "" || line == "\r") { break; }
        // 去掉 \r
        如果 (子串(line, 长度(line)-1, 1) == "\r") {
            line = 子串(line, 0, 长度(line)-1);
        }
        colon = 0; found = 假;
        j = 0;
        while (j < 长度(line)) {
            如果 (子串(line, j, 1) == ":") { colon = j; found = 真; break; }
            j = j + 1;
        }
        如果 (found) {
            key = 子串(line, 0, colon);
            val = 子串(line, colon + 2, 长度(line) - colon - 2);
            表设(headers, key, val);
        }
        i = i + 1;
    }
    表设(请求, "头部", headers);
    
    // 解析 body (如果 Content-Length 存在)
    contentLen = 0;
    如果 (表有(headers, "Content-Length")) {
        contentLen = 转整数(表取(headers, "Content-Length"));
    }
    如果 (contentLen > 0 && i + 1 < 表长(lines)) {
        body = lines[i + 1];
        i2 = i + 2;
        while (i2 < 表长(lines)) { body = body + "\n" + lines[i2]; i2 = i2 + 1; }
        表设(请求, "主体", body);
    }
    
    返回 请求;
}

// ── HTTP 响应构建 ──
函数 构建响应(状态码, 内容类型, body) {
    statusText = "200 OK";
    如果 (状态码 == 404) { statusText = "404 Not Found"; }
    如果 (状态码 == 500) { statusText = "500 Internal Error"; }
    如果 (状态码 == 302) { statusText = "302 Found"; }
    
    响应 = "HTTP/1.1 " + 转字符串(状态码) + " " + statusText + "\r\n";
    响应 = 响应 + "Content-Type: " + 内容类型 + "\r\n";
    响应 = 响应 + "Content-Length: " + 转字符串(长度(body)) + "\r\n";
    响应 = 响应 + "Connection: close\r\n";
    响应 = 响应 + "Server: CP-HTTP/0.1\r\n";
    响应 = 响应 + "\r\n";
    响应 = 响应 + body;
    返回 响应;
}

// ── 路由匹配 ──
函数 匹配路由(请求) {
    method = 表取(请求, "方法");
    path = 表取(请求, "路径");

    i = 0;
    while (i < 表长(路由表)) {
        route = 路由表[i];
        如果 (表取(route, "方法") == method && 表取(route, "路径") == path) {
            变量 handler = 表取(route, "处理器");
            // handler 是函数时调用它，否则直接返回
            返回 handler(请求);
        }
        i = i + 1;
    }

    // 静态文件 fallback
    如果 (method == "GET") {
        filePath = 静态目录 + path;
        如果 (path == "/") { filePath = 静态目录 + "/index.html"; }
        如果 (文件存在(filePath)) {
            content = 读取文件(filePath);
            returnType = 获取MIME(filePath);
            返回 构建响应(200, returnType, content);
        }
    }

    返回 构建响应(404, "text/plain", "404 Not Found: " + path);
}

// ── 启动服务器 ──
函数 启动服务器(端口) {
    打印("CP HTTP 服务器启动在 http://localhost:" + 转字符串(端口));

    sock = TCP监听(端口);
    如果 (sock < 0) {
        打印("错误: 无法监听端口 " + 转字符串(端口));
        返回;
    }

    while (真) {
        client = TCP接受(sock);
        如果 (client < 0) { continue; }

        // 接收 HTTP 请求
        raw = TCP接收(client, 8192);
        如果 (raw == "") { TCP关闭(client); continue; }

        // 解析请求
        请求 = 解析请求(raw);
        方法 = 表取(请求, "方法");
        路径 = 表取(请求, "路径");

        打印(方法 + " " + 路径);

        // 匹配路由并处理
        响应 = 匹配路由(请求);

        // 发送响应
        TCP发送(client, 响应);
        TCP关闭(client);
    }
}
