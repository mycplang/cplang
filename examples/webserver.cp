导入 "@cp/net";  // 网络模块 (HTTP + JSON)
// webserver — CP HTTP 服务器演示（使用 TCP 原生函数）
// 依赖: TCP监听 TCP接受 TCP发送 TCP接收 TCP关闭
//       读取文件 文件存在 查找 子串 长度

函数 响应(c, 码, 类型, 内容) {
    变量 状态文本 = "OK";
    如果 (码 == 404) { 状态文本 = "Not Found"; }
    如果 (码 == 500) { 状态文本 = "Internal Error"; }

    变量 回应 = "HTTP/1.1 " + 转字符串(码) + " " + 状态文本 + "\r\n";
    回应 = 回应 + "Content-Type: " + 类型 + "\r\n";
    回应 = 回应 + "Content-Length: " + 转字符串(长度(内容)) + "\r\n";
    回应 = 回应 + "Connection: close\r\n";
    回应 = 回应 + "Server: CP-HTTP/0.1\r\n\r\n";
    回应 = 回应 + 内容;
    TCP发送(c, 回应);
}

函数 响应JSON(c, 码, 数据) {
    响应(c, 码, "application/json", 数据);
}

函数 提取路径(r) {
    变量 p1 = 查找(r, " ");
    如果 (p1 == -1) { 返回 "/"; }
    变量 after = 子串(r, 取整(p1 + 1));
    变量 p2 = 查找(after, " ");
    如果 (p2 == -1) { 返回 "/"; }
    返回 子串(after, 0, 取整(p2));
}

函数 提取方法(r) {
    变量 p1 = 查找(r, " ");
    如果 (p1 == -1) { 返回 "GET"; }
    返回 子串(r, 0, 取整(p1));
}

函数 获取查询参数(r, 键) {
    变量 p = 提取路径(r);
    变量 q = 查找(p, "?");
    如果 (q == -1) { 返回 ""; }
    变量 qs = 子串(p, 取整(q + 1));
    变量 kp = 查找(qs, 键 + "=");
    如果 (kp == -1) { 返回 ""; }
    变量 vs = 子串(qs, 取整(kp + 长度(键) + 1));
    变量 amp = 查找(vs, "&");
    如果 (amp == -1) { 返回 vs; }
    返回 子串(vs, 0, 取整(amp));
}

函数 路由静态文件(c, p) {
    变量 root = "C:/cplang/www";
    变量 real = root + p;
    如果 (文件存在(real)) {
        变量 content = 读取文件(real);
        变量 mime = "text/plain";
        如果 (包含(real, ".html")) { mime = "text/html"; }
        如果 (包含(real, ".css"))  { mime = "text/css"; }
        如果 (包含(real, ".js"))   { mime = "application/javascript"; }
        如果 (包含(real, ".json")) { mime = "application/json"; }
        如果 (包含(real, ".png"))  { mime = "image/png"; }
        如果 (包含(real, ".svg"))  { mime = "image/svg+xml"; }
        响应(c, 200, mime, content);
        返回 真;
    }
    返回 假;
}

函数 处理(c, method, path) {
    // === 静态文件 ===
    如果 (路由静态文件(c, path)) { 返回; }

    // === 首页 ===
    如果 (path == "/") {
        变量 html = "<!DOCTYPE html><html><head><meta charset=utf-8><title>CP HTTP v2</title><style>body{font-family:system-ui;max-width:800px;margin:2rem auto;padding:0 1rem;background:#0d1117;color:#c9d1d9}h1{color:#58a6ff}.card{background:#161b22;border:1px solid #30363d;border-radius:8px;padding:1.5rem;margin:1rem 0}code{background:#21262d;padding:2px 6px;border-radius:4px;color:#ffa657}a{color:#58a6ff}pre{background:#161b22;padding:1rem;border-radius:8px;overflow-x:auto}</style></head><body><h1> CP HTTP v2</h1><div class=card><h2> 路由</h2><p><code>GET /</code> — 本页面</p><p><code>GET /hello</code> — Hello World</p><p><code>GET /echo?msg=...</code> — 回声</p><p><code>GET /api/time</code> — 服务器时间</p></div><div class=card><h2> 原生函数</h2><pre>TCP监听  TCP接受  TCP发送\nTCP关闭  TCP接收</pre></div></body></html>";
        响应(c, 200, "text/html", html);
        返回;
    }

    // === Hello ===
    如果 (path == "/hello") {
        响应(c, 200, "text/html", "<h1>Hello, CP!</h1>");
        返回;
    }

    // === Echo GET ===
    如果 (包含(path, "/echo")) {
        变量 msg = 获取查询参数(path, "msg");
        如果 (msg == "") { msg = "(空)"; }
        响应(c, 200, "text/plain", "Echo: " + msg);
        返回;
    }

    // === API: 服务器时间 ===
    如果 (path == "/api/time") {
        响应JSON(c, 200, "{\"time\":\"" + 转字符串(时间戳()) + "\"}");
        返回;
    }

    // === 404 ===
    响应(c, 404, "text/html", "<h1>404</h1><p>" + path + "</p>");
}

函数 主() {
    变量 sock = TCP监听(8080);
    如果 (sock < 0) {
        打印("错误: 端口 8080 被占用");
        返回;
    }
    打印("CP HTTP v2 — http://localhost:8080");

    当 (1 > 0) {
        变量 c = TCP接受(sock);
        如果 (c < 0) { 继续; }

        变量 r = TCP接收(c, 8192);
        如果 (是空(r)) { TCP关闭(c); 继续; }

        变量 path = 提取路径(r);
        变量 method = 提取方法(r);

        处理(c, method, path);
        TCP关闭(c);
    }
}

主();
