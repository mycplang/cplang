initWindow(400, 200, "font debug");
setTargetFPS(60);
当 (windowShouldClose() == 0) {
    beginDrawing();
    clearBackground(RAYWHITE);
    drawText("0123456789", 40, 40, 30, BLACK);
    drawText("16", 40, 80, 36, RED);
    drawText("32", 40, 130, 36, RED);
    drawFPS(300, 170);
    endDrawing();
}
closeWindow();
