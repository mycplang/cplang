#include "stdlib/stdlib.hpp"

namespace cplang {

// Raylib standard library bindings for CP language
// #include'd from stdlib.cpp, already inside namespace cplang
// Requires: raylib.h, raylib.lib linked

#include <cstdio>
#include <cstring>
#include <cstdlib>

#ifndef RAYLIB_H
#include <raylib.h>
#endif // standalone guard

// ═══════════════════════════════════════════════════════════════════
//  Helper: diagnostic logging
// ═══════════════════════════════════════════════════════════════════

static void _diag(const char* msg) {
    FILE* f = fopen("C:\\cplang\\diag.txt", "a");
    if (f) { fputs(msg, f); fputs("\n", f); fclose(f); }
}

// ═══════════════════════════════════════════════════════════════════
//  Helper: convert between CP table and raylib Color
// ═══════════════════════════════════════════════════════════════════

static Color valueToColor(const Value& v) {
    if (v.isFunction()) {
        // auto-call zero-arg color functions like BLACK() RED() etc.
        std::vector<Value> emptyArgs;
        Value r = VM::current()->callFunction(v, emptyArgs);
        return valueToColor(r);
    }
    if (!v.isTable()) return BLACK;
    auto t = v.asTable();
    Color c;
    c.r = (unsigned char)(t->has(makeStringVal(VMString::create("r"))) ? std::max(0, std::min(255, (int)t->get(makeStringVal(VMString::create("r"))).asInt())) : 0);
    c.g = (unsigned char)(t->has(makeStringVal(VMString::create("g"))) ? std::max(0, std::min(255, (int)t->get(makeStringVal(VMString::create("g"))).asInt())) : 0);
    c.b = (unsigned char)(t->has(makeStringVal(VMString::create("b"))) ? std::max(0, std::min(255, (int)t->get(makeStringVal(VMString::create("b"))).asInt())) : 0);
    c.a = (unsigned char)(t->has(makeStringVal(VMString::create("a"))) ? std::max(0, std::min(255, (int)t->get(makeStringVal(VMString::create("a"))).asInt())) : 255);
    return c;
}

static Value colorToTable(Color c) {
    VM* vm = VM::current();
    if (!vm) return Value::Int(0);
    auto t = VMTable::create();
    t->set(makeStringVal(VMString::create("r")), Value::Int(c.r));
    t->set(makeStringVal(VMString::create("g")), Value::Int(c.g));
    t->set(makeStringVal(VMString::create("b")), Value::Int(c.b));
    t->set(makeStringVal(VMString::create("a")), Value::Int(c.a));
    return makeTableVal(t);
}

static Vector2 valueToVec2(const Value& v) {
    if (!v.isTable()) return {0,0};
    auto t = v.asTable();
    return Vector2{
        (float)(t->has(makeStringVal(VMString::create("x"))) ? t->get(makeStringVal(VMString::create("x"))).asFloat() : 0.0),
        (float)(t->has(makeStringVal(VMString::create("y"))) ? t->get(makeStringVal(VMString::create("y"))).asFloat() : 0.0)
    };
}

static Value vec2ToTable(Vector2 v) {
    auto t = VMTable::create();
    t->set(makeStringVal(VMString::create("x")), Value::Float(v.x));
    t->set(makeStringVal(VMString::create("y")), Value::Float(v.y));
    return makeTableVal(t);
}

static Rectangle valueToRect(const Value& v) {
    if (!v.isTable()) return {0,0,0,0};
    auto t = v.asTable();
    return Rectangle{
        (float)(t->has(makeStringVal(VMString::create("x"))) ? t->get(makeStringVal(VMString::create("x"))).asFloat() : 0.0),
        (float)(t->has(makeStringVal(VMString::create("y"))) ? t->get(makeStringVal(VMString::create("y"))).asFloat() : 0.0),
        (float)(t->has(makeStringVal(VMString::create("width"))) ? t->get(makeStringVal(VMString::create("width"))).asFloat() : 0.0),
        (float)(t->has(makeStringVal(VMString::create("height"))) ? t->get(makeStringVal(VMString::create("height"))).asFloat() : 0.0)
    };
}

static Value rectToTable(Rectangle r) {
    auto t = VMTable::create();
    t->set(makeStringVal(VMString::create("x")), Value::Float(r.x));
    t->set(makeStringVal(VMString::create("y")), Value::Float(r.y));
    t->set(makeStringVal(VMString::create("width")), Value::Float(r.width));
    t->set(makeStringVal(VMString::create("height")), Value::Float(r.height));
    return makeTableVal(t);
}

// ═══════════════════════════════════════════════════════════════════
//  Window functions
// ═══════════════════════════════════════════════════════════════════

Value rl_initWindow(std::vector<Value>& args) {
    int w = 800, h = 600;
    const char* title = "CP + raylib";
    if (args.size() >= 1 && args[0].isInt()) w = (int)args[0].asInt();
    if (args.size() >= 2 && args[1].isInt()) h = (int)args[1].asInt();
    SetTraceLogLevel(LOG_ERROR);  // 静默 raylib 的 INFO/WARNING 日志
    if (args.size() >= 3 && args[2].isString()) {
        auto s = args[2].asString();
        title = s->c_str();
    }
    InitWindow(w, h, title);
    return Value::Int(0);
}

Value rl_windowShouldClose(std::vector<Value>&) {
    return Value::Bool(WindowShouldClose());
}

Value rl_closeWindow(std::vector<Value>&) {
    CloseWindow();
    return Value::Int(0);
}

Value rl_keyPressed(std::vector<Value>& args) {
    int key = args.empty() ? 0 : (args[0].isInt() ? (int)args[0].asInt() : 0);
    return Value::Bool(IsKeyPressed(key));
}

Value rl_setTargetFPS(std::vector<Value>& args) {
    int fps = args.size() > 0 && args[0].isInt() ? (int)args[0].asInt() : 60;
    SetTargetFPS(fps);
    return Value::Int(0);
}

Value rl_getFPS(std::vector<Value>&) {
    return Value::Int(GetFPS());
}

Value rl_getFrameTime(std::vector<Value>&) {
    return Value::Float(GetFrameTime());
}

Value rl_setWindowTitle(std::vector<Value>& args) {
    if (!args.empty() && args[0].isString()) {
        SetWindowTitle(args[0].asString()->c_str());
    }
    return Value::Int(0);
}

Value rl_isWindowResized(std::vector<Value>&) {
    return Value::Bool(IsWindowResized());
}

Value rl_getScreenWidth(std::vector<Value>&) {
    return Value::Int(GetScreenWidth());
}

Value rl_getScreenHeight(std::vector<Value>&) {
    return Value::Int(GetScreenHeight());
}

// ═══════════════════════════════════════════════════════════════════
//  Drawing functions
// ═══════════════════════════════════════════════════════════════════

Value rl_beginDrawing(std::vector<Value>&) {
    BeginDrawing();
    return Value::Int(0);
}

Value rl_endDrawing(std::vector<Value>&) {
    EndDrawing();
    return Value::Int(0);
}

Value rl_clearBackground(std::vector<Value>& args) {
    Color c = args.empty() ? BLACK : valueToColor(args[0]);
    ClearBackground(c);
    return Value::Int(0);
}

Value rl_drawFPS(std::vector<Value>& args) {
    int x = 10, y = 10;
    if (args.size() >= 1 && args[0].isInt()) x = (int)args[0].asInt();
    if (args.size() >= 2 && args[1].isInt()) y = (int)args[1].asInt();
    DrawFPS(x, y);
    return Value::Int(0);
}

// CJK font support — loaded on first use with needed glyphs only
static Font g_cjkFont = {0};

static Font& getCjkFont() {
    if (g_cjkFont.glyphCount != 0) return g_cjkFont;

    // Build codepoint list: ASCII + CJK + CJK Symbols + Fullwidth Forms
    // Covers Chinese characters, Chinese punctuation (，。！？), and fullwidth forms
    std::vector<int> codepoints;
    codepoints.reserve(22500);
    for (int i = 32; i <= 126; i++) codepoints.push_back(i);
    for (int cp = 0x4E00; cp <= 0x9FFF; cp++) codepoints.push_back(cp);
    for (int cp = 0x3000; cp <= 0x303F; cp++) codepoints.push_back(cp);
    for (int cp = 0xFF00; cp <= 0xFFEF; cp++) codepoints.push_back(cp);
    for (int cp = 0x2000; cp <= 0x206F; cp++) codepoints.push_back(cp);   // General Punctuation
    for (int cp = 0xFE30; cp <= 0xFE4F; cp++) codepoints.push_back(cp);   // CJK Compatibility Forms
    for (int cp = 0xFE10; cp <= 0xFE1F; cp++) codepoints.push_back(cp);   // Vertical Forms
    for (int cp = 0x2100; cp <= 0x214F; cp++) codepoints.push_back(cp);   // Letterlike Symbols (℃℉№)
    for (int cp = 0x2190; cp <= 0x21FF; cp++) codepoints.push_back(cp);   // Arrows (←↑→↓↔)
    for (int cp = 0x2200; cp <= 0x22FF; cp++) codepoints.push_back(cp);   // Mathematical Operators (≈≠≤≥±×÷∑)
    for (int cp = 0x2460; cp <= 0x24FF; cp++) codepoints.push_back(cp);   // Enclosed Alphanumerics (①②③)
    for (int cp = 0x2500; cp <= 0x257F; cp++) codepoints.push_back(cp);   // Box Drawing (─│┌┐└┘)
    for (int cp = 0x25A0; cp <= 0x25FF; cp++) codepoints.push_back(cp);   // Geometric Shapes (■□▲△●○)
    for (int cp = 0x2600; cp <= 0x26FF; cp++) codepoints.push_back(cp);   // Miscellaneous Symbols (★☆☎♠)
    for (int cp = 0x0391; cp <= 0x03A9; cp++) codepoints.push_back(cp);   // Greek uppercase
    for (int cp = 0x03B1; cp <= 0x03C9; cp++) codepoints.push_back(cp);   // Greek lowercase
    for (int cp = 0x0400; cp <= 0x04FF; cp++) codepoints.push_back(cp);   // Cyrillic

    // Try fonts in order; reduce fontSize if full-range atlas exceeds GPU texture limit
    const char* paths[] = { "C:/Windows/Fonts/simhei.ttf", "C:/Windows/Fonts/simsun.ttc", NULL };
    int sizes[] = { 24, 20, 16 };  // fallback sizes if 24px atlas is too large
    int cpCount = (int)codepoints.size();

    for (int si = 0; si < 3; si++) {
        int fontSize = sizes[si];
        for (int fi = 0; paths[fi] != NULL; fi++) {
            g_cjkFont = LoadFontEx(paths[fi], fontSize, codepoints.data(), cpCount);
            if (g_cjkFont.texture.id != 0 && g_cjkFont.glyphCount >= cpCount * 9 / 10) goto font_ok;
            if (g_cjkFont.texture.id != 0) { UnloadFont(g_cjkFont); g_cjkFont = {0}; }
        }
    }
font_ok:
    if (g_cjkFont.glyphCount == 0) g_cjkFont = GetFontDefault();
    return g_cjkFont;
}

Value rl_drawText(std::vector<Value>& args) {
    if (args.size() < 3) return Value::Int(0);
    const char* text = args[0].isString() ? args[0].asString()->c_str() : "";
    int x = args[1].isInt() ? (int)args[1].asInt() : 0;
    int y = args[2].isInt() ? (int)args[2].asInt() : 0;
    int fontSize = 20;
    Color c = BLACK;
    if (args.size() >= 4 && args[3].isInt()) fontSize = (int)args[3].asInt();
    if (args.size() >= 5) c = valueToColor(args[4]);
    Color textColor = { c.r, c.g, c.b, c.a };
    Font asciiFont = GetFontDefault();
    Font* cjkFont = nullptr;
    float posX = (float)x;
    float posY = (float)y;
    for (int i = 0; text[i] != '\0'; ) {
        int codePoint = 0;
        if ((text[i] & 0x80) == 0) {
            codePoint = (unsigned char)text[i];
            i += 1;
        } else if ((text[i] & 0xE0) == 0xC0) {
            codePoint = ((text[i] & 0x1F) << 6) | (text[i+1] & 0x3F);
            i += 2;
        } else if ((text[i] & 0xF0) == 0xE0) {
            codePoint = ((text[i] & 0x0F) << 12) | ((text[i+1] & 0x3F) << 6) | (text[i+2] & 0x3F);
            i += 3;
        } else if ((text[i] & 0xF8) == 0xF0) {
            codePoint = ((text[i] & 0x07) << 18) | ((text[i+1] & 0x3F) << 12) | ((text[i+2] & 0x3F) << 6) | (text[i+3] & 0x3F);
            i += 4;
        } else {
            i += 1; continue;
        }
        Font* useFont = &asciiFont;
        if (codePoint > 127) {
            if (!cjkFont) cjkFont = &getCjkFont();
            useFont = cjkFont;
        }
        int idx = GetGlyphIndex(*useFont, codePoint);
        if (idx < useFont->glyphCount) {
            Vector2 pos = { posX, posY };
            DrawTextCodepoint(*useFont, codePoint, pos, (float)fontSize, textColor);
            // advanceX is 0 in this raylib build; use recs width * 1.25 for spacing
            float adv = (float)(useFont->recs[idx].width) * 1.25f * (float)fontSize / (float)useFont->baseSize;
            if (adv <= 0) adv = (float)fontSize * 0.6f;
            posX += adv;
        }
    }
    return Value::Int(0);
}

Value rl_drawRectangle(std::vector<Value>& args) {
    if (args.size() < 4) return Value::Int(0);
    int x = (int)args[0].asInt();
    int y = (int)args[1].asInt();
    int w = (int)args[2].asInt();
    int h = (int)args[3].asInt();
    Color c = args.size() >= 5 ? valueToColor(args[4]) : BLACK;
    DrawRectangle(x, y, w, h, c);
    return Value::Int(0);
}

Value rl_drawRectangleRec(std::vector<Value>& args) {
    if (args.size() < 1) return Value::Int(0);
    Rectangle rec = valueToRect(args[0]);
    Color c = args.size() >= 2 ? valueToColor(args[1]) : BLACK;
    DrawRectangleRec(rec, c);
    return Value::Int(0);
}

Value rl_drawCircle(std::vector<Value>& args) {
    if (args.size() < 3) return Value::Int(0);
    int cx = (int)args[0].asInt();
    int cy = (int)args[1].asInt();
    float r = (float)args[2].asFloat();
    Color c = args.size() >= 4 ? valueToColor(args[3]) : BLACK;
    DrawCircle(cx, cy, r, c);
    return Value::Int(0);
}

Value rl_drawLine(std::vector<Value>& args) {
    if (args.size() < 4) return Value::Int(0);
    int x1 = (int)args[0].asInt();
    int y1 = (int)args[1].asInt();
    int x2 = (int)args[2].asInt();
    int y2 = (int)args[3].asInt();
    Color c = args.size() >= 5 ? valueToColor(args[4]) : BLACK;
    DrawLine(x1, y1, x2, y2, c);
    return Value::Int(0);
}

Value rl_drawLineEx(std::vector<Value>& args) {
    if (args.size() < 3) return Value::Int(0);
    Vector2 start = valueToVec2(args[0]);
    Vector2 end = valueToVec2(args[1]);
    float thick = (float)args[2].asFloat();
    Color c = args.size() >= 4 ? valueToColor(args[3]) : BLACK;
    DrawLineEx(start, end, thick, c);
    return Value::Int(0);
}

Value rl_drawPixel(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Int(0);
    int x = (int)args[0].asInt();
    int y = (int)args[1].asInt();
    Color c = args.size() >= 3 ? valueToColor(args[2]) : BLACK;
    DrawPixel(x, y, c);
    return Value::Int(0);
}

// ═══════════════════════════════════════════════════════════════════
//  Color constructors and constants
// ═══════════════════════════════════════════════════════════════════

Value rl_color(std::vector<Value>& args) {
    int r = 0, g = 0, b = 0, a = 255;
    if (args.size() >= 1 && args[0].isInt()) r = (int)args[0].asInt();
    if (args.size() >= 2 && args[1].isInt()) g = (int)args[1].asInt();
    if (args.size() >= 3 && args[2].isInt()) b = (int)args[2].asInt();
    if (args.size() >= 4 && args[3].isInt()) a = (int)args[3].asInt();
    return colorToTable(Color{(unsigned char)r,(unsigned char)g,(unsigned char)b,(unsigned char)a});
}

#define RL_COLOR_CONST(name, rv, gv, bv, av) \
    static Value _##name; \
    Value rl_color_##name(std::vector<Value>&) { \
        if (_##name.isTable()) return _##name; \
        Color _c; _c.r=(unsigned char)(rv); _c.g=(unsigned char)(gv); _c.b=(unsigned char)(bv); _c.a=(unsigned char)(av); \
        _##name = colorToTable(_c); \
        return _##name; \
    }

RL_COLOR_CONST(lightgray,  200,200,200,255)
RL_COLOR_CONST(gray,       130,130,130,255)
RL_COLOR_CONST(darkgray,   80,80,80,255)
RL_COLOR_CONST(yellow,     253,249,0,255)
RL_COLOR_CONST(gold,       255,203,0,255)
RL_COLOR_CONST(orange,     255,161,0,255)
RL_COLOR_CONST(pink,       255,109,194,255)
RL_COLOR_CONST(red,        230,41,55,255)
RL_COLOR_CONST(maroon,     190,33,55,255)
RL_COLOR_CONST(green,      0,228,48,255)
RL_COLOR_CONST(lime,       0,158,47,255)
RL_COLOR_CONST(darkgreen,  0,117,44,255)
RL_COLOR_CONST(skyblue,    102,191,255,255)
RL_COLOR_CONST(blue,       0,121,241,255)
RL_COLOR_CONST(darkblue,   0,82,172,255)
RL_COLOR_CONST(purple,     200,122,255,255)
RL_COLOR_CONST(violet,     135,60,190,255)
RL_COLOR_CONST(darkpurple, 112,31,126,255)
RL_COLOR_CONST(beige,      211,176,131,255)
RL_COLOR_CONST(brown,      127,106,79,255)
RL_COLOR_CONST(darkbrown,  76,63,47,255)
RL_COLOR_CONST(white,      255,255,255,255)
RL_COLOR_CONST(black,      0,0,0,255)
RL_COLOR_CONST(blank,      0,0,0,0)
RL_COLOR_CONST(magenta,    255,0,255,255)
RL_COLOR_CONST(raywhite,   245,245,245,255)

#undef RL_COLOR_CONST

// ═══════════════════════════════════════════════════════════════════
//  Input functions
// ═══════════════════════════════════════════════════════════════════

Value rl_isKeyDown(std::vector<Value>& args) {
    if (args.empty() || !args[0].isInt()) return Value::Bool(false);
    return Value::Bool(IsKeyDown((int)args[0].asInt()));
}

Value rl_isKeyPressed(std::vector<Value>& args) {
    if (args.empty() || !args[0].isInt()) return Value::Bool(false);
    return Value::Bool(IsKeyPressed((int)args[0].asInt()));
}

Value rl_isKeyReleased(std::vector<Value>& args) {
    if (args.empty() || !args[0].isInt()) return Value::Bool(false);
    return Value::Bool(IsKeyReleased((int)args[0].asInt()));
}

Value rl_getMousePosition(std::vector<Value>&) {
    Vector2 pos = GetMousePosition();
    return vec2ToTable(pos);
}

Value rl_getMouseX(std::vector<Value>&) {
    return Value::Int(GetMouseX());
}

Value rl_getMouseY(std::vector<Value>&) {
    return Value::Int(GetMouseY());
}

Value rl_isMouseButtonDown(std::vector<Value>& args) {
    int btn = (args.empty() || !args[0].isInt()) ? MOUSE_LEFT_BUTTON : (int)args[0].asInt();
    return Value::Bool(IsMouseButtonDown(btn));
}

Value rl_isMouseButtonPressed(std::vector<Value>& args) {
    int btn = (args.empty() || !args[0].isInt()) ? MOUSE_LEFT_BUTTON : (int)args[0].asInt();
    return Value::Bool(IsMouseButtonPressed(btn));
}

Value rl_isMouseButtonReleased(std::vector<Value>& args) {
    int btn = (args.empty() || !args[0].isInt()) ? MOUSE_LEFT_BUTTON : (int)args[0].asInt();
    return Value::Bool(IsMouseButtonReleased(btn));
}

Value rl_getMouseWheelMove(std::vector<Value>&) {
    return Value::Float(GetMouseWheelMove());
}

// ═══════════════════════════════════════════════════════════════════
//  Vector2 / Rectangle constructors
// ═══════════════════════════════════════════════════════════════════

Value rl_vector2(std::vector<Value>& args) {
    float x = 0, y = 0;
    if (args.size() >= 1) x = (float)args[0].asFloat();
    if (args.size() >= 2) y = (float)args[1].asFloat();
    return vec2ToTable(Vector2{x, y});
}

Value rl_rectangle(std::vector<Value>& args) {
    float x = 0, y = 0, w = 0, h = 0;
    if (args.size() >= 1) x = (float)args[0].asFloat();
    if (args.size() >= 2) y = (float)args[1].asFloat();
    if (args.size() >= 3) w = (float)args[2].asFloat();
    if (args.size() >= 4) h = (float)args[3].asFloat();
    return rectToTable(Rectangle{x, y, w, h});
}

// ═══════════════════════════════════════════════════════════════════
//  Collision detection
// ═══════════════════════════════════════════════════════════════════

Value rl_checkCollisionPointRec(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Bool(false);
    Vector2 point = valueToVec2(args[0]);
    Rectangle rec = valueToRect(args[1]);
    return Value::Bool(CheckCollisionPointRec(point, rec));
}

Value rl_checkCollisionRecs(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Bool(false);
    Rectangle r1 = valueToRect(args[0]);
    Rectangle r2 = valueToRect(args[1]);
    return Value::Bool(CheckCollisionRecs(r1, r2));
}

Value rl_checkCollisionCircles(std::vector<Value>& args) {
    if (args.size() < 4) return Value::Bool(false);
    Vector2 c1 = valueToVec2(args[0]);
    float r1 = (float)args[1].asFloat();
    Vector2 c2 = valueToVec2(args[2]);
    float r2 = (float)args[3].asFloat();
    return Value::Bool(CheckCollisionCircles(c1, r1, c2, r2));
}

// ═══════════════════════════════════════════════════════════════════
//  Texture functions
// ═══════════════════════════════════════════════════════════════════

Value rl_loadTexture(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Int(0);
    auto s = args[0].asString();
    Texture2D tex = LoadTexture(s->c_str());
    auto vt = VMTexture2D::create(tex.id, tex.width, tex.height, tex.mipmaps, tex.format);
    return makePtrVal(vt);
}

Value rl_unloadTexture(std::vector<Value>& args) {
    if (!args.empty() && args[0].isRaylib() && args[0].obj->typeTag == ObjectHeader::TAG_TEXTURE2D) {
        auto vt = static_cast<VMTexture2D*>(args[0].obj);
        UnloadTexture({vt->id, vt->width, vt->height, vt->mipmaps, vt->format});
        vt->id = 0;
    }
    return Value::Int(0);
}

Value rl_drawTexture(std::vector<Value>& args) {
    if (args.size() < 3) return Value::Int(0);
    if (!args[0].isRaylib() || args[0].obj->typeTag != ObjectHeader::TAG_TEXTURE2D) return Value::Int(0);
    auto vt = static_cast<VMTexture2D*>(args[0].obj);
    Texture2D tex = {vt->id, vt->width, vt->height, vt->mipmaps, vt->format};
    int x = (int)args[1].asInt();
    int y = (int)args[2].asInt();
    Color tint = args.size() >= 4 ? valueToColor(args[3]) : WHITE;
    DrawTexture(tex, x, y, tint);
    return Value::Int(0);
}

Value rl_drawTextureEx(std::vector<Value>& args) {
    if (args.size() < 4) return Value::Int(0);
    if (!args[0].isRaylib() || args[0].obj->typeTag != ObjectHeader::TAG_TEXTURE2D) return Value::Int(0);
    auto vt = static_cast<VMTexture2D*>(args[0].obj);
    Texture2D tex = {vt->id, vt->width, vt->height, vt->mipmaps, vt->format};
    Vector2 pos = valueToVec2(args[1]);
    float rotation = (float)args[2].asFloat();
    float scale = (float)args[3].asFloat();
    Color tint = args.size() >= 5 ? valueToColor(args[4]) : WHITE;
    DrawTextureEx(tex, pos, rotation, scale, tint);
    return Value::Int(0);
}

// ═══════════════════════════════════════════════════════════════════
//  Image functions
// ═══════════════════════════════════════════════════════════════════

Value rl_loadImage(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Int(0);
    Image img = LoadImage(args[0].asString()->c_str());
    // Store image data - VMImage destructor will call UnloadImage
    auto vi = VMImage::create(img.data, img.width, img.height, img.mipmaps, img.format);
    return makePtrVal(vi);
}

Value rl_unloadImage(std::vector<Value>& args) {
    if (!args.empty() && args[0].isRaylib() && args[0].obj->typeTag == ObjectHeader::TAG_IMAGE) {
        auto vi = static_cast<VMImage*>(args[0].obj);
        UnloadImage({vi->data, vi->width, vi->height, vi->mipmaps, vi->format});
        vi->data = nullptr;
    }
    return Value::Int(0);
}

Value rl_imageToTexture(std::vector<Value>& args) {
    if (args.empty() || !args[0].isRaylib() || args[0].obj->typeTag != ObjectHeader::TAG_IMAGE)
        return Value::Int(0);
    auto vi = static_cast<VMImage*>(args[0].obj);
    Image img = {vi->data, vi->width, vi->height, vi->mipmaps, vi->format};
    Texture2D tex = LoadTextureFromImage(img);
    auto vt = VMTexture2D::create(tex.id, tex.width, tex.height, tex.mipmaps, tex.format);
    return makePtrVal(vt);
}

// ═══════════════════════════════════════════════════════════════════
//  Audio functions
// ═══════════════════════════════════════════════════════════════════

Value rl_initAudioDevice(std::vector<Value>&) {
    InitAudioDevice();
    return Value::Int(0);
}

Value rl_closeAudioDevice(std::vector<Value>&) {
    CloseAudioDevice();
    return Value::Int(0);
}

Value rl_loadSound(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Int(0);
    Sound snd = LoadSound(args[0].asString()->c_str());
    auto vs = VMSound::create();
    std::memcpy(vs->rlData, &snd, sizeof(Sound));
    return makePtrVal(vs);
}

Value rl_unloadSound(std::vector<Value>& args) {
    if (!args.empty() && args[0].isRaylib() && args[0].obj->typeTag == ObjectHeader::TAG_SOUND) {
        auto vs = static_cast<VMSound*>(args[0].obj);
        Sound snd;
        std::memcpy(&snd, vs->rlData, sizeof(Sound));
        UnloadSound(snd);
        std::memset(vs->rlData, 0, sizeof(Sound));
    }
    return Value::Int(0);
}

Value rl_playSound(std::vector<Value>& args) {
    if (args.empty() || !args[0].isRaylib() || args[0].obj->typeTag != ObjectHeader::TAG_SOUND)
        return Value::Int(0);
    auto vs = static_cast<VMSound*>(args[0].obj);
    Sound snd;
    std::memcpy(&snd, vs->rlData, sizeof(Sound));
    PlaySound(snd);
    return Value::Int(0);
}

Value rl_stopSound(std::vector<Value>& args) {
    if (args.empty() || !args[0].isRaylib() || args[0].obj->typeTag != ObjectHeader::TAG_SOUND)
        return Value::Int(0);
    auto vs = static_cast<VMSound*>(args[0].obj);
    Sound snd;
    std::memcpy(&snd, vs->rlData, sizeof(Sound));
    StopSound(snd);
    return Value::Int(0);
}

Value rl_setSoundVolume(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isRaylib() || args[0].obj->typeTag != ObjectHeader::TAG_SOUND)
        return Value::Int(0);
    auto vs = static_cast<VMSound*>(args[0].obj);
    Sound snd;
    std::memcpy(&snd, vs->rlData, sizeof(Sound));
    SetSoundVolume(snd, (float)args[1].asFloat());
    return Value::Int(0);
}

