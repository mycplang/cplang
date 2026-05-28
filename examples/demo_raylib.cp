打印("=== Raylib Window Demo ===");
打印("Opening 800x600 window...");
打印("Press ESC or close window to exit");

初始化窗口(800, 600, "CP + Raylib 游戏窗口");
设置目标帧率(60);

变量 x = 400;
变量 y = 300;
变量 radius = 50;
变量 speed = 5;

当 (!窗口应关闭()) {
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
