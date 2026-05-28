#include <cstdio>
#include <cstdint>
#include <chrono>

using namespace std::chrono;

int64_t fib(int64_t n) {
    if (n <= 1) return n;
    return fib(n-1) + fib(n-2);
}

int main() {
    printf("=== C++ JIT-bench ===\n");
    
    auto t0 = high_resolution_clock::now();
    auto r = fib(35);
    auto t1 = high_resolution_clock::now();
    auto ms = duration_cast<milliseconds>(t1 - t0).count();
    printf("fib(35) = %lld (%lldms)\n", (long long)r, (long long)ms);

    t0 = high_resolution_clock::now();
    int64_t x = 0;
    for (int64_t i = 0; i < 100000000; ++i) {
        x += i;
    }
    t1 = high_resolution_clock::now();
    ms = duration_cast<milliseconds>(t1 - t0).count();
    printf("loopSum(100M) = %lld (%lldms)\n", (long long)x, (long long)ms);
    
    return 0;
}