// ═══════════════════════════════════════════════════════════════════
//  Dear ImGui CP 语言绑定（基于 rlImGui + raylib 后端）
// ═══════════════════════════════════════════════════════════════════

// NO_FONT_AWESOME 已在命令行定义
#include "stdlib/stdlib.hpp"
#include "imgui.h"
#include "rlImGui.h"
#include "rlgl.h"

using namespace cplang;

namespace imgui_ns {

// ─── 上下文管理 ──────────────────────────────────────────────────

Value init_(std::vector<Value>& args) {
    bool dark = args.empty() || !args[0].isBool() || args[0].isTrue();
    { ImGuiIO& io = ImGui::GetIO(); io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/simhei.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesChineseSimplifiedCommon()); io.Fonts->Build(); }
    rlImGuiSetup(dark);
    // 加载中文字体(加载后字体,覆盖默认字体)
    ImGuiIO& gio = ImGui::GetIO();
    gio.Fonts->Clear();
    gio.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/simhei.ttf", 18.0f, NULL, gio.Fonts->GetGlyphRangesChineseSimplifiedCommon());
    gio.Fonts->Build();
    // 加载中文字体
    ImGuiIO& io = ImGui::GetIO();
    const char* fontPaths[] = {
        "C:/Windows/Fonts/msyh.ttf",
        "C:/Windows/Fonts/simhei.ttf",
        "C:/Windows/Fonts/simsun.ttc",
        NULL
    };
    for (int fi = 0; fontPaths[fi] != NULL; fi++) {
        ImFont* font = io.Fonts->AddFontFromFileTTF(fontPaths[fi], 18.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
        if (font) break;
    }
    return Value::Int(0);
}

Value beginDraw_(std::vector<Value>&) {
    rlImGuiBegin();
    return Value::Int(0);
}

Value endDraw_(std::vector<Value>&) {
    rlImGuiEnd();
    return Value::Int(0);
}

Value shutdown_(std::vector<Value>&) {
    rlImGuiShutdown();
    return Value::Int(0);
}

// ─── 窗口 ────────────────────────────────────────────────────────
Value begin_(std::vector<Value>& args) {
    const char* name = args.empty() || !args[0].isString() ? "Window" : args[0].asString()->data;
    bool* pOpen = nullptr;
    int flags = (args.size() >= 3 && args[2].isInt()) ? (int)args[2].asInt() : 0;
    return Value::Bool(ImGui::Begin(name, pOpen, flags));
}

Value end_(std::vector<Value>&) {
    ImGui::End();
    return Value::Int(0);
}

Value beginChild_(std::vector<Value>& args) {
    const char* name = args.empty() || !args[0].isString() ? "Child" : args[0].asString()->data;
    float w = (args.size() >= 2 && args[1].isFloat()) ? (float)args[1].asFloat() : 0;
    float h = (args.size() >= 3 && args[2].isFloat()) ? (float)args[2].asFloat() : 0;
    int flags = (args.size() >= 4 && args[3].isInt()) ? (int)args[3].asInt() : 0;
    return Value::Bool(ImGui::BeginChild(name, ImVec2(w, h), 0, flags));
}

Value endChild_(std::vector<Value>&) {
    ImGui::EndChild();
    return Value::Int(0);
}

// ─── 布局 ────────────────────────────────────────────────────────
Value sameLine_(std::vector<Value>& args) {
    float offset = (args.size() >= 1 && args[0].isFloat()) ? (float)args[0].asFloat() : 0;
    float spacing = (args.size() >= 2 && args[1].isFloat()) ? (float)args[1].asFloat() : -1;
    ImGui::SameLine(offset, spacing);
    return Value::Int(0);
}

Value separator_(std::vector<Value>&) { ImGui::Separator(); return Value::Int(0); }
Value spacing_(std::vector<Value>&) { ImGui::Spacing(); return Value::Int(0); }
Value newlineFn_(std::vector<Value>&) { ImGui::NewLine(); return Value::Int(0); }

Value dummy_(std::vector<Value>& args) {
    float w = args.empty() || !args[0].isFloat() ? 0 : (float)args[0].asFloat();
    float h = args.size() < 2 || !args[1].isFloat() ? 0 : (float)args[1].asFloat();
    ImGui::Dummy(ImVec2(w, h));
    return Value::Int(0);
}

// ─── 基础控件 ────────────────────────────────────────────────────
Value text_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Int(0);
    ImGui::TextUnformatted(args[0].asString()->data, args[0].asString()->data + args[0].asString()->length);
    return Value::Int(0);
}

Value textWrapped_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Int(0);
    ImGui::TextWrapped("%s", args[0].asString()->data);
    return Value::Int(0);
}

