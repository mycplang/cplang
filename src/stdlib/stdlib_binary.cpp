// CP语言标准库 — 字节数组（二进制数据）模块
#include "stdlib/stdlib.hpp"
#include "stdlib/stdlib_binary.hpp"
#include <cstring>
#include <sstream>
#include <iomanip>

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  注册
// ═══════════════════════════════════════════════════════════════════

void StdLib::registerBinary(VM* vm) {
    // ── 创建 ──
    registerFunction(vm, "byteArrayCreate",   binary::create);
    registerAlias(vm, "创建字节数组",          "byteArrayCreate");
    registerFunction(vm, "byteArrayFromStr",  binary::fromString);
    registerAlias(vm, "字节数组自字符串",       "byteArrayFromStr");
    registerFunction(vm, "byteArrayFromArray", binary::fromArray);
    registerAlias(vm, "字节数组自数组",         "byteArrayFromArray");
    registerFunction(vm, "byteArrayFromHex",  binary::fromHex);
    registerAlias(vm, "字节数组自十六进制",      "byteArrayFromHex");

    // ── 属性 ──
    registerFunction(vm, "byteArrayLen",      binary::len);
    registerAlias(vm, "字节数组长度",           "byteArrayLen");
    registerFunction(vm, "byteArrayCap",      binary::cap);
    registerAlias(vm, "字节数组容量",           "byteArrayCap");

    // ── 读写 ──
    registerFunction(vm, "byteArrayGet",      binary::get);
    registerAlias(vm, "字节数组获取",           "byteArrayGet");
    registerFunction(vm, "byteArraySet",      binary::set);
    registerAlias(vm, "字节数组设置",           "byteArraySet");
    registerFunction(vm, "byteArrayFill",     binary::fill);
    registerAlias(vm, "字节数组填充",           "byteArrayFill");
    registerFunction(vm, "byteArrayCopy",     binary::copy);
    registerAlias(vm, "字节数组拷贝",           "byteArrayCopy");

    // ── 视图 ──
    registerFunction(vm, "byteArraySlice",    binary::slice);
    registerAlias(vm, "字节数组切片",           "byteArraySlice");

    // ── 转换 ──
    registerFunction(vm, "byteArrayToStr",    binary::toString);
    registerAlias(vm, "字节数组转字符串",        "byteArrayToStr");
    registerFunction(vm, "byteArrayToArray",  binary::toArray);
    registerAlias(vm, "字节数组转数组",          "byteArrayToArray");
    registerFunction(vm, "byteArrayToHex",    binary::toHex);
    registerAlias(vm, "字节数组转十六进制",       "byteArrayToHex");

    // ── 写入 ──
    registerFunction(vm, "byteArrayWrite",    binary::write);
    registerAlias(vm, "字节数组写入",           "byteArrayWrite");
    registerFunction(vm, "byteArrayAppend",   binary::append);
    registerAlias(vm, "字节数组追加",           "byteArrayAppend");

    // ── 比较 ──
    registerFunction(vm, "byteArrayCompare",  binary::compare);
    registerAlias(vm, "字节数组比较",           "byteArrayCompare");

    // ── 调整 ──
    registerFunction(vm, "byteArrayResize",   binary::resize);
    registerAlias(vm, "字节数组设置长度",        "byteArrayResize");
}

// ═══════════════════════════════════════════════════════════════════
//  辅助函数
// ═══════════════════════════════════════════════════════════════════

namespace {

/// 把整数参数（asInt()）转成 UInt32，超出范围时钳制
inline UInt32 toUInt32(Int64 v) {
    if (v < 0) return 0;
    if (v > 0xFFFFFFFFLL) return 0xFFFFFFFF;
    return static_cast<UInt32>(v);
}

/// 十六进制字符→数字（-1=无效）
inline int hexDigit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

} // anonymous

// ═══════════════════════════════════════════════════════════════════
//  创建
// ═══════════════════════════════════════════════════════════════════

Value binary::create(std::vector<Value>& args) {
    if (args.empty()) return makeByteArrayVal(VMByteArray::create(0));
    UInt32 size = toUInt32(args[0].asInt());
    return makeByteArrayVal(VMByteArray::create(size));
}

Value binary::fromString(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    VMString* s = args[0].asString();
    UInt32 len = s->length;
    VMByteArray* ba = VMByteArray::create(len);
    if (len > 0) {
        std::memcpy(ba->data, s->data, len);
    }
    return makeByteArrayVal(ba);
}

Value binary::fromArray(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::nil();
    VMArray* arr = args[0].asArray();
    UInt32 len = static_cast<UInt32>(arr->data.size());
    VMByteArray* ba = VMByteArray::create(len);
    for (UInt32 i = 0; i < len; i++) {
        ba->data[i] = static_cast<UInt8>(arr->data[i].asInt() & 0xFF);
    }
    return makeByteArrayVal(ba);
}

