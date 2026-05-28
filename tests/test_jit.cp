// JIT 编译测试
// 测试各种 JIT 场景

// 场景1：简单函数
函数 simple_add(a, b) {
    返回 a + b;
}

// 场景2：递归函数
函数 factorial(n) {
    如果 n <= 1 则 返回 1;
    返回 n * factorial(n - 1);
}

// 场景3：循环计算
函数 sum_loop(n) {
    sum = 0;
    循环 (i = 1; i <= n; i = i + 1) {
        sum = sum + i;
    }
    返回 sum;
}

// 场景4：斐波那契（递归）
函数 fib(n) {
    如果 n <= 1 则 返回 n;
    返回 fib(n - 1) + fib(n - 2);
}

// 场景5：数组操作
函数 array_sum(arr, n) {
    sum = 0;
    循环 (i = 0; i < n; i = i + 1) {
        sum = sum + arr[i];
    }
    返回 sum;
}

// 场景6：条件分支
函数 max_of_three(a, b, c) {
    max = a;
    如果 b > max 则 {
        max = b;
    }
    如果 c > max 则 {
        max = c;
    }
    返回 max;
}

// 场景7：数学计算
函数 math_compute(x) {
    a = x * x;
    b = a + x;
    c = b * 2;
    返回 c;
}

// 场景8：嵌套循环
函数 nested_loop(n) {
    count = 0;
    循环 (i = 0; i < n; i = i + 1) {
        循环 (j = 0; j < n; j = j + 1) {
            count = count + 1;
        }
    }
    返回 count;
}

// 场景9：字符串处理（如果支持）
函数 string_length(s) {
    返回 长度(s);
}

// 场景10：表操作（如果支持）
函数 table_lookup(t, key) {
    返回 t[key];
}

// 测试主函数
函数 main() {
    打印("JIT 编译测试");
    
    // 测试简单函数
    打印("simple_add(10, 20) =");
    打印(simple_add(10, 20));
    
    // 测试递归
    打印("factorial(10) =");
    打印(factorial(10));
    
    // 测试循环
    打印("sum_loop(100) =");
    打印(sum_loop(100));
    
    // 测试斐波那契
    打印("fib(20) =");
    打印(fib(20));
    
    // 测试条件
    打印("max_of_three(10, 30, 20) =");
    打印(max_of_three(10, 30, 20));
    
    // 测试数学计算
    打印("math_compute(5) =");
    打印(math_compute(5));
    
    // 测试嵌套循环
    打印("nested_loop(10) =");
    打印(nested_loop(10));
    
    打印("测试完成");
    返回 0;
}
