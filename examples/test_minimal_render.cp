导入 "@cp/graphics";  // 图形模块 (Raylib 2D/3D + ImGui)
// 最小渲染测试：纯色背景 + 矩形，单帧即停
initWindow(400, 300, "RENDER TEST");
setTargetFPS(60);
while (!windowShouldClose()) {
    beginDrawing();
    clearBackground(RAYWHITE());
    drawRectangle(100, 50, 200, 100, BLUE());
    drawCircle(80, 140, 25, RED());
    drawFPS(10, 10);
    endDrawing();
}
closeWindow();
