// CP语言 NaN-boxing 值表示
// 
// 64-bit tagged union:
// - 非 NaN double: 直接存 IEEE 754 double
// - bits 48-63 = 0xFFFF: 标记为非浮点值
//
// 标记值内部布局:
//   0xFFFF_8000_0000_0000 = Nil
//   0xFFFF_8000_0000_0001 = False
//   0xFFFF_8000_0000_0002 = True
//   0xFFFF_C000_0000_iiii = Int8  (低8位)
//   0xFFFF_C000_0001_iiii = Int16 (低16位)
//   0xFFFF_C000_1000_iiii = Int32 (低32位)
//   0xFFFF_C000_2000_ffff = Float32 (装箱)
//   0xFFFF_C000_4000_pppp = 对象指针 (低48位)
//
// 核心优势:
//   - Int8/Int16/Int32/Float32/Float64/Bool/Nil 完全不分配堆内存
//   - 类型检查只需检查 high 16 bits
//   - 指针提取只需位掩码操作
//   - sizeof(Value) = 8 字节 (之前 12+padding=16)
#pragma once

#include "common/types.hpp"
#include <cstring>
#include <string>

namespace cplang {

// ── 前置声明 ──
struct VMObject;
struct VMString;
struct VMArray;
struct VMTable;
struct VMSet;
struct VMStack;
struct VMQueue;
struct VMDeque;
struct VMPriorityQueue;
struct VMLinkedList;
struct VMSLinkedList;
struct VMMultiSet;
struct VMMultiMap;
struct VMUnorderedSet;
struct VMUnorderedMultiSet;
struct VMUnorderedMap;
struct VMUnorderedMultiMap;
struct VMOrderedSet;
struct VMOrderedMap;
struct VMFunction;
struct VMClosure;
struct VMGenerator;  // 生成器（P9.1）
struct VMPromise;    // 承诺（P9.3）
struct VMClass;
struct VMInstance;
struct VMUpvalue;
struct VMByteArray;  // 字节数组（二进制数据）

// ═══════════════════════════════════════════════════════════════════
//  NaN-boxing Value (64-bit)
// ═══════════════════════════════════════════════════════════════════

class Value {
    uint64_t raw_;

    // 编码常量
    // Scheme: bits 48-63 = 0xFFFF → tagged (not double)
    //         bit 47 = 1 → immediate value
    //           bit 46 = 0 → special (Nil/False/True via low bits)
    //           bit 46 = 1 → typed immediate (Int8/16/32, Float32)
    //         bit 47 = 0 → object pointer (bits 0-47 = 48-bit ptr)
    static constexpr uint64_t TAG_SHIFT    = 48;
    static constexpr uint64_t TAG_MASK     = 0xFFFF000000000000ULL;
    static constexpr uint64_t TAG_VALUE    = 0xFFFFULL;
    
    static constexpr uint64_t IMM_BIT      = 0x0000800000000000ULL;  // bit 47
    static constexpr uint64_t TYPED_BIT    = 0x0000400000000000ULL;  // bit 46
    static constexpr uint64_t TYPE_MASK    = 0x00003FFF00000000ULL;  // bits 32-45 for type tag
    
    // Type tags (bits 32-45)
    static constexpr uint64_t TYPE_INT8    = 0x0000000000000000ULL;  // 0000
    static constexpr uint64_t TYPE_INT16   = 0x0000000100000000ULL;  // 0001
    static constexpr uint64_t TYPE_INT32   = 0x0000100000000000ULL;  // 1000
    static constexpr uint64_t TYPE_FLOAT32 = 0x0000200000000000ULL;  // 0010
    static constexpr uint64_t PTR_MASK     = 0x0000FFFFFFFFFFFFULL;  // bits 0-47 for ptr
    
    // 特殊值的完整编码
    static constexpr uint64_t NIL_VAL   = 0xFFFF800000000000ULL;  // tagged, imm, special=0
    static constexpr uint64_t FALSE_VAL = 0xFFFF800000000001ULL;
    static constexpr uint64_t TRUE_VAL  = 0xFFFF800000000002ULL;
    
    // Typed immediate base
    static constexpr uint64_t TYPED_BASE = 0xFFFFC00000000000ULL;  // tagged, imm, typed

public:
    // ── 构造 ──
    constexpr Value() : raw_(NIL_VAL) {}
    explicit constexpr Value(uint64_t raw) : raw_(raw) {}
    
