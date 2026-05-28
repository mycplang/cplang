// Debug: test toString + drawText with same APIs as 2048
println("=== DEBUG START ===");
变量 v = 42;
变量 s = toString(v);
println("toString(42) = " + s);
println("len(s) = " + len(s));

initWindow(400, 200, "toString debug");
setTargetFPS(60);
beginDrawing();
clearBackground(RAYWHITE);

// literal → should work
drawText("literal: HELLO", 20, 20, 20, BLACK);

// toString variable → does it work?
drawText(s, 20, 60, 20, BLACK);

// concat approach
drawText("" + v, 20, 100, 20, BLACK);

endDrawing();


当 (1 == 1) {
    如果 (windowShouldClose()) { 跳出; }
}
closeWindow();
println("=== DEBUG END ===");
