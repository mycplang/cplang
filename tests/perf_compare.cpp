// ==========================================
// C++ 性能基准测试（CPLANG 对比用）
// 与 perf_compare.cp 完全一致的计算负载
// ==========================================
#include <cstdint>
#include <cstdio>
#include <chrono>
#include <functional>
#include <vector>
#include <string>

using namespace std::chrono;

int64_t fib(int64_t n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

bool isPrime(int64_t n) {
    if (n < 2) return false;
    for (int64_t i = 2; i * i <= n; ++i) {
        if (n % i == 0) return false;
    }
    return true;
}

int64_t countPrimes(int64_t limit) {
    int64_t c = 0;
    for (int64_t i = 2; i <= limit; ++i) {
        if (isPrime(i)) ++c;
    }
    return c;
}

int64_t loopSum(int64_t n) {
    int64_t s = 0;
    for (int64_t i = 0; i < n; ++i) {
        s += i;
    }
    return s;
}

int64_t tinyFn(int64_t x) { return x + 1; }

int64_t fnCallBench(int64_t n) {
    int64_t s = 0;
    for (int64_t i = 0; i < n; ++i) {
        s = tinyFn(s);
    }
    return s;
}

struct BenchResult {
    double avgMs;
};

BenchResult runBench(const char* name, int runs, std::function<int64_t()> fn) {
    printf("  %s:\n", name);
    double total = 0;
    int64_t result = 0;
    for (int i = 0; i < runs; ++i) {
        auto t0 = high_resolution_clock::now();
        result = fn();
        auto t1 = high_resolution_clock::now();
        double elapsed = duration_cast<milliseconds>(t1 - t0).count();
        printf("    #%d: %.0fms\n", i + 1, elapsed);
        total += elapsed;
    }
    double avg = total / runs;
    printf("    平均: %.0fms (验证=%lld)\n", avg, (long long)result);
    return { avg };
}

int main() {
    printf("=============================\n");
    printf(" C++ 性能基准测试\n");
    printf("=============================\n\n");

    printf("[1/4] 斐波那契 fib(38)\n");
    auto f1 = runBench("fib(38)", 3, []() { return fib(38); });

    printf("\n[2/4] 质数计数 countPrimes(5000)\n");
    auto f2 = runBench("countPrimes(5000)", 3, []() { return countPrimes(5000); });

    printf("\n[3/4] 循环求和 loopSum(10000000)\n");
    auto f3 = runBench("loopSum(10M)", 3, []() { return loopSum(10000000); });

    printf("\n[4/4] 函数调用 fnCallBench(5000000)\n");
    auto f4 = runBench("fnCall(5M)", 3, []() { return fnCallBench(5000000); });

    printf("\n=============================\n");
    printf(" 结果汇总:\n");
    printf("   fib(38):         %.0fms\n", f1.avgMs);
    printf("   countPrimes(5K): %.0fms\n", f2.avgMs);
    printf("   loopSum(10M):    %.0fms\n", f3.avgMs);
    printf("   fnCall(5M):      %.0fms\n", f4.avgMs);
    printf("=============================\n");
    printf(" 运行完成\n");
    return 0;
}