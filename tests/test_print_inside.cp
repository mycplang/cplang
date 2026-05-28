函数 乘十(x) { 返回 x * 10; }

函数 process() {
    变量 arr = [1, 2, 3];
    变量 result = 数组变换(arr, 乘十);
    打印("inside process");
    返回 长度(result);
}

函数 main() {
    打印("before");
    变量 r = process();
    打印("after");
    打印(r);
    返回 0;
}
