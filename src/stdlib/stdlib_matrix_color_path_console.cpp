#include "stdlib/stdlib.hpp"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace cplang {

// Matrix, Color, Path, Console functions
// #include'd from stdlib.cpp, already inside namespace cplang

void StdLib::registerMatrix(VM* vm) {
    registerFunction(vm, "matrixAdd", matrix::matrixAdd);
    registerFunction(vm, "matrixMul", matrix::matrixMul);
    registerFunction(vm, "matrixTranspose", matrix::matrixTranspose);
    registerFunction(vm, "matrixIdentity", matrix::matrixIdentity);
    registerFunction(vm, "matrixDet", matrix::matrixDet);
    registerFunction(vm, "vectorDot", matrix::vectorDot);
    registerFunction(vm, "vectorCross", matrix::vectorCross);
    registerFunction(vm, "vectorNorm", matrix::vectorNorm);
    registerFunction(vm, "vectorNormalize", matrix::vectorNormalize);

    registerAlias(vm, "矩阵加", "matrixAdd");
    registerAlias(vm, "矩阵乘", "matrixMul");
    registerAlias(vm, "矩阵转置", "matrixTranspose");
    registerAlias(vm, "单位矩阵", "matrixIdentity");
    registerAlias(vm, "矩阵行列式", "matrixDet");
    registerAlias(vm, "向量点积", "vectorDot");
    registerAlias(vm, "向量叉积", "vectorCross");
    registerAlias(vm, "向量模", "vectorNorm");
    registerAlias(vm, "向量归一化", "vectorNormalize");
}

void StdLib::registerColor(VM* vm) {
    registerFunction(vm, "hexToRgb", color::hexToRgb);
    registerFunction(vm, "rgbToHex", color::rgbToHex);
    registerFunction(vm, "rgbToHsl", color::rgbToHsl);
    registerFunction(vm, "hslToRgb", color::hslToRgb);

    registerAlias(vm, "十六进制转RGB", "hexToRgb");
    registerAlias(vm, "RGB转十六进制", "rgbToHex");
    registerAlias(vm, "RGB转HSL", "rgbToHsl");
    registerAlias(vm, "HSL转RGB", "hslToRgb");
}

void StdLib::registerPath(VM* vm) {
    registerFunction(vm, "pathBasename", path::basename);
    registerFunction(vm, "pathDirname", path::dirname);
    registerFunction(vm, "pathExtname", path::extname);
    registerFunction(vm, "pathJoin", path::join);

    registerAlias(vm, "路径基名", "pathBasename");
    registerAlias(vm, "路径目录", "pathDirname");
    registerAlias(vm, "路径扩展名", "pathExtname");
    registerAlias(vm, "路径连接", "pathJoin");
}

void StdLib::registerConsole(VM* vm) {
    registerFunction(vm, "conColor", console::color);
    registerFunction(vm, "conReset", console::reset);
    registerFunction(vm, "conInputHidden", console::inputHidden);

    registerAlias(vm, "控制台颜色", "conColor");
    registerAlias(vm, "控制台重置", "conReset");
    registerAlias(vm, "隐藏输入", "conInputHidden");
    
    registerFunction(vm, "conClear", console::clear);
    registerFunction(vm, "conSize", console::size);
    registerFunction(vm, "conCursor", console::cursor);
    
    registerAlias(vm, "控制台清屏", "conClear");
    registerAlias(vm, "控制台大小", "conSize");
    registerAlias(vm, "控制台光标", "conCursor");
}

