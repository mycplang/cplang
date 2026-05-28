// CP语言 高性能虚拟机核心 — 优化版本
// 核心优化：
// 1. 指令格式从16字节压缩到4字节（4倍缓存命中率）
// 2. 全局变量Slot化（消除哈希查找）
// 3. Direct Threading（computed goto消除switch开销）
// 4. 函数调用零拷贝（消除args vector）
// 5. 寄存器分配优化（循环内回收）

#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <cstring>

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  基础类型
// ═══════════════════════════════════════════════════════════════════
using UInt8  = uint8_t;
using UInt16 = uint16_t;
using UInt32 = uint32_t;
using UInt64 = uint64_t;
using Int8   = int8_t;
using Int16  = int16_t;
using Int32  = int32_t;
using Int64  = int64_t;
using Float64 = double;

// ═══════════════════════════════════════════════════════════════════
//  值类型
// ═══════════════════════════════════════════════════════════════════
enum ValueTag : UInt8 {
    T_NIL = 0, T_BOOL = 1, T_INT = 2, T_FLOAT = 3,
    T_STRING = 4, T_ARRAY = 5, T_TABLE = 6, T_FUNCTION = 7,
    T_CLOSURE = 8, T_CLASS = 9, T_INSTANCE = 10, T_NATIVE = 11,
    T_UPVALUE = 12
};

struct VMObject;
struct VMString;
struct VMArray;
struct VMTable;
struct VMFunction;
struct VMClosure;
struct VMNativeFunc;

struct Value {
    ValueTag tag;
    union {
        Int64 i;
        Float64 f;
        VMObject* obj;
        VMString* str;
        VMArray* arr;
        VMTable* tbl;
        VMFunction* func;
        VMClosure* closure;
        VMNativeFunc* native;
    };
    
    static Value nil()  { Value v; v.tag = T_NIL; v.i = 0; return v; }
    static Value Bool(bool b) { Value v; v.tag = T_BOOL; v.i = b ? 1 : 0; return v; }
    static Value Int(Int64 n) { Value v; v.tag = T_INT; v.i = n; return v; }
    static Value Float(Float64 n) { Value v; v.tag = T_FLOAT; v.f = n; return v; }
    
    bool isNil() const { return tag == T_NIL; }
    bool isBool() const { return tag == T_BOOL; }
    bool isInt() const { return tag == T_INT; }
    bool isFloat() const { return tag == T_FLOAT; }
    bool isNumber() const { return tag == T_INT || tag == T_FLOAT; }
    bool isString() const { return tag == T_STRING; }
    bool isArray() const { return tag == T_ARRAY; }
    bool isTable() const { return tag == T_TABLE; }
    bool isFunction() const { return tag == T_FUNCTION || tag == T_CLOSURE; }
    bool isNative() const { return tag == T_NATIVE; }
    bool isObject() const { return tag >= T_STRING; }
    
    bool isTrue() const {
        if (tag == T_BOOL) return i != 0;
        if (tag == T_INT) return i != 0;
        if (tag == T_FLOAT) return f != 0.0;
        return tag != T_NIL;
    }
    
    Int64 asInt() const { return tag == T_FLOAT ? static_cast<Int64>(f) : i; }
    Float64 asFloat() const { return tag == T_FLOAT ? f : static_cast<Float64>(i); }
    double toNumber() const { return tag == T_FLOAT ? f : static_cast<double>(i); }
    
    bool equals(const Value& other) const;
};

// ═══════════════════════════════════════════════════════════════════
//  VM对象
// ═══════════════════════════════════════════════════════════════════
struct VMObject {
    UInt8 typeTag;
    bool marked = false;
    VMObject* next = nullptr;
};

struct VMString : VMObject {
    UInt32 hash = 0;
    UInt32 length = 0;
    char data[1];
    static VMString* create(const char* s, UInt32 len);
    static VMString* create(const std::string& s);
    const char* c_str() const { return data; }
};

