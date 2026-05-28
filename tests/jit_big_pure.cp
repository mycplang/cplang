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

函数 testFib() { 变量 t0 = tick(); 变量 r = fib(38); 变量 t1 = tick(); 返回 r + (t1 - t0) * 1000000; }
函数 testPrime() { 变量 t0 = tick(); 变量 r = countPrimes(5000); 变量 t1 = tick(); 返回 r + (t1 - t0) * 1000000; }
函数 testLoop() { 变量 t0 = tick(); 变量 r = loopSum(10000000); 变量 t1 = tick(); 返回 r + (t1 - t0) * 1000000; }

函数 main() {
    变量 tf = testFib();
    变量 tp = testPrime();
    变量 tl = testLoop();
    返回 tf + tp + tl;
}