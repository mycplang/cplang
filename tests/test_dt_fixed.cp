initWindow(600, 200, "DrawText v3");
setTargetFPS(60);
println("window ready");

变量 帧 = 0;
当 (帧 < 2) {
    beginDrawing();
    clearBackground(BLACK);
    
    drawText("HELLO CP!", 10, 30, 24, GREEN);
    drawText("drawText FIXED", 10, 70, 16, WHITE);
    drawText("中文测试", 10, 110, 20, WHITE);
    drawFPS(500, 10);
    
    endDrawing();
    帧 = 帧 + 1;
}

closeWindow();
println("ALL TEXT OK!");
