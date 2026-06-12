导入 "@cp/net";  // 网络模块 (HTTP + JSON)
// miniserv v8 — 从已验证的 echo 模式重构
变量 STORE = JSON解析("{}");

函数 提取路径(r) {
    变量 p1 = 查找(r, " ");
    变量 after = 子串(r, 取整(p1 + 1));
    变量 p2 = 查找(after, " ");
    返回 子串(after, 0, 取整(p2));
 }

函数 提取体(r) {
    变量 hd = 查找(r, "\r\n\r\n");
    如果 (hd == -1) {
        hd = 查找(r, "\n\n");
        如果 (hd == -1) { 返回 ""; }
        返回 子串(r, 取整(hd + 2));
    }
    返回 子串(r, 取整(hd + 4));
 }

函数 去除查询(p) {
    变量 q = 查找(p, "?");
    如果 (q == -1) { 返回 p; }
    返回 子串(p, 0, 取整(q));
 }

函数 提取查询参数(p, 键) {
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

函数 响应(状态码, 类型, 内容) {
    变量 状态文本 = "OK";
    如果 (状态码 == 404) { 状态文本 = "Not Found"; }
    如果 (状态码 == 403) { 状态文本 = "Forbidden"; }
    如果 (状态码 == 400) { 状态文本 = "Bad Request"; }
    
    变量 回应 = "HTTP/1.1 " + 转字符串(状态码) + " " + 状态文本 + "\r\n";
    回应 = 回应 + "Content-Type: " + 类型 + "; charset=utf-8\r\n";
    回应 = 回应 + "Content-Length: " + 转字符串(长度(内容)) + "\r\n";
    回应 = 回应 + "Connection: close\r\n";
    回应 = 回应 + "Server: CP/1.0\r\n\r\n";
    回应 = 回应 + 内容;
    返回 回应;
 }

函数 获取MIME(后缀) {
    如果 (后缀 == ".html") { 返回 "text/html"; }
    如果 (后缀 == ".css")  { 返回 "text/css"; }
    如果 (后缀 == ".js")   { 返回 "application/javascript"; }
    如果 (后缀 == ".json") { 返回 "application/json"; }
    如果 (后缀 == ".cp")   { 返回 "text/plain"; }
    返回 "text/plain";
 }

函数 处理请求(r) {
    变量 path = 提取路径(r);
    变量 real = 去除查询(path);
    变量 body = 提取体(r);
    
    // 安全
    如果 (包含(path, "..")) {
        返回 响应(403, "text/html", "<h1>403</h1>");
    }
    
    // API
    如果 (包含(real, "/api/")) {
        如果 (real == "/api/ping") {
            返回 响应(200, "application/json", "{\"ok\":true}");
         }
        如果 (real == "/api/set") {
            变量 data = JSON解析(body);
            变量 key = 表取(data, "key");
            变量 val = 表取(data, "value");
            如果 (是空(key)) {
                返回 响应(400, "application/json", "{\"error\":\"missing key\"}");
             }
            表设(STORE, key, val);
            返回 响应(200, "application/json", "{\"ok\":true}");
         }
        如果 (real == "/api/get") {
            变量 key = 提取查询参数(path, "key");
            如果 (key == "") {
                返回 响应(400, "application/json", "{\"error\":\"missing key\"}");
             }
            如果 (表有(STORE, key)) {
                变量 val = 表取(STORE, key);
                返回 响应(200, "application/json", 转JSON([["key",key],["value",val]]));
             }
            返回 响应(404, "application/json", "{\"error\":\"not found\"}");
         }
        如果 (real == "/api/keys") {
            变量 keys = 表键(STORE);
            返回 响应(200, "application/json", 转JSON(keys));
         }
        如果 (real == "/api/del") {
            变量 key = 提取查询参数(path, "key");
            如果 (key == "") {
                返回 响应(400, "application/json", "{\"error\":\"missing key\"}");
             }
            变量 ok = 表删(STORE, key);
            如果 (ok) {
                返回 响应(200, "application/json", "{\"ok\":true}");
             }
            返回 响应(404, "application/json", "{\"error\":\"not found\"}");
         }
        返回 响应(404, "application/json", "{\"error\":\"unknown api\"}");
    }
    
    // 路由
    如果 (real == "/") {
        变量 html = "<!DOCTYPE html><html><head><meta charset=utf-8><title>CP HTTP</title><link rel=stylesheet href=/style.css></head><body><div class=hero><h1>CP HTTP Server</h1><p>静态站点 + JSON API | CP 语言</p></div><div class=grid><div class=card><h2> 静态文件</h2><p>www/ 目录 | HTML/CSS/JS</p><a href=/index.html class=btn>首页</a></div><div class=card><h2> 路由</h2><p>/hello /echo /api/*</p><a href=/hello class=btn>打招呼</a></div><div class=card><h2>  JSON API</h2><p>KV 存储 GET/POST/DELETE</p><a href=/api/keys class=btn>列出 Keys</a></div></div><div class=api-demo><h2>  API 测试</h2><div class=form-row><input id=key placeholder=Key><input id=val placeholder=Value><button onclick=S()>POST /api/set</button><button onclick=G()>GET /api/get</button><button onclick=K()>GET /api/keys</button><button onclick=D()>GET /api/del</button></div><pre id=r>等待操作...</pre></div><script>async function call(m,p,b){try{let o={method:m};if(b){o.headers={'Content-Type':'application/json'};o.body=JSON.stringify(b)}let r=await fetch(p,o);let t=await r.text();document.getElementById('r').textContent='['+r.status+'] '+t}catch(e){document.getElementById('r').textContent='err:'+e.message}}function S(){let k=document.getElementById('key').value,v=document.getElementById('val').value;if(!k||!v){document.getElementById('r').textContent='Key和Value都填';return}call('POST','/api/set',{key:k,value:v})}function G(){let k=document.getElementById('key').value;if(!k){document.getElementById('r').textContent='填Key';return}call('GET','/api/get?key='+encodeURIComponent(k))}function K(){call('GET','/api/keys')}function D(){let k=document.getElementById('key').value;if(!k){document.getElementById('r').textContent='填Key';return}call('GET','/api/del?key='+encodeURIComponent(k))}</script></body></html>";
        返回 响应(200, "text/html", html);
    }
    如果 (real == "/hello") {
        返回 响应(200, "text/html", "<h1>Hello, CP!</h1>");
    }
    如果 (real == "/echo") {
        返回 响应(200, "text/plain", r);
    }
    
    // 静态文件
    变量 local = "C:/cplang/www" + real;
    如果 (文件存在(local)) {
        变量 content = 读取文件(local);
        变量 mime = "text/plain";
        如果 (包含(real, ".css"))  { mime = "text/css"; }
        如果 (包含(real, ".js"))   { mime = "application/javascript"; }
        如果 (包含(real, ".html")) { mime = "text/html"; }
        如果 (包含(real, ".json")) { mime = "application/json"; }
        如果 (包含(real, ".svg"))  { mime = "image/svg+xml"; }
        返回 响应(200, mime, content);
     }
    
    local = "C:/cplang" + real;
    如果 (文件存在(local)) {
        变量 content = 读取文件(local);
        变量 mime = "text/plain";
        如果 (包含(real, ".css"))  { mime = "text/css"; }
        如果 (包含(real, ".js"))   { mime = "application/javascript"; }
        如果 (包含(real, ".html")) { mime = "text/html"; }
        返回 响应(200, mime, content);
     }
    
    返回 响应(404, "text/html", "<h1>404</h1><p>" + real + "</p>");
 }

函数 提取内容长度(r) {
    变量 cl = 查找(r, "Content-Length: ");
    如果 (cl == -1) { 返回 0; }
    变量 start = 取整(cl + 16);
    变量 余 = 子串(r, start);
    变量 end = 查找(余, "\r\n");
    如果 (end == -1) { 返回 0; }
    返回 转整数(子串(余, 0, 取整(end)));
 }

函数 主() {
    变量 s = TCP监听(8080);
    打印("HTTP: http://localhost:8080");
    当 (1 > 0) {
        变量 c = TCP接受(s);
        如果 (c == -1) { 继续; }
        变量 r = TCP接收(c, 8192);
        如果 (是空(r)) { TCP关闭(c); 继续; }
        如果 (r == "") { TCP关闭(c); 继续; }
        
        // 处理TCP分片：Content-Length > 实收body时补读
        变量 cl = 提取内容长度(r);
        变量 hdEnd = 查找(r, "\r\n\r\n");
        如果 (cl > 0 && hdEnd != -1) {
            变量 expectedLen = 取整(hdEnd) + 4 + cl;
            变量 tries = 0;
            当 (长度(r) < expectedLen && tries < 5) {
                变量 more = TCP接收(c, expectedLen - 长度(r));
                如果 (是空(more)) { 跳出; }
                如果 (more == "") { 跳出; }
                r = r + more;
                tries = tries + 1;
             }
         }
        
        变量 resp = 处理请求(r);
        TCP发送(c, resp);
        TCP关闭(c);
     }
 }

主();
