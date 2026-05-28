打印("=== 单帧渲染测试 ===");

初始化窗口(400, 300, "Single Frame Test");

开始绘图();
清空背景(红);
打印("clearBackground(RED) called");

绘制圆形(200, 150, 50, 绿);
打印("drawCircle called");

绘制文本("RED + GREEN CIRCLE", 90, 260, 16, 白);
打印("drawText called");

结束绘图();
打印("endDrawing called");

打印("Window should show red bg + green circle + white text");
打印("Close window to exit...");

// 用当循环等关闭——但不用 窗口应关闭() 做条件
// 直接无限等
变量 帧数 = 0;
设置目标帧率(60);
当 (1 == 1) {
    帧数 = 帧数 + 1;
    如果 (帧数 > 120) { 跳出; }
    变量 应关闭 = 窗口应关闭();
    如果 (应关闭) { 跳出; }
}

关闭窗口();
打印("done");
