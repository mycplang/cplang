#include <stdio.h>
#include <raylib.h>

int main() {
    InitWindow(800, 200, "Font Diag");
    Font f = GetFontDefault();
    printf("baseSize=%d, glyphCount=%d, texture.id=%d\n", f.baseSize, f.glyphCount, f.texture.id);

    // Check ASCII digit glyphs
    for (int c = '0'; c <= '9'; c++) {
        int idx = GetGlyphIndex(f, c);
        if (idx >= 0 && idx < f.glyphCount) {
            GlyphInfo *g = &f.glyphs[idx];
            Rectangle *r = &f.recs[idx];
            printf("'%c' idx=%d offsetX=%d offsetY=%d advanceX=%d rec=(%.0f,%.0f, %.0fx%.0f)\n",
                c, idx, g->offsetX, g->offsetY, g->advanceX,
                r->x, r->y, r->width, r->height);
        }
    }

    // Test: DrawTextEx at y=50
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        DrawText("HELLO 2048 Score: 128", 20, 60, 32, RED);
        DrawText("BASELINE TEST", 20, 120, 24, GREEN);
        DrawFPS(10, 10);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
