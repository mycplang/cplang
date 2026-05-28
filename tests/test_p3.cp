打印("=== CSV Write ===");
变量 data = [["姓名", "年龄", "城市"], ["张三", "25", "北京"], ["李四", "30", "上海"]];
变量 csv = csvWrite(data);
打印(csv);
变量 csv2 = csvWrite(data, ";");
打印(csv2);
如果 (strContains(csv, "张三")) {
    打印("csvWrite OK");
} 否则 {
    打印("csvWrite FAILED");
}

打印("=== Log levels ===");
logSetLevel("INFO");
logDebug("这个不应该显示");
logInfo("Hello from INFO level");
logWarn("Warning message");
logError("Error message");
logSetLevel("ERROR");
logWarn("这个WARN不应该显示");
logError("Only ERROR should show");
logSetLevel("DEBUG");

打印("=== Log to file ===");
logSetFile("tests/_temp_log.txt");
logInfo("This goes to file");
logInfo("Line 2 to file");
logFlush();
变量 fcontent = readFile("tests/_temp_log.txt");
打印("Log file content:", fcontent);
如果 (strContains(fcontent, "Line 2 to file")) {
    打印("logSetFile OK");
} 否则 {
    打印("logSetFile FAILED");
}
fileDelete("tests/_temp_log.txt");

打印("=== Case-insensitive compare ===");
如果 (strEqualsI("Hello", "hello")) {
    打印("strEqualsI OK");
} 否则 {
    打印("strEqualsI FAILED");
}
变量 cmp = strCompareI("abc", "ABC");
如果 (cmp == 0) {
    打印("strCompareI equal OK");
} 否则 {
    打印("strCompareI equal FAILED:", cmp);
}
变量 cmp2 = strCompareI("abc", "def");
如果 (cmp2 == -1) {
    打印("strCompareI lt OK");
} 否则 {
    打印("strCompareI lt FAILED:", cmp2);
}
如果 (strContainsI("Hello World", "world")) {
    打印("strContainsI OK");
} 否则 {
    打印("strContainsI FAILED");
}

打印("ALL_P3_OK");
