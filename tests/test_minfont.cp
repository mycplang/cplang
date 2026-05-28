变量 运行 = 1;
变量 帧数 = 0;
initWindow(400, 300, "font test");
setTargetFPS(60);
当 (运行) {
    如果 (windowShouldClose()) { 运行 = 0; }
    帧数 = 帧数 + 1;
    如果 (帧数 > 120) { 运行 = 0; }
    beginDrawing();
    clearBackground(RAYWHITE);
    drawText("HELLO 123 ABC", 50, 100, 40, RED);
    drawRectangle(320, 90, 40, 40, RED);
    drawFPS(10, 10);
    endDrawing();
}
closeWindow();
