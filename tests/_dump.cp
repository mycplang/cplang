函数 提取体(r) {
    变量 hd = 查找(r, "\r\n\r\n");
    如果 (hd != -1) { 返回 子串(r, 取整(hd + 4)); }
    hd = 查找(r, "\n\n");
    如果 (hd != -1) { 返回 子串(r, 取整(hd + 2)); }
    返回 "";
 }

函数 主() {
    变量 s = TCP监听(8080);
    变量 c = TCP接受(s);
    变量 r = TCP接收(c, 8192);
    
    打印("LEN=" + toString(长度(r)));
    打印("RAW_START");
    打印(r);
    打印("RAW_END");
    
    变量 body = 提取体(r);
    打印("BDY=[" + body + "] BLEN=" + toString(长度(body)));
    
    如果 (长度(body) > 0) {
        变量 data = jsonParse(body);
        打印("JSON=" + jsonStringify(data));
        打印("KEY=" + 表取(data, "key"));
     }
    
    TCP发送(c, "HTTP/1.1 200 OK\r\n\r\nDONE");
    TCP关闭(c);
    TCP关闭(s);
 }
主();
