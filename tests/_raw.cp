// Raw echo - dump TCP receive
函数 主() {
    变量 s = TCP监听(8080);
    打印("OK " + toString(s));
     
    变量 c = TCP接受(s);
    打印("ACC " + toString(c));
    变量 r = TCP接收(c, 4096);
    打印("LEN=" + toString(长度(r)));
    打印("RAW=[" + r + "]");
    TCP关闭(c);
    TCP关闭(s);
    返回 0;
 }
主();
