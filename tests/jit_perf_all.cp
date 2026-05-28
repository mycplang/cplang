// 纯 JIT benchmark — 全部纯计算
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

函数 tinyFn(x) { 返回 x + 1; }

函数 fnCallBench(n) {
    变量 s = 0;
    变量 i = 0;
    当 (i < n) {
        s = tinyFn(s);
        i = i + 1;
    }
    返回 s;
}

函数 bench(label, fn) {
    变量 t0 = tick();
    变量 r = fn();
    变量 t1 = tick();
    打印("  " + label + ": " + (t1-t0) + "ms (验证=" + r + ")");
    返回 t1 - t0;
}

函数 runFib() { 返回 fib(38); }
函数 runPrimes() { 返回 countPrimes(5000); }
函数 runLoop10M() { 返回 loopSum(10000000); }
函数 runFn5M() { 返回 fnCallBench(5000000); }

函数 main() {
    打印("=============================");
    打印(" CPLANG JIT 性能基准测试");
    打印("=============================");
    打印("");
    打印("[1/4] 斐波那契 fib(38):");
    bench("fib(38)", runFib);
    打印("");
    打印("[2/4] 质数计数 countPrimes(5000):");
    bench("countPrimes(5K)", runPrimes);
    打印("");
    打印("[3/4] 循环求和 loopSum(10M):");
    bench("loopSum(10M)", runLoop10M);
    打印("");
    打印("[4/4] 函数调用 fnCall(5M):");
    bench("fnCall(5M)", runFn5M);
    打印("");
    打印("=============================");
    打印(" 运行完成");
}