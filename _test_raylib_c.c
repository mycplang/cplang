#include <raylib.h>
int main() {
    Color cream = {245, 245, 245, 255};
    Color blue = {0, 121, 241, 255};
    Color red = {230, 41, 55, 255};
    InitWindow(400, 300, "RL C TEST");
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(cream);
        DrawRectangle(50, 50, 200, 100, blue);
        DrawCircle(200, 200, 30, red);
        DrawFPS(10, 10);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