Value textColored_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[1].isString()) return Value::Int(0);
    if (!args[0].isTable()) { ImGui::TextUnformatted(args[1].asString()->data, args[1].asString()->data + args[1].asString()->length); return Value::Int(0); }
    VMTable* c = args[0].asTable();
    float r = c->has(makeStringVal(VMString::create("r"))) ? (float)c->get(makeStringVal(VMString::create("r"))).asFloat() : 1.0f;
    float g = c->has(makeStringVal(VMString::create("g"))) ? (float)c->get(makeStringVal(VMString::create("g"))).asFloat() : 1.0f;
    float b = c->has(makeStringVal(VMString::create("b"))) ? (float)c->get(makeStringVal(VMString::create("b"))).asFloat() : 1.0f;
    float a = c->has(makeStringVal(VMString::create("a"))) ? (float)c->get(makeStringVal(VMString::create("a"))).asFloat() : 1.0f;
    ImGui::TextColored(ImVec4(r, g, b, a), "%s", args[1].asString()->data);
    return Value::Int(0);
}

Value bulletText_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Int(0);
    ImGui::BulletText("%s", args[0].asString()->data);
    return Value::Int(0);
}

// ─── 按钮 ────────────────────────────────────────────────────────
Value button_(std::vector<Value>& args) {
    const char* label = args.empty() || !args[0].isString() ? "Button" : args[0].asString()->data;
    float w = (args.size() >= 2 && args[1].isFloat()) ? (float)args[1].asFloat() : 0;
    float h = (args.size() >= 3 && args[2].isFloat()) ? (float)args[2].asFloat() : 0;
    return Value::Bool(ImGui::Button(label, ImVec2(w, h)));
}

Value smallButton_(std::vector<Value>& args) {
    const char* label = args.empty() || !args[0].isString() ? "##" : args[0].asString()->data;
    return Value::Bool(ImGui::SmallButton(label));
}

Value arrowButton_(std::vector<Value>& args) {
    const char* id = args.empty() || !args[0].isString() ? "##" : args[0].asString()->data;
    int dir = (args.size() >= 2 && args[1].isInt()) ? (int)args[1].asInt() : 0;
    return Value::Bool(ImGui::ArrowButton(id, (ImGuiDir)dir));
}

// ─── 复选框/单选框 ────────────────────────────────────────────────
Value checkbox_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::nil();
    const char* label = args[0].isString() ? args[0].asString()->data : "##";
    bool v = args[1].isTrue();
    bool changed = ImGui::Checkbox(label, &v);
    VMTable* tbl = VMTable::create();
    tbl->set(makeStringVal(VMString::create("v")), Value::Bool(v));
    tbl->set(makeStringVal(VMString::create("changed")), Value::Bool(changed));
    return makeTableVal(tbl);
}

Value radioButton_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::nil();
    const char* label = args[0].isString() ? args[0].asString()->data : "##";
    if (args[1].isInt()) {
        int v = (int)args[1].asInt();
        int btn = (args.size() >= 3 && args[2].isInt()) ? (int)args[2].asInt() : 0;
        bool changed = ImGui::RadioButton(label, &v, btn);
        VMTable* tbl = VMTable::create();
        tbl->set(makeStringVal(VMString::create("v")), Value::Int(v));
        tbl->set(makeStringVal(VMString::create("changed")), Value::Bool(changed));
        return makeTableVal(tbl);
    } else {
        bool v = args[1].isTrue();
        bool changed = ImGui::RadioButton(label, v);
        VMTable* tbl = VMTable::create();
        tbl->set(makeStringVal(VMString::create("v")), Value::Bool(v));
        tbl->set(makeStringVal(VMString::create("changed")), Value::Bool(changed));
        return makeTableVal(tbl);
    }
}

// ─── 滑块 ────────────────────────────────────────────────────────
Value sliderFloat_(std::vector<Value>& args) {
    if (args.size() < 3) return Value::nil();
    const char* label = args[0].isString() ? args[0].asString()->data : "##";
    float v = args[1].isFloat() ? (float)args[1].asFloat() : 0;
    float vmin = args[2].isFloat() ? (float)args[2].asFloat() : 0;
    float vmax = (args.size() >= 4 && args[3].isFloat()) ? (float)args[3].asFloat() : 1;
    bool changed = ImGui::SliderFloat(label, &v, vmin, vmax);
    VMTable* tbl = VMTable::create();
    tbl->set(makeStringVal(VMString::create("v")), Value::Float(v));
    tbl->set(makeStringVal(VMString::create("changed")), Value::Bool(changed));
    return makeTableVal(tbl);
}

