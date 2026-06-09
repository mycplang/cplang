打印("=== Raylib Window Demo ===");
打印("Opening 800x600 window...");
打印("Press ESC or close window to exit");

// 颜色表（绕过函数名解析 bug）
var 黑 = {r: 0, g: 0, b: 0, a: 255};
var 白 = {r: 255, g: 255, b: 255, a: 255};
var 红 = {r: 230, g: 41, b: 55, a: 255};
var 绿 = {r: 0, g: 158, b: 47, a: 255};
var 蓝 = {r: 0, g: 121, b: 241, a: 255};
var 金 = {r: 255, g: 203, b: 0, a: 255};
var 灰 = {r: 130, g: 130, b: 130, a: 255};
var 深灰 = {r: 80, g: 80, b: 80, a: 255};
var 浅灰 = {r: 200, g: 200, b: 200, a: 255};
var 深绿 = {r: 0, g: 117, b: 44, a: 255};
var 亮绿 = {r: 0, g: 228, b: 48, a: 255};
var 乳白 = {r: 245, g: 245, b: 245, a: 255};
var 橙 = {r: 255, g: 161, b: 0, a: 255};
初始化窗口(800, 600, "CP + Raylib 游戏窗口");
设置目标帧率(60);

变量 x = 400;
变量 y = 300;
变量 radius = 50;
变量 speed = 5;

当 (窗口应关闭() == 假) {
    // Move circle with arrow keys
    如果 (键盘按下(263)) { x = x - speed; }  // KEY_LEFT
    如果 (键盘按下(262)) { x = x + speed; }  // KEY_RIGHT
    如果 (键盘按下(265)) { y = y - speed; }  // KEY_UP
    如果 (键盘按下(264)) { y = y + speed; }  // KEY_DOWN

    开始绘图();
    清空背景(乳白);

    绘制文本("CP + Raylib - 用方向键移动圆圈", 10, 10, 20, 深灰);
    绘制帧率(700, 10);

    绘制圆形(x, y, radius, 红);
    绘制圆形(x, y, 5, 黑);

    结束绘图();
}

关闭窗口();
打印("Window closed.");
