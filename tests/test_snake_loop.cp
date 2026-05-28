初始化窗口(800, 630, "Debug Snake");
设置目标帧率(60);
println("窗口已创建");

变量 结束 = 假;
变量 帧计数 = 0;

当 (结束 == 假) {
    println("frame " + 帧计数);
    
    println("  input...");
    如果 (键盘按下(键_上)) { println("  UP"); }
    如果 (键盘按下(键_下)) { println("  DOWN"); }
    如果 (键盘按下(键_左)) { println("  LEFT"); }
    如果 (键盘按下(键_右)) { println("  RIGHT"); }
    如果 (键盘按下(键_R)) { println("  R"); }
    如果 (键盘按下(键_退出)) { println("  ESC -> exit"); break; }
    
    println("  draw...");
    beginDrawing();
    clearBackground(BLACK);
    drawRectangle(10, 10, 100, 20, GREEN);
    drawFPS(700, 10);
    endDrawing();
    
    帧计数 = 帧计数 + 1;
    如果 (帧计数 >= 5) {
        println("5 frames done, exiting");
        break;
    }
    
    如果 (窗口应关闭()) { break; }
}

关闭窗口();
println("DONE");
