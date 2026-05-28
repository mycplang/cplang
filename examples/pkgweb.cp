// ═══════════════════════════════════════════════════════════
// CP 包注册表 Web 应用 — 全栈 CP 语言验证项目
// ═══════════════════════════════════════════════════════════

// 引入 HTTP 框架
导入 httpserver;

// 静态文件目录
静态文件("./web");

// ── 处理 /api/packages ──
函数 处理包列表(请求) {
    数据 = {};

    // 读取本地 packages/ 目录
    文件列表 = 目录列表("packages");
    如果 (文件列表 != nil) {
        i = 0;
        while (i < 表长(文件列表)) {
            name = 文件列表[i];
            pkgPath = "packages/" + name + "/index.cp";
            如果 (文件存在(pkgPath)) {
                信息 = {};
                表设(信息, "desc", "CP语言包");
                表设(信息, "version", "0.1.0");
                表设(数据, name, 信息);
            }
            i = i + 1;
        }
    }

    // 也列出 ~/.cpkg/packages/ 中的已安装包
    已安装 = 目录列表("/root/.cpkg/packages");
    如果 (已安装 != nil) {
        j = 0;
        while (j < 表长(已安装)) {
            name = 已安装[j];
            如果 (!表有(数据, name)) {
                信息 = {};
                表设(信息, "desc", "已安装");
                表设(信息, "version", "0.1.0");
                表设(数据, name, 信息);
            }
            j = j + 1;
        }
    }

    返回 构建响应(200, "application/json", 转JSON(数据));
}

// ── 处理 /api/package/xxx ──
函数 处理包详情(请求) {
    path = 表取(请求, "路径");
    name = 子串(path, 13, 长度(path) - 13);  // /api/package/xxx
    pkgPath = "packages/" + name + "/index.cp";

    如果 (文件存在(pkgPath)) {
        content = 读取文件(pkgPath);
        信息 = {};
        表设(信息, "name", name);
        表设(信息, "code", content);
        表设(信息, "lines", 转字符串(长度(content)));
        返回 构建响应(200, "application/json", 转JSON(信息));
    }
    返回 构建响应(404, "application/json", '{"error":"not found"}');
}

// ── 处理 / ──
函数 处理主页(请求) {
    html = 读取文件("web/index.html");
    返回 构建响应(200, "text/html", html);
}

// ── 处理 /health ──
函数 处理健康检查(请求) {
    返回 构建响应(200, "application/json", '{"status":"ok","lang":"CP 语言","version":"0.1.0-beta"}');
}

// 注册路由
路由("GET", "/api/packages", 处理包列表);
路由("GET", "/api/package/", 处理包详情);
路由("GET", "/", 处理主页);
路由("GET", "/health", 处理健康检查);

// ── 启动！ ──
打印("═══════════════════════════════════════════");
打印("  CP 包注册表 — 全栈 CP 语言验证项目");
打印("  http://localhost:8080");
打印("═══════════════════════════════════════════");
启动服务器(8080);
