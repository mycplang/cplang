// 测试1: 直接 RGBA 整数调用 clearBackground
initWindow(400, 300, "RGBA TEST");
setTargetFPS(60);
var frame = 0;
while (frame < 60) {
    beginDrawing();
    // 直接用 RGBA 值 (245,245,245,255) = RAYWHITE
    clearBackground(245, 245, 245, 255);
    // 用 RGBA 画矩形 (0,121,241,255) = BLUE
    drawRectangle(50, 50, 200, 100, 0, 121, 241, 255);
    // FPS
    drawFPS(10, 10);
    drawText("Frame " + frame, 10, 280, 20, 80, 80, 80, 255);
    endDrawing();
    frame = frame + 1;
}
closeWindow();
