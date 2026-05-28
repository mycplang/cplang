变量 大小 = 4;
变量 格宽 = 100;
变量 边距 = 12;
变量 W = 大小 * 格宽 + 边距 * (大小 + 1);
变量 H = W + 100;
变量 格子 = [0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0];
变量 分数 = 0;
变量 最高 = 0;
变量 赢了 = 0;
变量 输了 = 0;
变量 背景色 = {r: 187, g: 173, b: 160, a: 255};
变量 空格色 = {r: 205, g: 193, b: 180, a: 255};
变量 深字色 = {r: 119, g: 110, b: 101, a: 255};
变量 浅字色 = {r: 249, g: 246, b: 242, a: 255};
变量 遮罩色 = {r: 0, g: 0, b: 0, a: 180};
变量 hx = 边距;
变量 hy = 边距;
变量 y0 = 90;
变量 n1 = getRandomValue(0, 15);
变量 n2 = getRandomValue(0, 15);
格子[n1] = 2;
当 (n2 == n1) { n2 = getRandomValue(0, 15); }
格子[n2] = 4;
initWindow(W, H, "step6");
setTargetFPS(60);
当 (windowShouldClose() == 0) {
    beginDrawing();
    clearBackground(背景色);
    drawRectangle(hx, hy, W - 边距*2, 56, 空格色);
    drawRectangle(0, y0 - 边距, W, W, 背景色);
    变量 idx = 0;
    当 (idx < 16) {
        变量 val = 格子[idx];
        变量 tx = 边距 + ((idx % 4) * (格宽 + 边距));
        变量 ty = y0 + (整除(idx, 4) * (格宽 + 边距));
        变量 块色 = 空格色;
        如果 (val == 2)    { 块色 = {r: 238, g: 228, b: 218, a: 255}; }
        否则 如果 (val == 4)    { 块色 = {r: 237, g: 224, b: 200, a: 255}; }
        否则 如果 (val == 8)    { 块色 = {r: 242, g: 177, b: 121, a: 255}; }
        drawRectangle(tx, ty, 格宽, 格宽, 块色);
        如果 (val > 0) {
            变量 tx2 = 边距 + ((idx % 4) * (格宽 + 边距));
            变量 ty2 = y0 + (整除(idx, 4) * (格宽 + 边距));
            变量 数字 = toString(val);
            变量 字色 = 深字色;
            变量 tsz = 36;
            变量 tox = 42;
            变量 toy = 28;
            如果 (val >= 8) { 字色 = 浅字色; }
            drawText(数字, tx2 + tox, ty2 + toy, tsz, 字色);
        }
        idx = idx + 1;
    }
    drawFPS(300, 10);
    endDrawing();
}
closeWindow();
