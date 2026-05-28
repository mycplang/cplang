函数 主() {
    写文件("_diag_out.txt", "START\n");
    变量 s = TCP监听(8080);
    写文件("_diag_out.txt", "监听=" + toString(s) + "\n");
    当 (1 > 0) {
        变量 c = TCP接受(s);
        写文件("_diag_out.txt", "接受" + toString(c) + "\n");
        如果 (c == -1) { 继续; }
        变量 r = TCP接收(c, 4096);
        
        变量 p1 = 查找(r, " ");
        变量 rest = 子串(r, 取整(p1 + 1));
        变量 p2 = 查找(rest, " ");
        变量 path = 子串(rest, 0, 取整(p2));
        
        // 空行查找
        变量 hd = 查找(r, "\r\n\r\n");
        写文件("_diag_out.txt", "空行位置=" + toString(hd) + "\n");
        如果 (hd != -1) {
            变量 body = 子串(r, 取整(hd + 4));
            写文件("_diag_out.txt", "Body=[" + body + "]\n");
        }
        
        // 文件存在
        变量 local = "www" + path;
        写文件("_diag_out.txt", "文件=" + local + " 存在=" + toString(文件存在(local)) + "\n");
        
        // CRLF 测试
        变量 crlf = "\r\n";
        写文件("_diag_out.txt", "CRLFlen=" + toString(长度(crlf)) + "\n");
        
        TCP发送(c, "HTTP/1.1 200 OK\r\n\r\nDIAG");
        TCP关闭(c);
     }
 }
主();
