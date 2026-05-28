函数 累加(x) { sum = sum + x; }

函数 process() {
    变量 arr = [1, 2, 3, 4, 5];
    变量 sum = 0;
    遍历(arr, 累加);
    返回 sum;
}

函数 main() {
    打印("before");
    变量 r = process();
    打印("after, r = ");
    打印(r);
    返回 0;
}
