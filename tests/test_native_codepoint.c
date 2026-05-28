#include "raylib.h"
int main() {
    InitWindow(400, 300, "native font test");
    SetTargetFPS(60);
    Font font = GetFontDefault();
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        // Test 1: DrawText (standard)
        DrawText("0123456789", 20, 20, 20, BLACK);
        DrawText("16", 20, 50, 36, RED);
        DrawText("32", 20, 100, 36, RED);
        // Test 2: DrawTextCodepoint per-char
        float px = 200, py = 20;
        const char* t = "32";
        for (int i = 0; t[i]; i++) {
            int cp = (unsigned char)t[i];
            DrawTextCodepoint(font, cp, (Vector2){px, py}, 36, BLUE);
            px += font.glyphs[cp-32].advanceX * 36.0f / font.baseSize;
        }
        DrawFPS(300, 270);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