Value rl_loadMusicStream(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Int(0);
    Music mus = LoadMusicStream(args[0].asString()->c_str());
    auto vm = VMMusic::create();
    std::memcpy(vm->rlData, &mus, sizeof(Music));
    return makePtrVal(vm);
}

Value rl_unloadMusicStream(std::vector<Value>& args) {
    if (!args.empty() && args[0].isRaylib() && args[0].obj->typeTag == ObjectHeader::TAG_MUSIC) {
        auto vm = static_cast<VMMusic*>(args[0].obj);
        Music mus;
        std::memcpy(&mus, vm->rlData, sizeof(Music));
        UnloadMusicStream(mus);
        std::memset(vm->rlData, 0, sizeof(Music));
    }
    return Value::Int(0);
}

Value rl_playMusicStream(std::vector<Value>& args) {
    if (args.empty() || !args[0].isRaylib() || args[0].obj->typeTag != ObjectHeader::TAG_MUSIC)
        return Value::Int(0);
    auto vm = static_cast<VMMusic*>(args[0].obj);
    Music mus;
    std::memcpy(&mus, vm->rlData, sizeof(Music));
    PlayMusicStream(mus);
    return Value::Int(0);
}

Value rl_updateMusicStream(std::vector<Value>& args) {
    if (args.empty() || !args[0].isRaylib() || args[0].obj->typeTag != ObjectHeader::TAG_MUSIC)
        return Value::Int(0);
    auto vm = static_cast<VMMusic*>(args[0].obj);
    Music mus;
    std::memcpy(&mus, vm->rlData, sizeof(Music));
    UpdateMusicStream(mus);
    return Value::Int(0);
}

