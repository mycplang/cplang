// 精确模拟 API 请求处理

函数 去除查询(路径) {
    变量 q = 查找(路径, "?");
    如果 (q == -1) { 返回 路径; }
    返回 子串(路径, 0, 取整(q));
 }

函数 提取体(请求) {
    变量 hd = 查找(请求, "\r\n\r\n");
    如果 (hd == -1) { 返回 ""; }
    返回 子串(请求, 取整(hd + 4));
 }

函数 主() {
    // 模拟 curl POST /api/set 的发包
    变量 请求 = "POST /api/set HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: curl/8.0.1\r\nAccept: */*\r\nContent-Type: application/json\r\nContent-Length: 26\r\n\r\n{\"key\":\"x\",\"value\":\"y\"}";
    
    打印("len=" + toString(长度(请求)));
    
    // 提取路径
    变量 p1 = 查找(请求, " ");
    变量 after = 子串(请求, 取整(p1 + 1));
    变量 p2 = 查找(after, " ");
    变量 path = 子串(after, 0, 取整(p2));
    打印("path=[" + path + "]");
    
    // 去查询
    变量 real = 去除查询(path);
    打印("real=[" + real + "]");
    
    // 提取 body
    变量 body = 提取体(请求);
    打印("body=[" + body + "] len=" + toString(长度(body)));
    
    // jsonParse
    变量 data = jsonParse(body);
    打印("json=" + jsonStringify(data));
    
    变量 key = 表取(data, "key");
    打印("key=[" + key + "]");
    打印("isNil=" + toString(是空(key)));
    打印("key==\"\": " + toString(key == ""));
    
    变量 val = 表取(data, "value");
    打印("val=[" + val + "]");
    
    打印("DONE");
    返回 0;
 }
主();
