println("Opening window...");
initWindow(400, 300, "Test Window");
println("Window opened");

变量 c = 0;
当 (c < 120) {
    如果 (windowShouldClose()) { c = 999; }
    beginDrawing();
    clearBackground(BLACK);
    drawRectangle(10, 10, 100, 50, RED);
    drawText("HELLO CP", 150, 130, 20, GREEN);
    endDrawing();
    c = c + 1;
}

println("Closing...");
closeWindow();
println("Done");
