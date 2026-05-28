#include <iostream>
#include <chrono>

int main() {
    // Test 1: Integer loop (100M iterations)
    auto t1 = std::chrono::high_resolution_clock::now();
    long long sum = 0;
    for (int i = 0; i < 100000000; i++) { sum += i; }
    auto t2 = std::chrono::high_resolution_clock::now();
    std::cout << "int_loop: " << std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count() << " us  sum=" << sum << "\n";

    // Test 2: Float loop (10M iterations)
    double area = 0.0;
    double pi = 3.14159265;
    auto t3 = std::chrono::high_resolution_clock::now();
    for (int r = 1; r <= 10000000; r++) { area += pi * r * r; }
    auto t4 = std::chrono::high_resolution_clock::now();
    std::cout << "float_loop: " << std::chrono::duration_cast<std::chrono::microseconds>(t4-t3).count() << " us  area=" << area << "\n";

    // Test 3: fib(35) recursive
    auto fib = [](auto& self, int n) -> long long {
        if (n <= 1) return n;
        return self(self, n-1) + self(self, n-2);
    };
    auto t5 = std::chrono::high_resolution_clock::now();
    long long f35 = fib(fib, 35);
    auto t6 = std::chrono::high_resolution_clock::now();
    std::cout << "fib(35): " << std::chrono::duration_cast<std::chrono::microseconds>(t6-t5).count() << " us  result=" << f35 << "\n";

    return 0;
}
