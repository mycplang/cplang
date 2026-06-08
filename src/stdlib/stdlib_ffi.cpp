#include "stdlib/stdlib.hpp"

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  FFI (外部函数接口) — 加载和调用外部 DLL
//  用法（CP 侧）:
//    lib = 加载库("user32.dll");
//    fn = 取函数(lib, "MessageBoxA");
//    结果 = 调用整数(fn, 0, "你好", "标题", 0);
//    // MessageBox(HWND, LPCSTR, LPCSTR, UINT) = MessageBoxA(0, "你好", "标题", 0)
//
//  混合参数签名（2026-05-31 新增）:
//    结果 = 调用签名(fn, "iipi", 0, "你好", "标题", 0);
//    结果 = 调用签名(fn, "ff", 64.0);           // double sqrt(double)
//    结果 = 调用签名(fn, "fif", 2.0, 10.0);     // double pow(double, double) — 无整数参
//    结果 = 调用签名(fn, "iif", 42, 3.14);      // int func(int, double)
//    结果 = 调用签名(fn, "fii", 1, 2);           // double func(int, int)
//    // 签名格式: 第1字符 = 返回类型, 后续每字符 = 参数类型
//    //   i = int64/指针, f = double, v = void 返回, s = 字符串返回
// ═══════════════════════════════════════════════════════════════════
//  #include'd from stdlib.cpp, already inside namespace cplang

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
#include <mutex>

// ─── FFI 全局：回调分发所需的 VM 指针（ffiCallSig→cbDispatcher）───
//  通过 RAII 守卫在 ffiCallSig 入口设置，所有 return 路径自动清理。
//  使用静态全局而非 VM::current() TLS 通路，因为 TLS 访问在 thunk 上下文中可能崩溃。
static VM* s_ffiCallbackVM = nullptr;
struct FfiCallbackVMGuard {
    FfiCallbackVMGuard() { s_ffiCallbackVM = VM::current(); }
    ~FfiCallbackVMGuard() { s_ffiCallbackVM = nullptr; }
};

