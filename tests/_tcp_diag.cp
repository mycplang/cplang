// TCP 诊断：打印接收到的原始数据
变量 s = TCP监听(9090);
打印("LISTEN:9090");
变量 c = TCP接受(s);
打印("ACCEPTED");
变量 r1 = TCP接收(c, 8192);
打印("RECV1 len=" + toString(长度(r1)) + " data=[" + r1 + "]");
变量 r2 = TCP接收(c, 4096);
如果 (是空(r2)) {
    打印("RECV2=nil");
} 否则 {
    如果 (r2 == "") {
        打印("RECV2=empty");
    } 否则 {
        打印("RECV2 len=" + toString(长度(r2)) + " data=[" + r2 + "]");
    }
}
TCP关闭(c);
TCP关闭(s);
打印("DONE");
