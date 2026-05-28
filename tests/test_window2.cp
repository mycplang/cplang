println("Opening window (press ESC to close)...");
initWindow(400, 300, "CP Raylib Test");

变量 c = 0;
当 (1) {
    如果 (windowShouldClose() || keyPressed(键_退出)) { 跳出; }
    beginDrawing();
    clearBackground(BLACK);
    drawRectangle(50, 50, 100, 60, RED);
    drawRectangle(200, 100, 120, 80, GREEN);
    drawText("Hello CP!", 150, 200, 20, WHITE);
    drawFPS(10, 10);
    endDrawing();
    c = c + 1;
}

println("Frames: " + c);
closeWindow();
println("Done");
