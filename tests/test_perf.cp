// 性能测试：计算斐波那契数列

func fib(n) {
    如果 n <= 1 则 {
        返回 n;
    }
    返回 fib(n - 1) + fib(n - 2);
}

func main() {
    变量 result = fib(35);
    打印(result);
}
