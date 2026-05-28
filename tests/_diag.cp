// 诊断版 HTTP Server
函数 主() {
    打印("1. 建表");
    变量 STORE = jsonParse("{}");
    打印("2. 监听");
    变量 s = TCP监听(8080);
    打印("3. 等待");
    当 (1 > 0) {
        变量 c = TCP接受(s);
        打印("4. 客户=" + toString(c));
        如果 (c == -1) { 继续; }
        变量 r = TCP接收(c, 2048);
        打印("5. 请求=" + r);
        
        变量 p1 = 查找(r, " ");
        打印("6. p1=" + toString(p1));
        变量 m = 子串(r, 0, 取整(p1));
        打印("7. 方法=" + m);
        变量 after = 子串(r, 取整(p1 + 1));
        打印("8. after=" + after);
        变量 p2 = 查找(after, " ");
        打印("9. p2=" + toString(p2));
        变量 path = 子串(after, 0, 取整(p2));
        打印("10. 路径=" + path);
        
        打印("11. 表有test=" + toString(表有(STORE, "test")));
        打印("12. 表长=" + toString(表长(STORE)));
        
        TCP发送(c, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nDIAG_OK:" + path);
        TCP关闭(c);
        打印("13. 完成");
     }
 }
主();
