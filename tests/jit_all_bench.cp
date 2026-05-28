// 用 tick() 在 main 中计时（main 走字节码 fallback）
// 计算函数由 JIT 编译

函数 fib(n) {
    如果 (n <= 1) { 返回 n; }
    返回 fib(n-1) + fib(n-2);
}

函数 isPrime(n) {
    如果 (n < 2) { 返回 0; }
    变量 i = 2;
    当 (i * i <= n) {
        如果 (n % i == 0) { 返回 0; }
        i = i + 1;
    }
    返回 1;
}

函数 countPrimes(limit) {
    变量 c = 0;
    变量 i = 2;
    当 (i <= limit) {
        如果 (isPrime(i) == 1) { c = c + 1; }
        i = i + 1;
    }
    返回 c;
}

函数 loopSum(n) {
    变量 s = 0;
    变量 i = 0;
    当 (i < n) {
        s = s + i;
        i = i + 1;
    }
    返回 s;
}

函数 fib35() { 返回 fib(35); }

函数 main() {
    打印("=== JIT 性能测试 ===");
    打印("");
    
    变量 t0 = tick();
    变量 r = fib(38);
    变量 t1 = tick();
    打印("fib(38) = " + r + " (" + (t1-t0) + "ms)");

    变量 t2 = tick();
    变量 r2 = countPrimes(5000);
    变量 t3 = tick();
    打印("countPrimes(5000) = " + r2 + " (" + (t3-t2) + "ms)");

    变量 t4 = tick();
    变量 r3 = loopSum(10000000);
    变量 t5 = tick();
    打印("loopSum(10M) = " + r3 + " (" + (t5-t4) + "ms)");

    返回 0;
}