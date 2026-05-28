打印("=== GBK ↔ UTF-8 ===");
变量 gbkBytes = utf8ToGbk("你好世界");
打印("UTF8->GBK length:", strlen(gbkBytes));

变量 utf8Back = gbkToUtf8(gbkBytes);
打印("GBK->UTF8:", utf8Back);

打印("=== Big5 ↔ UTF-8 ===");
变量 big5Bytes = utf8ToBig5("臺灣");
打印("UTF8->Big5 length:", strlen(big5Bytes));

变量 utf8Back2 = big5ToUtf8(big5Bytes);
打印("Big5->UTF8:", utf8Back2);

打印("=== Shift-JIS ↔ UTF-8 ===");
变量 sjisBytes = utf8ToShiftJis("日本語");
变量 utf8Back3 = shiftJisToUtf8(sjisBytes);
打印("ShiftJIS roundtrip:", utf8Back3);

打印("=== 检测编码 ===");
打印("UTF8:", detectEncoding("Hello世界"));
打印("isValidUtf8:", isValidUtf8("Hello世界"));

打印("=== 通用转换 (GBK->Big5) ===");
变量 gbk2 = utf8ToGbk("中国");
变量 big5FromGbk = convertEncoding(gbk2, 936, 950);
变量 utf8FromBig5 = big5ToUtf8(big5FromGbk);
打印("GBK->Big5->UTF8:", utf8FromBig5);

打印("ALL_CHARSET_OK");
