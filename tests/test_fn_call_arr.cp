函数 process() {
    变量 arr = [1, 2, 3];
    变量 sum = 0;
    循环 (变量 i = 0; i < 长度(arr); i++) {
        sum = sum + arr[i];
    }
    打印("inside process, sum = " + sum);
    返回 sum;
}

函数 main() {
    变量 r = process();
    打印("back in main, r = " + r);
    返回 0;
}