Value binary::fromHex(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    VMString* s = args[0].asString();
    const char* p = s->data;
    UInt32 slen = s->length;
    
    // 估算所需空间（每2个十六进制字符→1字节）
    UInt32 estLen = slen / 2 + 1;
    VMByteArray* ba = VMByteArray::create(0);
    ba->ensure(estLen);
    
    UInt32 pos = 0;
    int hi = -1;
    for (UInt32 i = 0; i < slen; i++) {
        int d = hexDigit(p[i]);
        if (d < 0) continue;  // 跳过非十六进制字符
        if (hi < 0) {
            hi = d;
        } else {
            ba->data[pos++] = static_cast<UInt8>((hi << 4) | d);
            hi = -1;
        }
    }
    ba->length = pos;
    return makeByteArrayVal(ba);
}

// ═══════════════════════════════════════════════════════════════════
//  属性
// ═══════════════════════════════════════════════════════════════════

Value binary::len(std::vector<Value>& args) {
    if (args.empty() || !args[0].isByteArray()) return Value::Int(0);
    return Value::Int(args[0].asByteArray()->length);
}

Value binary::cap(std::vector<Value>& args) {
    if (args.empty() || !args[0].isByteArray()) return Value::Int(0);
    VMByteArray* ba = args[0].asByteArray();
    // 对于切片，容量即其可见长度
    return Value::Int(ba->parent ? ba->length : ba->capacity);
}

// ═══════════════════════════════════════════════════════════════════
//  读写
// ═══════════════════════════════════════════════════════════════════

Value binary::get(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isByteArray()) return Value::Int(0);
    VMByteArray* ba = args[0].asByteArray();
    UInt32 idx = toUInt32(args[1].asInt());
    return Value::fromInt8(static_cast<Int8>(ba->get(idx)));
}

Value binary::set(std::vector<Value>& args) {
    if (args.size() < 3 || !args[0].isByteArray()) return Value::nil();
    VMByteArray* ba = args[0].asByteArray();
    UInt32 idx = toUInt32(args[1].asInt());
    UInt8 val = static_cast<UInt8>(args[2].asInt() & 0xFF);
    ba->set(idx, val);
    return Value::nil();
}

Value binary::fill(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isByteArray()) return Value::nil();
    VMByteArray* ba = args[0].asByteArray();
    UInt8 val = static_cast<UInt8>(args[1].asInt() & 0xFF);
    UInt32 off = (args.size() > 2) ? toUInt32(args[2].asInt()) : 0;
    UInt32 len = (args.size() > 3) ? toUInt32(args[3].asInt()) : (ba->length > off ? ba->length - off : 0);
    if (ba->parent) return Value::nil();  // 切片不可写入
    if (off >= ba->length) return Value::nil();
    if (off + len > ba->length) len = ba->length - off;
    std::memset(ba->data + off, val, len);
    return Value::nil();
}

Value binary::copy(std::vector<Value>& args) {
    if (args.empty() || !args[0].isByteArray()) return Value::nil();
    VMByteArray* src = args[0].asByteArray();
    UInt32 off = (args.size() > 1) ? toUInt32(args[1].asInt()) : 0;
    UInt32 len = (args.size() > 2) ? toUInt32(args[2].asInt()) : (src->length > off ? src->length - off : 0);
    if (off >= src->length) return makeByteArrayVal(VMByteArray::create(0));
    if (off + len > src->length) len = src->length - off;
    VMByteArray* dst = VMByteArray::create(len);
    const UInt8* p = src->ptr();
    std::memcpy(dst->data, p + off, len);
    return makeByteArrayVal(dst);
}

// ═══════════════════════════════════════════════════════════════════
//  视图
// ═══════════════════════════════════════════════════════════════════

Value binary::slice(std::vector<Value>& args) {
    if (args.size() < 3 || !args[0].isByteArray()) return Value::nil();
    VMByteArray* ba = args[0].asByteArray();
    UInt32 off = toUInt32(args[1].asInt());
    UInt32 len = toUInt32(args[2].asInt());
    VMByteArray* sl = VMByteArray::createSlice(ba, off, len);
    if (!sl) return Value::nil();
    return makeByteArrayVal(sl);
}

// ═══════════════════════════════════════════════════════════════════
//  转换
// ═══════════════════════════════════════════════════════════════════

Value binary::toString(std::vector<Value>& args) {
    if (args.empty() || !args[0].isByteArray()) {
        return makeStringVal(VMString::create(""));
    }
    VMByteArray* ba = args[0].asByteArray();
    const UInt8* p = ba->ptr();
    // 默认截断到第一个 null 字节以兼容 C 字符串
    UInt32 len = ba->length;
    for (UInt32 i = 0; i < len; i++) {
        if (p[i] == 0) { len = i; break; }
    }
    return makeStringVal(VMString::create(reinterpret_cast<const char*>(p), len));
}