Value rl_stopMusicStream(std::vector<Value>& args) {
    if (args.empty() || !args[0].isRaylib() || args[0].obj->typeTag != ObjectHeader::TAG_MUSIC)
        return Value::Int(0);
    auto vm = static_cast<VMMusic*>(args[0].obj);
    Music mus;
    std::memcpy(&mus, vm->rlData, sizeof(Music));
    StopMusicStream(mus);
    return Value::Int(0);
}

Value rl_setMusicVolume(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isRaylib() || args[0].obj->typeTag != ObjectHeader::TAG_MUSIC)
        return Value::Int(0);
    auto vm = static_cast<VMMusic*>(args[0].obj);
    Music mus;
    std::memcpy(&mus, vm->rlData, sizeof(Music));
    SetMusicVolume(mus, (float)args[1].asFloat());
    return Value::Int(0);
}

// ═══════════════════════════════════════════════════════════════════
//  Random
// ═══════════════════════════════════════════════════════════════════

Value rl_getRandomValue(std::vector<Value>& args) {
    int min = 0, max = 100;
    if (args.size() >= 1) min = (int)args[0].asInt();
    if (args.size() >= 2) max = (int)args[1].asInt();
    return Value::Int(GetRandomValue(min, max));
}

// ═══════════════════════════════════════════════════════════════════
//  Timing
// ═══════════════════════════════════════════════════════════════════