namespace matrix {

static double getNum(const Value& v) {
    if (v.isInt()) return static_cast<double>(v.asInt());
    if (v.isFloat()) return v.asFloat();
    return 0.0;
}

static bool isMatrix(VMArray* arr) {
    if (arr->data.empty()) return false;
    for (auto& row : arr->data) {
        if (!row.isArray()) return false;
    }
    return true;
}

static size_t matrixRows(VMArray* arr) { return arr->data.size(); }
static size_t matrixCols(VMArray* arr) { return arr->data.empty() ? 0 : arr->data[0].asArray()->data.size(); }

static double getMatrixElem(VMArray* arr, size_t r, size_t c) {
    if (r >= arr->data.size()) return 0;
    auto* row = arr->data[r].asArray();
    if (c >= row->data.size()) return 0;
    return getNum(row->data[c]);
}

static void setMatrixElem(VMArray* arr, size_t r, size_t c, double val) {
    if (r >= arr->data.size()) return;
    auto* row = arr->data[r].asArray();
    if (c >= row->data.size()) return;
    row->data[c] = Value::Float(val);
}

Value matrixAdd(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray() || !args[1].isArray()) return Value::nil();
    VMArray* a = args[0].asArray();
    VMArray* b = args[1].asArray();
    if (!isMatrix(a) || !isMatrix(b)) return Value::nil();
    size_t rows = matrixRows(a), cols = matrixCols(a);
    if (rows != matrixRows(b) || cols != matrixCols(b)) return Value::nil();
    VMArray* result = VMArray::create(0);
    for (size_t r = 0; r < rows; r++) {
        VMArray* row = VMArray::create(0);
        for (size_t c = 0; c < cols; c++) {
            row->data.push_back(Value::Float(getMatrixElem(a, r, c) + getMatrixElem(b, r, c)));
        }
        result->data.push_back(Value::Array(row));
    }
    return Value::Array(result);
}

Value matrixMul(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray() || !args[1].isArray()) return Value::nil();
    VMArray* a = args[0].asArray();
    VMArray* b = args[1].asArray();
    if (!isMatrix(a) || !isMatrix(b)) return Value::nil();
    size_t rows = matrixRows(a), inner = matrixCols(a);
    if (inner != matrixRows(b)) return Value::nil();
    size_t cols = matrixCols(b);
    VMArray* result = VMArray::create(0);
    for (size_t r = 0; r < rows; r++) {
        VMArray* row = VMArray::create(0);
        for (size_t c = 0; c < cols; c++) {
            double sum = 0;
            for (size_t k = 0; k < inner; k++) {
                sum += getMatrixElem(a, r, k) * getMatrixElem(b, k, c);
            }
            row->data.push_back(Value::Float(sum));
        }
        result->data.push_back(Value::Array(row));
    }
    return Value::Array(result);
}

Value matrixTranspose(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::nil();
    VMArray* a = args[0].asArray();
    if (!isMatrix(a)) return Value::nil();
    size_t rows = matrixRows(a), cols = matrixCols(a);
    VMArray* result = VMArray::create(0);
    for (size_t c = 0; c < cols; c++) {
        VMArray* row = VMArray::create(0);
        for (size_t r = 0; r < rows; r++) {
            row->data.push_back(Value::Float(getMatrixElem(a, r, c)));
        }
        result->data.push_back(Value::Array(row));
    }
    return Value::Array(result);
}

Value matrixIdentity(std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::nil();
    size_t n = static_cast<size_t>(getNum(args[0]));
    VMArray* result = VMArray::create(0);
    for (size_t r = 0; r < n; r++) {
        VMArray* row = VMArray::create(0);
        for (size_t c = 0; c < n; c++) {
            row->data.push_back(Value::Float(r == c ? 1.0 : 0.0));
        }
        result->data.push_back(Value::Array(row));
    }
    return Value::Array(result);
}

static double detRecursive(VMArray* a, size_t n) {
    if (n == 1) return getMatrixElem(a, 0, 0);
    if (n == 2) return getMatrixElem(a, 0, 0) * getMatrixElem(a, 1, 1) - getMatrixElem(a, 0, 1) * getMatrixElem(a, 1, 0);
    double det = 0;
    for (size_t c = 0; c < n; c++) {
        VMArray* minor = VMArray::create(0);
        for (size_t r = 1; r < n; r++) {
            VMArray* row = VMArray::create(0);
            for (size_t cc = 0; cc < n; cc++) {
                if (cc == c) continue;
                row->data.push_back(Value::Float(getMatrixElem(a, r, cc)));
            }
            minor->data.push_back(Value::Array(row));
        }
        double sign = (c % 2 == 0) ? 1.0 : -1.0;
        det += sign * getMatrixElem(a, 0, c) * detRecursive(minor, n - 1);
    }
    return det;
}

