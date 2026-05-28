// 尾递归优化测试

// 经典尾递归：阶乘
函数 factorial(n, acc) {
    如果 n <= 1 则 {
        返回 acc;
    }
    返回 factorial(n - 1, n * acc);  // 尾递归调用
}

// 经典尾递归：斐波那契
函数 fib_tail(n, a, b) {
    如果 n == 0 则 {
        返回 a;
    }
    如果 n == 1 则 {
        返回 b;
    }
    返回 fib_tail(n - 1, b, a + b);  // 尾递归调用
}

// 非尾递归（不会被优化）
函数 fib_normal(n) {
    如果 n <= 1 则 {
        返回 n;
    }
    返回 fib_normal(n - 1) + fib_normal(n - 2);  // 不是尾递归
}

// 尾递归：累加
函数 sum_tail(n, acc) {
    如果 n == 0 则 {
        返回 acc;
    }
    返回 sum_tail(n - 1, acc + n);  // 尾递归调用
}

// 主函数
函数 main() {
    打印("尾递归优化测试:");
    
    // 阶乘
    打印("factorial(10, 1) =");
    打印(factorial(10, 1));  // 3628800
    
    // 斐波那契
    打印("fib_tail(20, 0, 1) =");
    打印(fib_tail(20, 0, 1));  // 6765
    
    // 累加
    打印("sum_tail(100, 0) =");
    打印(sum_tail(100, 0));  // 5050
    
    // 对比非尾递归
    打印("fib_normal(20) =");
    打印(fib_normal(20));  // 6765
    
    打印("测试完成");
    返回 0;
}
