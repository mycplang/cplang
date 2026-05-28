#include <iostream>
#include <vector>
#include <chrono>

// fib函数
long long fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

int main() {
    auto start = std::chrono::high_resolution_clock::now();

    // 1. 整数运算
    std::cout << "===== 整数运算 =====\n";
    long long sum = 0;
    for (int i = 0; i < 100000; i++) {
        sum += i;
    }
    std::cout << "sum = " << sum << "\n";

    // 2. 浮点运算
    std::cout << "===== 浮点运算 =====\n";
    double pi = 3.14159265;
    double area = 0.0;
    for (int r = 1; r <= 1000; r++) {
        area += pi * r * r;
    }
    std::cout << "area = " << area << "\n";

    // 3. 递归函数
    std::cout << "===== 递归函数 =====\n";
    std::cout << "fib(30) = " << fib(30) << "\n";

    // 4. 数组操作
    std::cout << "===== 数组操作 =====\n";
    std::vector<int> arr;
    for (int j = 0; j < 1000; j++) {
        arr.push_back(j);
    }
    std::cout << "arr len = " << arr.size() << "\n";

    std::cout << "完成\n";

    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "\n执行时间: " << ms << " ms\n";

    return 0;
}