Value sliderInt_(std::vector<Value>& args) {
    if (args.size() < 3) return Value::nil();
    const char* label = args[0].isString() ? args[0].asString()->data : "##";
    int v = args[1].isInt() ? (int)args[1].asInt() : 0;
    int vmin = args[2].isInt() ? (int)args[2].asInt() : 0;
    int vmax = (args.size() >= 4 && args[3].isInt()) ? (int)args[3].asInt() : 100;
    bool changed = ImGui::SliderInt(label, &v, vmin, vmax);
    VMTable* tbl = VMTable::create();
    tbl->set(makeStringVal(VMString::create("v")), Value::Int(v));
    tbl->set(makeStringVal(VMString::create("changed")), Value::Bool(changed));
    return makeTableVal(tbl);
}

// ─── 输入框 ──────────────────────────────────────────────────────
Value inputText_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::nil();
    const char* label = args[0].isString() ? args[0].asString()->data : "##";
    std::string text(args[1].isString() ? std::string(args[1].asString()->data, args[1].asString()->length) : "");
    text.resize(256, '\0');
    int flags = (args.size() >= 3 && args[2].isInt()) ? (int)args[2].asInt() : 0;
    bool changed = ImGui::InputText(label, &text[0], text.size(), flags);
    // trim null
    text.resize(strlen(text.c_str()));
    VMTable* tbl = VMTable::create();
    tbl->set(makeStringVal(VMString::create("v")), makeStringVal(VMString::create(text)));
    tbl->set(makeStringVal(VMString::create("changed")), Value::Bool(changed));
    return makeTableVal(tbl);
}

Value inputInt_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::nil();
    const char* label = args[0].isString() ? args[0].asString()->data : "##";
    int v = args[1].isInt() ? (int)args[1].asInt() : 0;
    int step = (args.size() >= 3 && args[2].isInt()) ? (int)args[2].asInt() : 1;
    int fast = (args.size() >= 4 && args[3].isInt()) ? (int)args[3].asInt() : 100;
    bool changed = ImGui::InputInt(label, &v, step, fast);
    VMTable* tbl = VMTable::create();
    tbl->set(makeStringVal(VMString::create("v")), Value::Int(v));
    tbl->set(makeStringVal(VMString::create("changed")), Value::Bool(changed));
    return makeTableVal(tbl);
}

Value inputFloat_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::nil();
    const char* label = args[0].isString() ? args[0].asString()->data : "##";
    float v = args[1].isFloat() ? (float)args[1].asFloat() : 0;
    float step = (args.size() >= 3 && args[2].isFloat()) ? (float)args[2].asFloat() : 0.1f;
    float fast = (args.size() >= 4 && args[3].isFloat()) ? (float)args[3].asFloat() : 1.0f;
    bool changed = ImGui::InputFloat(label, &v, step, fast);
    VMTable* tbl = VMTable::create();
    tbl->set(makeStringVal(VMString::create("v")), Value::Float(v));
    tbl->set(makeStringVal(VMString::create("changed")), Value::Bool(changed));
    return makeTableVal(tbl);
}

// ─── 多行文本编辑器 ────────────────────────────────────────────
Value inputTextMultiline_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::nil();
    const char* label = args[0].isString() ? args[0].asString()->data : "##";
    std::string text(args[1].isString() ? std::string(args[1].asString()->data, args[1].asString()->length) : "");
    text.resize(65536, '\0');
    float w = (args.size() >= 3 && args[2].isFloat()) ? (float)args[2].asFloat() : -1;
    float h = (args.size() >= 4 && args[3].isFloat()) ? (float)args[3].asFloat() : -1;
    int flags = (args.size() >= 5 && args[4].isInt()) ? (int)args[4].asInt() : 0;
    bool changed = ImGui::InputTextMultiline(label, &text[0], text.size(), ImVec2(w, h), flags);
    text.resize(strlen(text.c_str()));
    VMTable* tbl = VMTable::create();
    tbl->set(makeStringVal(VMString::create("v")), makeStringVal(VMString::create(text)));
    tbl->set(makeStringVal(VMString::create("changed")), Value::Bool(changed));
    return makeTableVal(tbl);
}

