initWindow(400, 250, "DrawText test");
setTargetFPS(60);
当 (windowShouldClose() == 0) {
    beginDrawing();
    clearBackground(RAYWHITE);
    drawRectangle(50, 50, 300, 60, RED);
    drawText("32", 160, 70, 36, BLACK);
    drawFPS(320, 200);
    endDrawing();
}
closeWindow();
