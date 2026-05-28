// 测试递归函数
函数 fibonacci(n) {
    如果 (n <= 1) {
        返回 n;
    }
    返回 fibonacci(n - 1) + fibonacci(n - 2);
}

打印(fibonacci(10));
