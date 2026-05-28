变量 运行 = 1;
变量 帧 = 0;
initWindow(560, 495, "render test");
setTargetFPS(60);

当 (运行) {
    如果 (windowShouldClose()) { 运行 = 0; }
    帧 = 帧 + 1;
    print("F" + 帧 + " ");
    如果 (帧 >= 5) { 运行 = 0; }

    beginDrawing();
    clearBackground(DARKGREEN);
    drawText("TEST " + 帧, 12, 12, 24, GOLD);
    drawFPS(400, 12);
    drawRectangle(100, 100, 50, 50, RED);
    endDrawing();
}

closeWindow();
print("\nDONE\n");
