导入 "@cp/graphics";  // 图形模块 (Raylib 2D/3D + ImGui)
// 测试: 手动构造颜色表
initWindow(400, 300, "TABLE COLOR TEST");
setTargetFPS(60);
var frame = 0;
while (frame < 60) {
    beginDrawing();

    // 手动构造颜色表
    var bg = table();
    bg["r"] = 245; bg["g"] = 245; bg["b"] = 245; bg["a"] = 255;
    clearBackground(bg);

    var rectColor = table();
    rectColor["r"] = 0; rectColor["g"] = 121; rectColor["b"] = 241; rectColor["a"] = 255;
    drawRectangle(50, 50, 200, 100, rectColor);

    drawFPS(10, 10);

    var textColor = table();
    textColor["r"] = 80; textColor["g"] = 80; textColor["b"] = 80; textColor["a"] = 255;
    drawText("Frame " + frame, 10, 280, 20, textColor);

    endDrawing();
    frame = frame + 1;
}
closeWindow();
