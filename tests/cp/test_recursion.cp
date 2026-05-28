// 递归测试
函数 fact(n) {
    如果 (n <= 1) {
        返回 1;
    }
    返回 n * fact(n - 1);
}
变量 result = fact(5);
