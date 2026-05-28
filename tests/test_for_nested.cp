println("TEST: nested if instead of &&");
变量 fs = 0;
循环 (变量 i = 0; i < 10; i = i + 1) {
    如果 (i >= 3) {
        如果 (i <= 5) { continue; }
    }
    如果 (i == 7) { break; }
    fs = fs + i;
}
// Expected: 0+1+2+6 = 9
如果 (fs == 9) { 打印("  PASS: fs=" + fs); }
否则 { 打印("  FAIL: fs=" + fs); }
打印("DONE");