namespace ffi_ns {

// ─── UTF-8 → 系统 ANSI 编码转换（解决 MessageBoxA 等中文乱码）───
#ifdef _WIN32
static std::string utf8ToAnsi(const char* utf8) {
    if (!utf8 || !*utf8) return "";
    int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
    if (wideLen <= 0) return "";
    std::wstring wide(static_cast<size_t>(wideLen), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, &wide[0], wideLen);
    int ansiLen = WideCharToMultiByte(CP_ACP, 0, &wide[0], -1, nullptr, 0, nullptr, nullptr);
    if (ansiLen <= 0) return "";
    std::string ansi(static_cast<size_t>(ansiLen), '\0');
    WideCharToMultiByte(CP_ACP, 0, &wide[0], -1, &ansi[0], ansiLen, nullptr, nullptr);
    return ansi;
}
#else
static std::string utf8ToAnsi(const char* utf8) {
    return utf8 ? std::string(utf8) : "";
}
#endif

// ─── 内联转换辅助：extractArgs 中替代 s_ffiStringBuf 的直接转换 ───
//   直接将 UTF-8 字符串转 ANSI 并返回 std::string，由调用方持有生命周期。
//   extractArgs / extractTypedArgs 接受 vector* 参数来暂存这些转换结果。
//   每个 FFI 函数在栈上创建 vector，调用结束后自动析构，无需手动 clear。

// ─── 值检测辅助（兼容内联 Int32 和装箱 Int64）───
static inline bool isInteger(const Value& v) {
    return v.isInt() || v.isInt64();
}
static inline int64_t asInteger(const Value& v) {
    return v.asInt();  // asInt() 已覆盖 Int8/16/32/64
}
static inline double asFloatValue(const Value& v) {
    if (v.isFloat() || v.isFloat32()) return v.asFloat();
    if (isInteger(v)) return static_cast<double>(asInteger(v));
    if (v.isBool()) return v.asBool() ? 1.0 : 0.0;
    return 0.0;
}

// ─── 通用函数类型（Windows x64 调用约定可接受最多 6 个 int64/指针参数）───
using FFIFunc6 = int64_t(*)(int64_t, int64_t, int64_t, int64_t, int64_t, int64_t);

// ─── 参数提取辅助 ───
//   strBuf: 非空时，将字符串参数做 UTF-8→ANSI 转换并存入此缓冲区
//   注意：先全部转换完再填写指针，避免 vector 扩容导致悬空指针。
static void extractArgs(const std::vector<Value>& args, int startIdx,
                         int64_t* out, int maxArgs,
                         std::vector<std::string>* strBuf = nullptr) {
    int n = static_cast<int>(args.size()) - startIdx;
    if (n > maxArgs) n = maxArgs;

    // 第一遍：先转换所有字符串并存入缓冲区
    if (strBuf) {
        strBuf->reserve(n);
        for (int i = 0; i < n; i++) {
            const Value& v = args[startIdx + i];
            if (v.isString()) {
                strBuf->push_back(utf8ToAnsi(v.asString()->data));
            }
        }
    }

    // 第二遍：填写参数数组（此时 strBuf 已稳定，no more reallocation）
    size_t strIdx = 0;
    for (int i = 0; i < n; i++) {
        const Value& v = args[startIdx + i];
        if (isInteger(v)) {
            out[i] = asInteger(v);
        } else if (v.isFloat() || v.isFloat32()) {
            double d = v.asFloat();
            std::memcpy(&out[i], &d, sizeof(double));
        } else if (v.isString()) {
            if (strBuf) {
                out[i] = reinterpret_cast<int64_t>((*strBuf)[strIdx++].data());
            } else {
                out[i] = reinterpret_cast<int64_t>(v.asString()->data);
            }
        } else if (v.isNil()) {
            out[i] = 0;
        } else if (v.isBool()) {
            out[i] = v.asBool() ? 1 : 0;
        } else {
            out[i] = 0;
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
//  加载库(path) → 句柄 (Int64)
// ═══════════════════════════════════════════════════════════════════
Value ffiLoadLibrary(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString())
        return Value::Int(0);
    // 路径字符串转 ANSI（支持含中文的 DLL 路径）
    std::string ansiPath = utf8ToAnsi(args[0].asString()->data);
    const char* path = ansiPath.c_str();
#ifdef _WIN32
    HMODULE h = LoadLibraryA(path);
    return Value::Int(reinterpret_cast<int64_t>(h));
#else
    void* h = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
    return Value::Int(reinterpret_cast<int64_t>(h));
#endif
}

// ═══════════════════════════════════════════════════════════════════
//  取函数(库句柄, 函数名) → 函数地址 (Int64)
// ═══════════════════════════════════════════════════════════════════
Value ffiGetProc(std::vector<Value>& args) {
    if (args.size() < 2 || !isInteger(args[0]) || !args[1].isString())
        return Value::Int(0);
    int64_t handle = asInteger(args[0]);
    // 函数名转 ANSI（支持含中文的符号名）
    std::string ansiName = utf8ToAnsi(args[1].asString()->data);
    const char* name = ansiName.c_str();
    if (handle == 0) return Value::Int(0);
#ifdef _WIN32
    FARPROC proc = GetProcAddress(reinterpret_cast<HMODULE>(handle), name);
    return Value::Int(reinterpret_cast<int64_t>(proc));
#else
    void* sym = dlsym(reinterpret_cast<void*>(handle), name);
    return Value::Int(reinterpret_cast<int64_t>(sym));
#endif
}

// ═══════════════════════════════════════════════════════════════════
//  调用整数(函数地址, args...) → Int64
//  通用调用，返回 int64 或指针。所有参数按整数/指针传递。
// ═══════════════════════════════════════════════════════════════════
Value ffiCallInt(std::vector<Value>& args) {
    if (args.empty() || !isInteger(args[0])) return Value::Int(0);
    int64_t addr = asInteger(args[0]);
    if (addr == 0) return Value::Int(0);

    int64_t a[6] = {0};
    std::vector<std::string> ansiBuf;  // 持有转换后的字符串，直到调用结束
    extractArgs(args, 1, a, 6, &ansiBuf);

    FFIFunc6 fn = reinterpret_cast<FFIFunc6>(addr);
    int64_t result = fn(a[0], a[1], a[2], a[3], a[4], a[5]);
    return Value::Int(result);
}

// ═══════════════════════════════════════════════════════════════════
//  调用浮点(函数地址, args...) → Float64
//  所有参数按 double 提取，通过 double(*)() 正确路由到 XMM 寄存器。
//  适用于 sqrt(double) / pow(double,double) 等全浮点签名。
// ═══════════════════════════════════════════════════════════════════
Value ffiCallFloat(std::vector<Value>& args) {
    if (args.empty() || !isInteger(args[0])) return Value::Float(0.0);
    int64_t addr = asInteger(args[0]);
    if (addr == 0) return Value::Float(0.0);

    int n = static_cast<int>(args.size()) - 1;
    if (n > 6) n = 6;

    double d[6] = {0};
    for (int i = 0; i < n; i++) {
        d[i] = asFloatValue(args[1 + i]);
    }

    typedef double (*DoubleFunc6)(double, double, double, double, double, double);
    DoubleFunc6 fn = reinterpret_cast<DoubleFunc6>(addr);
    double result = fn(d[0], d[1], d[2], d[3], d[4], d[5]);
    return Value::Float(result);
}

// ═══════════════════════════════════════════════════════════════════
//  调用空(函数地址, args...) → Nil
// ═══════════════════════════════════════════════════════════════════
Value ffiCallVoid(std::vector<Value>& args) {
    if (args.empty() || !isInteger(args[0])) return Value::nil();
    int64_t addr = asInteger(args[0]);
    if (addr == 0) return Value::nil();

    int64_t a[6] = {0};
    std::vector<std::string> ansiBuf;
    extractArgs(args, 1, a, 6, &ansiBuf);

    FFIFunc6 fn = reinterpret_cast<FFIFunc6>(addr);
    fn(a[0], a[1], a[2], a[3], a[4], a[5]);
    return Value::nil();
}

// ═══════════════════════════════════════════════════════════════════
//  调用字符串(函数地址, args...) → 字符串
// ═══════════════════════════════════════════════════════════════════
Value ffiCallString(std::vector<Value>& args) {
    if (args.empty() || !isInteger(args[0])) return Value::nil();
    int64_t addr = asInteger(args[0]);
    if (addr == 0) return Value::nil();

    int64_t a[6] = {0};
    std::vector<std::string> ansiBuf;
    extractArgs(args, 1, a, 6, &ansiBuf);

    typedef const char* (*StrFunc)(int64_t, int64_t, int64_t, int64_t, int64_t, int64_t);
    StrFunc fn = reinterpret_cast<StrFunc>(addr);
    const char* result = fn(a[0], a[1], a[2], a[3], a[4], a[5]);
    if (!result) return Value::nil();
    return Value::String(VMString::create(result));
}

// ═══════════════════════════════════════════════════════════════════
//  调用签名(函数地址, 签名模板, args...) → 按签名返回
//  签名格式: 第1字符=返回类型, 后续每字符=参数类型
//    返回: i=int64, f=double, v=void, s=string
//    参数: i=int64/指针, f=double, b=bool
//  示例:
//    调用签名(fn, "ii", arg)           → int64(int64)
//    调用签名(fn, "ff", arg)           → double(double)
//    调用签名(fn, "fii", a, b)         → double(int64, int64)
//    调用签名(fn, "iif", a, b)         → int64(int64, double)
//    调用签名(fn, "ifif", a, b, c)     → int64(double, int64, double)
//    调用签名(fn, "vii", a, b)         → void(int64, int64)
//    调用签名(fn, "si", arg)           → string(int64)
// ═══════════════════════════════════════════════════════════════════
//  参数按类型提取器 — 将 CP Value 转换为 int64 或 double
//  返回一个新分配的数组，大小 = paramCount
//  values[i] = 第 i 个参数的原始地址（用于后续 reinterpret_cast）
//  isFloat[i] = 该参数是否应走 XMM 寄存器
// ═══════════════════════════════════════════════════════════════════
static void extractTypedArgs(const std::vector<Value>& args, int startIdx,
                              const char* types, int nParams,
                              int64_t* iOut, double* fOut, bool* isFloatOut,
                              std::vector<std::string>* strBuf = nullptr) {
    // 第一遍：先转换所有字符串
    if (strBuf) {
        strBuf->reserve(nParams);
        for (int i = 0; i < nParams; i++) {
            if (types[i] != 'f' && types[i] != 'b' && !isInteger(args[startIdx + i])) {
                const Value& v = args[startIdx + i];
                if (v.isString()) {
                    strBuf->push_back(utf8ToAnsi(v.asString()->data));
                }
            }
        }
    }

    // 第二遍：填写参数数组（strBuf 已稳定）
    size_t strIdx = 0;
    for (int i = 0; i < nParams; i++) {
        char t = types[i];
        const Value& v = args[startIdx + i];
        if (t == 'f') {
            fOut[i] = asFloatValue(v);
            isFloatOut[i] = true;
            iOut[i] = 0;
        } else {
            isFloatOut[i] = false;
            fOut[i] = 0.0;
            if (t == 'b') {
                iOut[i] = v.isTrue() ? 1 : 0;
            } else if (isInteger(v)) {
                iOut[i] = asInteger(v);
            } else if (v.isString()) {
                if (strBuf) {
                    iOut[i] = reinterpret_cast<int64_t>((*strBuf)[strIdx++].data());
                } else {
                    iOut[i] = reinterpret_cast<int64_t>(v.asString()->data);
                }
            } else {
                iOut[i] = 0;
            }
        }
    }
}

// ─── 签名分发方式 ───
// 所有宏已内联到代码中

// ─── 主签名分发函数 ───
Value ffiCallSig(std::vector<Value>& args) {
    if (args.size() < 2 || !isInteger(args[0]) || !args[1].isString())
        return Value::Int(0);
    int64_t addr = asInteger(args[0]);
    if (addr == 0) return Value::Int(0);

    const char* sig = args[1].asString()->data;
    int sigLen = static_cast<int>(args[1].asString()->length);
    if (sigLen < 1) return Value::Int(0);

    char retType = sig[0];
    int paramCount = sigLen - 1;
    if (paramCount > 4) paramCount = 4;  // 支持最多 4 参

    // 提取参数（保持原始顺序）
    int64_t iArgs[4] = {0};
    double  fArgs[4] = {0.0};
    bool    isFloat[4] = {false};
    std::vector<std::string> ansiBuf;
    extractTypedArgs(args, 2, sig + 1, paramCount, iArgs, fArgs, isFloat, &ansiBuf);

    // 设置回调 VM 指针（RAII 守卫，任何时候返回自动清理）
    FfiCallbackVMGuard _cbGuard;

    // 构造签名参数字符串
    const char* sigStr = sig;          // 完整签名（含返回类型）
    const char* paramTypes = sig + 1;  // 仅参数类型

// ═══════════════════════════════════════════════════════════════════
//  第1类：纯整数参 (all i) — 走 RCX/RDX/R8/R9
//  第2类：纯浮点参 (all f) — 走 XMM0/XMM1/XMM2/XMM3
//  第3类：混合参 — 按精确序列匹配
// ═══════════════════════════════════════════════════════════════════

    // ── 1 个参数 ──
    if (paramCount == 1) {
        if (isFloat[0]) {
            // double(double)
            if (retType == 'f') { auto fn = reinterpret_cast<double(*)(double)>(addr); return Value::Float(fn(fArgs[0])); }
            // int64(double)
            if (retType == 'i') { auto fn = reinterpret_cast<int64_t(*)(double)>(addr); return Value::Int(fn(fArgs[0])); }
            // void(double)
            if (retType == 'v') { auto fn = reinterpret_cast<void(*)(double)>(addr); fn(fArgs[0]); return Value::nil(); }
        } else {
            // double(int64)
            if (retType == 'f') { auto fn = reinterpret_cast<double(*)(int64_t)>(addr); return Value::Float(fn(iArgs[0])); }
            // int64(int64)
            if (retType == 'i' || retType == 'b') { auto fn = reinterpret_cast<int64_t(*)(int64_t)>(addr); return Value::Int(fn(iArgs[0])); }
            // void(int64)
            if (retType == 'v') { auto fn = reinterpret_cast<void(*)(int64_t)>(addr); fn(iArgs[0]); return Value::nil(); }
            // string(int64)
            if (retType == 's') { auto fn = reinterpret_cast<const char*(*)(int64_t)>(addr); const char* r = fn(iArgs[0]); return r ? Value::String(VMString::create(r)) : Value::nil(); }
        }
    }

    // ── 2 个参数 ──
    if (paramCount == 2) {
        // 纯整数 ii
        if (!isFloat[0] && !isFloat[1]) {
            if (retType == 'f') { auto fn = reinterpret_cast<double(*)(int64_t, int64_t)>(addr); return Value::Float(fn(iArgs[0], iArgs[1])); }
            if (retType == 'i' || retType == 'b') { auto fn = reinterpret_cast<int64_t(*)(int64_t, int64_t)>(addr); return Value::Int(fn(iArgs[0], iArgs[1])); }
            if (retType == 'v') { auto fn = reinterpret_cast<void(*)(int64_t, int64_t)>(addr); fn(iArgs[0], iArgs[1]); return Value::nil(); }
            if (retType == 's') { auto fn = reinterpret_cast<const char*(*)(int64_t, int64_t)>(addr); const char* r = fn(iArgs[0], iArgs[1]); return r ? Value::String(VMString::create(r)) : Value::nil(); }
        }
        // 纯浮点 ff
        if (isFloat[0] && isFloat[1]) {
            if (retType == 'f') { auto fn = reinterpret_cast<double(*)(double, double)>(addr); return Value::Float(fn(fArgs[0], fArgs[1])); }
            if (retType == 'i') { auto fn = reinterpret_cast<int64_t(*)(double, double)>(addr); return Value::Int(fn(fArgs[0], fArgs[1])); }
            if (retType == 'v') { auto fn = reinterpret_cast<void(*)(double, double)>(addr); fn(fArgs[0], fArgs[1]); return Value::nil(); }
        }
        // 混合: int, double (if)
        if (!isFloat[0] && isFloat[1]) {
            if (retType == 'f') { auto fn = reinterpret_cast<double(*)(int64_t, double)>(addr); return Value::Float(fn(iArgs[0], fArgs[1])); }
            if (retType == 'i') { auto fn = reinterpret_cast<int64_t(*)(int64_t, double)>(addr); return Value::Int(fn(iArgs[0], fArgs[1])); }
            if (retType == 'v') { auto fn = reinterpret_cast<void(*)(int64_t, double)>(addr); fn(iArgs[0], fArgs[1]); return Value::nil(); }
            if (retType == 's') { auto fn = reinterpret_cast<const char*(*)(int64_t, double)>(addr); const char* r = fn(iArgs[0], fArgs[1]); return r ? Value::String(VMString::create(r)) : Value::nil(); }
        }
        // 混合: double, int (fi)
        if (isFloat[0] && !isFloat[1]) {
            if (retType == 'f') { auto fn = reinterpret_cast<double(*)(double, int64_t)>(addr); return Value::Float(fn(fArgs[0], iArgs[1])); }
            if (retType == 'i') { auto fn = reinterpret_cast<int64_t(*)(double, int64_t)>(addr); return Value::Int(fn(fArgs[0], iArgs[1])); }
            if (retType == 'v') { auto fn = reinterpret_cast<void(*)(double, int64_t)>(addr); fn(fArgs[0], iArgs[1]); return Value::nil(); }
        }
    }

    // ── 3 个参数 ──
    if (paramCount == 3) {
        // 纯整数 iii
        if (!isFloat[0] && !isFloat[1] && !isFloat[2]) {
            typedef int64_t (*I3)(int64_t, int64_t, int64_t);
            typedef double (*F3I)(int64_t, int64_t, int64_t);
            if (retType == 'i') { auto fn = reinterpret_cast<I3>(addr); return Value::Int(fn(iArgs[0], iArgs[1], iArgs[2])); }
            if (retType == 'f') { auto fn = reinterpret_cast<F3I>(addr); return Value::Float(fn(iArgs[0], iArgs[1], iArgs[2])); }
        }
        // 纯浮点 fff
        if (isFloat[0] && isFloat[1] && isFloat[2]) {
            typedef double (*F3)(double, double, double);
            typedef int64_t (*IF3)(double, double, double);
            if (retType == 'f') { auto fn = reinterpret_cast<F3>(addr); return Value::Float(fn(fArgs[0], fArgs[1], fArgs[2])); }
            if (retType == 'i') { auto fn = reinterpret_cast<IF3>(addr); return Value::Int(fn(fArgs[0], fArgs[1], fArgs[2])); }
        }
        // i,i,f
        if (!isFloat[0] && !isFloat[1] && isFloat[2]) {
            if (retType == 'i') { typedef int64_t (*IIF)(int64_t, int64_t, double); auto fn = reinterpret_cast<IIF>(addr); return Value::Int(fn(iArgs[0], iArgs[1], fArgs[2])); }
            if (retType == 'f') { typedef double (*FIIF)(int64_t, int64_t, double); auto fn = reinterpret_cast<FIIF>(addr); return Value::Float(fn(iArgs[0], iArgs[1], fArgs[2])); }
        }
        // i,f,i
        if (!isFloat[0] && isFloat[1] && !isFloat[2]) {
            if (retType == 'i') { typedef int64_t (*IFI)(int64_t, double, int64_t); auto fn = reinterpret_cast<IFI>(addr); return Value::Int(fn(iArgs[0], fArgs[1], iArgs[2])); }
            if (retType == 'f') { typedef double (*FIFI)(int64_t, double, int64_t); auto fn = reinterpret_cast<FIFI>(addr); return Value::Float(fn(iArgs[0], fArgs[1], iArgs[2])); }
        }
        // f,i,i
        if (isFloat[0] && !isFloat[1] && !isFloat[2]) {
            if (retType == 'i') { typedef int64_t (*IFII)(double, int64_t, int64_t); auto fn = reinterpret_cast<IFII>(addr); return Value::Int(fn(fArgs[0], iArgs[1], iArgs[2])); }
            if (retType == 'f') { typedef double (*FFII)(double, int64_t, int64_t); auto fn = reinterpret_cast<FFII>(addr); return Value::Float(fn(fArgs[0], iArgs[1], iArgs[2])); }
        }
        // f,f,i
        if (isFloat[0] && isFloat[1] && !isFloat[2]) {
            if (retType == 'f') { typedef double (*FFI)(double, double, int64_t); auto fn = reinterpret_cast<FFI>(addr); return Value::Float(fn(fArgs[0], fArgs[1], iArgs[2])); }
        }
        // f,i,f
        if (isFloat[0] && !isFloat[1] && isFloat[2]) {
            if (retType == 'f') { typedef double (*FIF2)(double, int64_t, double); auto fn = reinterpret_cast<FIF2>(addr); return Value::Float(fn(fArgs[0], iArgs[1], fArgs[2])); }
        }
        // i,f,f
        if (!isFloat[0] && isFloat[1] && isFloat[2]) {
            if (retType == 'f') { typedef double (*FIFF2)(int64_t, double, double); auto fn = reinterpret_cast<FIFF2>(addr); return Value::Float(fn(iArgs[0], fArgs[1], fArgs[2])); }
            if (retType == 'i') { typedef int64_t (*IIFF)(int64_t, double, double); auto fn = reinterpret_cast<IIFF>(addr); return Value::Int(fn(iArgs[0], fArgs[1], fArgs[2])); }
        }
    }

    // ── 4 个参数 ──
    if (paramCount == 4) {
        // 仅支持纯整数（最常见的 Windows API 模式）
        if (!isFloat[0] && !isFloat[1] && !isFloat[2] && !isFloat[3]) {
            typedef int64_t (*I4)(int64_t, int64_t, int64_t, int64_t);
            if (retType == 'i' || retType == 'b') { auto fn = reinterpret_cast<I4>(addr); return Value::Int(fn(iArgs[0], iArgs[1], iArgs[2], iArgs[3])); }
        }
    }

    // ── 回退：通过 FFIFunc6（纯整数降级） ──
    // 把 double 参数按 memcpy 打包进 int64（降级到整数寄存器）
    int64_t fallback[6] = {0};
    for (int i = 0; i < paramCount; i++) {
        if (isFloat[i]) {
            std::memcpy(&fallback[i], &fArgs[i], sizeof(double));
        } else {
            fallback[i] = iArgs[i];
        }
    }

    if (retType == 'v') {
        FFIFunc6 fn = reinterpret_cast<FFIFunc6>(addr);
        fn(fallback[0], fallback[1], fallback[2], fallback[3], fallback[4], fallback[5]);
        return Value::nil();
    } else if (retType == 'f') {
        typedef double (*DF6)(int64_t, int64_t, int64_t, int64_t, int64_t, int64_t);
        auto fn = reinterpret_cast<DF6>(addr);
        return Value::Float(fn(fallback[0], fallback[1], fallback[2], fallback[3], fallback[4], fallback[5]));
    } else if (retType == 's') {
        FFIFunc6 fn = reinterpret_cast<FFIFunc6>(addr);
        int64_t r = fn(fallback[0], fallback[1], fallback[2], fallback[3], fallback[4], fallback[5]);
        const char* cr = reinterpret_cast<const char*>(r);
        return cr ? Value::String(VMString::create(cr)) : Value::nil();
    } else {
        FFIFunc6 fn = reinterpret_cast<FFIFunc6>(addr);
        return Value::Int(fn(fallback[0], fallback[1], fallback[2], fallback[3], fallback[4], fallback[5]));
    }
}
} // namespace ffi_ns

// ═══════════════════════════════════════════════════════════════════
//  回调支持 — 将 CP 函数包装为 C 函数指针（供 DLL 回调使用）
//
//  用法（CP 侧）:
//    回调地址 = 创建回调(函数名, "ii");    // CP 函数 → C 函数指针
//    SetWindowsHookEx(WH_KEYBOARD, 回调地址, ...);
//    ...
//    释放回调(回调地址);
//
//  原理:
//    1. 分配可执行内存页
//    2. 写入 x64 thunk：保存参数 → 设置 callback_id → 调用分发器
//    3. 分发器通过 callback_id 查找 CP 函数 → 转换参数 → 调用 VM
//
//  限制: 最多 4 个参数（覆盖 >95% Windows 回调场景）
//        单线程安全（回调在 CP VM 所在线程执行）
// ═══════════════════════════════════════════════════════════════════

namespace callback_ns {

// ─── 常量 ───
static constexpr int MAX_CALLBACKS = 256;
static constexpr int THUNK_SIZE    = 128; // 每个 thunk 128 字节（整数+浮点寄存器保存）

// ─── 回调参数保存区（thunk 写入，cbDispatcher 读取，支持多参数）───
//   [0-3] = 整数寄存器 RCX/RDX/R8/R9
//   [4-7] = 浮点寄存器 XMM0/XMM1/XMM2/XMM3 (movsd 64位)
static int64_t s_cbArgs[8] = {0};

// ─── 回调条目 ───
struct CallbackSlot {
    Value    func;           // CP 函数/闭包 Value（创建时从参数直接获取）
    std::string sig;        // 签名（如 "ii" = int(int,int)）
    char     retType;       // 返回类型: 'i','f','v'
    int      paramCount;    // 参数量
    bool     inUse = false;
};

// ─── 全局状态 ───
static CallbackSlot s_slots[MAX_CALLBACKS];
static void*        s_thunkBase = nullptr;  // VirtualAlloc 的可执行内存
static int          s_nextSlot = 0;
static std::mutex   s_cbMutex;

// ─── 分发器函数（C 链接，由 thunk 调用）───
extern "C" int64_t __cdecl cbDispatcher(int64_t id, int64_t, int64_t,
                                         int64_t, int64_t) {
    // id 来自 RCX（thunk 设置的 slotId）
    // 参数值从 s_cbArgs 读取（thunk 已保存 RCX/RDX/R8/R9 到该数组）
    int64_t safeId = id;
    if (safeId < 0 || safeId >= MAX_CALLBACKS || !s_slots[safeId].inUse)
        return 0;

    int64_t rawArgs[4];
    rawArgs[0] = s_cbArgs[0];
    rawArgs[1] = s_cbArgs[1];
    rawArgs[2] = s_cbArgs[2];
    rawArgs[3] = s_cbArgs[3];

    auto& slot = s_slots[safeId];
    Value func = slot.func;

    // 提取闭包→函数
    if (func.isClosure() && func.asPtr()) {
        VMClosure* closure = reinterpret_cast<VMClosure*>(func.asPtr());
        if (closure->func)
            func = Value::Ptr(reinterpret_cast<VMObject*>(closure->func));
    }

    if (!func.isFunction())
        return rawArgs[0];  // 没有可调用的函数，返回第一个参数

    // 构造 CP 参数：浮点从 s_cbArgs[4-7] (XMM 保存区)，整数从 rawArgs[0-3]
    std::vector<Value> cpArgs;
    cpArgs.reserve(slot.paramCount);
    for (int i = 0; i < slot.paramCount && i < 4; i++) {
        if (i < (int)slot.sig.size() && slot.sig[i] == 'f') {
            double d;
            std::memcpy(&d, &s_cbArgs[4 + i], sizeof(double));
            cpArgs.push_back(Value::Float(d));
        } else {
            cpArgs.push_back(Value::Int(rawArgs[i]));
        }
    }

    // 获取 VM
    VM* vm = s_ffiCallbackVM;
    if (!vm) return rawArgs[0];

    // 调用 CP 函数
    Value result = vm->callFunction(func, cpArgs);

    // 按返回类型转换
    if (slot.retType == 'f') {
        double d = result.asFloat();
        int64_t ret;
        std::memcpy(&ret, &d, sizeof(double));
        return ret;
    } else if (slot.retType == 'v') {
        return 0;
    }
    return result.asInt();
}

// ─── 生成 x64 thunk（多参数通用）───
//  thunk 负责:
//    1. 保存 RCX/RDX/R8/R9 到静态全局 s_cbArgs[]
//    2. 设 RCX = slotId
//    3. call cbDispatcher（从 s_cbArgs 读取参数）
//    4. 恢复栈并返回（cbDispatcher 的返回值在 RAX）
//  采用 mov rax, imm64 加载基地址方式，总码长 56 字节，适合 64 字节 thunk 页
static bool generateThunk(int slotId, void* dispatcher, uint8_t* code) {
    int p = 0;

    // 1. sub rsp, 0x28    ; 对齐 + 为 cbDispatcher 留影子空间
    //    thunk入口RSP≡8 mod 16 → sub 40(0x28) → RSP≡0 mod 16 → call → RSP≡8 ✓
    code[p++] = 0x48; code[p++] = 0x83; code[p++] = 0xEC; code[p++] = 0x28;

    // 2. mov rax, imm64   ; RAX = &s_cbArgs[0]
    code[p++] = 0x48; code[p++] = 0xB8;
    { int64_t v = reinterpret_cast<int64_t>(&s_cbArgs[0]);
      std::memcpy(code + p, &v, 8); p += 8; }

    // 3. mov [rax],   rcx  ; s_cbArgs[0] = arg1 (RCX)
    code[p++] = 0x48; code[p++] = 0x89; code[p++] = 0x08;

    // 4. mov [rax+8], rdx  ; s_cbArgs[1] = arg2 (RDX)
    code[p++] = 0x48; code[p++] = 0x89; code[p++] = 0x50; code[p++] = 0x08;

    // 5. mov [rax+16], r8  ; s_cbArgs[2] = arg3 (R8)
    code[p++] = 0x4C; code[p++] = 0x89; code[p++] = 0x40; code[p++] = 0x10;

    // 6. mov [rax+24], r9  ; s_cbArgs[3] = arg4 (R9)
    code[p++] = 0x4C; code[p++] = 0x89; code[p++] = 0x48; code[p++] = 0x18;

    // 7-10. movsd [rax+32], xmm0  ; s_cbArgs[4-7] = XMM0-XMM3（浮点参数）
    //    movsd [rax+40], xmm1
    //    movsd [rax+48], xmm2
    //    movsd [rax+56], xmm3
    //    编码: F2 0F 11 /r = movsd 存到内存
    //    ModRM: mod=01(base+disp8), reg=XMMn, r/m=000(rax)
    //    xmm0(reg=000): 0x40, xmm1(reg=001): 0x48, xmm2(reg=010): 0x50, xmm3(reg=011): 0x58
    code[p++] = 0xF2; code[p++] = 0x0F; code[p++] = 0x11; code[p++] = 0x40; code[p++] = 0x20; // xmm0, +32
    code[p++] = 0xF2; code[p++] = 0x0F; code[p++] = 0x11; code[p++] = 0x48; code[p++] = 0x28; // xmm1, +40
    code[p++] = 0xF2; code[p++] = 0x0F; code[p++] = 0x11; code[p++] = 0x50; code[p++] = 0x30; // xmm2, +48
    code[p++] = 0xF2; code[p++] = 0x0F; code[p++] = 0x11; code[p++] = 0x58; code[p++] = 0x38; // xmm3, +56

    // 11. mov rcx, imm64   ; RCX = slotId
    code[p++] = 0x48; code[p++] = 0xB9;
    { int64_t v = static_cast<int64_t>(slotId);
      std::memcpy(code + p, &v, 8); p += 8; }

    // 8. mov rax, imm64   ; RAX = cbDispatcher addr
    code[p++] = 0x48; code[p++] = 0xB8;
    std::memcpy(code + p, &dispatcher, 8); p += 8;

    // 9. call rax
    code[p++] = 0xFF; code[p++] = 0xD0;

    // 10. add rsp, 0x28
    code[p++] = 0x48; code[p++] = 0x83; code[p++] = 0xC4; code[p++] = 0x28;

    // 11. ret
    code[p++] = 0xC3;

    return true;
}

    // ─── 创建回调(cpFunc, sig) → 函数地址 ───
    //  第一个参数可以是 函数值(isFunction/isClosure) 或 函数名字符串
Value ffiCreateCallback(std::vector<Value>& args) {
    if (args.size() < 2 || !args[1].isString())
        return Value::Int(0);

    const char* sig = args[1].asString()->data;
    int sigLen = static_cast<int>(args[1].asString()->length);
    if (sigLen < 1) return Value::Int(0);
    char retType = sig[0];
    int paramCount = sigLen - 1;
    if (paramCount > 4) paramCount = 4;

    // 获取函数值：支持传入函数值本身或函数名字符串
    Value func = Value::nil();
    if (args[0].isFunction() || args[0].isClosure()) {
        func = args[0];
    }
    // 字符串名方式暂关闭（getGlobalSlot 在 ffiCreateCallback 内也有不稳定现象）
    // 推荐始终传函数值： 创建回调(双倍, "ii") 而非 创建回调("双倍", "ii")
    if (!func.isFunction() && !func.isClosure())
        return Value::Int(0);

    std::lock_guard<std::mutex> lock(s_cbMutex);

    // 找空槽
    int slotId = -1;
    for (int i = 0; i < MAX_CALLBACKS; i++) {
        if (!s_slots[i].inUse) { slotId = i; break; }
    }
    if (slotId < 0) return Value::Int(0);

    // 初始化可执行内存池
    if (!s_thunkBase) {
        s_thunkBase = VirtualAlloc(nullptr, MAX_CALLBACKS * THUNK_SIZE,
                                   MEM_COMMIT | MEM_RESERVE,
                                   PAGE_EXECUTE_READWRITE);
        if (!s_thunkBase) return Value::Int(0);
    }

    // 生成 thunk
    uint8_t* thunkAddr = static_cast<uint8_t*>(s_thunkBase) + slotId * THUNK_SIZE;
    std::memset(thunkAddr, 0xCC, THUNK_SIZE);  // INT3 填充
    generateThunk(slotId, reinterpret_cast<void*>(cbDispatcher), thunkAddr);

    // 写入槽
    s_slots[slotId].func = func;
    s_slots[slotId].sig = std::string(sig + 1, paramCount);  // 仅参数类型（不含返回）
    s_slots[slotId].retType = retType;
    s_slots[slotId].paramCount = paramCount;
    s_slots[slotId].inUse = true;

    // 确保指令缓存刷新（x64 需要）
    FlushInstructionCache(GetCurrentProcess(), thunkAddr, THUNK_SIZE);

    return Value::Int(reinterpret_cast<int64_t>(thunkAddr));
}

// ─── 释放回调(地址) → Bool ───
Value ffiFreeCallback(std::vector<Value>& args) {
    if (args.empty() || !ffi_ns::isInteger(args[0]))
        return Value::Bool(false);
    int64_t addr = ffi_ns::asInteger(args[0]);
    if (addr == 0 || !s_thunkBase) return Value::Bool(false);

    // 计算 slot id
    int64_t offset = addr - reinterpret_cast<int64_t>(s_thunkBase);
    int slotId = static_cast<int>(offset / THUNK_SIZE);
    if (slotId < 0 || slotId >= MAX_CALLBACKS || offset % THUNK_SIZE != 0)
        return Value::Bool(false);
    if (!s_slots[slotId].inUse)
        return Value::Bool(false);

    std::lock_guard<std::mutex> lock(s_cbMutex);
    s_slots[slotId].inUse = false;
    s_slots[slotId].func = Value::nil();

    return Value::Bool(true);
}

} // namespace callback_ns

namespace ffi_ns {

// ═══════════════════════════════════════════════════════════════════
//  错误码() → Int64（最近一次错误的码）
// ═══════════════════════════════════════════════════════════════════
Value ffiLastError(std::vector<Value>& /*args*/) {
#ifdef _WIN32
    return Value::Int(static_cast<int64_t>(GetLastError()));
#else
    return Value::Int(static_cast<int64_t>(errno));
#endif
}

// ═══════════════════════════════════════════════════════════════════
//  错误信息(错误码) → 字符串
// ═══════════════════════════════════════════════════════════════════
Value ffiErrorMsg(std::vector<Value>& args) {
    int64_t code = 0;
    if (!args.empty() && isInteger(args[0])) code = asInteger(args[0]);
#ifdef _WIN32
    LPSTR buf = nullptr;
    DWORD len = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, static_cast<DWORD>(code), 0,
        reinterpret_cast<LPSTR>(&buf), 0, nullptr);
    if (len == 0) return Value::String(VMString::create(""));
    // 去掉末尾换行和句号
    while (len > 0 && (buf[len-1] == '\r' || buf[len-1] == '\n' || buf[len-1] == '.'))
        len--;
    std::string msg(buf, len);
    LocalFree(buf);
    return Value::String(VMString::create(msg));
#else
    return Value::String(VMString::create(strerror(static_cast<int>(code))));
#endif
}

// ═══════════════════════════════════════════════════════════════════
//  释放库(句柄) → Bool
// ═══════════════════════════════════════════════════════════════════
Value ffiFreeLibrary(std::vector<Value>& args) {
    if (args.empty() || !isInteger(args[0])) return Value::Bool(false);
    int64_t handle = asInteger(args[0]);
    if (handle == 0) return Value::Bool(false);
#ifdef _WIN32
    BOOL ok = FreeLibrary(reinterpret_cast<HMODULE>(handle));
    return Value::Bool(ok != FALSE);
#else
    dlclose(reinterpret_cast<void*>(handle));
    return Value::Bool(true);
#endif
}

// ═══════════════════════════════════════════════════════════════════
//  取字符串指针(字符串) → Int64
// ═══════════════════════════════════════════════════════════════════
Value ffiStrPtr(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString())
        return Value::Int(0);
    return Value::Int(reinterpret_cast<int64_t>(args[0].asString()->data));
}

// ═══════════════════════════════════════════════════════════════════
//  读内存(地址, 字节数) → 字符串
// ═══════════════════════════════════════════════════════════════════
Value ffiReadMem(std::vector<Value>& args) {
    if (args.size() < 2 || !isInteger(args[0]) || !isInteger(args[1]))
        return Value::nil();
    int64_t addr = asInteger(args[0]);
    int64_t len = asInteger(args[1]);
    if (addr == 0 || len <= 0 || len > 1024 * 1024)
        return Value::nil();
    const volatile char* src = reinterpret_cast<const volatile char*>(addr);
    std::string result(static_cast<size_t>(len), '\0');
    for (int64_t i = 0; i < len; i++) {
        result[static_cast<size_t>(i)] = src[i];
    }
    return Value::String(VMString::create(result));
}

// ═══════════════════════════════════════════════════════════════════
//  写内存(地址, 数据) → Bool
// ═══════════════════════════════════════════════════════════════════
Value ffiWriteMem(std::vector<Value>& args) {
    if (args.size() < 2 || !isInteger(args[0]))
        return Value::Bool(false);
    int64_t addr = asInteger(args[0]);
    if (addr == 0) return Value::Bool(false);
    volatile char* dst = reinterpret_cast<volatile char*>(addr);

    if (args[1].isString()) {
        const char* data = args[1].asString()->data;
        UInt32 len = args[1].asString()->length;
        for (UInt32 i = 0; i < len; i++) {
            dst[i] = data[i];
        }
        return Value::Bool(true);
    } else if (isInteger(args[1])) {
        int64_t val = asInteger(args[1]);
        volatile int64_t* dst64 = reinterpret_cast<volatile int64_t*>(addr);
        *dst64 = val;
        return Value::Bool(true);
    }
    return Value::Bool(false);
}

// ═══════════════════════════════════════════════════════════════════
//  地址偏移(地址, 偏移) → Int64
// ═══════════════════════════════════════════════════════════════════
Value ffiAddrOffset(std::vector<Value>& args) {
    if (args.size() < 2 || !isInteger(args[0]) || !isInteger(args[1]))
        return Value::Int(0);
    int64_t addr = asInteger(args[0]);
    int64_t offset = asInteger(args[1]);
    return Value::Int(addr + offset);
}

// ═══════════════════════════════════════════════════════════════════
//  读整数(地址) → Int64  (从地址读取 8 字节)
// ═══════════════════════════════════════════════════════════════════
Value ffiReadInt(std::vector<Value>& args) {
    if (args.empty() || !isInteger(args[0]))
        return Value::Int(0);
    int64_t addr = asInteger(args[0]);
    if (addr == 0) return Value::Int(0);
    const volatile int64_t* src = reinterpret_cast<const volatile int64_t*>(addr);
    return Value::Int(*src);
}

// ═══════════════════════════════════════════════════════════════════
//  精确定尺寸读 / 写（2026-05-31 新增）
//  用于读取结构体中的不同尺寸字段
// ═══════════════════════════════════════════════════════════════════

//  读8位(地址) → Int64
Value ffiRead8(std::vector<Value>& args) {
    if (args.empty() || !isInteger(args[0])) return Value::Int(0);
    int64_t addr = asInteger(args[0]);
    if (addr == 0) return Value::Int(0);
    return Value::Int(static_cast<int64_t>(*reinterpret_cast<const volatile int8_t*>(addr)));
}

//  读16位(地址) → Int64
Value ffiRead16(std::vector<Value>& args) {
    if (args.empty() || !isInteger(args[0])) return Value::Int(0);
    int64_t addr = asInteger(args[0]);
    if (addr == 0) return Value::Int(0);
    return Value::Int(static_cast<int64_t>(*reinterpret_cast<const volatile int16_t*>(addr)));
}

//  读32位(地址) → Int64
Value ffiRead32(std::vector<Value>& args) {
    if (args.empty() || !isInteger(args[0])) return Value::Int(0);
    int64_t addr = asInteger(args[0]);
    if (addr == 0) return Value::Int(0);
    return Value::Int(static_cast<int64_t>(*reinterpret_cast<const volatile int32_t*>(addr)));
}

//  读浮点(地址) → Float64
Value ffiReadFloat(std::vector<Value>& args) {
    if (args.empty() || !isInteger(args[0])) return Value::Float(0.0);
    int64_t addr = asInteger(args[0]);
    if (addr == 0) return Value::Float(0.0);
    return Value::Float(static_cast<double>(*reinterpret_cast<const volatile float*>(addr)));
}

//  写8位(地址, 值) → Bool
Value ffiWrite8(std::vector<Value>& args) {
    if (args.size() < 2 || !isInteger(args[0]) || !isInteger(args[1])) return Value::Bool(false);
    int64_t addr = asInteger(args[0]);
    if (addr == 0) return Value::Bool(false);
    *reinterpret_cast<volatile int8_t*>(addr) = static_cast<int8_t>(asInteger(args[1]));
    return Value::Bool(true);
}

//  写16位(地址, 值) → Bool
Value ffiWrite16(std::vector<Value>& args) {
    if (args.size() < 2 || !isInteger(args[0]) || !isInteger(args[1])) return Value::Bool(false);
    int64_t addr = asInteger(args[0]);
    if (addr == 0) return Value::Bool(false);
    *reinterpret_cast<volatile int16_t*>(addr) = static_cast<int16_t>(asInteger(args[1]));
    return Value::Bool(true);
}

//  写32位(地址, 值) → Bool
Value ffiWrite32(std::vector<Value>& args) {
    if (args.size() < 2 || !isInteger(args[0]) || !isInteger(args[1])) return Value::Bool(false);
    int64_t addr = asInteger(args[0]);
    if (addr == 0) return Value::Bool(false);
    *reinterpret_cast<volatile int32_t*>(addr) = static_cast<int32_t>(asInteger(args[1]));
    return Value::Bool(true);
}

// ═══════════════════════════════════════════════════════════════════
//  取结构体字段(结构体地址, 字段偏移, 字段类型) → 值
//  字段类型: "i8"/"i16"/"i32"/"i64"/"f32"/"f64"/"ptr"
//  简化结构体字段读取，无需手动地址偏移+读内存
// ═══════════════════════════════════════════════════════════════════
Value ffiStructField(std::vector<Value>& args) {
    if (args.size() < 3 || !isInteger(args[0]) || !isInteger(args[1]) || !args[2].isString())
        return Value::Int(0);
    int64_t base = asInteger(args[0]);
    int64_t offset = asInteger(args[1]);
    const char* typeStr = args[2].asString()->data;
    if (base == 0) return Value::Int(0);
    int64_t fieldAddr = base + offset;
    // 仅处理已知类型，忽略未知的末尾空白
    if (typeStr[0] == 'i' && typeStr[1] == '8' && typeStr[2] == '\0')
        return Value::Int(*reinterpret_cast<const volatile int8_t*>(fieldAddr));
    if (typeStr[0] == 'i' && typeStr[1] == '1' && typeStr[2] == '6' && typeStr[3] == '\0')
        return Value::Int(*reinterpret_cast<const volatile int16_t*>(fieldAddr));
    if (typeStr[0] == 'i' && typeStr[1] == '3' && typeStr[2] == '2' && typeStr[3] == '\0')
        return Value::Int(*reinterpret_cast<const volatile int32_t*>(fieldAddr));
    if (typeStr[0] == 'i' && typeStr[1] == '6' && typeStr[2] == '4' && typeStr[3] == '\0')
        return Value::Int(*reinterpret_cast<const volatile int64_t*>(fieldAddr));
    if (typeStr[0] == 'f' && typeStr[1] == '3' && typeStr[2] == '2' && typeStr[3] == '\0') {
        float val = *reinterpret_cast<const volatile float*>(fieldAddr);
        return Value::Float(static_cast<double>(val));
    }
    if (typeStr[0] == 'f' && typeStr[1] == '6' && typeStr[2] == '4' && typeStr[3] == '\0')
        return Value::Float(*reinterpret_cast<const volatile double*>(fieldAddr));
    if (typeStr[0] == 'p' && typeStr[1] == 't' && typeStr[2] == 'r' && typeStr[3] == '\0')
        return Value::Int(*reinterpret_cast<const volatile int64_t*>(fieldAddr));
    return Value::Int(0);
}

// ═══════════════════════════════════════════════════════════════════
//  分配内存(字节数) → Int64
// ═══════════════════════════════════════════════════════════════════
Value ffiAllocMem(std::vector<Value>& args) {
    if (args.empty() || !isInteger(args[0]))
        return Value::Int(0);
    int64_t size = asInteger(args[0]);
    if (size <= 0 || size > 64 * 1024 * 1024)
        return Value::Int(0);
    void* ptr = std::malloc(static_cast<size_t>(size));
    if (!ptr) return Value::Int(0);
    std::memset(ptr, 0, static_cast<size_t>(size));
    return Value::Int(reinterpret_cast<int64_t>(ptr));
}

// ═══════════════════════════════════════════════════════════════════
//  释放内存(地址) → Bool
// ═══════════════════════════════════════════════════════════════════
Value ffiFreeMem(std::vector<Value>& args) {
    if (args.empty() || !isInteger(args[0]))
        return Value::Bool(false);
    int64_t addr = asInteger(args[0]);
    if (addr == 0) return Value::Bool(false);
    std::free(reinterpret_cast<void*>(addr));
    return Value::Bool(true);
}

} // namespace ffi_ns

