变量 大小 = 4;
变量 格宽 = 100;
变量 边距 = 12;
变量 W = 大小 * 格宽 + 边距 * (大小 + 1);
变量 H = W + 100;
变量 背景色 = {r: 187, g: 173, b: 160, a: 255};
变量 深字色 = {r: 119, g: 110, b: 101, a: 255};
变量 hx = 边距;
变量 hy = 边距;
initWindow(W, H, "2048");
setTargetFPS(60);
当 (windowShouldClose() == 0) {
    beginDrawing();
    clearBackground(背景色);
    drawText("2048", hx + 18, hy + 6, 36, 深字色);
    drawText("Score: 0", W/2 + 30, hy + 10, 28, 深字色);
    drawFPS(300, 10);
    endDrawing();
}
closeWindow();
