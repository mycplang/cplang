// @名称 贪吃蛇
// @描述 经典贪吃蛇 | 吃食物变长加速

变量 格 = 20;
变量 列 = 28;
变量 行 = 22;
变量 头高 = 55;

变量 蛇X = [];
变量 蛇Y = [];
push(蛇X, 10); push(蛇Y, 12);
push(蛇X, 9);  push(蛇Y, 12);
push(蛇X, 8);  push(蛇Y, 12);

变量 方向X = 1; 变量 方向Y = 0;
变量 食物X = 15; 变量 食物Y = 12;
变量 分数 = 0;
变量 帧计数 = 0;
变量 速度 = 9;
变量 运行 = 1;
变量 游戏结束 = 0;

initWindow(列 * 格, 行 * 格 + 头高, "贪吃蛇 - CP Language");
setTargetFPS(60);

当 (运行) {
    如果 (windowShouldClose()) { 运行 = 0; }
    如果 (keyPressed(键_退出)) { 运行 = 0; }

    如果 (游戏结束 == 0) {
        如果 (keyPressed(键_上) && 方向Y == 0) { 方向X = 0; 方向Y = -1; }
        如果 (keyPressed(键_下) && 方向Y == 0) { 方向X = 0; 方向Y = 1; }
        如果 (keyPressed(键_左) && 方向X == 0) { 方向X = -1; 方向Y = 0; }
        如果 (keyPressed(键_右) && 方向X == 0) { 方向X = 1; 方向Y = 0; }
    }

    如果 (游戏结束) {
        如果 (keyPressed(键_R) || keyPressed(键_回车)) {
            蛇X = []; 蛇Y = [];
            push(蛇X, 10); push(蛇Y, 12);
            push(蛇X, 9);  push(蛇Y, 12);
            push(蛇X, 8);  push(蛇Y, 12);
            方向X = 1; 方向Y = 0;
            分数 = 0; 帧计数 = 0; 速度 = 9;
            游戏结束 = 0;
            食物X = 15; 食物Y = 12;
        }
    }

    如果 (游戏结束 == 0) {
        帧计数 = 帧计数 + 1;
        如果 (帧计数 >= 速度) {
            帧计数 = 0;
            变量 头X = 蛇X[0] + 方向X;
            变量 头Y = 蛇Y[0] + 方向Y;

            如果 (头X < 0 || 头X >= 列 || 头Y < 0 || 头Y >= 行) {
                游戏结束 = 1;
            }

            如果 (游戏结束 == 0) {
                变量 i = 0;
                当 (i < arrlen(蛇X)) {
                    如果 (蛇X[i] == 头X && 蛇Y[i] == 头Y) {
                        游戏结束 = 1;
                    }
                    i = i + 1;
                }
            }

            如果 (游戏结束 == 0) {
                insert(蛇X, 0, 头X);
                insert(蛇Y, 0, 头Y);
                如果 (头X == 食物X && 头Y == 食物Y) {
                    分数 = 分数 + 10;
                    如果 (速度 > 4) { 速度 = 速度 - 1; }
                    变量 ok = 0;
                    当 (ok == 0) {
                        食物X = getRandomValue(0, 列 - 1);
                        食物Y = getRandomValue(0, 行 - 1);
                        ok = 1;
                        变量 j = 0;
                        当 (j < arrlen(蛇X)) {
                            如果 (蛇X[j] == 食物X && 蛇Y[j] == 食物Y) {
                                ok = 0;
                            }
                            j = j + 1;
                        }
                    }
                } 否则 {
                    pop(蛇X);
                    pop(蛇Y);
                }
            }
        }
    }

    beginDrawing();
    clearBackground(DARKGREEN);

    变量 W = 列 * 格;
    变量 H = 行 * 格;

    drawRectangle(0, 0, W, 头高, BLACK);
    drawText("贪吃蛇", 12, 12, 24, GOLD);
    drawText("分数: " + 分数, 160, 16, 20, WHITE);
    drawText("速度: " + (13 - 速度), 350, 16, 20, LIME);
    drawFPS(W - 90, 16);

    drawRectangle(0, 头高, W, H, BLACK);

    变量 idx = 0;
    当 (idx < arrlen(蛇X)) {
        变量 sx = 蛇X[idx] * 格 + 2;
        变量 sy = 蛇Y[idx] * 格 + 头高 + 2;
        如果 (idx == 0) {
            drawRectangle(sx, sy, 格 - 4, 格 - 4, LIME);
        } 否则 {
            drawRectangle(sx, sy, 格 - 4, 格 - 4, GREEN);
        }
        idx = idx + 1;
    }

    变量 fx = 食物X * 格 + 2;
    变量 fy = 食物Y * 格 + 头高 + 2;
    drawRectangle(fx, fy, 格 - 4, 格 - 4, RED);

    如果 (游戏结束) {
        变量 ox = W / 2 - 150;
        变量 oy = 头高 + H / 2 - 70;
        drawRectangle(ox, oy, 300, 140, BLACK);
        drawRectangle(ox + 2, oy + 2, 296, 136, GRAY);
        drawText("游戏结束!", ox + 60, oy + 18, 30, GOLD);
        drawText("得分: " + 分数, ox + 80, oy + 56, 24, WHITE);
        drawText("按 R 或 回车 重新开始", ox + 30, oy + 96, 18, LIME);
    }

    endDrawing();
}

closeWindow();