Value binary::toArray(std::vector<Value>& args) {
    if (args.empty() || !args[0].isByteArray()) return makeArrayVal(VMArray::create());
    VMByteArray* ba = args[0].asByteArray();
    const UInt8* p = ba->ptr();
    VMArray* arr = VMArray::create();
    for (UInt32 i = 0; i < ba->length; i++) {
        arr->data.push_back(Value::Int(p[i]));
    }
    return makeArrayVal(arr);
}

Value binary::toHex(std::vector<Value>& args) {
    if (args.empty() || !args[0].isByteArray()) {
        return makeStringVal(VMString::create(""));
    }
    VMByteArray* ba = args[0].asByteArray();
    const char* sep = "";
    if (args.size() > 1 && args[1].isString()) {
        sep = args[1].asString()->c_str();
    }
    const UInt8* p = ba->ptr();
    std::ostringstream oss;
    for (UInt32 i = 0; i < ba->length; i++) {
        if (i > 0) oss << sep;
        oss << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
            << static_cast<int>(p[i]);
    }
    std::string s = oss.str();
    return makeStringVal(VMString::create(s.c_str(), static_cast<UInt32>(s.size())));
}

// ═══════════════════════════════════════════════════════════════════
//  写入
// ═══════════════════════════════════════════════════════════════════

Value binary::write(std::vector<Value>& args) {
    if (args.size() < 3 || !args[0].isByteArray()) return Value::Int(0);
    VMByteArray* ba = args[0].asByteArray();
    if (ba->parent) return Value::Int(0);  // 切片不可写入
    UInt32 off = toUInt32(args[1].asInt());
    
    const UInt8* srcData = nullptr;
    UInt32 srcLen = 0;
    if (args[2].isString()) {
        VMString* s = args[2].asString();
        srcData = reinterpret_cast<const UInt8*>(s->data);
        srcLen = s->length;
    } else if (args[2].isByteArray()) {
        VMByteArray* src = args[2].asByteArray();
        srcData = src->ptr();
        srcLen = src->length;
    } else {
        return Value::Int(0);
    }
    
    if (srcLen == 0) return Value::Int(0);
    // 确保容量足够
    if (off + srcLen > ba->capacity) ba->ensure(off + srcLen);
    std::memcpy(ba->data + off, srcData, srcLen);
    if (off + srcLen > ba->length) ba->length = off + srcLen;
    return Value::Int(srcLen);
}

Value binary::append(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isByteArray()) return Value::nil();
    VMByteArray* ba = args[0].asByteArray();
    if (ba->parent) return Value::nil();  // 切片不可追加
    
    const UInt8* srcData = nullptr;
    UInt32 srcLen = 0;
    if (args[1].isByteArray()) {
        VMByteArray* src = args[1].asByteArray();
        srcData = src->ptr();
        srcLen = src->length;
    } else if (args[1].isString()) {
        VMString* s = args[1].asString();
        srcData = reinterpret_cast<const UInt8*>(s->data);
        srcLen = s->length;
    } else {
        return Value::nil();
    }
    
    if (srcLen == 0) return makeByteArrayVal(ba);
    UInt32 oldLen = ba->length;
    ba->resize(oldLen + srcLen);
    std::memcpy(ba->data + oldLen, srcData, srcLen);
    return makeByteArrayVal(ba);
}

// ═══════════════════════════════════════════════════════════════════
//  比较
// ═══════════════════════════════════════════════════════════════════

Value binary::compare(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Int(0);
    if (!args[0].isByteArray() || !args[1].isByteArray()) return Value::Int(0);
    VMByteArray* a = args[0].asByteArray();
    VMByteArray* b = args[1].asByteArray();
    const UInt8* ap = a->ptr();
    const UInt8* bp = b->ptr();
    UInt32 minLen = a->length < b->length ? a->length : b->length;
    for (UInt32 i = 0; i < minLen; i++) {
        if (ap[i] < bp[i]) return Value::Int(-1);
        if (ap[i] > bp[i]) return Value::Int(1);
    }
    if (a->length < b->length) return Value::Int(-1);
    if (a->length > b->length) return Value::Int(1);
    return Value::Int(0);
}

// ═══════════════════════════════════════════════════════════════════
//  调整
// ═══════════════════════════════════════════════════════════════════

Value binary::resize(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isByteArray()) return Value::nil();
    VMByteArray* ba = args[0].asByteArray();
    UInt32 newSize = toUInt32(args[1].asInt());
    ba->resize(newSize);
    return Value::nil();
}

} // namespace cplang
