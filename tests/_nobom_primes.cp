函数 isPrime(n) {
    如果 (n <= 1) { 返回 0; }
    如果 (n <= 3) { 返回 1; }
    如果 (n % 2 == 0) { 返回 0; }
    变量 i = 3;
    当 (i < n) {
        如果 (n % i == 0) { 返回 0; }
        i = i + 2;
    }
    返回 1;
}
函数 countPrimes(limit) {
    变量 c = 0;
    变量 i = 2;
    当 (i <= limit) {
        如果 (isPrime(i)) { c = c + 1; }
        i = i + 1;
    }
    返回 c;
}
函数 main() {
    变量 n = countPrimes(2000);
    打印("primes <= 2000: ", n);
    返回 n;
}