Value matrixDet(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::Float(0);
    VMArray* a = args[0].asArray();
    if (!isMatrix(a)) return Value::Float(0);
    size_t n = matrixRows(a);
    if (n != matrixCols(a)) return Value::Float(0);
    return Value::Float(detRecursive(a, n));
}

Value vectorDot(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray() || !args[1].isArray()) return Value::Float(0);
    VMArray* a = args[0].asArray();
    VMArray* b = args[1].asArray();
    if (a->data.size() != b->data.size()) return Value::Float(0);
    double sum = 0;
    for (size_t i = 0; i < a->data.size(); i++) {
        sum += getNum(a->data[i]) * getNum(b->data[i]);
    }
    return Value::Float(sum);
}

Value vectorCross(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray() || !args[1].isArray()) return Value::nil();
    VMArray* a = args[0].asArray();
    VMArray* b = args[1].asArray();
    if (a->data.size() != 3 || b->data.size() != 3) return Value::nil();
    double ax = getNum(a->data[0]), ay = getNum(a->data[1]), az = getNum(a->data[2]);
    double bx = getNum(b->data[0]), by = getNum(b->data[1]), bz = getNum(b->data[2]);
    VMArray* result = VMArray::create(0);
    result->data.push_back(Value::Float(ay * bz - az * by));
    result->data.push_back(Value::Float(az * bx - ax * bz));
    result->data.push_back(Value::Float(ax * by - ay * bx));
    return Value::Array(result);
}

Value vectorNorm(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::Float(0);
    VMArray* a = args[0].asArray();
    double sum = 0;
    for (size_t i = 0; i < a->data.size(); i++) {
        double v = getNum(a->data[i]);
        sum += v * v;
    }
    return Value::Float(std::sqrt(sum));
}

Value vectorNormalize(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::nil();
    VMArray* a = args[0].asArray();
    double sum = 0;
    for (size_t i = 0; i < a->data.size(); i++) {
        double v = getNum(a->data[i]);
        sum += v * v;
    }
    double norm = std::sqrt(sum);
    if (norm == 0) return Value::nil();
    VMArray* result = VMArray::create(0);
    for (size_t i = 0; i < a->data.size(); i++) {
        result->data.push_back(Value::Float(getNum(a->data[i]) / norm));
    }
    return Value::Array(result);
}
} // namespace matrix

// ═══════════════════════════════════════════════════════════════════
//  颜色转换函数实现
// ═══════════════════════════════════════════════════════════════════

