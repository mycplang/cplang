println("=== TRACE ===");
initWindow(400, 300, "Trace");
setTargetFPS(60);

变量 ok = 1;
当 (ok == 1) {
    println("  A: loop start");
    
    println("  B: checking windowShouldClose...");
    如果 (windowShouldClose()) { ok = 0; }
    
    println("  C: after win close check");
    如果 (keyPressed(键_退出)) { ok = 0; }
    
    println("  D: draw");
    beginDrawing();
    clearBackground(BLACK);
    drawFPS(10, 10);
    endDrawing();
    
    如果 (ok == 0) { break; }
}

closeWindow();
println("DONE");
