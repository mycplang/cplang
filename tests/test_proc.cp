打印("procSystem test");
变量 rc = procSystem("echo hello > C:\\Temp\\_proc_test.txt");
打印("rc=" + toString(rc));
如果 (文件存在("C:\\Temp\\_proc_test.txt")) {
    打印("OK: file created");
} 否则 {
    打印("FAIL: file not created");
}