Value rl_getTime(std::vector<Value>&) {
    return Value::Float(GetTime());
}

// ═══════════════════════════════════════════════════════════════════
//  VM type implementations
// ═══════════════════════════════════════════════════════════════════

VMTexture2D* VMTexture2D::create(unsigned int id, int w, int h, int mipmaps, int fmt) {
    auto t = new VMTexture2D();
    t->id = id; t->width = w; t->height = h; t->mipmaps = mipmaps; t->format = fmt;
    return t;
}

VMTexture2D::~VMTexture2D() {
    if (id != 0) {
        UnloadTexture({id, width, height, mipmaps, format});
        id = 0;
    }
}

VMImage* VMImage::create(void* data, int w, int h, int mipmaps, int fmt) {
    auto img = new VMImage();
    img->data = data; img->width = w; img->height = h; img->mipmaps = mipmaps; img->format = fmt;
    return img;
}

VMImage::~VMImage() {
    if (data) {
        UnloadImage({data, width, height, mipmaps, format});
        data = nullptr;
    }
}

VMSound* VMSound::create() { return new VMSound(); }
VMSound::~VMSound() {
    Sound snd;
    std::memcpy(&snd, rlData, sizeof(Sound));
    if (snd.frameCount > 0) {
        UnloadSound(snd);
    }
    std::memset(rlData, 0, sizeof(Sound));
}

