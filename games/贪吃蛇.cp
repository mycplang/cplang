// @名称 贪吃蛇
// @描述 经典贪吃蛇 | 吃食物变长加速
导入 "图形";
导入 "算法";
var 黑 = {r: 0, g: 0, b: 0, a: 255};
var 白 = {r: 255, g: 255, b: 255, a: 255};
var 金 = {r: 255, g: 203, b: 0, a: 255};
var 青柠 = {r: 0, g: 228, b: 48, a: 255};
var 绿 = {r: 0, g: 158, b: 47, a: 255};
var 红 = {r: 230, g: 41, b: 55, a: 255};
var 灰 = {r: 80, g: 80, b: 80, a: 255};
var 深绿 = {r: 0, g: 117, b: 44, a: 255};

var 格 = 20;
var 列 = 28;
var 行 = 22;
var 头高 = 55;

var 蛇X = [];
var 蛇Y = [];
push(蛇X, 10); push(蛇Y, 12);
push(蛇X, 9);  push(蛇Y, 12);
push(蛇X, 8);  push(蛇Y, 12);

var 方向X = 1; var 方向Y = 0;
var 食物X = 15; var 食物Y = 12;
var 分数 = 0;
var 帧计数 = 0;
var 速度 = 9;
var 运行 = 1;
var 游戏结束 = 0;

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
            var 头X = 蛇X[0] + 方向X;
            var 头Y = 蛇Y[0] + 方向Y;
            如果 (头X < 0 || 头X >= 列 || 头Y < 0 || 头Y >= 行) { 游戏结束 = 1; }
            如果 (游戏结束 == 0) {
                var i = 0;
                当 (i < arrlen(蛇X)) {
                    如果 (蛇X[i] == 头X && 蛇Y[i] == 头Y) { 游戏结束 = 1; }
                    i = i + 1;
                }
            }
            如果 (游戏结束 == 0) {
                insert(蛇X, 0, 头X); insert(蛇Y, 0, 头Y);
                如果 (头X == 食物X && 头Y == 食物Y) {
                    分数 = 分数 + 10;
                    如果 (速度 > 4) { 速度 = 速度 - 1; }
                    var ok = 0;
                    当 (ok == 0) {
                        食物X = getRandomValue(0, 列 - 1);
                        食物Y = getRandomValue(0, 行 - 1);
                        ok = 1;
                        var j = 0;
                        当 (j < arrlen(蛇X)) {
                            如果 (蛇X[j] == 食物X && 蛇Y[j] == 食物Y) { ok = 0; }
                            j = j + 1;
                        }
                    }
                } 否则 { pop(蛇X); pop(蛇Y); }
            }
        }
    }

    beginDrawing();
    clearBackground(深绿);

    var W = 列 * 格;
    var H = 行 * 格;

    drawRectangle(0, 0, W, 头高, 黑);
    drawText("贪吃蛇", 12, 12, 24, 金);
    drawText("分数: " + 分数, 160, 16, 20, 白);
    drawText("速度: " + (13 - 速度), 350, 16, 20, 青柠);
    drawFPS(W - 90, 16);

    drawRectangle(0, 头高, W, H, 黑);

    var idx = 0;
    当 (idx < arrlen(蛇X)) {
        var sx = 蛇X[idx] * 格 + 2;
        var sy = 蛇Y[idx] * 格 + 头高 + 2;
        如果 (idx == 0) { drawRectangle(sx, sy, 格 - 4, 格 - 4, 青柠); }
        否则 { drawRectangle(sx, sy, 格 - 4, 格 - 4, 绿); }
        idx = idx + 1;
    }

    var fx = 食物X * 格 + 2;
    var fy = 食物Y * 格 + 头高 + 2;
    drawRectangle(fx, fy, 格 - 4, 格 - 4, 红);

    如果 (游戏结束) {
        var ox = W / 2 - 150;
        var oy = 头高 + H / 2 - 70;
        drawRectangle(ox, oy, 300, 140, 黑);
        drawRectangle(ox + 2, oy + 2, 296, 136, 灰);
        drawText("游戏结束!", ox + 60, oy + 18, 30, 金);
        drawText("得分: " + 分数, ox + 80, oy + 56, 24, 白);
        drawText("按 R 或 回车 重新开始", ox + 30, oy + 96, 18, 青柠);
    }
    endDrawing();
}
closeWindow();