// Value::equals (inline — 必须在 VMString 完整定义之后)
inline bool Value::equals(const Value& other) const {
    if (tag != other.tag) {
        if (isNumber() && other.isNumber()) {
            return toNumber() == other.toNumber();
        }
        return false;
    }
    switch (tag) {
        case T_NIL: return true;
        case T_BOOL:
        case T_INT: return i == other.i;
        case T_FLOAT: return f == other.f;
        case T_STRING: {
            if (str->length != other.str->length) return false;
            return std::memcmp(str->data, other.str->data, str->length) == 0;
        }
        default: return obj == other.obj;
    }
}

struct VMArray : VMObject {
    std::vector<Value> data;
    static VMArray* create(UInt32 cap = 0);
    Value get(Int64 index);
    void set(Int64 index, const Value& v);
    Int64 length() const { return static_cast<Int64>(data.size()); }
};

struct VMTable : VMObject {
    std::vector<std::pair<Value, Value>> data;
    static VMTable* create();
    Value get(const Value& key);
    void set(const Value& key, const Value& val);
};

// ═══════════════════════════════════════════════════════════════════
//  函数对象
// ═══════════════════════════════════════════════════════════════════
struct VMFunction : VMObject {
    VMString* name = nullptr;
    UInt32 maxStack = 256;
    UInt32 numParams = 0;
    UInt32 numLocals = 0;
    std::vector<UInt8> code;      // 4字节指令格式
    std::vector<Value> constants;
    std::vector<Int32> lineInfo;
    std::vector<UInt16> globalSlots;  // 全局变量slot映射（编译期确定）
    
    // 指令编码辅助函数
    void emit(UInt8 op, UInt8 a, UInt8 b, UInt8 c);
    void emitI(UInt8 op, UInt8 a, Int16 sbx);  // I格式：立即数/跳转
};

struct VMClosure : VMObject {
    VMFunction* func = nullptr;
    std::vector<struct VMUpvalue*> upvalues;
    static VMClosure* create(VMFunction* f);
};

struct VMUpvalue : VMObject {
    Value* location = nullptr;
    Value closed;
    VMUpvalue* next = nullptr;
    static VMUpvalue* create(Value* slot);
};

struct VMNativeFunc : VMObject {
    using Fn = std::function<Value(int argc, Value* argv)>;  // 零拷贝接口
    Fn fn;
    VMString* name = nullptr;
};

// ═══════════════════════════════════════════════════════════════════
//  字节码指令（4字节格式）
// ═══════════════════════════════════════════════════════════════════
// 格式A: [op:8][a:8][b:8][c:8]     — 三寄存器操作
// 格式I: [op:8][a:8][sbx:16]        — 立即数/跳转（有符号16位）

enum Opcode : UInt8 {
    OP_LOADNIL=0x00, OP_LOADBOOL=0x01, OP_LOADINT=0x02, OP_LOADFLT=0x03,
    OP_LOADSTR=0x04, OP_LOADCONST=0x05, OP_MOVE=0x06,
    OP_LOADGLOBAL=0x07, OP_STOREGLOBAL=0x08,
    OP_LOADLOCAL=0x10, OP_STORELOCAL=0x11,
    OP_ADD=0x20, OP_SUB=0x21, OP_MUL=0x22, OP_DIV=0x23,
    OP_IDIV=0x24, OP_MOD=0x25, OP_POW=0x26, OP_NEG=0x27,
    OP_BAND=0x28, OP_BOR=0x29, OP_BXOR=0x2A,
    OP_BSHL=0x2B, OP_BSHR=0x2C, OP_BNOT=0x2D,
    OP_CMPEQ=0x30, OP_CMPNE=0x31,
    OP_CMPLT=0x32, OP_CMPLE=0x33, OP_CMPGT=0x34, OP_CMPGE=0x35,
    OP_JUMP=0x40, OP_JUMPIF=0x41, OP_JUMPNIF=0x42,
    OP_CALL=0x50, OP_RETURN=0x52,
    OP_NEWARRAY=0x60, OP_GETELEM=0x62, OP_SETELEM=0x63,
    OP_GETIDX=0x64, OP_SETIDX=0x65,
    OP_CONCAT=0x68, OP_STRLEN=0x69,
    OP_TONUM=0x70, OP_TOSTR=0x71, OP_TOBOOL=0x72,
    OP_TYPEOF=0x73, OP_ISNULL=0x74,
    OP_NEWCLASS=0x80,
    OP_NOP=0xFF,
};

