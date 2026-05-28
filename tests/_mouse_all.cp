// 综合鼠标测试：press vs down
初始化窗口(640, 480, "鼠标综合测试");
设置目标帧率(60);

变量 格 = [];
变量 i = 0; 当 (i < 100) { push(格, 0); i = i + 1; }

当 (窗口应关闭() == 假) {
    开始绘制();
    清除背景({r:20,g:20,b:30,a:255});

    变量 mx = 鼠标X();
    变量 my = 鼠标Y();

    // 三个检测方式
    变量 pressed = 鼠标刚按下();           // isMouseButtonPressed
    变量 down = isMouseButtonDown(0);      // 持续按住, 直接英文名
    变量 released = 鼠标刚释放();          // isMouseButtonReleased

    // 用 pressed 改格子
    如果 (pressed && mx >= 80 && mx < 480 && my >= 80 && my < 480) {
        变量 gx = 整除((mx - 80), 40);
        变量 gy = 整除((my - 80), 40);
        格[gy * 10 + gx] = 1 - 格[gy * 10 + gx];
    }

    // 画格子
    变量 y = 0;
    当 (y < 10) {
        变量 x = 0;
        当 (x < 10) {
            变量 rx = 80 + x * 40;
            变量 ry = 80 + y * 40;
            如果 (格[y * 10 + x]) { drawRectangle(rx, ry, 38, 38, {r:255,g:80,b:80,a:255}); }
            否则 { drawRectangle(rx, ry, 38, 38, {r:180,g:180,b:200,a:255}); }
            x = x + 1;
        }
        y = y + 1;
    }

    // 诊断——大字高亮
    drawRectangle(0, 0, 640, 120, {r:0,g:0,b:0,a:230});
    drawText("MOUSE: " + toString(mx) + "," + toString(my), 20, 5, 18, {r:0,g:255,b:0,a:255});
    drawText("pressed(刚按): " + toString(pressed), 20, 30, 22, {r:255,g:255,b:0,a:255});
    drawText("down(按住):   " + toString(down), 20, 58, 22, {r:0,g:200,b:255,a:255});
    drawText("released(释放): " + toString(released), 20, 86, 22, {r:255,g:150,b:0,a:255});

    结束绘制();
}
关闭窗口();
