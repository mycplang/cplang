函数 累加(x) { 全局变量 sum; sum = sum + x; }

函数 run_test() {
    变量 arr = [1, 2, 3, 4, 5];
    全局变量 sum = 0;
    遍历(arr, 累加);
    打印("inside run_test, sum = " + sum);
    返回 sum;
}

函数 main() {
    全局变量 sum = 0;
    变量 r = run_test();
    打印("back in main, r = " + r);
    打印("global sum = " + sum);
    返回 0;
}
