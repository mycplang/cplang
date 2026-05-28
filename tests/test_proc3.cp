打印("test Chinese path");
变量 rc = procSystem("build_llvm\\bin\\Release\\cplang.exe -c games\\贪吃蛇.cp");
打印("rc=" + toString(rc));
