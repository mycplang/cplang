函数 累加(x) { 全局变量 sum; sum = sum + x; }

函数 test_forEach() {
    变量 arr = [1, 2, 3, 4, 5];
    全局变量 sum = 0;
    遍历(arr, 累加);
    返回 sum;
}

函数 main() {
    变量 r = test_forEach();
    打印("result: " + r);
    返回 0;
}
