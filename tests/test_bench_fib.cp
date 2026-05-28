函数 fib(n) {
    如果 (n <= 1) { 返回 n; }
    返回 fib(n - 1) + fib(n - 2);
}
函数 main() {
    变量 r = fib(35);
    打印("fib(35) = ", r);
    返回 r;
}
