导入 "@cp/net";  // 网络模块 (HTTP + JSON)
// CP Web 演示 — 简单静态文件服务器 + API
// 依赖: TCP监听 TCP接受 TCP发送 TCP接收 TCP关闭 读取文件 文件存在

函数 响应(b) {
    返回 "HTTP/1.1 200 OK\r\nContent-Length: " + 长度(b) + "\r\nContent-Type: text/html; charset=utf-8\r\nConnection: close\r\n\r\n" + b;
}

函数 处理请求(path) {
    变量 realPath = "./web" + path;
    如果 (path == "/") { realPath = "./web/index.html"; }
    如果 (文件存在(realPath)) {
        返回 响应(读取文件(realPath));
    }
    返回 响应("<h1>404 Not Found</h1><p>" + path + "</p>");
}

函数 主() {
    打印("CP Web 服务器启动: http://localhost:8080");
    变量 s = TCP监听(8080);
    当 (真) {
        变量 c = TCP接受(s);
        如果 (c == -1) { 继续; }
        变量 r = TCP接收(c, 8192);
        如果 (是空(r)) { TCP关闭(c); 继续; }

        // 提取路径
        变量 p1 = 查找(r, " ");
        变量 path = "/";
        如果 (p1 != -1) {
            变量 after = 子串(r, 取整(p1 + 1));
            变量 p2 = 查找(after, " ");
            如果 (p2 != -1) { path = 子串(after, 0, 取整(p2)); }
        }

        变量 resp = 处理请求(path);
        TCP发送(c, resp);
        TCP关闭(c);
    }
}

主();