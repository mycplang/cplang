打印("=== 文件流式读 ===");
变量 f = fileOpenRead("tests/test_charset.cp");
如果 (f != nil) {
    打印("open OK");
    变量 pos = fileTell(f);
    打印("pos:", pos);
    变量 line = fileReadLine(f);
    打印("line:", line);
    fileSeek(f, 0);
    变量 chunk = fileReadChunk(f, 20);
    打印("chunk:", chunk);
    打印("eof:", fileEof(f));
    fileClose(f);
}

打印("=== SHA-512 ===");
打印("sha512(\"hello\"):", sha512("hello"));

打印("=== HMAC-SHA256 ===");
变量 hmac = hmacSha256("key", "The quick brown fox");
打印("hmac:", hmac);

打印("=== Base32 ===");
变量 b32 = base32Encode("hello");
打印("base32('hello'):", b32);
打印("decode:", base32Decode(b32));

打印("=== Logger ===");
logSetLevel("info");
logInfo("测试日志: 一切正常");

打印("=== FileWalk ===");
变量 entries = fileWalk("tests", 1);
打印("walk tests/ (depth=1):", arrlen(entries));

打印("ALL_P2_OK");
