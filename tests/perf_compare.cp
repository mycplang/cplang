// ==========================================
// CPLANG vs C++ 性能对比基准
// 每个场景跑 3 次算平均
// ==========================================

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

函数 bench(label, fn, n) {
    打印("  " + label + ":");
    变量 total = 0;
    变量 r = 0;
    变量 i = 0;
    当 (i < n) {
        变量 t0 = tick();
        如果 (i == 0) { r = fn(); } 否则 { fn(); }
        变量 t1 = tick();
        变量 elapsed = t1 - t0;
        打印("    #" + (i+1) + ": " + elapsed + "ms");
        total = total + elapsed;
        i = i + 1;
    }
    变量 avg = total / n;
    打印("    平均: " + avg + "ms (验证=" + r + ")");
    返回 avg;
}

// ---- 包装函数（作为参数传递）----
函数 runFib() { 返回 fib(38); }
函数 runPrimes() { 返回 countPrimes(5000); }
函数 runLoop() { 返回 loopSum(10000000); }
函数 runFnCall() { 返回 fnCallBench(5000000); }

函数 main() {
    打印("=============================");
    打印(" CPLANG 性能基准测试");
    打印("=============================");
    打印("");

    打印("[1/4] 斐波那契 fib(38)");
    变量 f1 = bench("fib(38)", runFib, 1);

    打印("");
    打印("[2/4] 质数计数 countPrimes(5000)");
    变量 f2 = bench("countPrimes(5K)", runPrimes, 1);

    打印("");
    打印("[3/4] 循环求和 loopSum(10000000)");
    变量 f3 = bench("loopSum(10M)", runLoop, 1);

    打印("");
    打印("[4/4] 函数调用 fnCallBench(5000000)");
    变量 f4 = bench("fnCall(5M)", runFnCall, 1);

    打印("");
    打印("=============================");
    打印(" 结果汇总:");
    打印("   fib(38):         " + f1 + "ms");
    打印("   countPrimes(5K): " + f2 + "ms");
    打印("   loopSum(10M):    " + f3 + "ms");
    打印("   fnCall(5M):      " + f4 + "ms");
    打印("=============================");
    打印(" 运行完成");
}