namespace color {

static int hexDigit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

Value hexToRgb(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string hex(args[0].asString()->data, args[0].asString()->length);
    size_t i = (hex.size() > 0 && hex[0] == '#') ? 1 : 0;
    if (i + 6 > hex.size()) return Value::nil();
    int r = (hexDigit(hex[i]) << 4) | hexDigit(hex[i+1]);
    int g = (hexDigit(hex[i+2]) << 4) | hexDigit(hex[i+3]);
    int b = (hexDigit(hex[i+4]) << 4) | hexDigit(hex[i+5]);
    VMArray* arr = VMArray::create();
    arr->data.push_back(Value::Int(r));
    arr->data.push_back(Value::Int(g));
    arr->data.push_back(Value::Int(b));
    return Value::Array(arr);
}

Value rgbToHex(std::vector<Value>& args) {
    if (args.size() < 3) return Value::nil();
    auto clamp = [](Int64 x) { return x < 0 ? 0 : (x > 255 ? 255 : (int)x); };
    int r = clamp(args[0].isInt() ? args[0].asInt() : 0);
    int g = clamp(args[1].isInt() ? args[1].asInt() : 0);
    int b = clamp(args[2].isInt() ? args[2].asInt() : 0);
    char buf[8];
    snprintf(buf, sizeof(buf), "#%02x%02x%02x", r, g, b);
    return Value::String(VMString::create(std::string(buf)));
}

Value rgbToHsl(std::vector<Value>& args) {
    if (args.size() < 3) return Value::nil();
    double r = args[0].isInt() ? args[0].asInt() / 255.0 : (args[0].isFloat() ? args[0].asFloat() : 0);
    double g = args[1].isInt() ? args[1].asInt() / 255.0 : (args[1].isFloat() ? args[1].asFloat() : 0);
    double b = args[2].isInt() ? args[2].asInt() / 255.0 : (args[2].isFloat() ? args[2].asFloat() : 0);
    double mx = std::max({r, g, b}), mn = std::min({r, g, b});
    double h = 0, s = 0, l = (mx + mn) / 2.0;
    if (mx != mn) {
        double d = mx - mn;
        s = l > 0.5 ? d / (2.0 - mx - mn) : d / (mx + mn);
        if (mx == r) h = (g - b) / d + (g < b ? 6 : 0);
        else if (mx == g) h = (b - r) / d + 2;
        else h = (r - g) / d + 4;
        h /= 6.0;
    }
    VMArray* arr = VMArray::create();
    arr->data.push_back(Value::Float(h * 360));
    arr->data.push_back(Value::Float(s * 100));
    arr->data.push_back(Value::Float(l * 100));
    return Value::Array(arr);
}

Value hslToRgb(std::vector<Value>& args) {
    if (args.size() < 3) return Value::nil();
    double h = args[0].isFloat() ? args[0].asFloat() / 360.0 : (args[0].isInt() ? args[0].asInt() / 360.0 : 0);
    double s = args[1].isFloat() ? args[1].asFloat() / 100.0 : (args[1].isInt() ? args[1].asInt() / 100.0 : 0);
    double l = args[2].isFloat() ? args[2].asFloat() / 100.0 : (args[2].isInt() ? args[2].asInt() / 100.0 : 0);
    auto hue2rgb = [](double p, double q, double t) -> double {
        if (t < 0) t += 1;
        if (t > 1) t -= 1;
        if (t < 1.0/6) return p + (q - p) * 6 * t;
        if (t < 1.0/2) return q;
        if (t < 2.0/3) return p + (q - p) * (2.0/3 - t) * 6;
        return p;
    };
    if (s == 0) {
        int v = (int)(l * 255);
        VMArray* arr = VMArray::create();
        arr->data.push_back(Value::Int(v)); arr->data.push_back(Value::Int(v)); arr->data.push_back(Value::Int(v));
        return Value::Array(arr);
    }
    double q = l < 0.5 ? l * (1 + s) : l + s - l * s;
    double p = 2 * l - q;
    VMArray* arr = VMArray::create();
    arr->data.push_back(Value::Int((Int64)(hue2rgb(p, q, h + 1.0/3) * 255)));
    arr->data.push_back(Value::Int((Int64)(hue2rgb(p, q, h) * 255)));
    arr->data.push_back(Value::Int((Int64)(hue2rgb(p, q, h - 1.0/3) * 255)));
    return Value::Array(arr);
}

} // namespace color

// ═══════════════════════════════════════════════════════════════════
//  路径操作实现
// ═══════════════════════════════════════════════════════════════════

namespace path {

static std::string getStr(const Value& v) {
    if (!v.isString()) return "";
    return std::string(v.asString()->data, v.asString()->length);
}

Value basename(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string p = getStr(args[0]);
    // 去末尾斜杠
    while (!p.empty() && (p.back() == '/' || p.back() == '\\')) p.pop_back();
    size_t pos = p.find_last_of("/\\");
    if (pos != std::string::npos) return Value::String(VMString::create(p.substr(pos + 1)));
    return Value::String(VMString::create(p));
}

Value dirname(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string p = getStr(args[0]);
    while (!p.empty() && (p.back() == '/' || p.back() == '\\')) p.pop_back();
    size_t pos = p.find_last_of("/\\");
    if (pos != std::string::npos) return Value::String(VMString::create(p.substr(0, pos)));
    return Value::String(VMString::create("."));
}

Value extname(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string p = getStr(args[0]);
    size_t slash = p.find_last_of("/\\");
    size_t dot = p.find_last_of('.');
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) return Value::String(VMString::create(""));
    return Value::String(VMString::create(p.substr(dot)));
}

