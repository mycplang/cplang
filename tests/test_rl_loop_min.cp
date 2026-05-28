变量 运行 = 1;
变量 帧 = 0;
print("A");
initWindow(560, 495, "loop test");
print("B");
setTargetFPS(60);
print("C");

当 (运行) {
    如果 (windowShouldClose()) { 运行 = 0; }
    帧 = 帧 + 1;
    print("F" + 帧);
    如果 (帧 >= 3) { 运行 = 0; }
}

print("D");
closeWindow();
print("E");
