变量 格 = 20;
变量 列 = 28;
变量 行 = 22;
变量 头高 = 55;

变量 运行 = 1;

initWindow(列 * 格, 行 * 格 + 头高, "最简测试");
setTargetFPS(60);

打印("ENTER LOOP");

变量 帧 = 0;
当 (运行) {
    如果 (windowShouldClose()) { 打印("CLOSE"); 运行 = 0; }
    如果 (keyPressed(键_退出)) { 打印("ESC"); 运行 = 0; }

    帧 = 帧 + 1;
    如果 (帧 >= 120) { 运行 = 0; }

    beginDrawing();
    clearBackground(DARKGREEN);

    drawText("最简测试 帧:" + 帧, 12, 12, 24, GOLD);
    drawText("HELLO", 12, 42, 20, WHITE);
    drawFPS(400, 12);

    drawRectangle(100, 100, 50, 50, RED);

    endDrawing();
}
打印("EXIT");
closeWindow();