// ─── 颜色编辑 ────────────────────────────────────────────────────
Value colorEdit3_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::nil();
    const char* label = args[0].isString() ? args[0].asString()->data : "##";
    float col[3] = {0, 0, 0};
    if (args[1].isTable()) {
        VMTable* t = args[1].asTable();
        if (t->has(makeStringVal(VMString::create("r")))) col[0] = (float)t->get(makeStringVal(VMString::create("r"))).asFloat();
        if (t->has(makeStringVal(VMString::create("g")))) col[1] = (float)t->get(makeStringVal(VMString::create("g"))).asFloat();
        if (t->has(makeStringVal(VMString::create("b")))) col[2] = (float)t->get(makeStringVal(VMString::create("b"))).asFloat();
    }
    int flags = (args.size() >= 3 && args[2].isInt()) ? (int)args[2].asInt() : 0;
    bool changed = ImGui::ColorEdit3(label, col, flags);
    VMTable* tbl = VMTable::create();
    VMTable* c = VMTable::create();
    c->set(makeStringVal(VMString::create("r")), Value::Float(col[0]));
    c->set(makeStringVal(VMString::create("g")), Value::Float(col[1]));
    c->set(makeStringVal(VMString::create("b")), Value::Float(col[2]));
    tbl->set(makeStringVal(VMString::create("v")), makeTableVal(c));
    tbl->set(makeStringVal(VMString::create("changed")), Value::Bool(changed));
    return makeTableVal(tbl);
}

Value colorEdit4_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::nil();
    const char* label = args[0].isString() ? args[0].asString()->data : "##";
    float col[4] = {0, 0, 0, 1};
    if (args[1].isTable()) {
        VMTable* t = args[1].asTable();
        if (t->has(makeStringVal(VMString::create("r")))) col[0] = (float)t->get(makeStringVal(VMString::create("r"))).asFloat();
        if (t->has(makeStringVal(VMString::create("g")))) col[1] = (float)t->get(makeStringVal(VMString::create("g"))).asFloat();
        if (t->has(makeStringVal(VMString::create("b")))) col[2] = (float)t->get(makeStringVal(VMString::create("b"))).asFloat();
        if (t->has(makeStringVal(VMString::create("a")))) col[3] = (float)t->get(makeStringVal(VMString::create("a"))).asFloat();
    }
    int flags = (args.size() >= 3 && args[2].isInt()) ? (int)args[2].asInt() : 0;
    bool changed = ImGui::ColorEdit4(label, col, flags);
    VMTable* tbl = VMTable::create();
    VMTable* c = VMTable::create();
    c->set(makeStringVal(VMString::create("r")), Value::Float(col[0]));
    c->set(makeStringVal(VMString::create("g")), Value::Float(col[1]));
    c->set(makeStringVal(VMString::create("b")), Value::Float(col[2]));
    c->set(makeStringVal(VMString::create("a")), Value::Float(col[3]));
    tbl->set(makeStringVal(VMString::create("v")), makeTableVal(c));
    tbl->set(makeStringVal(VMString::create("changed")), Value::Bool(changed));
    return makeTableVal(tbl);
}

