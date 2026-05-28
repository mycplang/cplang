#include "raylib.h"
#include <cstdio>

int main() {
    printf("C++: before InitWindow\n");
    fflush(stdout);
    
    InitWindow(400, 300, "C++ Raylib Test");
    
    printf("C++: after InitWindow\n");
    fflush(stdout);
    
    BeginDrawing();
    printf("C++: after BeginDrawing\n");
    fflush(stdout);
    
    ClearBackground(RED);
    printf("C++: after ClearBackground\n");
    fflush(stdout);
    
    DrawCircle(200, 150, 50, GREEN);
    DrawText("RED + GREEN", 90, 260, 16, WHITE);
    
    EndDrawing();
    printf("C++: after EndDrawing - window should be RED with GREEN circle\n");
    fflush(stdout);
    
    printf("C++: close window to exit\n");
    fflush(stdout);
    
    while (!WindowShouldClose()) {}
    
    CloseWindow();
    printf("C++: done\n");
    return 0;
}
