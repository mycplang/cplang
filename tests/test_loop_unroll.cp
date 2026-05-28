// 循环展开测试

// 简单循环 - 适合完全展开
函数 simple_loop() {
    整数 sum = 0;
    循环 (i = 0; i < 4; i = i + 1) {
        sum = sum + i;
    }
    返回 sum;  // 0 + 1 + 2 + 3 = 6
}

// 数组初始化 - 适合部分展开
函数 array_init() {
    变量 arr = [];
    循环 (i = 0; i < 10; i = i + 1) {
        arr[i] = i * 2;
    }
    返回 arr[5];  // 10
}

// 矩阵运算 - 展开后性能提升明显
函数 matrix_sum() {
    整数 sum = 0;
    循环 (i = 0; i < 8; i = i + 1) {
        循环 (j = 0; j < 8; j = j + 1) {
            sum = sum + i * j;
        }
    }
    返回 sum;
}

// 点积计算 - 展开后减少循环开销
函数 dot_product() {
    变量 a = [1,2,3,4];
    变量 b = [5,6,7,8];
    
    整数 result = 0;
    循环 (i = 0; i < 4; i = i + 1) {
        result = result + a[i] * b[i];
    }
    返回 result;  // 1*5 + 2*6 + 3*7 + 4*8 = 70
}

// 累加 - 完全展开
函数 accumulate() {
    整数 total = 0;
    循环 (i = 1; i < 6; i = i + 1) {
        total = total + i;
    }
    返回 total;  // 1 + 2 + 3 + 4 + 5 = 15
}

// 主函数
函数 main() {
    打印("循环展开测试:");
    
    打印("simple_loop() =");
    打印(simple_loop());  // 6
    
    打印("array_init() =");
    打印(array_init());  // 10
    
    打印("matrix_sum() =");
    打印(matrix_sum());
    
    打印("dot_product() =");
    打印(dot_product());  // 70
    
    打印("accumulate() =");
    打印(accumulate());  // 15
    
    打印("测试完成");
    返回 0;
}
