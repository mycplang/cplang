变量 运行 = 1;
变量 帧 = 0;
initWindow(560, 495, "min render");
setTargetFPS(60);

当 (运行) {
    如果 (windowShouldClose()) { 运行 = 0; }
    帧 = 帧 + 1;
    如果 (帧 >= 3) { 运行 = 0; }
    beginDrawing();
    clearBackground(DARKGREEN);
    drawRectangle(10, 10, 100, 50, RED);
    drawText("HELLO", 200, 10, 24, GOLD);
    endDrawing();
}

closeWindow();
print("DONE");