// ─── 下拉框 / 列表 ───────────────────────────────────────────────
Value combo_(std::vector<Value>& args) {
    if (args.size() < 3) return Value::nil();
    const char* label = args[0].isString() ? args[0].asString()->data : "##";
    int current = args[1].isInt() ? (int)args[1].asInt() : 0;
    if (!args[2].isArray()) return Value::nil();
    VMArray* items = args[2].asArray();
    std::string preview = current >= 0 && current < (int)items->data.size() && items->data[current].isString()
        ? std::string(items->data[current].asString()->data, items->data[current].asString()->length) : "";
    bool changed = false;
    if (ImGui::BeginCombo(label, preview.c_str())) {
        for (int i = 0; i < (int)items->data.size(); i++) {
            if (!items->data[i].isString()) continue;
            bool isSel = (i == current);
            if (ImGui::Selectable(std::string(items->data[i].asString()->data, items->data[i].asString()->length).c_str(), isSel)) {
                current = i;
                changed = true;
            }
            if (isSel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    VMTable* tbl = VMTable::create();
    tbl->set(makeStringVal(VMString::create("v")), Value::Int(current));
    tbl->set(makeStringVal(VMString::create("changed")), Value::Bool(changed));
    return makeTableVal(tbl);
}

// ─── 树节点 / 可折叠标题 ─────────────────────────────────────────
Value treeNode_(std::vector<Value>& args) {
    const char* label = args.empty() || !args[0].isString() ? "Node" : args[0].asString()->data;
    int flags = (args.size() >= 2 && args[1].isInt()) ? (int)args[1].asInt() : 0;
    return Value::Bool(ImGui::TreeNodeEx(label, flags));
}

Value treePop_(std::vector<Value>&) { ImGui::TreePop(); return Value::Int(0); }

Value collapsingHeader_(std::vector<Value>& args) {
    const char* label = args.empty() || !args[0].isString() ? "Header" : args[0].asString()->data;
    int flags = (args.size() >= 2 && args[1].isInt()) ? (int)args[1].asInt() : 0;
    return Value::Bool(ImGui::CollapsingHeader(label, flags));
}

// ─── 表格 ────────────────────────────────────────────────────────
Value beginTable_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Bool(false);
    const char* id = args[0].isString() ? args[0].asString()->data : "Table";
    int cols = args[1].isInt() ? (int)args[1].asInt() : 1;
    int flags = (args.size() >= 3 && args[2].isInt()) ? (int)args[2].asInt() : 0;
    return Value::Bool(ImGui::BeginTable(id, cols, flags));
}

Value endTable_(std::vector<Value>&) { ImGui::EndTable(); return Value::Int(0); }
Value tableNextRow_(std::vector<Value>&) { ImGui::TableNextRow(); return Value::Int(0); }

Value tableSetColumnIndex_(std::vector<Value>& args) {
    int col = args.empty() || !args[0].isInt() ? 0 : (int)args[0].asInt();
    return Value::Bool(ImGui::TableSetColumnIndex(col));
}

Value tableSetupColumn_(std::vector<Value>& args) {
    const char* label = args.empty() || !args[0].isString() ? "Col" : args[0].asString()->data;
    int flags = (args.size() >= 2 && args[1].isInt()) ? (int)args[1].asInt() : 0;
    float width = (args.size() >= 3 && args[2].isFloat()) ? (float)args[2].asFloat() : 0;
    ImGui::TableSetupColumn(label, flags, width);
    return Value::Int(0);
}

Value tableHeadersRow_(std::vector<Value>&) { ImGui::TableHeadersRow(); return Value::Int(0); }

// ─── 菜单 ────────────────────────────────────────────────────────
Value beginMenuBar_(std::vector<Value>&) { return Value::Bool(ImGui::BeginMenuBar()); }
Value endMenuBar_(std::vector<Value>&) { ImGui::EndMenuBar(); return Value::Int(0); }

Value beginMenu_(std::vector<Value>& args) {
    const char* label = args.empty() || !args[0].isString() ? "Menu" : args[0].asString()->data;
    bool enabled = !(args.size() >= 2 && args[1].isBool() && !args[1].isTrue());
    return Value::Bool(ImGui::BeginMenu(label, enabled));
}
Value endMenu_(std::vector<Value>&) { ImGui::EndMenu(); return Value::Int(0); }

Value menuItem_(std::vector<Value>& args) {
    const char* label = args.empty() || !args[0].isString() ? "Item" : args[0].asString()->data;
    const char* shortcut = (args.size() >= 2 && args[1].isString()) ? args[1].asString()->data : nullptr;
    bool enabled = !(args.size() >= 3 && args[2].isBool() && !args[2].isTrue());
    return Value::Bool(ImGui::MenuItem(label, shortcut, false, enabled));
}

// ─── 弹出窗口 ────────────────────────────────────────────────────
Value openPopup_(std::vector<Value>& args) {
    const char* id = args.empty() || !args[0].isString() ? "Popup" : args[0].asString()->data;
    ImGui::OpenPopup(id);
    return Value::Int(0);
}

Value beginPopup_(std::vector<Value>& args) {
    const char* name = args.empty() || !args[0].isString() ? "Popup" : args[0].asString()->data;
    int flags = (args.size() >= 2 && args[1].isInt()) ? (int)args[1].asInt() : 0;
    return Value::Bool(ImGui::BeginPopup(name, flags));
}
Value endPopup_(std::vector<Value>&) { ImGui::EndPopup(); return Value::Int(0); }
Value closeCurrentPopup_(std::vector<Value>&) { ImGui::CloseCurrentPopup(); return Value::Int(0); }

Value beginPopupContextItem_(std::vector<Value>& args) {
    const char* id = (args.size() >= 1 && args[0].isString()) ? args[0].asString()->data : nullptr;
    return Value::Bool(ImGui::BeginPopupContextItem(id));
}

// ─── 工具提示 ────────────────────────────────────────────────────
Value beginTooltip_(std::vector<Value>&) { ImGui::BeginTooltip(); return Value::Int(0); }
Value endTooltip_(std::vector<Value>&) { ImGui::EndTooltip(); return Value::Int(0); }

Value setTooltip_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Int(0);
    ImGui::SetTooltip("%s", args[0].asString()->data);
    return Value::Int(0);
}

// ─── 值显示 ──────────────────────────────────────────────────────
Value valueBool_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Int(0);
    const char* label = args[0].isString() ? args[0].asString()->data : "";
    ImGui::Value(label, args[1].isTrue());
    return Value::Int(0);
}
Value valueInt_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Int(0);
    const char* label = args[0].isString() ? args[0].asString()->data : "";
    int v = args[1].isInt() ? (int)args[1].asInt() : 0;
    ImGui::Value(label, v);
    return Value::Int(0);
}
Value valueFloat_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Int(0);
    const char* label = args[0].isString() ? args[0].asString()->data : "";
    ImGui::Value(label, (float)(args[1].isFloat() ? args[1].asFloat() : 0));
    return Value::Int(0);
}

