println("TEST for basic");
变量 s = 0;
循环 (变量 i = 0; i < 5; i = i + 1) {
    s = s + i;
}
如果 (s == 10) { 打印("  for basic PASS (s=" + s + ")"); }
否则 { 打印("  for basic FAIL: s=" + s); }

println("TEST for+continue");
变量 sc = 0;
循环 (变量 i = 0; i < 5; i = i + 1) {
    如果 (i == 3) { continue; }
    sc = sc + i;
}
// Expected: 0+1+2+4 = 7
如果 (sc == 7) { 打印("  for+continue PASS (sc=" + sc + ")"); }
否则 { 打印("  for+continue FAIL: sc=" + sc); }

println("TEST for+break");
变量 sb = 0;
循环 (变量 i = 0; i < 10; i = i + 1) {
    如果 (i == 5) { break; }
    sb = sb + i;
}
// Expected: 0+1+2+3+4 = 10
如果 (sb == 10) { 打印("  for+break PASS (sb=" + sb + ")"); }
否则 { 打印("  for+break FAIL: sb=" + sb); }

打印("DONE");
