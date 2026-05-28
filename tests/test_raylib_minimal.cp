打印("=== 最小渲染测试 ===");

初始化窗口(400, 300, "Render Test");

// 先验证函数是否被调用
打印("Window opened, entering loop...");

变量 计数 = 0;
当 (窗口应关闭() == 假) {
    如果 (计数 == 0) { 打印("loop running"); }
    如果 (计数 == 60) { 打印("60 frames rendered, closing..."); 跳出; }
    
    开始绘图();
    清空背景(红);
    
    绘制圆形(200, 150, 50, 绿);
    绘制文本("RED BG + GREEN CIRCLE", 80, 260, 16, 白);
    
    结束绘图();
    计数 = 计数 + 1;
}

打印("loop ended");
关闭窗口();
打印("done");
