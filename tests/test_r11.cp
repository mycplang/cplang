打印("--- 文件系统 ---");
打印(isDir("."));
打印(arrlen(listDir(".")));

打印("--- 随机分布 ---");
distSeed(42);
打印(uniformInt(1, 100));
打印(uniformFloat(0, 1));
打印(normalDist(0, 1));
打印(bernoulliDist(0.5));
打印(poissonDist(5));
打印(exponentialDist(2));

打印("--- chrono ---");
打印(secToMs(2));
打印(msToSec(2000));
打印(nsToSec(1000000000));

打印("=== R11 OK ===");