// ─── 进度条 ──────────────────────────────────────────────────────
Value progressBar_(std::vector<Value>& args) {
    float fraction = args.empty() || !args[0].isFloat() ? 0 : (float)args[0].asFloat();
    float w = (args.size() >= 2 && args[1].isFloat()) ? (float)args[1].asFloat() : -1;
    float h = (args.size() >= 3 && args[2].isFloat()) ? (float)args[2].asFloat() : 0;
    const char* overlay = (args.size() >= 4 && args[3].isString()) ? args[3].asString()->data : nullptr;
    ImGui::ProgressBar(fraction, ImVec2(w, h), overlay);
    return Value::Int(0);
}

// ─── 交互查询 ────────────────────────────────────────────────────
Value isItemHovered_(std::vector<Value>&) { return Value::Bool(ImGui::IsItemHovered()); }
Value isItemClicked_(std::vector<Value>&) { return Value::Bool(ImGui::IsItemClicked()); }
Value isItemActive_(std::vector<Value>&) { return Value::Bool(ImGui::IsItemActive()); }

Value getMousePos_(std::vector<Value>&) {
    ImVec2 pos = ImGui::GetMousePos();
    VMTable* tbl = VMTable::create();
    tbl->set(makeStringVal(VMString::create("x")), Value::Float(pos.x));
    tbl->set(makeStringVal(VMString::create("y")), Value::Float(pos.y));
    return makeTableVal(tbl);
}

Value isMouseDown_(std::vector<Value>& args) {
    int btn = args.empty() || !args[0].isInt() ? 0 : (int)args[0].asInt();
    return Value::Bool(ImGui::IsMouseDown(btn));
}

// ─── 演示窗口 ────────────────────────────────────────────────────
Value showDemoWindow_(std::vector<Value>&) { /* ImGui::ShowDemoWindow requires imgui_demo.cpp */ return Value::Int(0); }

// ─── 样式 ────────────────────────────────────────────────────────
Value pushStyleColor_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Int(0);
    int idx = args[0].isInt() ? (int)args[0].asInt() : 0;
    if (!args[1].isTable()) return Value::Int(0);
    VMTable* c = args[1].asTable();
    float r = c->has(makeStringVal(VMString::create("r"))) ? (float)c->get(makeStringVal(VMString::create("r"))).asFloat() : 1.0f;
    float g = c->has(makeStringVal(VMString::create("g"))) ? (float)c->get(makeStringVal(VMString::create("g"))).asFloat() : 1.0f;
    float b = c->has(makeStringVal(VMString::create("b"))) ? (float)c->get(makeStringVal(VMString::create("b"))).asFloat() : 1.0f;
    float a = c->has(makeStringVal(VMString::create("a"))) ? (float)c->get(makeStringVal(VMString::create("a"))).asFloat() : 1.0f;
    ImGui::PushStyleColor(idx, ImVec4(r, g, b, a));
    return Value::Int(0);
}
Value popStyleColor_(std::vector<Value>& args) {
    int count = args.empty() || !args[0].isInt() ? 1 : (int)args[0].asInt();
    ImGui::PopStyleColor(count);
    return Value::Int(0);
}

// ─── 光标 / 窗口定位 ─────────────────────────────────────────────
Value setCursorPos_(std::vector<Value>& args) {
    float x = (args.size() >= 1 && args[0].isFloat()) ? (float)args[0].asFloat() : 0;
    float y = (args.size() >= 2 && args[1].isFloat()) ? (float)args[1].asFloat() : 0;
    ImGui::SetCursorPos(ImVec2(x, y));
    return Value::Int(0);
}


// ─── 窗口布局 ────────────────────────────────────────────────────
Value setNextWindowPos_(std::vector<Value>& args) {
    float x = args.size() > 0 && args[0].isNumber() ? (float)args[0].asFloat() : 0;
    float y = args.size() > 1 && args[1].isNumber() ? (float)args[1].asFloat() : 0;
    int cond = args.size() > 2 && args[2].isInt() ? (int)args[2].asInt() : 0;
    ImGui::SetNextWindowPos(ImVec2(x, y), cond);
    return Value::Int(0);
}
Value setNextWindowSize_(std::vector<Value>& args) {
    float w = args.size() > 0 && args[0].isNumber() ? (float)args[0].asFloat() : 0;
    float h = args.size() > 1 && args[1].isNumber() ? (float)args[1].asFloat() : 0;
    int cond = args.size() > 2 && args[2].isInt() ? (int)args[2].asInt() : 0;
    ImGui::SetNextWindowSize(ImVec2(w, h), cond);
    return Value::Int(0);
}

} // namespace imgui_ns