// ═══════════════════════════════════════════════════════════════════
//  调用帧（优化版）
// ═══════════════════════════════════════════════════════════════════
struct CallFrame {
    VMFunction* func = nullptr;
    VMClosure* closure = nullptr;
    Value* base = nullptr;        // 寄存器基址
    const UInt8* pc = nullptr;    // 程序计数器
    const UInt8* retPC = nullptr; // 返回地址
    Value* retBase = nullptr;     // 返回时的基址
    int retReg = 0;               // 返回值寄存器
};

// ═══════════════════════════════════════════════════════════════════
//  高性能VM
// ═══════════════════════════════════════════════════════════════════
class VM {
public:
    static constexpr int MAX_STACK = 65536;
    static constexpr int GC_THRESHOLD = 1024 * 1024;
    static constexpr int MAX_GLOBALS = 65535;  // 全局变量slot上限

    VM();
    ~VM();
    
    // 执行函数
    bool call(VMFunction* func, int argc = 0, Value* argv = nullptr);
    
    // 注册全局
    void registerGlobal(const char* name, Value val);
    void registerNative(const char* name, VMNativeFunc::Fn fn);
    Int32 getGlobalSlot(const char* name) {
        auto it = globalNames_.find(name);
        if (it != globalNames_.end()) return it->second;
        if (nextGlobalSlot_ >= MAX_GLOBALS) { raiseError("too many globals"); return -1; }
        UInt16 slot = nextGlobalSlot_++;
        globalNames_[name] = slot;
        globalSlots_.resize(slot + 1);
        return slot;
    }
    
    // 状态查询
    const std::string& error() const { return error_; }
    bool hasError() const { return !error_.empty(); }
    Int64 totalInstructions() const { return instructionCount_; }
    
    // GC接口
    VMString* internString(const char* s, UInt32 len);
    void trackGC(VMObject* obj);
    void gc();

private:
    // 核心执行循环（Direct Threading实现）
    bool run();
    
    // GC实现
    void gcMarkRoots();
    void gcMarkObject(VMObject* obj);
    void gcMarkValue(const Value& v);
    void gcSweep();
    
    // 辅助函数
    bool callNative(VMNativeFunc* nf, int argc, Value* argv, Value* out);
    void raiseError(const char* msg) { error_ = msg; }

    // 执行状态
    std::vector<Value> stack_;           // 值栈
    Value* base_ = nullptr;              // 当前寄存器基址
    const UInt8* pc_ = nullptr;          // 程序计数器
    VMFunction* func_ = nullptr;         // 当前函数
    
    // 调用栈
    CallFrame frames_[64];               // 固定大小调用栈（足够深）
    int frameCount_ = 0;
    
    // 全局变量（Slot化）
    std::vector<Value> globalSlots_;     // 全局变量值
    std::unordered_map<std::string, UInt16> globalNames_;  // 名字→slot映射
    UInt16 nextGlobalSlot_ = 0;
    
    // GC状态
    VMObject* allObjects_ = nullptr;
    size_t gcAllocated_ = 0;
    Int64 gcCount_ = 0;
    bool gcRunning_ = false;
    
    // 字符串驻留
    std::unordered_map<std::string, VMString*> stringTable_;
    
    // 统计
    Int64 instructionCount_ = 0;
    std::string error_;
};

} // namespace cplang