VMMusic* VMMusic::create() { return new VMMusic(); }
VMMusic::~VMMusic() {
    Music mus;
    std::memcpy(&mus, rlData, sizeof(Music));
    if (mus.ctxData) {
        UnloadMusicStream(mus);
    }
    std::memset(rlData, 0, sizeof(Music));
}

VMFont* VMFont::create() { return new VMFont(); }
VMFont::~VMFont() {
    if (glyphs) {
        Font f; f.baseSize = (int)baseSize; f.glyphCount = (int)glyphCount; f.glyphPadding = (int)glyphPadding;
        f.texture = {}; f.recs = nullptr; f.glyphs = nullptr;
        UnloadFont(f);
    }
}

// ═══════════════════════════════════════════════════════════════════
//  Registration
// ═══════════════════════════════════════════════════════════════════

void StdLib::registerRaylib(VM* vm) {
    // Window
    registerFunction(vm, "initWindow",       rl_initWindow);
    registerFunction(vm, "windowShouldClose", rl_windowShouldClose);
    registerFunction(vm, "closeWindow",      rl_closeWindow);
    registerFunction(vm, "keyPressed",       rl_keyPressed);
    registerAlias(vm,   "键盘按下",          "keyPressed");
    registerFunction(vm, "setTargetFPS",     rl_setTargetFPS);
    registerFunction(vm, "getFPS",           rl_getFPS);
    registerFunction(vm, "getFrameTime",     rl_getFrameTime);
    registerFunction(vm, "setWindowTitle",   rl_setWindowTitle);
    registerFunction(vm, "isWindowResized",  rl_isWindowResized);
    registerFunction(vm, "getScreenWidth",   rl_getScreenWidth);
    registerFunction(vm, "getScreenHeight",  rl_getScreenHeight);
    
    // Drawing
    registerFunction(vm, "beginDrawing",     rl_beginDrawing);
    registerFunction(vm, "endDrawing",       rl_endDrawing);
    registerFunction(vm, "clearBackground",  rl_clearBackground);
    registerFunction(vm, "drawFPS",          rl_drawFPS);
    registerFunction(vm, "drawText",         rl_drawText);
    registerFunction(vm, "drawRectangle",    rl_drawRectangle);
    registerFunction(vm, "drawRectangleRec", rl_drawRectangleRec);
    registerFunction(vm, "drawCircle",       rl_drawCircle);
    registerFunction(vm, "drawLine",         rl_drawLine);
    registerFunction(vm, "drawLineEx",       rl_drawLineEx);
    registerFunction(vm, "drawPixel",        rl_drawPixel);
    
    // Colors
    registerFunction(vm, "color",            rl_color);
    registerFunction(vm, "LIGHTGRAY",        rl_color_lightgray);
    registerFunction(vm, "GRAY",             rl_color_gray);
    registerFunction(vm, "DARKGRAY",         rl_color_darkgray);
    registerFunction(vm, "YELLOW",           rl_color_yellow);
    registerFunction(vm, "GOLD",             rl_color_gold);
    registerFunction(vm, "ORANGE",           rl_color_orange);
    registerFunction(vm, "PINK",             rl_color_pink);
    registerFunction(vm, "RED",              rl_color_red);
    registerFunction(vm, "MAROON",           rl_color_maroon);
    registerFunction(vm, "GREEN",            rl_color_green);
    registerFunction(vm, "LIME",             rl_color_lime);
    registerFunction(vm, "DARKGREEN",        rl_color_darkgreen);
    registerFunction(vm, "SKYBLUE",          rl_color_skyblue);
    registerFunction(vm, "BLUE",             rl_color_blue);
    registerFunction(vm, "DARKBLUE",         rl_color_darkblue);
    registerFunction(vm, "PURPLE",           rl_color_purple);
    registerFunction(vm, "VIOLET",           rl_color_violet);
    registerFunction(vm, "DARKPURPLE",       rl_color_darkpurple);
    registerFunction(vm, "BEIGE",            rl_color_beige);
    registerFunction(vm, "BROWN",            rl_color_brown);
    registerFunction(vm, "DARKBROWN",        rl_color_darkbrown);
    registerFunction(vm, "WHITE",            rl_color_white);
    registerFunction(vm, "BLACK",            rl_color_black);
    registerFunction(vm, "BLANK",            rl_color_blank);
    registerFunction(vm, "MAGENTA",          rl_color_magenta);
    registerFunction(vm, "RAYWHITE",         rl_color_raywhite);
    // Register all colors as global values (for use as bare identifiers)
    { std::vector<Value> _ea;
    vm->registerGlobal("BLACK", rl_color_black(_ea)); vm->registerGlobal("WHITE", rl_color_white(_ea));
    vm->registerGlobal("BLUE", rl_color_blue(_ea)); vm->registerGlobal("RED", rl_color_red(_ea));
    vm->registerGlobal("GREEN", rl_color_green(_ea)); vm->registerGlobal("YELLOW", rl_color_yellow(_ea));
    vm->registerGlobal("GRAY", rl_color_gray(_ea)); vm->registerGlobal("DARKGRAY", rl_color_darkgray(_ea));
    vm->registerGlobal("LIGHTGRAY", rl_color_lightgray(_ea)); vm->registerGlobal("PURPLE", rl_color_purple(_ea));
    vm->registerGlobal("ORANGE", rl_color_orange(_ea)); vm->registerGlobal("PINK", rl_color_pink(_ea));
    vm->registerGlobal("BROWN", rl_color_brown(_ea)); vm->registerGlobal("MAGENTA", rl_color_magenta(_ea));
    vm->registerGlobal("RAYWHITE", rl_color_raywhite(_ea)); vm->registerGlobal("LIME", rl_color_lime(_ea));
    vm->registerGlobal("DARKGREEN", rl_color_darkgreen(_ea)); vm->registerGlobal("SKYBLUE", rl_color_skyblue(_ea));
    vm->registerGlobal("DARKBLUE", rl_color_darkblue(_ea)); vm->registerGlobal("VIOLET", rl_color_violet(_ea));
    vm->registerGlobal("DARKPURPLE", rl_color_darkpurple(_ea)); vm->registerGlobal("BEIGE", rl_color_beige(_ea));
    vm->registerGlobal("MAROON", rl_color_maroon(_ea)); vm->registerGlobal("GOLD", rl_color_gold(_ea));
    vm->registerGlobal("DARKBROWN", rl_color_darkbrown(_ea)); vm->registerGlobal("BLANK", rl_color_blank(_ea)); }
    
    // Input
    registerFunction(vm, "isKeyDown",        rl_isKeyDown);
    registerFunction(vm, "isKeyPressed",     rl_isKeyPressed);
    registerFunction(vm, "isKeyReleased",    rl_isKeyReleased);
    registerFunction(vm, "getMousePosition", rl_getMousePosition);
    registerFunction(vm, "getMouseX",         rl_getMouseX);
    registerFunction(vm, "getMouseY",         rl_getMouseY);
    registerFunction(vm, "isMouseButtonDown",    rl_isMouseButtonDown);
    registerFunction(vm, "isMouseButtonPressed", rl_isMouseButtonPressed);
    registerFunction(vm, "isMouseButtonReleased", rl_isMouseButtonReleased);
    registerFunction(vm, "getMouseWheelMove", rl_getMouseWheelMove);
    
    // Math / constructors
    registerFunction(vm, "vector2",          rl_vector2);
    registerFunction(vm, "rectangle",        rl_rectangle);
    
    // Collision
    registerFunction(vm, "checkCollisionPointRec",  rl_checkCollisionPointRec);
    registerFunction(vm, "checkCollisionRecs",      rl_checkCollisionRecs);
    registerFunction(vm, "checkCollisionCircles",   rl_checkCollisionCircles);
    
    // Textures
    registerFunction(vm, "loadTexture",      rl_loadTexture);
    registerFunction(vm, "unloadTexture",    rl_unloadTexture);
    registerFunction(vm, "drawTexture",      rl_drawTexture);
    registerFunction(vm, "drawTextureEx",    rl_drawTextureEx);
    
    // Images
    registerFunction(vm, "loadImage",        rl_loadImage);
    registerFunction(vm, "unloadImage",      rl_unloadImage);
    registerFunction(vm, "imageToTexture",   rl_imageToTexture);
    
    // Audio
    registerFunction(vm, "initAudioDevice",   rl_initAudioDevice);
    registerFunction(vm, "closeAudioDevice",  rl_closeAudioDevice);
    registerFunction(vm, "loadSound",        rl_loadSound);
    registerFunction(vm, "unloadSound",      rl_unloadSound);
    registerFunction(vm, "playSound",        rl_playSound);
    registerFunction(vm, "stopSound",        rl_stopSound);
    registerFunction(vm, "setSoundVolume",   rl_setSoundVolume);
    registerFunction(vm, "loadMusicStream",  rl_loadMusicStream);
    registerFunction(vm, "unloadMusicStream", rl_unloadMusicStream);
    registerFunction(vm, "playMusicStream",  rl_playMusicStream);
    registerFunction(vm, "updateMusicStream", rl_updateMusicStream);
    registerFunction(vm, "stopMusicStream",  rl_stopMusicStream);
    registerFunction(vm, "setMusicVolume",   rl_setMusicVolume);
    
    // Random
    registerFunction(vm, "getRandomValue",   rl_getRandomValue);
    
    // Time
    registerFunction(vm, "getTime",          rl_getTime);

    // ── Chinese aliases: Window ──
    registerAlias(vm, "初始化窗口",           "initWindow");
    registerAlias(vm, "窗口应关闭",           "windowShouldClose");
    registerAlias(vm, "关闭窗口",             "closeWindow");
    registerAlias(vm, "设置目标帧率",         "setTargetFPS");
    registerAlias(vm, "获取帧率",             "getFPS");
    registerAlias(vm, "获取帧时间",           "getFrameTime");
    registerAlias(vm, "设置窗口标题",         "setWindowTitle");
    registerAlias(vm, "窗口已调整大小",       "isWindowResized");
    registerAlias(vm, "屏幕宽度",             "getScreenWidth");
    registerAlias(vm, "屏幕高度",             "getScreenHeight");

    // ── Chinese aliases: Drawing ──
    registerAlias(vm, "开始绘图",             "beginDrawing");
    registerAlias(vm, "开始绘制",             "beginDrawing");
    registerAlias(vm, "结束绘图",             "endDrawing");
    registerAlias(vm, "结束绘制",             "endDrawing");
    registerAlias(vm, "清空背景",             "clearBackground");
    registerAlias(vm, "清除背景",             "clearBackground");
    registerAlias(vm, "绘制帧率",             "drawFPS");
    registerAlias(vm, "绘制文本",             "drawText");
    registerAlias(vm, "绘制矩形",             "drawRectangle");
    registerAlias(vm, "绘制矩形区域",         "drawRectangleRec");
    registerAlias(vm, "绘制圆形",             "drawCircle");
    registerAlias(vm, "绘制三角形",           "drawTriangle");
    registerAlias(vm, "绘制线条",             "drawLine");
    registerAlias(vm, "绘制粗线条",           "drawLineEx");
    registerAlias(vm, "绘制像素",             "drawPixel");

    // ── Chinese aliases: Color ──
    registerAlias(vm, "颜色",                 "color");
    registerAlias(vm, "浅灰",                 "LIGHTGRAY");
    registerAlias(vm, "灰",                   "GRAY");
    registerAlias(vm, "深灰",                 "DARKGRAY");
    registerAlias(vm, "黄",                   "YELLOW");
    registerAlias(vm, "金",                   "GOLD");
    registerAlias(vm, "橙",                   "ORANGE");
    registerAlias(vm, "粉",                   "PINK");
    registerAlias(vm, "红",                   "RED");
    registerAlias(vm, "栗红",                 "MAROON");
    registerAlias(vm, "绿",                   "GREEN");
    registerAlias(vm, "亮绿",                 "LIME");
    registerAlias(vm, "深绿",                 "DARKGREEN");
    registerAlias(vm, "天蓝",                 "SKYBLUE");
    registerAlias(vm, "蓝",                   "BLUE");
    registerAlias(vm, "深蓝",                 "DARKBLUE");
    registerAlias(vm, "紫",                   "PURPLE");
    registerAlias(vm, "蓝紫",                 "VIOLET");
    registerAlias(vm, "深紫",                 "DARKPURPLE");
    registerAlias(vm, "米色",                 "BEIGE");
    registerAlias(vm, "棕",                   "BROWN");
    registerAlias(vm, "深棕",                 "DARKBROWN");
    registerAlias(vm, "白",                   "WHITE");
    registerAlias(vm, "黑",                   "BLACK");
    registerAlias(vm, "透明",                 "BLANK");
    registerAlias(vm, "品红",                 "MAGENTA");
    registerAlias(vm, "乳白",                 "RAYWHITE");
    // Register Chinese color names as global values (for use as bare identifiers)
    { std::vector<Value> _ea2;
    vm->registerGlobal("浅灰", rl_color_lightgray(_ea2)); vm->registerGlobal("灰", rl_color_gray(_ea2));
    vm->registerGlobal("深灰", rl_color_darkgray(_ea2)); vm->registerGlobal("黄", rl_color_yellow(_ea2));
    vm->registerGlobal("金", rl_color_gold(_ea2)); vm->registerGlobal("橙", rl_color_orange(_ea2));
    vm->registerGlobal("粉", rl_color_pink(_ea2)); vm->registerGlobal("红", rl_color_red(_ea2));
    vm->registerGlobal("栗红", rl_color_maroon(_ea2)); vm->registerGlobal("绿", rl_color_green(_ea2));
    vm->registerGlobal("亮绿", rl_color_lime(_ea2)); vm->registerGlobal("深绿", rl_color_darkgreen(_ea2));
    vm->registerGlobal("天蓝", rl_color_skyblue(_ea2)); vm->registerGlobal("蓝", rl_color_blue(_ea2));
    vm->registerGlobal("深蓝", rl_color_darkblue(_ea2)); vm->registerGlobal("紫", rl_color_purple(_ea2));
    vm->registerGlobal("蓝紫", rl_color_violet(_ea2)); vm->registerGlobal("深紫", rl_color_darkpurple(_ea2));
    vm->registerGlobal("米色", rl_color_beige(_ea2)); vm->registerGlobal("棕", rl_color_brown(_ea2));
    vm->registerGlobal("深棕", rl_color_darkbrown(_ea2)); vm->registerGlobal("白", rl_color_white(_ea2));
    vm->registerGlobal("黑", rl_color_black(_ea2)); vm->registerGlobal("透明", rl_color_blank(_ea2));
    vm->registerGlobal("品红", rl_color_magenta(_ea2)); vm->registerGlobal("乳白", rl_color_raywhite(_ea2)); }
    // "色" suffix aliases (e.g. 白色, 红色, 蓝色)
    registerAlias(vm, "白色", "WHITE"); registerAlias(vm, "黑色", "BLACK");
    registerAlias(vm, "红色", "RED"); registerAlias(vm, "绿色", "GREEN");
    registerAlias(vm, "蓝色", "BLUE"); registerAlias(vm, "黄色", "YELLOW");
    registerAlias(vm, "灰色", "GRAY"); registerAlias(vm, "橙色", "ORANGE");
    registerAlias(vm, "紫色", "PURPLE"); registerAlias(vm, "粉色", "PINK");
    registerAlias(vm, "棕色", "BROWN"); registerAlias(vm, "金色", "GOLD");
    registerAlias(vm, "青色", "SKYBLUE"); registerAlias(vm, "浅灰色", "LIGHTGRAY");
    registerAlias(vm, "深灰色", "DARKGRAY"); registerAlias(vm, "深蓝色", "DARKBLUE");
    registerAlias(vm, "深绿色", "DARKGREEN"); registerAlias(vm, "深紫色", "DARKPURPLE");
    registerAlias(vm, "亮绿色", "LIME");
    
    // ── Chinese aliases: Input ──
    registerAlias(vm, "键盘按下",             "isKeyDown");
    registerAlias(vm, "键盘刚按下",           "isKeyPressed");
    registerAlias(vm, "键盘刚释放",           "isKeyReleased");
    registerAlias(vm, "鼠标准位置",           "getMousePosition");
    registerAlias(vm, "鼠标X",               "getMouseX");
    registerAlias(vm, "鼠标Y",               "getMouseY");
    registerAlias(vm, "鼠标按下",             "isMouseButtonDown");
    registerAlias(vm, "鼠标刚按下",           "isMouseButtonPressed");
    registerAlias(vm, "鼠标刚释放",           "isMouseButtonReleased");
    registerAlias(vm, "鼠标滚轮",             "getMouseWheelMove");

    // ── Chinese aliases: Math ──
    registerAlias(vm, "向量2",                "vector2");
    registerAlias(vm, "矩形",                 "rectangle");

    // ── Chinese aliases: Collision ──
    registerAlias(vm, "碰撞检测点矩形",       "checkCollisionPointRec");
    registerAlias(vm, "碰撞检测矩形",         "checkCollisionRecs");
    registerAlias(vm, "碰撞检测圆形",         "checkCollisionCircles");

    // ── Chinese aliases: Texture & Image ──
    registerAlias(vm, "加载纹理",             "loadTexture");
    registerAlias(vm, "卸载纹理",             "unloadTexture");
    registerAlias(vm, "绘制纹理",             "drawTexture");
    registerAlias(vm, "绘制纹理Ex",           "drawTextureEx");
    registerAlias(vm, "加载图像",             "loadImage");
    registerAlias(vm, "卸载图像",             "unloadImage");
    registerAlias(vm, "图像转纹理",           "imageToTexture");

    // ── Chinese aliases: Audio ──
    registerAlias(vm, "初始化音频",           "initAudioDevice");
    registerAlias(vm, "关闭音频",             "closeAudioDevice");
    registerAlias(vm, "加载音效",             "loadSound");
    registerAlias(vm, "卸载音效",             "unloadSound");
    registerAlias(vm, "播放音效",             "playSound");
    registerAlias(vm, "停止音效",             "stopSound");
    registerAlias(vm, "设置音效音量",         "setSoundVolume");
    registerAlias(vm, "加载音乐",             "loadMusicStream");
    registerAlias(vm, "卸载音乐",             "unloadMusicStream");
    registerAlias(vm, "播放音乐",             "playMusicStream");
    registerAlias(vm, "更新音乐流",           "updateMusicStream");
    registerAlias(vm, "停止音乐",             "stopMusicStream");
    registerAlias(vm, "设置音乐音量",         "setMusicVolume");

    // ── Chinese aliases: Misc ──
    registerAlias(vm, "随机值",               "getRandomValue");
    registerAlias(vm, "获取时间",             "getTime");

    // ── Key code constants ──
    auto K = [&](const char* name, Int64 code) {
        vm->registerGlobal(name, Value::Int(code));
    };
    K("键_左",      263);  K("键_右",      262);
    K("键_上",      265);  K("键_下",      264);
    K("键_空格",    32);   K("键_退出",    256);
    K("键_回车",    257);  K("键_TAB",     258);
    K("键_退格",    259);  K("键_DELETE",  261);
    K("键_左SHIFT", 340);  K("键_左CTRL",  341);
    K("键_A",       65);   K("键_B",       66);
    K("键_C",       67);   K("键_D",       68);
    K("键_E",       69);   K("键_F",       70);
    K("键_G",       71);   K("键_H",       72);
    K("键_I",       73);   K("键_J",       74);
    K("键_K",       75);   K("键_L",       76);
    K("键_M",       77);   K("键_N",       78);
    K("键_O",       79);   K("键_P",       80);
    K("键_Q",       81);   K("键_R",       82);
    K("键_S",       83);   K("键_T",       84);
    K("键_U",       85);   K("键_V",       86);
    K("键_W",       87);   K("键_X",       88);
    K("键_Y",       89);   K("键_Z",       90);
    K("键_0",       48);   K("键_1",       49);
    K("键_2",       50);   K("键_3",       51);
    K("键_4",       52);   K("键_5",       53);
    K("键_6",       54);   K("键_7",       55);
    K("键_8",       56);   K("键_9",       57);
    K("键_F1",      290);  K("键_F2",      291);
    K("键_F3",      292);  K("键_F4",      293);
    K("键_F5",      294);  K("键_F6",      295);
    K("键_F7",      296);  K("键_F8",      297);
    K("键_F9",      298);  K("键_F10",     299);
    K("键_F11",     300);  K("键_F12",     301);

    // ── Mouse button constants ──
    K("鼠标_左键",   0);   K("鼠标_右键",   1);
    K("鼠标_中键",   2);   K("鼠标_侧键1",  3);
    K("鼠标_侧键2",  4);
}

} // namespace cplang
