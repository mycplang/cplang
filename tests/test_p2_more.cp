打印("=== 文件属性 ===");
变量 sz = fileSize("tests/test_charset.cp");
打印("test_charset.cp size:", sz);
打印("mtime:", fileMtime("tests/test_charset.cp"));
打印("exists:", fileExists("tests/test_charset.cp"));
打印("isDir:", fileIsDir("tests"));

打印("=== 临时文件 ===");
变量 tf = tempFile("cptest_", ".txt");
打印("tempFile:", tf);
打印("exists:", fileExists(tf));
打印("delete:", fileDelete(tf));

打印("=== 临时目录 ===");
变量 td = tempDir("cptest_");
打印("tempDir:", td);
打印("isDir:", fileIsDir(td));
打印("remove:", dirRemove(td));

打印("=== Duration ===");
变量 d = durationFormat(1234567);
打印("1234567ms:", d);
变量 d2 = durationFormat(3661000);
打印("3661000ms:", d2);
打印("elapsed:", elapsed());

打印("ALL_P2_MORE_OK");
