// 路径工具测试
打印("=== Path ===");

// pathBasename
打印("basename /a/b/c.txt: ", 路径基名("/a/b/c.txt"));
打印("basename foo.exe: ", 路径基名("foo.exe"));
打印("basename dir/: ", 路径基名("dir/"));

// pathDirname
打印("dirname /a/b/c.txt: ", 路径目录("/a/b/c.txt"));
打印("dirname foo.exe: ", 路径目录("foo.exe"));

// pathExtname
打印("extname /a/b/c.txt: ", 路径扩展名("/a/b/c.txt"));
打印("extname foo.exe: ", 路径扩展名("foo.exe"));
打印("extname noext: ", 路径扩展名("noext"));

// pathJoin
打印("join a b c: ", 路径连接("a", "b", "c"));
打印("join /root sub: ", 路径连接("/root", "sub"));

打印("PATH_OK");