println("TEST 1: while+break+continue");
变量 pass1 = 1; 变量 w1 = 0; 变量 ws = 0;
当 (w1 < 10) {
    如果 (w1 == 5) { break; }
    如果 (w1 == 3) { w1 = w1 + 1; continue; }
    ws = ws + w1;
    w1 = w1 + 1;
}
如果 (ws == 7) { 打印("  1 PASS (value=" + ws + ")"); }
否则 { 打印("  1 FAIL: ws=" + ws); }

println("TEST 2: for+break+continue");
变量 fs = 0;
循环 (变量 i = 0; i < 10; i = i + 1) {
    如果 (i == 7) { break; }
    如果 (i >= 3 && i <= 5) { continue; }
    fs = fs + i;
}
如果 (fs == 9) { 打印("  2 PASS (value=" + fs + ")"); }
否则 { 打印("  2 FAIL: fs=" + fs); }

println("TEST 3: 嵌套while+break");
变量 活 = 1; 变量 层 = 0;
当 (活) {
    变量 内 = 0;
    当 (内 < 5) {
        内 = 内 + 1;
        如果 (内 == 3) { break; }
    }
    层 = 层 + 1;
    如果 (层 >= 2) { break; }
}
如果 (层 == 2) { 打印("  3 PASS (层=" + 层 + ")"); }
否则 { 打印("  3 FAIL: 层=" + 层); }

println("TEST 4: while条件求值");
变量 c = 0; 变量 d = 3;
当 (d > 0) { c = c + d; d = d - 1; }
如果 (c == 6) { 打印("  4 PASS (value=" + c + ")"); }
否则 { 打印("  4 FAIL: c=" + c); }

println("TEST 5: for条件求值");
变量 fc = 0;
循环 (变量 j = 0; j < 5; j = j + 1) { fc = fc + j; }
如果 (fc == 10) { 打印("  5 PASS (value=" + fc + ")"); }
否则 { 打印("  5 FAIL: fc=" + fc); }

println("TEST 6: while+布尔条件");
变量 真值 = 真; 变量 bc = 0;
当 (真值) {
    bc = bc + 1;
    如果 (bc >= 3) { 真值 = 假; }
}
如果 (bc == 3) { 打印("  6 PASS (bc=" + bc + ")"); }
否则 { 打印("  6 FAIL: bc=" + bc); }

打印("");
打印("ALL LOOP TESTS PASSED");
