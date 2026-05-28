// 原生测试：模拟 CP 的 rl_drawText → DrawText 调用链
// 链接到相同的 raylib.lib，排除依赖/链接层面的差异
#include "raylib.h"
#include <cstdio>
#include <string>
#include <vector>

// 模拟结构（与 CP Value 无关，只测 DrawText 调用）
int main() {
    SetTraceLogLevel(LOG_INFO);
    
    printf("NATIVE: InitWindow\n");
    InitWindow(400, 300, "Native DrawText Test");
    SetTargetFPS(60);
    printf("NATIVE: Window ready\n");
    
    printf("NATIVE: BeginDrawing\n");
    BeginDrawing();
    ClearBackground(RAYWHITE);
    
    printf("NATIVE: DrawFPS (works as control)\n");
    DrawFPS(10, 10);
    
    printf("NATIVE: DrawText simple\n");
    DrawText("HELLO", 10, 80, 20, BLACK);
    
    printf("NATIVE: DrawText from c_str()\n");
    std::string s = "FROM_STRING";
    DrawText(s.c_str(), 10, 110, 16, RED);
    
    printf("NATIVE: DrawText static\n");
    static const char* t = "STATIC_PTR";
    DrawText(t, 10, 140, 14, BLUE);
    
    printf("NATIVE: DrawCircle\n");
    DrawCircle(200, 150, 40, GREEN);
    
    printf("NATIVE: EndDrawing\n");
    EndDrawing();
    
    printf("NATIVE: Second frame\n");
    BeginDrawing();
    ClearBackground(BLACK);
    DrawFPS(10, 10);
    DrawText("FRAME 2", 10, 80, 20, WHITE);
    EndDrawing();
    
    printf("NATIVE: CloseWindow\n");
    // CloseWindow();  // no, let's not close - just check if DrawText worked
    
    printf("NATIVE: ALL PASSED - DrawText works!\n");
    return 0;
}