// ═══════════════════════════════════════════════════════════════════
//  注册函数
// ═══════════════════════════════════════════════════════════════════
void StdLib::registerFFI(VM* vm) {
    registerFunction(vm, "加载库",      ffi_ns::ffiLoadLibrary);
    registerFunction(vm, "取函数",      ffi_ns::ffiGetProc);
    registerFunction(vm, "调用整数",    ffi_ns::ffiCallInt);
    registerFunction(vm, "调用浮点",    ffi_ns::ffiCallFloat);
    registerFunction(vm, "调用空",      ffi_ns::ffiCallVoid);
    registerFunction(vm, "调用字符串",  ffi_ns::ffiCallString);
    registerFunction(vm, "调用签名",    ffi_ns::ffiCallSig);
    registerFunction(vm, "错误码",      ffi_ns::ffiLastError);
    registerFunction(vm, "错误信息",    ffi_ns::ffiErrorMsg);
    registerFunction(vm, "释放库",      ffi_ns::ffiFreeLibrary);
    registerFunction(vm, "取字符串指针", ffi_ns::ffiStrPtr);
    registerFunction(vm, "读内存",      ffi_ns::ffiReadMem);
    registerFunction(vm, "写内存",      ffi_ns::ffiWriteMem);
    registerFunction(vm, "地址偏移",    ffi_ns::ffiAddrOffset);
    registerFunction(vm, "读整数",      ffi_ns::ffiReadInt);
    registerFunction(vm, "读8位",       ffi_ns::ffiRead8);
    registerFunction(vm, "读16位",      ffi_ns::ffiRead16);
    registerFunction(vm, "读32位",      ffi_ns::ffiRead32);
    registerFunction(vm, "读浮点",      ffi_ns::ffiReadFloat);
    registerFunction(vm, "写8位",       ffi_ns::ffiWrite8);
    registerFunction(vm, "写16位",      ffi_ns::ffiWrite16);
    registerFunction(vm, "写32位",      ffi_ns::ffiWrite32);
    registerFunction(vm, "取结构体字段", ffi_ns::ffiStructField);
    registerFunction(vm, "分配内存",    ffi_ns::ffiAllocMem);
    registerFunction(vm, "释放内存",    ffi_ns::ffiFreeMem);
    registerFunction(vm, "创建回调",    callback_ns::ffiCreateCallback);
    registerFunction(vm, "释放回调",    callback_ns::ffiFreeCallback);

    // 英文别名
    registerAlias(vm, "loadLibrary",    "加载库");
    registerAlias(vm, "getProcAddress", "取函数");
    registerAlias(vm, "ffiCallInt",     "调用整数");
    registerAlias(vm, "ffiCallFloat",   "调用浮点");
    registerAlias(vm, "ffiCallVoid",    "调用空");
    registerAlias(vm, "ffiCallString",  "调用字符串");
    registerAlias(vm, "ffiCallSig",     "调用签名");
    registerAlias(vm, "readMemory",     "读内存");
    registerAlias(vm, "writeMemory",    "写内存");
    registerAlias(vm, "ptrOffset",      "地址偏移");
    registerAlias(vm, "readInt",        "读整数");
    registerAlias(vm, "readI8",         "读8位");
    registerAlias(vm, "readI16",        "读16位");
    registerAlias(vm, "readI32",        "读32位");
    registerAlias(vm, "readFloat",      "读浮点");
    registerAlias(vm, "writeI8",        "写8位");
    registerAlias(vm, "writeI16",       "写16位");
    registerAlias(vm, "writeI32",       "写32位");
    registerAlias(vm, "structField",    "取结构体字段");
    registerAlias(vm, "allocMemory",    "分配内存");
    registerAlias(vm, "freeMemory",     "释放内存");
    registerAlias(vm, "createCallback", "创建回调");
    registerAlias(vm, "freeCallback",   "释放回调");
    registerAlias(vm, "getLastError",   "错误码");
    registerAlias(vm, "formatError",    "错误信息");
    registerAlias(vm, "freeLibrary",    "释放库");
}

} // namespace cplang