    // ── 工厂方法 ──
    static constexpr Value nil()   { return Value(NIL_VAL); }
    static constexpr Value False() { return Value(FALSE_VAL); }
    static constexpr Value True()  { return Value(TRUE_VAL); }
    static Value Bool(bool b) { return b ? True() : False(); }
    
    // 向后兼容：旧 type tag 常量（T_*）
    static constexpr uint16_t T_NIL = 0;
    static constexpr uint16_t T_BOOL = 1;
    static constexpr uint16_t T_INT8 = 2;
    static constexpr uint16_t T_INT16 = 3;
    static constexpr uint16_t T_INT32 = 4;

    static constexpr uint16_t T_FLOAT32 = 5;
    static constexpr uint16_t T_FLOAT = 6;
    static constexpr uint16_t T_STRING = 7;
    static constexpr uint16_t T_ARRAY = 8;
    static constexpr uint16_t T_TABLE = 9;
    static constexpr uint16_t T_FUNCTION = 10;
    static constexpr uint16_t T_CFUNCTION = 11;
    static constexpr uint16_t T_CLOSURE = 12;
    static constexpr uint16_t T_CLASS = 13;
    static constexpr uint16_t T_INSTANCE = 14;
    static constexpr uint16_t T_SET = 15;
    static constexpr uint16_t T_STACK = 16;
    static constexpr uint16_t T_QUEUE = 17;
    static constexpr uint16_t T_PTR = 18;
    
    // 向后兼容：旧 API 工厂方法
    static Value String(VMString* s)  { return Ptr(reinterpret_cast<VMObject*>(s)); }
    static Value Array(VMArray* a)    { return Ptr(reinterpret_cast<VMObject*>(a)); }
    static Value Table(VMTable* t)    { return Ptr(reinterpret_cast<VMObject*>(t)); }
    static Value Set(VMSet* s)        { return Ptr(reinterpret_cast<VMObject*>(s)); }
    static Value Stack(VMStack* s)    { return Ptr(reinterpret_cast<VMObject*>(s)); }
    static Value Queue(VMQueue* q)    { return Ptr(reinterpret_cast<VMObject*>(q)); }
    static Value Deque(VMDeque* d)    { return Ptr(reinterpret_cast<VMObject*>(d)); }
    static Value PriorityQueue(VMPriorityQueue* p) { return Ptr(reinterpret_cast<VMObject*>(p)); }
    static Value LinkedList(VMLinkedList* l)       { return Ptr(reinterpret_cast<VMObject*>(l)); }
    static Value SLinkedList(VMSLinkedList* l)      { return Ptr(reinterpret_cast<VMObject*>(l)); }
    static Value MultiSet(VMMultiSet* m)            { return Ptr(reinterpret_cast<VMObject*>(m)); }
    static Value MultiMap(VMMultiMap* m)            { return Ptr(reinterpret_cast<VMObject*>(m)); }
    static Value Nil()                { return nil(); }
    static Value Float(double d)      { return fromFloat(d); }
    static Value UnorderedSet(VMUnorderedSet* s)           { return Ptr(reinterpret_cast<VMObject*>(s)); }
    static Value UnorderedMultiSet(VMUnorderedMultiSet* s) { return Ptr(reinterpret_cast<VMObject*>(s)); }
    static Value UnorderedMap(VMUnorderedMap* m)           { return Ptr(reinterpret_cast<VMObject*>(m)); }
    static Value UnorderedMultiMap(VMUnorderedMultiMap* m) { return Ptr(reinterpret_cast<VMObject*>(m)); }
    
    // 整数类型：内联存储（零开销，无堆分配）
    static Value fromInt8(Int8 x) {
        uint64_t raw = TYPED_BASE | TYPE_INT8;
        raw |= (static_cast<uint64_t>(static_cast<uint8_t>(x)) & 0xFFULL);
        return Value(raw);
    }
    static Value fromInt16(Int16 x) {
        uint64_t raw = TYPED_BASE | TYPE_INT16;
        raw |= (static_cast<uint64_t>(static_cast<uint16_t>(x)) & 0xFFFFULL);
        return Value(raw);
    }
    static Value fromInt32(Int32 x) {
        uint64_t raw = TYPED_BASE | TYPE_INT32;
        raw |= (static_cast<uint64_t>(static_cast<uint32_t>(x)));
        return Value(raw);
    }
    static Value Int(Int64 x) {
        if (x >= static_cast<Int64>(INT32_MIN) && x <= static_cast<Int64>(INT32_MAX))
            return fromInt32(static_cast<Int32>(x));
        return boxInt64(x);
    }  // 通用Int入口（Int32以内内联，更大值装箱）
    
