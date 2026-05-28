函数 loopSum(n) {
    变量 s = 0;
    变量 i = 0;
    当 (i < n) {
        s = s + i;
        i = i + 1;
    }
    返回 s;
}
变量 sum = loopSum(100000000);
打印(sum);
