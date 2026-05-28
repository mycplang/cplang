打印("=== DEBUG: before initWindow ===");

初始化窗口(400, 300, "Debug Test");

打印("=== DEBUG: after initWindow ===");

开始绘图();
打印("=== DEBUG: after beginDrawing ===");

清空背景(红);
打印("=== DEBUG: after clearBackground ===");

绘制文本("RED SCREEN", 100, 140, 30, 白);
打印("=== DEBUG: after drawText ===");

结束绘图();
打印("=== DEBUG: after endDrawing ===");

打印("=== HOLDING WINDOW - close to exit ===");

变量 帧数 = 0;
设置目标帧率(60);
当 (1 == 1) {
    帧数 = 帧数 + 1;
    如果 (帧数 > 120) { 跳出; }
    如果 (窗口应关闭()) { 跳出; }
}

关闭窗口();
打印("=== DEBUG: done ===");
