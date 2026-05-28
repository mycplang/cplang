函数 loopSum(n) {
    变量 s = 0;
    变量 i = 0;
    当 (i < n) {
        s = s + i;
        i = i + 1;
    }
    返回 s;
}

函数 main() {
    打印("=== CPLANG JIT 循环测试 ===");
    变量 t0 = tick();
    变量 x = loopSum(100000000);
    变量 t1 = tick();
    打印("loopSum(100M) = " + x + " (" + (t1-t0) + "ms)");
    返回 x;
}