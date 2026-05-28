// CP语言JIT性能基准测试
// 对比解释执行 vs JIT编译执行

// 1. 基准循环 - 最常见的操作
打印("===== 1. 基准循环测试 =====");
变量 t_start = 0;
变量 t_end = 0;
变量 result = 0;

函数 test_loop(n) {
    变量 sum = 0;
    变量 i = 0;
    循环 (i = 0; i < n; i = i + 1) {
        sum = sum + i;
    }
    返回 sum;
}

// 2. 斐波那契递归 - 测试函数调用开销
打印("===== 2. 斐波那契递归 =====");
函数 fib(n) {
    如果 (n <= 1) {
        返回 n;
    }
    返回 fib(n - 1) + fib(n - 2);
}

// 3. 斐波那契迭代 - 测试循环优化
函数 fib_iter(n) {
    如果 (n <= 1) { 返回 n; }
    变量 a = 0;
    变量 b = 1;
    变量 i = 2;
    循环 (i = 2; i <= n; i = i + 1) {
        变量 c = a + b;
        a = b;
        b = c;
    }
    返回 b;
}

// 4. 素数计算 - 测试数学运算
函数 is_prime(n) {
    如果 (n < 2) { 返回 0; }
    如果 (n == 2) { 返回 1; }
    如果 (n % 2 == 0) { 返回 0; }
    变量 i = 3;
    循环 (i = 3; i * i <= n; i = i + 2) {
        如果 (n % i == 0) {
            返回 0;
        }
    }
    返回 1;
}

函数 count_primes(limit) {
    变量 count = 0;
    变量 i = 0;
    循环 (i = 0; i < limit; i = i + 1) {
        如果 (is_prime(i) == 1) {
            count = count + 1;
        }
    }
    返回 count;
}

// 5. 数组操作 - 测试内存访问
函数 array_test(size) {
    变量 arr = [];
    变量 i = 0;
    循环 (i = 0; i < size; i = i + 1) {
        追加(arr, i);
    }
    变量 sum = 0;
    循环 (i = 0; i < size; i = i + 1) {
        sum = sum + arr[i];
    }
    返回 sum;
}

// 测试运行
打印("===== JIT性能基准测试 =====");
打印("请使用 --jit 或 --hotspot 模式运行以启用JIT\n");

// 预热
打印("预热中...");
test_loop(10000);
fib_iter(20);

打印("\n开始正式测试...\n");

// 测试1: 循环
变量 loop_n = 1000000;
打印("测试1: 循环 ", loop_n, " 次");
结果 = test_loop(loop_n);
打印("结果: ", 结果);

// 测试2: 斐波那契迭代
变量 fib_n = 40;
打印("\n测试2: 斐波那契迭代 fib(", fib_n, ")");
结果 = fib_iter(fib_n);
打印("结果: ", 结果);

// 测试3: 斐波那契递归
变量 fib_rec_n = 30;
打印("\n测试3: 斐波那契递归 fib(", fib_rec_n, ")");
结果 = fib(fib_rec_n);
打印("结果: ", 结果);

// 测试4: 素数计数
变量 prime_limit = 10000;
打印("\n测试4: 计算素数到 ", prime_limit);
结果 = count_primes(prime_limit);
打印("结果: ", 结果, " 个素数");

// 测试5: 数组操作
变量 array_size = 50000;
打印("\n测试5: 数组操作 大小 ", array_size);
结果 = array_test(array_size);
打印("结果: sum = ", 结果);

打印("\n===== 测试完成 =====");
