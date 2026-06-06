// CP语言 NaN-boxing Value 实现 (v2: fix pointer/subtype overlap)
#include "vm/value.hpp"
#include "vm/vm.hpp"  // for VMString internals (equals, hash)
#include <sstream>
#include <cmath>

namespace cplang {

// 向后兼容：返回旧版类型 tag（0-11 旧编号，用于 __declspec(property) shim）
// 旧系统: NIL=0, BOOL=1, INT=2, FLOAT=3, STRING=4, ARRAY=5, TABLE=6,
//          FUNCTION=7, CFUNCTION=8, CLOSURE=9, CLASS=10, INSTANCE=11
uint16_t Value::backwardTag() const {
    if (isNil()) return 0;
    if (isBool()) return 1;
    if (isInt8() || isInt16() || isInt32()) return 2;  // old T_INT
    if (isInt64()) return 2;
    if (isFloat32() || isFloat64()) return 3;           // old T_FLOAT
    if (isString()) return 4;
    if (isArray()) return 5;
    if (isTable()) return 6;
    if (isFunction()) return 7;
    if (isCFunction()) return 8;
    if (isClosure()) return 9;
    if (isClass()) return 10;
    if (isInstance()) return 11;
    return 0;
}

struct BoxedInt64 : VMObject {
    Int64 value;
    BoxedInt64() { typeTag = ObjectHeader::TAG_BOXED_INT64; }
    static BoxedInt64* create(Int64 x) {
        auto* b = new BoxedInt64();
        b->value = x;
        b->size = sizeof(BoxedInt64);
        return b;
    }
};

struct BoxedFloat : VMObject {
    Float64 value;
    BoxedFloat() { typeTag = ObjectHeader::TAG_BOXED_FLOAT; }
    static BoxedFloat* create(Float64 x) {
        auto* b = new BoxedFloat();
        b->value = x;
        b->size = sizeof(BoxedFloat);
        return b;
    }
};

Value Value::boxInt64(::cplang::Int64 x) {
    auto* box = BoxedInt64::create(x);
    uint64_t raw = (TAG_VALUE << TAG_SHIFT);
    raw |= (reinterpret_cast<uint64_t>(box) & PTR_MASK);
    return Value(raw);
}

Value Value::boxFloat(Float64 x) {
    auto* box = BoxedFloat::create(x);
    uint64_t raw = (TAG_VALUE << TAG_SHIFT);
    raw |= (reinterpret_cast<uint64_t>(box) & PTR_MASK);
    return Value(raw);
}

bool Value::isInt64() const {
    return isPtr() && asPtr() != nullptr && asPtr()->typeTag == ObjectHeader::TAG_BOXED_INT64;
}

Int8 Value::asInt8() const {
    if (isInt8()) return static_cast<Int8>(static_cast<int8_t>(raw_ & 0xFFULL));
    if (isInt16()) return static_cast<Int8>(asInt16());
    if (isInt32()) return static_cast<Int8>(asInt32());
    if (isFloat32()) return static_cast<Int8>(asFloat32());
    if (isFloat64()) return static_cast<Int8>(asFloat());
    return 0;
}

Int16 Value::asInt16() const {
    if (isInt16()) return static_cast<Int16>(static_cast<int16_t>(raw_ & 0xFFFFULL));
    if (isInt8()) return asInt8();
    if (isInt32()) return asInt32();
    if (isFloat32()) return static_cast<Int16>(asFloat32());
    if (isFloat64()) return static_cast<Int16>(asFloat());
    return 0;
}

Int32 Value::asInt32() const {
    if (isInt32()) return static_cast<Int32>(static_cast<int32_t>(raw_ & 0xFFFFFFFFULL));
    if (isInt16()) return asInt16();
    if (isInt8()) return asInt8();
    if (isFloat32()) return static_cast<Int32>(asFloat32());
    if (isFloat64()) return static_cast<Int32>(asFloat());
    return 0;
}

Int64 Value::asInt() const {
    if (isInt()) {
        if (isInt8()) return asInt8();
        if (isInt16()) return asInt16();
        if (isInt32()) return asInt32();
    }
    if (isInt64()) {
        auto* box = reinterpret_cast<BoxedInt64*>(raw_ & PTR_MASK);
        return box->value;
    }
    if (isDouble()) {
        Float64 f;
        std::memcpy(&f, &raw_, sizeof(Float64));
        return static_cast<Int64>(f);
    }
    if (isFloat32()) return static_cast<Int64>(asFloat32());
    return 0;
}

Float32 Value::asFloat32() const {
    if (isFloat32()) {
        uint32_t fbits = static_cast<uint32_t>(raw_ & 0xFFFFFFFFULL);
        Float32 f;
        std::memcpy(&f, &fbits, sizeof(f));
        return f;
    }
    if (isInt8()) return static_cast<Float32>(asInt8());
    if (isInt16()) return static_cast<Float32>(asInt16());
    if (isInt32()) return static_cast<Float32>(asInt32());
    if (isFloat64()) return static_cast<Float32>(asFloat());
    return 0.0f;
}

Float64 Value::asFloat() const {
    if (isDouble()) {
        Float64 f;
        std::memcpy(&f, &raw_, sizeof(Float64));
        return f;
    }
    if (isInt()) return static_cast<Float64>(asInt());
    if (isInt64()) return static_cast<Float64>(asInt());
    return 0.0;
}

bool Value::equals(const Value& other) const {
    if (raw_ == other.raw_) return true;
    if (isDouble() && other.isDouble()) {
        return asFloat() == other.asFloat();
    }
    // String content comparison
    if (isString() && other.isString()) {
        auto* a = asString(); auto* b = other.asString();
        if (!a || !b) return a == b;
        return a->length == b->length && std::memcmp(a->data, b->data, a->length) == 0;
    }
    if (isNumber() && other.isNumber()) {
        if (isDouble() || other.isDouble()) {
            return asFloat() == other.asFloat();
        }
        return asInt() == other.asInt();
    }
    if (isBool() && other.isBool()) {
        return asBool() == other.asBool();
    }
    if (isObject() && other.isObject()) {
        return asPtr() == other.asPtr();
    }
    return false;
}

std::string Value::toString() const {
    if (isNil()) return "nil";
    if (isBool()) return asBool() ? "真" : "假";
    if (isInt()) return std::to_string(asInt());
    if (isInt64()) return std::to_string(asInt());
    if (isDouble()) {
        std::ostringstream oss;
        oss << asFloat();
        return oss.str();
    }
    if (isString()) {
        auto* s = asString();
        if (s && s->data) return std::string(s->data, s->length);
        return "";
    }
    if (isArray()) {
        auto* arr = asArray();
        std::string r = "[";
        for (Int64 i = 0; i < arr->length(); i++) {
            if (i > 0) r += ", ";
            r += arr->get(i).toString();
        }
        r += "]";
        return r;
    }
    return "<object>";
}

bool ValueLess::operator()(const Value& a, const Value& b) const {
    if (a.isInt() && b.isInt()) return a.asInt() < b.asInt();
    if (a.isDouble() && b.isDouble()) return a.asFloat() < b.asFloat();
    if (a.isNumber() && b.isNumber()) return a.asFloat() < b.asFloat();
    if (a.isString() && b.isString()) {
        auto* sa = a.asString(); auto* sb = b.asString();
        if (!sa || !sb) return sa < sb;
        size_t ml = sa->length < sb->length ? sa->length : sb->length;
        int cmp = std::memcmp(sa->data, sb->data, ml);
        if (cmp != 0) return cmp < 0;
        return sa->length < sb->length;
    }
    return a.raw() < b.raw();
}

} // namespace cplang
