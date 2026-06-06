#include "stdlib/stdlib.hpp"

namespace cplang {

// Raylib stubs for Linux — thin wrappers around raylib C API
// This file is #included inside namespace cplang
#include "vm/vm.hpp"
#include <raylib.h>

static void registerRaylibStubs(VM* vm) {
    vm->registerNative("initWindow", [](std::vector<Value>& a) -> Value {
        int w = a.size() > 0 ? (int)a[0].asInt() : 800;
        int h = a.size() > 1 ? (int)a[1].asInt() : 600;
        const char* title = a.size() > 2 && a[2].isString()
            ? std::string(a[2].asString()->data, a[2].asString()->length).c_str()
            : "CP Window";
        InitWindow(w, h, title);
        return Value::nil();
    });
    vm->registerNative("setTargetFPS", [](std::vector<Value>& a) -> Value {
        SetTargetFPS(a.size() > 0 ? (int)a[0].asInt() : 60);
        return Value::nil();
    });
    vm->registerNative("windowShouldClose", [](std::vector<Value>&) -> Value {
        return Value::Bool(WindowShouldClose());
    });
    vm->registerNative("beginDrawing", [](std::vector<Value>&) -> Value {
        BeginDrawing(); return Value::nil();
    });
    vm->registerNative("endDrawing", [](std::vector<Value>&) -> Value {
        EndDrawing(); return Value::nil();
    });
    vm->registerNative("clearBackground", [](std::vector<Value>& a) -> Value {
        if (a.size() >= 4) ClearBackground({(unsigned char)a[0].asInt(), (unsigned char)a[1].asInt(), (unsigned char)a[2].asInt(), (unsigned char)a[3].asInt()});
        else ClearBackground(RAYWHITE);
        return Value::nil();
    });
    vm->registerNative("drawText", [](std::vector<Value>& a) -> Value {
        if (a.size() < 1 || !a[0].isString()) return Value::nil();
        std::string text(a[0].asString()->data, a[0].asString()->length);
        int x = a.size() > 1 ? (int)a[1].asInt() : 0;
        int y = a.size() > 2 ? (int)a[2].asInt() : 0;
        int size = a.size() > 3 ? (int)a[3].asInt() : 20;
        DrawText(text.c_str(), x, y, size, BLACK);
        return Value::nil();
    });
    vm->registerNative("drawFPS", [](std::vector<Value>& a) -> Value {
        int x = a.size() > 0 ? (int)a[0].asInt() : 10;
        int y = a.size() > 1 ? (int)a[1].asInt() : 10;
        DrawFPS(x, y); return Value::nil();
    });
    vm->registerNative("closeWindow", [](std::vector<Value>&) -> Value {
        CloseWindow(); return Value::nil();
    });
    vm->registerNative("drawRectangle", [](std::vector<Value>& a) -> Value {
        int x = a.size() > 0 ? (int)a[0].asInt() : 0;
        int y = a.size() > 1 ? (int)a[1].asInt() : 0;
        int w = a.size() > 2 ? (int)a[2].asInt() : 100;
        int h = a.size() > 3 ? (int)a[3].asInt() : 100;
        DrawRectangle(x, y, w, h, RED);
        return Value::nil();
    });
    vm->registerNative("getRandomValue", [](std::vector<Value>& a) -> Value {
        int lo = a.size() > 0 ? (int)a[0].asInt() : 0;
        int hi = a.size() > 1 ? (int)a[1].asInt() : 100;
        return Value::Int(GetRandomValue(lo, hi));
    });
    vm->registerNative("keyPressed", [](std::vector<Value>& a) -> Value {
        int key = a.size() > 0 ? (int)a[0].asInt() : 0;
        return Value::Bool(IsKeyPressed(key));
    });
    vm->registerNative("isKeyDown", [](std::vector<Value>& a) -> Value {
        int key = a.size() > 0 ? (int)a[0].asInt() : 0;
        return Value::Bool(IsKeyDown(key));
    });
    // Chinese aliases
    vm->registerNative("初始化窗口", [](std::vector<Value>& a) -> Value {
        int w = a.size() > 0 ? (int)a[0].asInt() : 800;
        int h = a.size() > 1 ? (int)a[1].asInt() : 600;
        const char* title = a.size() > 2 && a[2].isString()
            ? std::string(a[2].asString()->data, a[2].asString()->length).c_str() : "CP";
        InitWindow(w, h, title); return Value::nil();
    });
    vm->registerNative("设置目标帧率", [](std::vector<Value>& a) -> Value {
        SetTargetFPS(a.size() > 0 ? (int)a[0].asInt() : 60); return Value::nil();
    });
    vm->registerNative("窗口应关闭", [](std::vector<Value>&) -> Value {
        return Value::Bool(WindowShouldClose());
    });
    vm->registerNative("开始绘制", [](std::vector<Value>&) -> Value {
        BeginDrawing(); return Value::nil();
    });
    vm->registerNative("结束绘制", [](std::vector<Value>&) -> Value {
        EndDrawing(); return Value::nil();
    });
    vm->registerNative("清除背景", [](std::vector<Value>& a) -> Value {
        if (a.size() >= 4) ClearBackground({(unsigned char)a[0].asInt(), (unsigned char)a[1].asInt(), (unsigned char)a[2].asInt(), (unsigned char)a[3].asInt()});
        else ClearBackground(RAYWHITE); return Value::nil();
    });
    vm->registerNative("关闭窗口", [](std::vector<Value>&) -> Value {
        CloseWindow(); return Value::nil();
    });
    vm->registerNative("绘制文本", [](std::vector<Value>& a) -> Value {
        if (a.size() < 1 || !a[0].isString()) return Value::nil();
        std::string text(a[0].asString()->data, a[0].asString()->length);
        int x = a.size() > 1 ? (int)a[1].asInt() : 0;
        int y = a.size() > 2 ? (int)a[2].asInt() : 0;
        int size = a.size() > 3 ? (int)a[3].asInt() : 20;
        DrawText(text.c_str(), x, y, size, BLACK); return Value::nil();
    });
    vm->registerNative("键盘按下", [](std::vector<Value>& a) -> Value {
        int key = a.size() > 0 ? (int)a[0].asInt() : 0;
        return Value::Bool(IsKeyPressed(key));
    });
    
    // Color constants
    vm->registerNative("RED", [](std::vector<Value>&) -> Value { return Value::Int(0xFF0000FF); });
    vm->registerNative("GREEN", [](std::vector<Value>&) -> Value { return Value::Int(0x00FF00FF); });
    vm->registerNative("BLUE", [](std::vector<Value>&) -> Value { return Value::Int(0x0000FFFF); });
    vm->registerNative("WHITE", [](std::vector<Value>&) -> Value { return Value::Int(0xFFFFFFFF); });
    vm->registerNative("RAYWHITE", [](std::vector<Value>&) -> Value { return Value::Int(0xF5F5F5FF); });
    vm->registerNative("红", [](std::vector<Value>&) -> Value { return Value::Int(0xFF0000FF); });
    vm->registerNative("绿", [](std::vector<Value>&) -> Value { return Value::Int(0x00FF00FF); });
    vm->registerNative("蓝", [](std::vector<Value>&) -> Value { return Value::Int(0x0000FFFF); });
    vm->registerNative("白", [](std::vector<Value>&) -> Value { return Value::Int(0xFFFFFFFF); });
    vm->registerNative("drawCircle", [](std::vector<Value>& a) -> Value {
        int cx=a.size()>0?(int)a[0].asInt():0, cy=a.size()>1?(int)a[1].asInt():0;
        float r=a.size()>2?(float)a[2].asFloat():10;
        DrawCircle(cx, cy, r, RED); return Value::nil();
    });
    vm->registerNative("绘制圆", [](std::vector<Value>& a) -> Value {
        int cx=a.size()>0?(int)a[0].asInt():0, cy=a.size()>1?(int)a[1].asInt():0;
        float r=a.size()>2?(float)a[2].asFloat():10;
        DrawCircle(cx, cy, r, RED); return Value::nil();
    });
    vm->registerNative("绘制圆形", [](std::vector<Value>& a) -> Value {
        int cx=a.size()>0?(int)a[0].asInt():0, cy=a.size()>1?(int)a[1].asInt():0;
        float r=a.size()>2?(float)a[2].asFloat():10;
        DrawCircle(cx, cy, r, RED); return Value::nil();
    });
    vm->registerNative("假", [](std::vector<Value>&) -> Value { return Value::Bool(false); });
    vm->registerNative("界面初始化", [](std::vector<Value>&) -> Value { return Value::nil(); });
    vm->registerNative("界面开始绘制", [](std::vector<Value>&) -> Value { return Value::nil(); });
    vm->registerNative("界面结束绘制", [](std::vector<Value>&) -> Value { return Value::nil(); });
    vm->registerNative("窗口开始", [](std::vector<Value>&) -> Value { return Value::nil(); });
    vm->registerNative("窗口结束", [](std::vector<Value>&) -> Value { return Value::nil(); });
    vm->registerNative("进度条", [](std::vector<Value>&) -> Value { return Value::nil(); });
    vm->registerNative("color", [](std::vector<Value>& a) -> Value {
        int r=a.size()>0?(int)a[0].asInt():255, g=a.size()>1?(int)a[1].asInt():255;
        int b=a.size()>2?(int)a[2].asInt():255, alpha=a.size()>3?(int)a[3].asInt():255;
        Color c={ (unsigned char)r,(unsigned char)g,(unsigned char)b,(unsigned char)alpha };
        return Value::Int(*(int*)&c);
    });
    vm->registerNative("rectangle", [](std::vector<Value>& a) -> Value {
        float x=a.size()>0?(float)a[0].asFloat():0, y=a.size()>1?(float)a[1].asFloat():0;
        float w=a.size()>2?(float)a[2].asFloat():100, h=a.size()>3?(float)a[3].asFloat():100;
        DrawRectangle((int)x,(int)y,w,h,RED); return Value::nil();
    });
    vm->registerNative("released", [](std::vector<Value>&) -> Value { return Value::Bool(false); });
    vm->registerNative("X", [](std::vector<Value>&) -> Value { return Value::Int(0); });
    vm->registerNative("Y", [](std::vector<Value>&) -> Value { return Value::Int(0); });
    vm->registerNative("清空背景", [](std::vector<Value>& a) -> Value {
        if (a.size() >= 4) ClearBackground({(unsigned char)a[0].asInt(), (unsigned char)a[1].asInt(), (unsigned char)a[2].asInt(), (unsigned char)a[3].asInt()});
        else ClearBackground(RAYWHITE); return Value::nil();
    });
}

static struct RaylibAutoRegister {
    RaylibAutoRegister() {
        // Will be called when the stubs are included
    }
} _raylib_auto;

} // namespace cplang
