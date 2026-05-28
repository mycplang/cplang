打印("=== 数学常数 ===");
打印("tau:", tau());
打印("sqrt2:", sqrt2());
打印("goldenRatio:", goldenRatio());

打印("\n=== 格式化 ===");
打印(fmt("x={0:.2f}, y={1}", 3.14159, 42));

打印("\n=== Result ===");
r = resOk(99);
如果 resIsOk(r) 则 {
    打印("OK:", resUnwrap(r));
} 否则 {
    打印("Err");
}

打印("\n=== 函数式 ===");
打印("identity(42):", identity(42));
打印("compare(3,5):", compare(3, 5));

打印("\n=== 数值转换 ===");
打印("intToStr(255,16):", intToStr(255, 16));
打印("strToInt('ff',16):", strToInt("ff", 16));

打印("\n=== Span ===");
arr = [10,20,30,40,50];
sp = spanNew(arr, 1, 3);
打印("spanLen:", spanLen(sp));
打印("spanGet(0):", spanGet(sp, 0));

打印("\n=== Box/Rc ===");
b = boxNew("hello");
打印("boxGet:", boxGet(b));
rc = rcNew(123);
打印("rcCount:", rcCount(rc));

打印("\n测试完成");
