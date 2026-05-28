// 文件系统操作测试
打印("=== File System ===");

变量 testDir = "_test_fs_dir";
变量 testFile = testDir + "/test.txt";
变量 testCopy = testDir + "/copy.txt";

// 清理：先删目录（如果存在）
如果 (目录存在(testDir)) {
    删除目录(testDir);
}

// 创建目录
变量 created = 创建目录(testDir);
打印("mkdir: ", created);
打印("dir exists: ", 目录存在(testDir));

// 写文件
变量 wrote = 写入文件(testFile, "Hello CP!");
打印("write: ", wrote);
打印("file exists: ", 文件存在(testFile));

// 文件大小
变量 sz = 文件大小(testFile);
打印("size: ", sz);

// 读文件
变量 content = 读取文件(testFile);
打印("read: ", content);

// 追加文件
变量 appended = 追加文件(testFile, " 追加内容");
打印("append: ", appended);
变量 content2 = 读取文件(testFile);
打印("read after append: ", content2);

// 复制文件
变量 copied = 复制文件(testFile, testCopy);
打印("copy: ", copied);
打印("copy exists: ", 文件存在(testCopy));

// 是文件
打印("isFile test.txt: ", 是文件(testFile));
打印("isFile dir: ", 是文件(testDir));

// 移动文件
变量 moved = 移动文件(testCopy, testDir + "/moved.txt");
打印("move: ", moved);

// 目录列表
变量 files = 目录列表(testDir);
打印("dirList count: ", len(files));
打印("dirList[0]: ", files[0]);

// 清理
变量 deleted = 删除文件(testFile);
打印("delete file: ", deleted);
变量 rmd = 删除目录(testDir);
打印("rmdir: ", rmd);

打印("FILE_OK");