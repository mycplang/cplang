打印("=== File info ===");
打印(isDir("C:/cplang"));
打印(isDir("C:/cplang/src"));
打印(fileSizeBytes("C:/cplang/src/stdlib/stdlib.cpp") > 100000);
打印(fileModified("C:/cplang/src/stdlib/stdlib.cpp") > 0);

打印("=== File watcher ===");
变量 w = fileWatchCreate("C:/cplang/tests");
变量 changes = fileWatchPoll(w);
打印(changes);
fileWatchClose(w);

打印("=== RLE ===");
打印(rleDecompress(rleCompress("hello")) == "hello");
打印(rleDecompress(rleCompress("aaaabbbcc")) == "aaaabbbcc");
打印(rleDecompress(rleCompress("a")) == "a");

打印("WATCHER_OK");
