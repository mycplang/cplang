变量 运行 = 1;
变量 帧数 = 0;
initWindow(400, 300, "shape only");
setTargetFPS(60);
当 (运行) {
    如果 (windowShouldClose()) { 运行 = 0; }
    帧数 = 帧数 + 1;
    如果 (帧数 > 120) { 运行 = 0; }
    beginDrawing();
    clearBackground(RAYWHITE);
    drawRectangle(50, 50, 100, 100, RED);
    drawCircle(250, 100, 40, BLUE);
    drawFPS(10, 10);
    endDrawing();
}
closeWindow();
