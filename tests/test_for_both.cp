println("TEST: compound condition + continue");
变量 fs = 0;
循环 (变量 i = 0; i < 10; i = i + 1) {
    如果 (i >= 3 && i <= 5) { continue; }
    fs = fs + i;
}
如果 (fs == 33) { 打印("  PASS: fs=" + fs); }  // 0+1+2+6+7+8+9=33
否则 { 打印("  FAIL: fs=" + fs); }

println("TEST: break + continue coexist");
变量 f2 = 0;
循环 (变量 i = 0; i < 10; i = i + 1) {
    如果 (i == 7) { break; }
    如果 (i == 3) { continue; }
    f2 = f2 + i;
}
如果 (f2 == 18) { 打印("  PASS: f2=" + f2); }  // 0+1+2+4+5+6=18
否则 { 打印("  FAIL: f2=" + f2); }

println("DONE");
