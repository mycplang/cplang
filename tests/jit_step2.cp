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

函数 main() {
    返回 countPrimes(500);
}