// ═══════════════════════════════════════════════════════════════════
//  注册
// ═══════════════════════════════════════════════════════════════════
void StdLib::registerImGui(VM* vm) {
    using namespace imgui_ns;
    // 上下文
    registerFunction(vm, "igInit",         init_);
    registerFunction(vm, "igBeginDraw",    beginDraw_);
    registerFunction(vm, "igEndDraw",      endDraw_);
    registerFunction(vm, "igShutdown",     shutdown_);
    // 窗口
    registerFunction(vm, "igBegin",        begin_);
    registerFunction(vm, "igEnd",          end_);
    registerFunction(vm, "igBeginChild",   beginChild_);
    registerFunction(vm, "igEndChild",     endChild_);
    // 布局
    registerFunction(vm, "igSameLine",     sameLine_);
    registerFunction(vm, "igSeparator",    separator_);
    registerFunction(vm, "igSpacing",      spacing_);
    registerFunction(vm, "igNewLine",      newlineFn_);
    registerFunction(vm, "igDummy",        dummy_);
    // 文本
    registerFunction(vm, "igText",         text_);
    registerFunction(vm, "igTextWrapped",  textWrapped_);
    registerFunction(vm, "igTextColored",  textColored_);
    registerFunction(vm, "igBulletText",   bulletText_);
    // 按钮
    registerFunction(vm, "igButton",       button_);
    registerFunction(vm, "igSmallButton",  smallButton_);
    registerFunction(vm, "igArrowButton",  arrowButton_);
    // 复选框/单选框
    registerFunction(vm, "igCheckbox",     checkbox_);
    registerFunction(vm, "igRadioButton",  radioButton_);
    // 滑块
    registerFunction(vm, "igSliderFloat",  sliderFloat_);
    registerFunction(vm, "igSliderInt",    sliderInt_);
    // 输入
    registerFunction(vm, "igInputText",    inputText_);
    registerFunction(vm, "igInputInt",     inputInt_);
    registerFunction(vm, "igInputFloat",   inputFloat_);
    // 多行编辑器
    registerFunction(vm, "igInputTextMultiline", inputTextMultiline_);
    // 颜色
    registerFunction(vm, "igColorEdit3",   colorEdit3_);
    registerFunction(vm, "igColorEdit4",   colorEdit4_);
    // 下拉
    registerFunction(vm, "igCombo",        combo_);
    // 树
    registerFunction(vm, "igTreeNode",     treeNode_);
    registerFunction(vm, "igTreePop",      treePop_);
    registerFunction(vm, "igCollapsingHeader", collapsingHeader_);
    // 表格
    registerFunction(vm, "igBeginTable",   beginTable_);
    registerFunction(vm, "igEndTable",     endTable_);
    registerFunction(vm, "igTableNextRow", tableNextRow_);
    registerFunction(vm, "igTableSetColumnIndex", tableSetColumnIndex_);
    registerFunction(vm, "igTableSetupColumn", tableSetupColumn_);
    registerFunction(vm, "igTableHeadersRow", tableHeadersRow_);
    // 菜单
    registerFunction(vm, "igBeginMenuBar", beginMenuBar_);
    registerFunction(vm, "igEndMenuBar",   endMenuBar_);
    registerFunction(vm, "igBeginMenu",    beginMenu_);
    registerFunction(vm, "igEndMenu",      endMenu_);
    registerFunction(vm, "igMenuItem",     menuItem_);
    // 弹出
    registerFunction(vm, "igOpenPopup",    openPopup_);
    registerFunction(vm, "igBeginPopup",   beginPopup_);
    registerFunction(vm, "igEndPopup",     endPopup_);
    registerFunction(vm, "igCloseCurrentPopup", closeCurrentPopup_);
    registerFunction(vm, "igBeginPopupContextItem", beginPopupContextItem_);
    // 提示
    registerFunction(vm, "igBeginTooltip", beginTooltip_);
    registerFunction(vm, "igEndTooltip",   endTooltip_);
    registerFunction(vm, "igSetTooltip",   setTooltip_);
    // 值显示
    registerFunction(vm, "igValueBool",    valueBool_);
    registerFunction(vm, "igValueInt",     valueInt_);
    registerFunction(vm, "igValueFloat",   valueFloat_);
    // 进度条
    registerFunction(vm, "igProgressBar",  progressBar_);
    // 交互查询
    registerFunction(vm, "igIsItemHovered", isItemHovered_);
    registerFunction(vm, "igIsItemClicked", isItemClicked_);
    registerFunction(vm, "igIsItemActive", isItemActive_);
    registerFunction(vm, "igGetMousePos",  getMousePos_);
    registerFunction(vm, "igIsMouseDown",  isMouseDown_);
    // 演示
    registerFunction(vm, "igShowDemoWindow", showDemoWindow_);
    // 样式
    registerFunction(vm, "igPushStyleColor", pushStyleColor_);
    registerFunction(vm, "igPopStyleColor", popStyleColor_);
    // 窗口布局
    registerFunction(vm, "igSetNextWindowPos", setNextWindowPos_);
    registerFunction(vm, "igSetNextWindowSize", setNextWindowSize_);
    // 光标
    registerFunction(vm, "igSetCursorPos", setCursorPos_);

    // 中文别名
    registerAlias(vm, "界面开始绘制", "igBeginDraw");
    registerAlias(vm, "界面结束绘制", "igEndDraw");
    registerAlias(vm, "界面初始化",   "igInit");
    registerAlias(vm, "窗口开始",     "igBegin");
    registerAlias(vm, "窗口结束",     "igEnd");
    registerAlias(vm, "同行",         "igSameLine");
    registerAlias(vm, "分隔线",       "igSeparator");
    registerAlias(vm, "占位",         "igSpacing");
    registerAlias(vm, "换行",         "igNewLine");
    registerAlias(vm, "文本",         "igText");
    registerAlias(vm, "自动换行文本", "igTextWrapped");
    registerAlias(vm, "彩色文本",     "igTextColored");
    registerAlias(vm, "要点文本",     "igBulletText");
    registerAlias(vm, "按钮",         "igButton");
    registerAlias(vm, "小按钮",       "igSmallButton");
    registerAlias(vm, "箭头按钮",     "igArrowButton");
    registerAlias(vm, "复选框",       "igCheckbox");
    registerAlias(vm, "单选框",       "igRadioButton");
    registerAlias(vm, "浮点滑块",     "igSliderFloat");
    registerAlias(vm, "整数滑块",     "igSliderInt");
    registerAlias(vm, "输入文本",     "igInputText");
    registerAlias(vm, "输入整数",     "igInputInt");
    registerAlias(vm, "输入浮点",     "igInputFloat");
    registerAlias(vm, "多行输入文本", "igInputTextMultiline");
    registerAlias(vm, "颜色编辑3",    "igColorEdit3");
    registerAlias(vm, "颜色编辑4",    "igColorEdit4");
    registerAlias(vm, "下拉框",       "igCombo");
    registerAlias(vm, "树节点",       "igTreeNode");
    registerAlias(vm, "树弹出",       "igTreePop");
    registerAlias(vm, "可折叠标题",   "igCollapsingHeader");
    registerAlias(vm, "表格开始",     "igBeginTable");
    registerAlias(vm, "表格结束",     "igEndTable");
    registerAlias(vm, "表格下一行",   "igTableNextRow");
    registerAlias(vm, "表格设列",     "igTableSetColumnIndex");
    registerAlias(vm, "表格配列",     "igTableSetupColumn");
    registerAlias(vm, "表格标题行",   "igTableHeadersRow");
    registerAlias(vm, "菜单栏开始",   "igBeginMenuBar");
    registerAlias(vm, "菜单栏结束",   "igEndMenuBar");
    registerAlias(vm, "菜单开始",     "igBeginMenu");
    registerAlias(vm, "菜单结束",     "igEndMenu");
    registerAlias(vm, "菜单项",       "igMenuItem");
    registerAlias(vm, "弹出打开",     "igOpenPopup");
    registerAlias(vm, "弹出开始",     "igBeginPopup");
    registerAlias(vm, "弹出结束",     "igEndPopup");
    registerAlias(vm, "弹出关闭",     "igCloseCurrentPopup");
    registerAlias(vm, "右键菜单",     "igBeginPopupContextItem");
    registerAlias(vm, "提示开始",     "igBeginTooltip");
    registerAlias(vm, "提示结束",     "igEndTooltip");
    registerAlias(vm, "设提示",       "igSetTooltip");
    registerAlias(vm, "显示布尔",     "igValueBool");
    registerAlias(vm, "显示整数",     "igValueInt");
    registerAlias(vm, "显示浮点",     "igValueFloat");
    registerAlias(vm, "进度条",       "igProgressBar");
    registerAlias(vm, "项目悬停",     "igIsItemHovered");
    registerAlias(vm, "项目点击",     "igIsItemClicked");
    registerAlias(vm, "项目活跃",     "igIsItemActive");
    registerAlias(vm, "鼠标位置",     "igGetMousePos");
    registerAlias(vm, "鼠标按下",     "igIsMouseDown");
    registerAlias(vm, "演示窗口",     "igShowDemoWindow");
    registerAlias(vm, "推样式颜色",   "igPushStyleColor");
    registerAlias(vm, "弹样式颜色",   "igPopStyleColor");
    registerAlias(vm, "设光标位置",   "igSetCursorPos");
}

