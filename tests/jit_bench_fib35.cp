// JIT 友好的 benchmark，所有函数直接调用
函数 fib(n) {
    如果 (n <= 1) { 返回 n; }
    返回 fib(n-1) + fib(n-2);
}

函数 main() {
    打印("=== CPLANG JIT-bench ===");
    变量 t0 = tick();
    变量 r = fib(35);
    变量 t1 = tick();
    打印("fib(35) = " + r + " (" + (t1-t0) + "ms)");
    返回 r;
}