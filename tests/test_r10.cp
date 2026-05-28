打印("--- arrFilter [1.5,0,3.14,0,-2.5] isnormal ---");
变量 a = [1.5, 0.0, 3.14, 0.0, -2.5];
打印(arrFilter(a, isnormal));

打印("--- arrClear ---");
变量 b = [1, 2, 3];
arrClear(b);
打印(arrlen(b));

打印("--- countIf ---");
打印(countIf(a, isnormal));

打印("--- deepCopy ---");
打印(深拷贝([1, 2]));

打印("--- math ---");
打印(isnormal(1.5));
打印(signbit(-3.14));
打印(fdim(10, 3));
打印(fmax(3.14, 2.71));
打印(fmin(3.14, 2.71));

打印("--- byteswap ---");
打印(byteswap(4660));

打印("=== R10 OK ===");
