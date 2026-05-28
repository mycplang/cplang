#include <iostream>
#include <vector>
#include <chrono>

int main() {
    auto start = std::chrono::high_resolution_clock::now();

    std::cout << "===== 循环性能 =====\n";
    long long sum = 0;
    for (int i = 0; i < 1000000; i++) {
        sum += i;
    }
    std::cout << "sum(0..999999) = " << sum << "\n";

    std::cout << "===== 数组性能 =====\n";
    std::vector<int> arr;
    for (int j = 0; j < 10000; j++) {
        arr.push_back(j);
    }
    std::cout << "arr len = " << arr.size() << "\n";
    std::cout << "完成\n";

    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "\n执行时间: " << ms << " ms\n";

    return 0;
}
