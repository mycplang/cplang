变量 大小 = 4;
变量 格宽 = 100;
变量 边距 = 12;
变量 W = 大小 * 格宽 + 边距 * (大小 + 1);
变量 H = W + 100;
变量 格子 = [2,0,0,4, 0,8,0,0, 0,0,16,0, 0,0,0,32];
变量 分数 = 16;
变量 背景色 = {r: 187, g: 173, b: 160, a: 255};
变量 空格色 = {r: 205, g: 193, b: 180, a: 255};
变量 深字色 = {r: 119, g: 110, b: 101, a: 255};
变量 浅字色 = {r: 249, g: 246, b: 242, a: 255};
变量 hx = 边距;
变量 hy = 边距;
变量 y0 = 90;
initWindow(W, H, "2048");
setTargetFPS(60);
当 (windowShouldClose() == 0) {
    beginDrawing();
    clearBackground(背景色);
    drawRectangle(hx, hy, W - 边距*2, 56, 空格色);
    drawRectangle(0, y0 - 边距, W, W, 背景色);
    drawText("2048", hx + 18, hy + 6, 36, 深字色);
    drawText("Score: " + 分数, W/2 + 30, hy + 10, 28, 深字色);
    变量 idx = 0;
    当 (idx < 16) {
        变量 val = 格子[idx];
        变量 tx = 边距 + ((idx % 4) * (格宽 + 边距));
        变量 ty = y0 + (整除(idx, 4) * (格宽 + 边距));
        变量 块色 = 空格色;
        如果 (val == 2)    { 块色 = {r: 238, g: 228, b: 218, a: 255}; }
        否则 如果 (val == 4)    { 块色 = {r: 237, g: 224, b: 200, a: 255}; }
        否则 如果 (val == 8)    { 块色 = {r: 242, g: 177, b: 121, a: 255}; }
        否则 如果 (val == 16)   { 块色 = {r: 245, g: 149, b: 99, a: 255};  }
        否则 如果 (val == 32)   { 块色 = {r: 246, g: 124, b: 95, a: 255};  }
        drawRectangle(tx, ty, 格宽, 格宽, 块色);
        如果 (val > 0) {
            变量 数字 = toString(val);
            变量 tsz = 36;
            变量 tox = 42;
            变量 toy = 28;
            如果 (val >= 8) { 字色 = 浅字色; } 否则 { 字色 = 深字色; }
            drawText(数字, tx + tox, ty + toy, tsz, 字色);
        }
        idx = idx + 1;
    }
    drawFPS(300, 10);
    endDrawing();
}
closeWindow();
