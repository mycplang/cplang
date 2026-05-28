函数 主() {
    变量 请求 = "POST /api/set HTTP/1.1\r\nHost: localhost:8080\r\nContent-Type: application/json\r\nContent-Length: 31\r\n\r\n{\"key\":\"hello\",\"value\":\"world\"}";
    
    打印("长度: " + toString(长度(请求)));
    变量 hd = 查找(请求, "\r\n\r\n");
    打印("空行: " + toString(hd));
    
    变量 body = 子串(请求, 取整(hd + 4));
    打印("Body: " + body);
    
    变量 data = jsonParse(body);
    打印("JSON: " + jsonStringify(data));
    打印("key: " + 表取(data, "key"));
    打印("val: " + 表取(data, "value"));
    
    打印("文件存在: " + toString(文件存在("www/style.css")));
    打印("DONE");
    返回 0;
 }
主();
