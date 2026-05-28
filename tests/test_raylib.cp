打印("=== Raylib Basic ===");

变量 c = color(255, 128, 64, 200);
打印("color():", c);

变量 v = vector2(3.5, -2.0);
打印("vector2():", v);

变量 r = rectangle(10, 20, 100, 200);
打印("rectangle():", r);

变量 w = RAYWHITE;
打印("RAYWHITE:", w);
变量 rd = RED;
打印("RED:", rd);
变量 blk = BLACK;
打印("BLACK:", blk);
变量 bl = BLUE;
打印("BLUE:", bl);
变量 gr = GREEN;
打印("GREEN:", gr);

变量 rv = getRandomValue(1, 100);
如果 (rv >= 1 && rv <= 100) {
    打印("getRandomValue() OK:", rv);
} 否则 {
    打印("getRandomValue() FAILED");
}

打印("ALL_RAYLIB_BASIC_OK");
