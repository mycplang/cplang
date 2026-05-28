initWindow(460, 200, "Text Test");
设置目标帧率(60);
当 (!窗口应关闭()) {
    beginDrawing();
    clearBackground({r: 50, g: 50, b: 50, a: 255});
    drawRectangle(10, 10, 440, 80, {r: 200, g: 200, b: 180, a: 255});
    drawText("2048", 30, 30, 36, {r: 119, g: 110, b: 101, a: 255});
    drawText("Score: 128", 260, 32, 28, {r: 119, g: 110, b: 101, a: 255});
    endDrawing();
}
closeWindow();
