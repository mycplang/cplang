// JIT 性能测试 - 递归斐波那契
// 预期: 多次调用后触发 JIT 编译

函数 fib(n) {
    如果 (n <= 1) {
        返回 n;
    }
    返回 fib(n - 1) + fib(n - 2);
}

// 多次调用触发 JIT（热点阈值 100）
变量 result = 0;
变量 i = 0;
当 (i < 150) {
    result = fib(25);
    i = i + 1;
}

打印(result);
