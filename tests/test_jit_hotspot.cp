函数 calc(x) {
    返回 x * x + x;
}

函数 main() {
    变量 s = 0;
    循环 (变量 i = 0; i < 10; i = i + 1) {
        s = s + calc(i);
    }
    打印(s);
    返回 s;
}
