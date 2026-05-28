// 最小鼠标测试
初始化窗口(640, 480, "Mouse Test");
设置目标帧率(60);

变量 宽 = 10; 高 = 10; 格 = 40;
变量 棋盘W = 宽 * 格;
变量 棋盘H = 高 * 格;
变量 px = (640 - 棋盘W) / 2;
变量 py = 40;
变量 点击 = 0;
变量 mxx = 0; 变量 myy = 0;

变量 grid = [];
变量 i = 0;
当 (i < 100) { push(grid, 0); i = i + 1; }

当 (窗口应关闭() == 假) {
    开始绘制();
    清除背景({r:20,g:20,b:30,a:255});

    mxx = 鼠标X();
    myy = 鼠标Y();
    点击 = 鼠标刚按下();

    // draw grid
    变量 y = 0;
    当 (y < 高) {
        变量 x = 0;
        当 (x < 宽) {
            变量 rx = px + x * 格;
            变量 ry = py + y * 格;
            变量 idx = y * 宽 + x;
            如果 (grid[idx] == 1) {
                drawRectangle(rx, ry, 格 - 1, 格 - 1, {r:255,g:100,b:100,a:255});
            } 否则 {
                drawRectangle(rx, ry, 格 - 1, 格 - 1, {r:190,g:190,b:210,a:255});
            }
            x = x + 1;
        }
        y = y + 1;
    }

    // click handling
    如果 (点击) {
        如果 (mxx >= px && mxx < px + 棋盘W && myy >= py && myy < py + 棋盘H) {
            变量 gx = 整除((mxx - px), 格);
            变量 gy = 整除((myy - py), 格);
            变量 idx = gy * 宽 + gx;
            grid[idx] = 1;
        }
    }

    // diagnostic
    drawRectangle(0, 0, 320, 80, {r:0,g:0,b:0,a:200});
    drawText("mouse: " + toString(mxx) + "," + toString(myy), 10, 10, 14, {r:0,g:255,b:0,a:255});
    drawText("click: " + toString(点击), 10, 30, 14, {r:0,g:255,b:0,a:255});
    drawText("px,py: " + toString(px) + "," + toString(py) + " grid: " + toString(棋盘W) + "x" + toString(棋盘H), 10, 50, 14, {r:150,g:150,b:150,a:255});

    结束绘制();
}

关闭窗口();
