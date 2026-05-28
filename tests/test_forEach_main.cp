函数 累加(x) { 全局变量 sum; sum = sum + x; }

函数 main() {
    变量 arr = [1, 2, 3, 4, 5];
    全局变量 sum = 0;
    遍历(arr, 累加);
    变量 r = sum;
    打印("after forEach, r = " + r);
    返回 0;
}