    // Float32: 内联存储（零开销，无堆分配）
    static Value fromFloat32(Float32 x) {
        uint64_t raw = TYPED_BASE | TYPE_FLOAT32;
        uint32_t fbits;
        std::memcpy(&fbits, &x, sizeof(fbits));
        raw |= static_cast<uint64_t>(fbits);
        return Value(raw);
    }
    
    // Int64: 装箱存储（堆分配）——用 boxInt64 避免和 Int64 类型别名冲突
    static Value boxInt64(::cplang::Int64 x);
    
    // Float64: 直接存 IEEE 754 double（若不是特殊NaN模式）
    static Value fromFloat(::cplang::Float64 x) {
        uint64_t raw;
        std::memcpy(&raw, &x, sizeof(::cplang::Float64));
        if ((raw >> TAG_SHIFT) == TAG_VALUE) {
            return boxFloat(x);
        }
        return Value(raw);  // MSVC 兼容: 不在类内创建 Value 局部变量
    }
    static Value fromFloat64(::cplang::Float64 x) { return fromFloat(x); }  // 显式版本
    
    // Object: 存储48位指针 (bit 47=0 表示指针)
    static Value Ptr(VMObject* obj) {
        uint64_t raw = (TAG_VALUE << TAG_SHIFT);  // 0xFFFF_0000_0000_0000
        raw |= (reinterpret_cast<uint64_t>(obj) & PTR_MASK);  // 低48位=指针
        return Value(raw);
    }
    
    // 从旧API兼容的工厂（逐步迁移用）
    static Value fromOldInt(Int64 x) {
        if (x >= INT32_MIN && x <= INT32_MAX) return Int(static_cast<int32_t>(x));
        return boxInt64(x);
    }
    
    // ── 类型检测 ──
    bool isDouble() const { return (raw_ >> TAG_SHIFT) != TAG_VALUE; }
    bool isFloat()  const { return isDouble(); }
    
    // Typed immediate checks
    bool isTypedImmediate() const {
        return (raw_ & (TAG_MASK | IMM_BIT | TYPED_BIT)) == TYPED_BASE;
    }
    
    uint64_t getTypeTag() const {
        return raw_ & TYPE_MASK;
    }
    
    // 各具体整型类型
    bool isInt8()  const { return isTypedImmediate() && getTypeTag() == TYPE_INT8; }
    bool isInt16() const { return isTypedImmediate() && getTypeTag() == TYPE_INT16; }
    bool isInt32() const { return isTypedImmediate() && getTypeTag() == TYPE_INT32; }
    bool isInt()   const { return isInt8() || isInt16() || isInt32(); }  // 通用整型
    bool isFloat32() const { return isTypedImmediate() && getTypeTag() == TYPE_FLOAT32; }
    bool isFloat64() const { return isDouble(); }
    
    // Tagged + bit47=0 (NOT imm) → pointer. Int64需要额外检查ObjectHeader类型
    bool isInt64()  const;  // 定义在 value.cpp（需 ObjectHeader）
    // Tagged + bit47=0 → pointer (object/null)
    bool isPtr()    const { return (raw_ >> TAG_SHIFT) == TAG_VALUE && !(raw_ & IMM_BIT); }
    bool isNil()    const { return raw_ == NIL_VAL; }
    bool isBool()   const { return raw_ == FALSE_VAL || raw_ == TRUE_VAL; }
    bool isNumber() const { return isDouble() || isInt() || isFloat32() || isInt64(); }
    bool isObject() const;  // defined in vm.hpp (depends on ObjectHeader)
    