Value join(std::vector<Value>& args) {
    if (args.empty()) return Value::String(VMString::create(""));
    std::string out;
    for (size_t i = 0; i < args.size(); i++) {
        if (!args[i].isString()) continue;
        std::string part = getStr(args[i]);
        if (part.empty()) continue;
        if (!out.empty()) {
            char last = out.back();
            if (last != '/' && last != '\\' && part[0] != '/' && part[0] != '\\') out += '\\';
        }
        out += part;
    }
    return Value::String(VMString::create(out));
}

} // namespace path

// ═══════════════════════════════════════════════════════════════════
//  控制台实现（Windows Console API）
// ═══════════════════════════════════════════════════════════════════

namespace console {

Value color(std::vector<Value>& args) {
    if (args.size() < 2) return Value::nil();
    int fg = args[0].isInt() ? (int)args[0].asInt() : 7;
    int bg = args[1].isInt() ? (int)args[1].asInt() : 0;
    if (fg < 0) fg = 0; if (fg > 15) fg = 15;
    if (bg < 0) bg = 0; if (bg > 15) bg = 15;
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h != INVALID_HANDLE_VALUE) SetConsoleTextAttribute(h, (WORD)((bg << 4) | fg));
    return Value::nil();
}

Value reset(std::vector<Value>& args) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h != INVALID_HANDLE_VALUE) SetConsoleTextAttribute(h, 7);
    return Value::nil();
}

Value inputHidden(std::vector<Value>& args) {
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    DWORD oldMode = 0;
    if (h != INVALID_HANDLE_VALUE) GetConsoleMode(h, &oldMode);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD oldOutMode = 0;
    if (hOut != INVALID_HANDLE_VALUE) {
        GetConsoleMode(hOut, &oldOutMode);
        SetConsoleMode(hOut, oldOutMode & ~ENABLE_ECHO_INPUT);
    }
    if (h != INVALID_HANDLE_VALUE) SetConsoleMode(h, oldMode & ~ENABLE_ECHO_INPUT);
    std::string input;
    std::getline(std::cin, input);
    if (h != INVALID_HANDLE_VALUE) SetConsoleMode(h, oldMode);
    if (hOut != INVALID_HANDLE_VALUE) SetConsoleMode(hOut, oldOutMode);
    return Value::String(VMString::create(input));
}

Value clear(std::vector<Value>& args) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h == INVALID_HANDLE_VALUE) return Value::nil();
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD topLeft = {0, 0};
    DWORD written;
    if (GetConsoleScreenBufferInfo(h, &csbi)) {
        DWORD size = csbi.dwSize.X * csbi.dwSize.Y;
        FillConsoleOutputCharacterW(h, L' ', size, topLeft, &written);
        FillConsoleOutputAttribute(h, csbi.wAttributes, size, topLeft, &written);
    }
    SetConsoleCursorPosition(h, topLeft);
    return Value::nil();
}

Value size(std::vector<Value>& args) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h == INVALID_HANDLE_VALUE) return Value::nil();
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(h, &csbi)) {
        auto arr = VMArray::create(2);
        arr->data.push_back(Value::Int(csbi.srWindow.Right - csbi.srWindow.Left + 1));
        arr->data.push_back(Value::Int(csbi.srWindow.Bottom - csbi.srWindow.Top + 1));
        return Value::Array(arr);
    }
    return Value::nil();
}

Value cursor(std::vector<Value>& args) {
    if (args.size() < 2) return Value::nil();
    int x = (int)(args[0].isNumber() ? args[0].asFloat() : 0);
    int y = (int)(args[1].isNumber() ? args[1].asFloat() : 0);
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h != INVALID_HANDLE_VALUE) {
        COORD pos = {(SHORT)x, (SHORT)y};
        SetConsoleCursorPosition(h, pos);
    }
    return Value::nil();
}

} // namespace console

// ═══════════════════════════════════════════════════════════════════
//  Optional 可选值实现（VMTable 内部存储）
// ═══════════════════════════════════════════════════════════════════

} // namespace cplang
