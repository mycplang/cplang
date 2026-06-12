导入 "@cp/graphics";  // 图形模块 (Raylib 2D/3D + ImGui)
// 固定帧数渲染测试：用 while 循环，兼容 CP 语言特性
initWindow(800, 620, "CP RENDER TEST");
setTargetFPS(60);
var frame = 0;
while (frame < 60) {
    beginDrawing();
    clearBackground(RAYWHITE());

    // 画网格（40x30 格子，用 while 不用 for）
    var cx = 0;
    while (cx < 40) {
        var cy = 0;
        while (cy < 30) {
            drawRectangle(cx * 20, cy * 20, 19, 19, LIGHTGRAY());
            cy = cy + 1;
        }
        cx = cx + 1;
    }

    // 蛇头（绿）
    drawRectangle(200, 300, 19, 19, DARKGREEN());
    // 食物（红）
    drawRectangle(400, 300, 19, 19, RED());
    // FPS
    drawFPS(10, 10);
    // 帧计数文本
    drawText("Frame: " + frame, 10, 600, 20, DARKGRAY());

    endDrawing();
    frame = frame + 1;
}
closeWindow();
