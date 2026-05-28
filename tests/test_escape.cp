// 逃逸分析测试（精简版）
// 测试基本的逃逸场景

函数 test_no_escape() {
    a = 10;
    b = 20;
    c = a + b;
    返回 c;
}

函数 test_local_no_escape() {
    sum = 0;
    循环 (i = 0; i < 100; i = i + 1) {
        temp = i;
        sum = sum + temp;
    }
    返回 sum;
}

函数 test_branch() {
    result = 0;
    如果 真 则 {
        temp = 5;
        result = temp;
    } 否则 {
        result = 0;
    }
    返回 result;
}

函数 test_loop_escape() {
    results = [];
    i = 0;
    当 (i < 100) {
        results = i * i;
        i = i + 1;
    }
    返回 results;
}

函数 test_point() {
    返回 10;
}

函数 main() {
    打印("逃逸分析测试:");
    打印(test_no_escape());
    打印(test_local_no_escape());
    打印(test_branch());
    打印(test_point());
    打印("测试完成");
    返回 0;
}