    // 对象类型检测 (定义在 vm.hpp, 依赖 ObjectHeader::TAG_*)
    bool isString() const;
    bool isArray() const;
    bool isTable() const;
    bool isSet() const;
    bool isStack() const;
    bool isQueue() const;
    bool isDeque() const;
    bool isPriorityQueue() const;
    bool isLinkedList() const;
    bool isSLinkedList() const;
    bool isMultiSet() const;
    bool isMultiMap() const;
    bool isUnorderedSet() const;
    bool isUnorderedMultiSet() const;
    bool isUnorderedMap() const;
    bool isUnorderedMultiMap() const;
    bool isOrderedSet() const;
    bool isOrderedMap() const;
    bool isFunction() const;
    bool isClosure() const;
    bool isClass() const;
    bool isInstance() const;
    bool isUpvalue() const;
    bool isCFunction() const;
    bool isUserData() const;
    bool isRaylib() const;
    bool isThread() const;
    bool isMutex() const;
    bool isCondition() const;
    bool isSemaphore() const;
    bool isAtomicInt() const;
    bool isChannel() const;
    bool isWebSocket() const;
    bool isGenerator() const;  // 生成器（P9.1）
    bool isPromise() const;    // 承诺（P9.3）
    bool isByteArray() const;  // 字节数组
    
    // 对象 as*() 访问器
    VMString*   asString() const;
    VMArray*    asArray() const;
    VMTable*    asTable() const;
    VMSet*      asSet() const;
    VMStack*    asStack() const;
    VMQueue*    asQueue() const;
    VMDeque*    asDeque() const;
    VMPriorityQueue* asPriorityQueue() const;
    VMLinkedList*    asLinkedList() const;
    VMSLinkedList*   asSLinkedList() const;
    VMMultiSet*      asMultiSet() const;
    VMMultiMap*      asMultiMap() const;
    VMUnorderedSet*        asUnorderedSet() const;
    VMUnorderedMultiSet*   asUnorderedMultiSet() const;
    VMUnorderedMap*        asUnorderedMap() const;
    VMUnorderedMultiMap*   asUnorderedMultiMap() const;
    VMOrderedSet*          asOrderedSet() const;
    VMOrderedMap*          asOrderedMap() const;
    VMFunction* asFunction() const;
    VMClosure*  asClosure() const;
    VMGenerator* asGenerator() const;
    VMPromise* asPromise() const;
    VMByteArray* asByteArray() const;
    VMObject*   asUserData() const;
    
    // ── 取值 ──
    Int8    asInt8()   const;
    Int16   asInt16()  const;
    Int32   asInt32()  const;
    Int64   asInt()    const;  // 通用整数读取
    Float32 asFloat32() const;
    Float64 asFloat()  const;  // isDouble→解包, isInt→转换, isInt64→转换
    Float64 asFloat64() const { return asFloat(); }
    
    bool    asBool()   const { return raw_ == TRUE_VAL; }
    VMObject* asPtr()  const { return reinterpret_cast<VMObject*>(raw_ & PTR_MASK); }
    
    // ── 真值判断 ──
    bool isTrue() const {
        if (isNil())   return false;
        if (isBool())  return raw_ == TRUE_VAL;
        if (isInt8() || isInt16() || isInt32()) return asInt() != 0;
        if (isFloat32()) return asFloat32() != 0.0f;
        if (isInt64()) return asInt() != 0;
        if (isDouble()) return asFloat() != 0.0;
        return true;  // 对象/字符串为真
    }
    
    // ── 比较 ──
    bool equals(const Value& other) const;
    
    // ── 调试 ──
    std::string toString() const;
    uint64_t raw() const { return raw_; }
    
    // ── 用于容器（unordered_map/unordered_set） ──
    bool operator==(const Value& other) const { return equals(other); }
    
    // 向后兼容：旧 API 字段访问（仅 MSVC，其他编译器应替换为 .asXxx() 方法）
#ifdef _MSC_VER
    uint16_t backwardTag() const;   // 定义在 vm.hpp
    Int64 backwardI() const { return asInt(); }
    VMObject* backwardObj() const { return asPtr(); }
    __declspec(property(get=backwardTag)) uint16_t tag;
    __declspec(property(get=backwardI))   Int64 i;
    __declspec(property(get=backwardObj)) VMObject* obj;
#endif
    
private:
    static Value boxFloat(Float64 x);
};

// ═══════════════════════════════════════════════════════════════════
//  Hash / Equal 函子 (需配合新的 Value)
// ═══════════════════════════════════════════════════════════════════

struct ValueHash {
    size_t operator()(const Value& v) const;  // defined in vm.hpp (needs VMString)
};

struct ValueEqual {
    bool operator()(const Value& a, const Value& b) const {
        return a.equals(b);
    }
};

struct ValueLess {
    bool operator()(const Value& a, const Value& b) const;
};

} // namespace cplang