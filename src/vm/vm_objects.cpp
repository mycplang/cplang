// CP语言 虚拟机实现 - 对象类型与 GC
#include "vm/vm.hpp"

namespace cplang {

// ========== valueToString ==========
static std::string valueToString(const Value& v) {
    if (v.isNil()) return "nil";
    if (v.isBool()) return v.asBool() ? "true" : "false";
    if (v.isInt()) return std::to_string(v.asInt());
    if (v.isFloat()) {
        std::string s = std::to_string(v.asFloat());
        if (s.find('.') == std::string::npos && s.find('e') == std::string::npos) s += ".0";
        return s;
    }
    if (v.isString()) {
        return std::string(v.asString()->data, v.asString()->length);
    }
    if (v.isArray()) {
        std::string s = "[";
        auto& arr = v.asArray()->data;
        for (size_t i = 0; i < arr.size(); i++) {
            if (i > 0) s += ", ";
            s += valueToString(arr[i]);
        }
        s += "]";
        return s;
    }
    if (v.isFunction()) return "<function>";
    if (v.isClosure()) return "<closure>";
    if (v.isCFunction()) return "<cfunction>";
    if (v.isClass()) return "<class>";
    if (v.isInstance()) return "<instance>";
    if (v.isUpvalue()) return "<upvalue>";
    if (v.isUserData()) return "<userdata>";
    if (v.isTable()) return "<table>";
    if (v.isSet()) return "<set>";
    if (v.isStack()) return "<stack>";
    if (v.isQueue()) return "<queue>";
    if (v.isDeque()) return "<deque>";
    if (v.isPriorityQueue()) return "<priority_queue>";
    if (v.isLinkedList()) return "<linked_list>";
    if (v.isSLinkedList()) return "<forward_list>";
    if (v.isMultiSet()) return "<multiset>";
    if (v.isMultiMap()) return "<multimap>";
    if (v.isUnorderedSet()) return "<unordered_set>";
    if (v.isUnorderedMultiSet()) return "<unordered_multiset>";
    if (v.isUnorderedMap()) return "<unordered_map>";
    if (v.isUnorderedMultiMap()) return "<unordered_multimap>";
    return "<unknown>";
}

// ========== Threading ==========
VMThread* VMThread::create() { return new VMThread(); }
VMMutex* VMMutex::create() { return new VMMutex(); }
VMCondition* VMCondition::create() { return new VMCondition(); }
VMSemaphore* VMSemaphore::create() { return new VMSemaphore(); }
VMAtomicInt* VMAtomicInt::create() { return new VMAtomicInt(); }
VMBarrier* VMBarrier::create() { return new VMBarrier(); }

VMFuture* VMFuture::create() { return new VMFuture(); }
VMChannel* VMChannel::create() { return new VMChannel(); }
VMRWLock* VMRWLock::create() { return new VMRWLock(); }

VMWebSocket* VMWebSocket::create() { return new VMWebSocket(); }
VMMap* VMMap::create() { return new VMMap(); }
VMOrderedMap* VMOrderedMap::create() { return new VMOrderedMap(); }
VMWebSocket::~VMWebSocket() {
    // Handles cleaned up via CP-side wsClose(); GC safety net only
    hWebSocket = nullptr;
    hSession = nullptr;
    hConnect = nullptr;
}

// ========== VMClosure ==========
VMClosure* VMClosure::create(VMFunction* f) {
    VMClosure* cl = new VMClosure();
    cl->func = f;
    cl->upvalues.resize(f->upvalueCount);
    return cl;
}

// ========== VMClass ==========
VMClass* VMClass::create(VMString* name) {
    VMClass* c = new VMClass();
    c->name = name;
    return c;
}

// ========== VMInstance ==========
VMInstance* VMInstance::create(VMClass* c) {
    VMInstance* inst = new VMInstance();
    inst->cls = c;
    inst->fields.resize(c->fieldNames.size(), Value::nil());
    return inst;
}
Value VMInstance::getField(Int32 index) {
    if (index >= 0 && index < static_cast<Int32>(fields.size())) return fields[index];
    return Value::nil();
}
void VMInstance::setField(Int32 index, const Value& v) {
    if (index >= 0 && index < static_cast<Int32>(fields.size())) fields[index] = v;
}

// ========== VMUpvalue ==========
VMUpvalue* VMUpvalue::create(Value* slot) {
    VMUpvalue* uv = new VMUpvalue();
    uv->location = slot;
    return uv;
}

// ========== VMStructDef ==========
VMStructDef* VMStructDef::create(VMString* name) {
    VMStructDef* def = new VMStructDef();
    def->name = name;
    return def;
}

Int32 VMStructDef::getFieldIndex(const char* name_) {
    for (size_t i = 0; i < fieldNames.size(); i++) {
        if (std::strcmp(fieldNames[i]->data, name_) == 0) {
            return static_cast<Int32>(i);
        }
    }
    return -1;
}

// ========== VMStruct ==========
VMStruct* VMStruct::create(VMStructDef* def) {
    VMStruct* st = new VMStruct();
    st->def = def;
    st->fields.resize(def->fieldNames.size(), Value::nil());
    return st;
}

Value VMStruct::getField(Int32 index) {
    if (index >= 0 && index < static_cast<Int32>(fields.size())) {
        return fields[index];
    }
    return Value::nil();
}

void VMStruct::setField(Int32 index, const Value& v) {
    if (index >= 0 && index < static_cast<Int32>(fields.size())) {
        fields[index] = v;
    }
}

// ========== Value (equals/toString 现在定义在 src/vm/value.cpp 中) ==========
// 此处不再有实现，避免重复定义

} // namespace cplang
