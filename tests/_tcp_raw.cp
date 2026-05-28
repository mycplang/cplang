函数 主() {
    变量 s = TCP监听(8080);
    变量 c = TCP接受(s);
    变量 r = TCP接收(c, 4096);
    print("[" + r + "]");
    print("LEN=" + toString(长度(r)));
    TCP关闭(c);
    TCP关闭(s);
 }
主();
