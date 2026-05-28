// imgui_impl_raylib_real.cpp — ImGui + raylib (ImGui 1.92+ API)
#include "imgui.h"
#include "imgui_impl_raylib.h"
#include "raylib.h"
#include "rlgl.h"

static bool g_Initialized = false;
static double g_Time = 0.0;

static const char* GetClipText(void*) { return GetClipboardText(); }
static void SetClipText(void*, const char* text) { SetClipboardText(text); }

bool ImGui_ImplRaylib_Init() {
    if (g_Initialized) return true;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.BackendPlatformName = "imgui_impl_raylib";
    io.SetClipboardTextFn = SetClipText;
    io.GetClipboardTextFn = GetClipText;
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_HasSetMousePos;
    
    // Keyboard mapping (ImGui 1.87+ uses AddKeyEvent)
    io.AddKeyEvent(ImGuiKey_Tab, false);       io.AddKeyEvent(ImGuiKey_LeftArrow, false);
    io.AddKeyEvent(ImGuiKey_RightArrow, false); io.AddKeyEvent(ImGuiKey_UpArrow, false);
    io.AddKeyEvent(ImGuiKey_DownArrow, false);  io.AddKeyEvent(ImGuiKey_PageUp, false);
    io.AddKeyEvent(ImGuiKey_PageDown, false);   io.AddKeyEvent(ImGuiKey_Home, false);
    io.AddKeyEvent(ImGuiKey_End, false);        io.AddKeyEvent(ImGuiKey_Insert, false);
    io.AddKeyEvent(ImGuiKey_Delete, false);     io.AddKeyEvent(ImGuiKey_Backspace, false);
    io.AddKeyEvent(ImGuiKey_Space, false);      io.AddKeyEvent(ImGuiKey_Enter, false);
    io.AddKeyEvent(ImGuiKey_Escape, false);
    
    g_Initialized = true;
    g_Time = GetTime();
    return true;
}

void ImGui_ImplRaylib_Shutdown() {
    if (!g_Initialized) return;
    ImGui::DestroyContext();
    g_Initialized = false;
}

static int RaylibKeyToImGuiKey(int key) {
    switch (key) {
        case KEY_TAB: return ImGuiKey_Tab;
        case KEY_LEFT: return ImGuiKey_LeftArrow;
        case KEY_RIGHT: return ImGuiKey_RightArrow;
        case KEY_UP: return ImGuiKey_UpArrow;
        case KEY_DOWN: return ImGuiKey_DownArrow;
        case KEY_PAGE_UP: return ImGuiKey_PageUp;
        case KEY_PAGE_DOWN: return ImGuiKey_PageDown;
        case KEY_HOME: return ImGuiKey_Home;
        case KEY_END: return ImGuiKey_End;
        case KEY_INSERT: return ImGuiKey_Insert;
        case KEY_DELETE: return ImGuiKey_Delete;
        case KEY_BACKSPACE: return ImGuiKey_Backspace;
        case KEY_SPACE: return ImGuiKey_Space;
        case KEY_ENTER: return ImGuiKey_Enter;
        case KEY_ESCAPE: return ImGuiKey_Escape;
        case KEY_A: return ImGuiKey_A;
        case KEY_C: return ImGuiKey_C;
        case KEY_V: return ImGuiKey_V;
        case KEY_X: return ImGuiKey_X;
        case KEY_Y: return ImGuiKey_Y;
        case KEY_Z: return ImGuiKey_Z;
        default: return ImGuiKey_None;
    }
}

