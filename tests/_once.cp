// 单次 API 测试
函数 主() {
    变量 s = TCP监听(8080);
    变量 c = TCP接受(s);
    变量 r = TCP接收(c, 4096);
    
    // API 处理
    变量 p1 = 查找(r, " ");
    变量 rest = 子串(r, 取整(p1 + 1));
    变量 p2 = 查找(rest, " ");
    变量 path = 子串(rest, 0, 取整(p2));
    
    变量 hd = 查找(r, "\r\n\r\n");
    变量 body = 子串(r, 取整(hd + 4));
    
    打印("PATH: " + path);
    打印("BDY: " + body);
    打印("LEN: " + toString(长度(body)));
    
    如果 (path == "/api/set") {
        变量 data = jsonParse(body);
        变量 k = 表取(data, "key");
        变量 v = 表取(data, "value");
        打印("K: " + k);
        打印("V: " + v);
        打印("K_NIL: " + toString(是空(k)));
    }
    
    TCP发送(c, "HTTP/1.1 200 OK\r\n\r\nOK");
    TCP关闭(c);
    TCP关闭(s);
 }
主();
