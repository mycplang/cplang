函数 myprint(x) {
    打印(x);
}

函数 process() {
    变量 arr = [1, 2, 3];
    遍历(arr, myprint);
    返回 100;
}

函数 main() {
    打印("before");
    变量 r = process();
    打印("after, r = ");
    打印(r);
    返回 0;
}
