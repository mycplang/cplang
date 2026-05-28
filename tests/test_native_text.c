#include "raylib.h"
#include <stdio.h>

int main() {
    printf("[NATIVE] InitWindow...\n");
    InitWindow(800, 600, "Native Text Test");
    
    printf("[NATIVE] Font: glyphs=%d tex=%d base=%d\n", 
        GetFontDefault().glyphCount, GetFontDefault().texture.id, GetFontDefault().baseSize);
    
    printf("[NATIVE] Entering loop...\n");
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        printf("[NATIVE] pre-DrawText\n");
        DrawText("HELLO NATIVE", 10, 10, 40, RED);
        printf("[NATIVE] post-DrawText\n");
        
        printf("[NATIVE] pre-FPS\n");
        DrawFPS(10, 60);
        printf("[NATIVE] post-FPS\n");
        
        EndDrawing();
        printf("[NATIVE] frame done\n");
    }
    
    CloseWindow();
    return 0;
}