void ImGui_ImplRaylib_NewFrame() {
    if (!g_Initialized) return;
    ImGuiIO& io = ImGui::GetIO();
    
    double t = GetTime();
    io.DeltaTime = (float)(t - g_Time);
    if (io.DeltaTime <= 0.0f) io.DeltaTime = 1.0f/60.0f;
    g_Time = t;
    
    io.DisplaySize.x = (float)GetScreenWidth();
    io.DisplaySize.y = (float)GetScreenHeight();
    
    // Mouse
    io.AddMousePosEvent((float)GetMouseX(), (float)GetMouseY());
    io.AddMouseButtonEvent(0, IsMouseButtonDown(MOUSE_LEFT_BUTTON));
    io.AddMouseButtonEvent(1, IsMouseButtonDown(MOUSE_RIGHT_BUTTON));
    io.AddMouseButtonEvent(2, IsMouseButtonDown(MOUSE_MIDDLE_BUTTON));
    io.AddMouseWheelEvent(0, GetMouseWheelMove());
    
    // Keyboard
    io.AddKeyEvent(ImGuiMod_Ctrl, IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL));
    io.AddKeyEvent(ImGuiMod_Shift, IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT));
    io.AddKeyEvent(ImGuiMod_Alt, IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT));
    
    for (int key = KEY_SPACE; key <= KEY_KP_EQUAL; key++) {
        int ik = RaylibKeyToImGuiKey(key);
        if (ik != ImGuiKey_None)
            io.AddKeyEvent(ik, IsKeyDown(key));
    }
    
    // Text input
    int ch = GetCharPressed();
    while (ch > 0) {
        io.AddInputCharacter(ch);
        ch = GetCharPressed();
    }
    
    ImGui::NewFrame();
}

void ImGui_ImplRaylib_RenderDrawData(ImDrawData* drawData) {
    if (!drawData || drawData->CmdListsCount == 0) return;
    
    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();
    
    for (int n = 0; n < drawData->CmdListsCount; n++) {
        const ImDrawList* cl = drawData->CmdLists[n];
        
        rlEnableVertexArray(cl->VtxBuffer.Size > 0 ? (unsigned int)(uintptr_t)cl->VtxBuffer.Data : 0);
        rlEnableVertexBuffer(cl->IdxBuffer.Size > 0 ? (unsigned int)(uintptr_t)cl->IdxBuffer.Data : 0);
        
        rlSetVertexAttribute(0, 2, RL_FLOAT, false, sizeof(ImDrawVert), (void*)offsetof(ImDrawVert, pos));
        rlSetVertexAttribute(1, 2, RL_FLOAT, false, sizeof(ImDrawVert), (void*)offsetof(ImDrawVert, uv));
        rlSetVertexAttribute(2, 4, RL_UNSIGNED_BYTE, true, sizeof(ImDrawVert), (void*)offsetof(ImDrawVert, col));
        rlEnableVertexAttribute(0); rlEnableVertexAttribute(1); rlEnableVertexAttribute(2);
        
        for (int i = 0; i < cl->CmdBuffer.Size; i++) {
            const ImDrawCmd* cmd = &cl->CmdBuffer[i];
            if (cmd->UserCallback) {
                cmd->UserCallback(cl, cmd);
            } else {
                if (cmd->ClipRect.z > cmd->ClipRect.x && cmd->ClipRect.w > cmd->ClipRect.y) {
                    rlEnableScissorTest();
                    rlScissor((int)cmd->ClipRect.x, (int)(ImGui::GetIO().DisplaySize.y - cmd->ClipRect.w),
                              (int)(cmd->ClipRect.z - cmd->ClipRect.x), (int)(cmd->ClipRect.w - cmd->ClipRect.y));
                }
                rlEnableTexture(cmd->TextureId ? (unsigned int)(uintptr_t)cmd->TextureId : 
                    (unsigned int)(uintptr_t)ImGui::GetIO().Fonts->TexID);
                rlActiveTextureSlot(0);
                rlDrawVertexArrayElements(0, cmd->ElemCount, RL_UNSIGNED_SHORT, (void*)(uintptr_t)(cmd->IdxOffset * sizeof(ImDrawIdx)));
                rlDisableScissorTest();
            }
        }
        rlDisableVertexArray();
        rlDisableVertexBuffer();
    }
    rlEnableBackfaceCulling();
    rlDrawRenderBatchActive();
}
