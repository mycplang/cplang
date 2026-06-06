// ============================================================
// vm_switch.cpp — Switch-dispatch VM 实现（历史存档）
//
// ⚠️ 重要警告：本文件当前未被任何构建系统编译！
// ⚠️ 这是 switch-dispatch 风格的 VM 实验性实现，已弃用。
// ⚠️ 如需重新启用，请确保：
//   1. 移除与 vm.cpp 重复的工厂方法（VMString::create 等）
//   2. 补全缺失的 77+ 个容器操作方法
//   3. 解决 Value::equals 的多重定义问题
//
// 活跃的 VM 实现请参见 vm.cpp（threaded-code 风格）
// ============================================================

// 编译防护：默认阻止编译。如需编译，定义 CPLANG_ALLOW_VM_SWITCH
#ifndef CPLANG_ALLOW_VM_SWITCH
#error "vm_switch.cpp 是历史存档文件，不应被编译。请使用 vm.cpp。如果要编译此文件，请在命令行定义 -DCPLANG_ALLOW_VM_SWITCH"
#endif

#include "vm/vm.hpp"
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cctype>
#include <algorithm>
#include <sstream>
#include <vector>
#include <string>
#include <iostream>

namespace cplang {

// ========== valueToString ==========
static std::string valueToString(const Value& v) {
    if (v.isNil()) return "nil";
    if (v.isBool()) return v.asInt() ? "true" : "false";
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
    return "<unknown>";
}

// ========== VMString ==========
VMString* VMString::create(const char* s, UInt32 len) {
    VMString* str = reinterpret_cast<VMString*>(
        ::operator new(sizeof(VMString) + len));
    new (str) VMString();
    str->length = len;
    std::memcpy(str->data, s, len);
    str->data[len] = '\0';
    return str;
}
VMString* VMString::create(const std::string& s) {
    return create(s.c_str(), static_cast<UInt32>(s.size()));
}

// ========== VMArray ==========
VMArray* VMArray::create(UInt32 cap) {
    VMArray* arr = new VMArray();
    arr->data.reserve(cap);
    return arr;
}
Value VMArray::get(Int64 index) {
    Int64 n = data.size();
    if (index < 0) index += n;
    if (index >= 0 && index < n) return data[index];
    return Value::nil();
}
void VMArray::set(Int64 index, const Value& v) {
    Int64 n = data.size();
    if (index < 0) index += n;
    if (index >= 0) {
        if (index >= static_cast<Int64>(n)) data.resize(index + 1);
        data[index] = v;
    }
}

// ========== VMTable ==========
VMTable* VMTable::create() { return new VMTable(); }
Value VMTable::get(const Value& key) {
    for (auto& kv : data) {
        if (kv.first.equals(key)) return kv.second;
    }
    return Value::nil();
}
void VMTable::set(const Value& key, const Value& val) {
    for (auto& kv : data) {
        if (kv.first.equals(key)) { kv.second = val; return; }
    }
    data.emplace_back(key, val);
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

// ========== Value ==========
bool Value::equals(const Value& other) const {
    if (tag != other.tag) {
        if (tag == T_INT && other.tag == T_FLOAT) return asFloat() == other.asFloat();
        if (tag == T_FLOAT && other.tag == T_INT) return asFloat() == other.asFloat();
        return false;
    }
    if (tag == T_NIL) return true;
    if (tag == T_BOOL || tag == T_INT) return i == other.i;
    if (tag == T_FLOAT) return f == other.f;
    if (tag == T_STRING) return asString() == other.asString();
    return obj == other.obj;
}

std::string Value::toString() const { return valueToString(*this); }

// ========== VM ==========
VM::VM() {
    stack_.resize(MAX_STACK);
    top_ = stack_.data();
    allObjects_ = nullptr;
    gcAllocated_ = 0;
    gcCount_ = 0;
    gcRunning_ = false;
    traceExec_ = false;
    instructionCount_ = 0;
    moduleExecDepth_ = 0;
    lastImportedFunc_ = nullptr;
    error_ = "";
    nextGlobalSlot_ = 0;
    globalSlots_.resize(MAX_GLOBAL_SLOTS);
    for (auto& v : globalSlots_) { v.tag = Value::T_NIL; v.asInt() = 0; }

    // 初始化随机种子
    srand((unsigned int)::time(nullptr));

    // ========== 内置函数 ==========
    registerNative("print", [](std::vector<Value>& args) -> Value {
        for (size_t i = 0; i < args.size(); i++) {
            if (i > 0) std::cout << " ";
            std::cout << valueToString(args[i]);
        }
        std::cout << std::endl;
        std::cout.flush();
        return Value::nil();
    });

    // ========== 数学函数库 ==========
    // abs(n) - 绝对值
    registerNative("abs", [](std::vector<Value>& args) -> Value {
        if (args.empty()) return Value::nil();
        Value v = args[0];
        if (v.isInt()) return Value::Int(std::abs(v.asInt()));
        if (v.isFloat()) return Value::Float(std::abs(v.asFloat()));
        return Value::nil();
    });

    // sqrt(n) - 平方根
    registerNative("sqrt", [](std::vector<Value>& args) -> Value {
        if (args.empty()) return Value::nil();
        Value v = args[0];
        double n = v.isInt() ? (double)v.asInt() : v.isFloat() ? v.asFloat() : 0.0;
        if (n < 0) return Value::nil();
        return Value::Float(std::sqrt(n));
    });

    // pow(base, exp) - 幂运算
    registerNative("pow", [](std::vector<Value>& args) -> Value {
        if (args.size() < 2) return Value::nil();
        Value base = args[0], exp = args[1];
        double b = base.isInt() ? (double)base.asInt() : base.isFloat() ? base.asFloat() : 0.0;
        double e = exp.isInt() ? (double)exp.asInt() : exp.isFloat() ? exp.asFloat() : 0.0;
        return Value::Float(std::pow(b, e));
    });

    // floor(n) - 向下取整
    registerNative("floor", [](std::vector<Value>& args) -> Value {
        if (args.empty()) return Value::nil();
        Value v = args[0];
        double n = v.isInt() ? (double)v.asInt() : v.isFloat() ? v.asFloat() : 0.0;
        return Value::Int((Int64)std::floor(n));
    });

    // ceil(n) - 向上取整
    registerNative("ceil", [](std::vector<Value>& args) -> Value {
        if (args.empty()) return Value::nil();
        Value v = args[0];
        double n = v.isInt() ? (double)v.asInt() : v.isFloat() ? v.asFloat() : 0.0;
        return Value::Int((Int64)std::ceil(n));
    });

    // round(n) - 四舍五入
    registerNative("round", [](std::vector<Value>& args) -> Value {
        if (args.empty()) return Value::nil();
        Value v = args[0];
        double n = v.isInt() ? (double)v.asInt() : v.isFloat() ? v.asFloat() : 0.0;
        return Value::Int((Int64)std::round(n));
    });

    // random(max) - [0, max) 随机整数
    registerNative("random", [](std::vector<Value>& args) -> Value {
        if (args.empty()) return Value::Int(std::rand());
        Value max = args[0];
        int m = max.isInt() ? (int)max.asInt() : 100;
        return m <= 0 ? Value::Int(0) : Value::Int(std::rand() % m);
    });

    // ========== 字符串函数库 ==========
    // len(s/arr) - 字符串/数组长度
    registerNative("len", [](std::vector<Value>& args) -> Value {
        if (args.empty()) return Value::Int(0);
        Value v = args[0];
        if (v.isString() && v.asString()) return Value::Int(v.asString()->length);
        if (v.isArray() && v.asArray()) return Value::Int((Int64)v.asArray()->data.size());
        return Value::Int(0);
    });

    // substr(s, start, length) - 截取子字符串
    registerNative("substr", [](std::vector<Value>& args) -> Value {
        if (args.size() < 3 || !args[0].isString() || !args[0].asString()) return Value::nil();
        VMString* str = args[0].asString();
        int start = args.size() > 1 && args[1].isInt() ? (int)args[1].asInt() : 0;
        int length = args.size() > 2 && args[2].isInt() ? (int)args[2].asInt() : -1;
        int len = (int)str->length;
        
        if (start < 0) start = len + start;
        if (start < 0) start = 0;
        if (start >= len) return makeStringVal(VMString::create("", 0));
        if (length < 0) length = len - start;
        if (start + length > len) length = len - start;
        
        return makeStringVal(VMString::create(str->data + start, (UInt32)length));
    });

    // concat(s1, s2, ...) - 拼接字符串
    registerNative("concat", [](std::vector<Value>& args) -> Value {
        std::stringstream ss;
        for (const Value& arg : args) {
            ss << valueToString(arg);
        }
        std::string s = ss.str();
        return makeStringVal(VMString::create(s.c_str(), (UInt32)s.size()));
    });

    // find(s, substr) - 查找子串位置，找不到返回 -1
    registerNative("find", [](std::vector<Value>& args) -> Value {
        if (args.size() < 2 || !args[0].isString() || !args[1].isString() || !args[0].asString() || !args[1].asString())
            return Value::Int(-1);
        std::string s(args[0].asString()->data, args[0].asString()->length);
        std::string sub(args[1].asString()->data, args[1].asString()->length);
        size_t pos = s.find(sub);
        return Value::Int(pos == std::string::npos ? -1 : (Int64)pos);
    });

    // lower(s) - 转换为小写
    registerNative("lower", [](std::vector<Value>& args) -> Value {
        if (args.empty() || !args[0].isString() || !args[0].asString()) return Value::nil();
        std::string s(args[0].asString()->data, args[0].asString()->length);
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return makeStringVal(VMString::create(s.c_str(), (UInt32)s.size()));
    });

    // upper(s) - 转换为大写
    registerNative("upper", [](std::vector<Value>& args) -> Value {
        if (args.empty() || !args[0].isString() || !args[0].asString()) return Value::nil();
        std::string s(args[0].asString()->data, args[0].asString()->length);
        std::transform(s.begin(), s.end(), s.begin(), ::toupper);
        return makeStringVal(VMString::create(s.c_str(), (UInt32)s.size()));
    });

    // ========== 数组函数库 ==========
    // push(arr, elem) - 尾部添加元素，返回新长度
    registerNative("push", [](std::vector<Value>& args) -> Value {
        if (args.size() < 2 || !args[0].isArray() || !args[0].asArray()) return Value::nil();
        VMArray* arr = args[0].asArray();
        arr->data.push_back(args[1]);
        return Value::Int((Int64)arr->data.size());
    });

    // pop(arr) - 移除尾部元素并返回
    registerNative("pop", [](std::vector<Value>& args) -> Value {
        if (args.empty() || !args[0].isArray() || !args[0].asArray()) return Value::nil();
        VMArray* arr = args[0].asArray();
        if (arr->data.empty()) return Value::nil();
        Value val = arr->data.back();
        arr->data.pop_back();
        return val;
    });

    // insert(arr, index, elem) - 指定位置插入元素，返回新长度
    registerNative("insert", [](std::vector<Value>& args) -> Value {
        if (args.size() < 3 || !args[0].isArray() || !args[0].asArray() || !args[1].isInt()) return Value::nil();
        VMArray* arr = args[0].asArray();
        int idx = (int)args[1].asInt();
        if (idx < 0 || idx > (int)arr->data.size()) return Value::nil();
        arr->data.insert(arr->data.begin() + idx, args[2]);
        return Value::Int((Int64)arr->data.size());
    });

    // remove(arr, index) - 删除指定位置元素并返回
    registerNative("remove", [](std::vector<Value>& args) -> Value {
        if (args.size() < 2 || !args[0].isArray() || !args[0].asArray() || !args[1].isInt()) return Value::nil();
        VMArray* arr = args[0].asArray();
        int idx = (int)args[1].asInt();
        if (idx < 0 || idx >= (int)arr->data.size()) return Value::nil();
        Value val = arr->data[idx];
        arr->data.erase(arr->data.begin() + idx);
        return val;
    });

    // slice(arr, start, end) - 返回子数组
    registerNative("slice", [this](std::vector<Value>& args) -> Value {
        if (args.size() < 3 || !args[0].isArray() || !args[0].asArray() || !args[1].isInt() || !args[2].isInt()) return Value::nil();
        VMArray* arr = args[0].asArray();
        int start = (int)args[1].asInt();
        int end = (int)args[2].asInt();
        int len = (int)arr->data.size();
        
        if (start < 0) start = len + start;
        if (end < 0) end = len + end;
        if (start < 0) start = 0;
        if (end > len) end = len;
        if (start >= end) {
            VMArray* newArr = new VMArray();
            trackGC(reinterpret_cast<VMObject*>(newArr));
            return makeArrayVal(newArr);
        }
        
        VMArray* newArr = new VMArray();
        trackGC(reinterpret_cast<VMObject*>(newArr));
        newArr->data.assign(arr->data.begin() + start, arr->data.begin() + end);
        return makeArrayVal(newArr);
    });

    // ========== 中文别名注册 ==========
    // 核心函数
    registerNativeAlias("打印", "print");
    
    // 数学函数
    registerNativeAlias("绝对值", "abs");
    registerNativeAlias("平方根", "sqrt");
    registerNativeAlias("幂", "pow");
    registerNativeAlias("向下取整", "floor");
    registerNativeAlias("向上取整", "ceil");
    registerNativeAlias("四舍五入", "round");
    registerNativeAlias("随机", "random");
    
    // 字符串函数
    registerNativeAlias("长度", "len");
    registerNativeAlias("子串", "substr");
    registerNativeAlias("连接", "concat");
    registerNativeAlias("查找", "find");
    registerNativeAlias("小写", "lower");
    registerNativeAlias("大写", "upper");
    
    // 数组函数
    registerNativeAlias("追加", "push");
    registerNativeAlias("弹出", "pop");
    registerNativeAlias("插入", "insert");
    registerNativeAlias("删除", "remove");
    registerNativeAlias("切片", "slice");
}

VM::~VM() {
    VMObject* p = allObjects_;
    while (p) {
        VMObject* next = p->next;
        p->~VMObject();
        ::operator delete(p);
        p = next;
    }
}

void VM::registerGlobal(const char* name, Value val) {
    // Slot化优化：同时存入slot表和兼容表
    Int32 slot = getOrCreateGlobalSlot(name);
    if (slot >= 0) globalSlots_[slot] = val;
    
    // 保留兼容
    globals_[std::string(name)] = val;
}

Int32 VM::getOrCreateGlobalSlot(const char* name) {
    auto it = globalNameToSlot_.find(name);
    if (it != globalNameToSlot_.end()) return it->second;
    
    if (nextGlobalSlot_ >= MAX_GLOBAL_SLOTS) return -1;
    
    UInt16 slot = nextGlobalSlot_++;
    globalNameToSlot_[name] = slot;
    return slot;
}

Value* VM::getGlobalBySlot(UInt16 slot) {
    if (slot < globalSlots_.size()) return &globalSlots_[slot];
    return nullptr;
}

void VM::registerNative(const char* name, VMNativeFunc::Fn fn) {
    VMNativeFunc* nf = new VMNativeFunc();
    nf->fn = fn;
    nf->name = VMString::create(name, static_cast<UInt32>(strlen(name)));
    trackGC(reinterpret_cast<VMObject*>(nf));
    Value v;
    v.tag=Value::T_FUNCTION;
    v.asFunction()=reinterpret_cast<VMFunction*>(nf);
    
    // Slot化：同时存入slot表
    Int32 slot = getOrCreateGlobalSlot(name);
    if (slot >= 0) globalSlots_[slot] = v;
    
    globals_[std::string(name)] = v;
}

void VM::registerNativeAlias(const char* aliasName, const char* originalName) {
    auto it = globals_.find(std::string(originalName));
    auto slotIt = globalNameToSlot_.find(std::string(originalName));
    if (it != globals_.end()) {
        // Slot化：别名指向同一个slot
        if (slotIt != globalNameToSlot_.end()) {
            UInt16 slot = slotIt->second;
            globalNameToSlot_[std::string(aliasName)] = slot;
            // 不需要写入globalSlots_，因为已经指向同一个值
        }
        
        globals_[std::string(aliasName)] = it->second;
    }
}

VMString* VM::internString(const char* s, UInt32 len) {
    std::string key(s, len);
    auto it = stringTable_.find(key);
    if (it != stringTable_.end()) return it->second;
    VMString* str = VMString::create(s, len);
    trackGC(reinterpret_cast<VMObject*>(str));
    stringTable_[key] = str;
    return str;
}

VMString* VM::internString(const std::string& s) {
    return internString(s.c_str(), static_cast<UInt32>(s.size()));
}

bool VM::doImport(const std::string& filename) {
    if (!importCallback) return false;
    return importCallback(filename);
}

void VM::raiseError(const std::string& msg) {
    error_ = msg;
}

void VM::writeBarrier(VMObject* obj, Value newValue) {
    // 只有在GC标记阶段才需要写屏障
    if (!gcRunning_ || !obj || !newValue.isObject()) return;
    
    VMObject* newObj = newValue.asPtr();
    // Yuasa删除屏障：如果赋值的对象已经被标记为黑色，新引用的对象是白色，就把新对象标记为灰色
    if (obj->color == GCColor::BLACK && newObj->color == GCColor::WHITE) {
        newObj->setGray();
        // TODO：将对象加入到标记栈，后续扫描其引用
        gcMarkObject(newObj);
    }
}

void VM::trackGC(VMObject* obj) {
    if (!obj) return;
    // 新对象默认分配到新生代
    obj->isOldGen = false;
    obj->age = 0;
    obj->color = GCColor::WHITE;
    
    // 检查是否已经在新生代中
    VMObject* p = newGenObjects_;
    while (p) {
        if (p == obj) return;  // 已跟踪
        p = p->next;
    }
    // 检查是否已经在老生代中
    p = oldGenObjects_;
    while (p) {
        if (p == obj) return;  // 已跟踪
        p = p->next;
    }
    
    // 添加到新生代链表
    obj->next = newGenObjects_;
    newGenObjects_ = obj;
    
    gcAllocated_ += obj->size;
    newGenAllocated_ += obj->size;
    
    // GC触发逻辑
    if (!gcRunning_) {
        if (newGenAllocated_ > GC_NEWGEN_THRESHOLD) {
            minorGc(); // 新生代达到阈值，执行Minor GC
        }
        if (gcAllocated_ > GC_THRESHOLD) {
            majorGc(); // 总内存达到阈值，执行全堆Major GC
        }
    }
}

void VM::gcMarkValue(const Value& v) {
    if (v.isObject()) gcMarkObject(v.asPtr());
}

void VM::gcMarkObject(VMObject* obj) {
    if (!obj || obj->color == GCColor::BLACK) return;
    obj->setBlack();
    if (obj->typeTag == ObjectHeader::TAG_STRING) return;
    if (obj->typeTag == ObjectHeader::TAG_ARRAY) {
        auto arr = static_cast<VMArray*>(obj);
        for (auto& v : arr->data) gcMarkValue(v);
    // VMTable 没有 data 成员，只有 get/set
    } else if (obj->typeTag == ObjectHeader::TAG_TABLE) {
        // no extra
    } else if (obj->typeTag == ObjectHeader::TAG_FUNCTION) {
        auto func = static_cast<VMFunction*>(obj);
        for (auto& c : func->constants) gcMarkValue(c);
    } else if (obj->typeTag == ObjectHeader::TAG_CLOSURE) {
        auto cl = static_cast<VMClosure*>(obj);
        gcMarkObject(reinterpret_cast<VMObject*>(cl->func));
        for (auto uv : cl->upvalues) gcMarkObject(reinterpret_cast<VMObject*>(uv));
    } else if (obj->typeTag == ObjectHeader::TAG_CLASS) {
        auto cls = static_cast<VMClass*>(obj);
        gcMarkObject(reinterpret_cast<VMObject*>(cls->name));
        gcMarkObject(reinterpret_cast<VMObject*>(cls->base));
        for (auto m : cls->methods) gcMarkObject(reinterpret_cast<VMObject*>(m));
    } else if (obj->typeTag == ObjectHeader::TAG_INSTANCE) {
        auto inst = static_cast<VMInstance*>(obj);
        gcMarkObject(reinterpret_cast<VMObject*>(inst->cls));
        for (auto& f : inst->fields) gcMarkValue(f);
    } else if (obj->typeTag == ObjectHeader::TAG_UPVALUE) {
        auto uv = static_cast<VMUpvalue*>(obj);
        if (!uv->closed.isNil()) gcMarkValue(uv->closed);
    }
}

void VM::gcMarkRoots() {
    for (auto& g : globals_) gcMarkValue(g.second);
    for (Value* p = stack_.data(); p < top_; p++) gcMarkValue(*p);
    for (auto& f : frames_) {
        gcMarkObject(reinterpret_cast<VMObject*>(f.func));
        gcMarkObject(reinterpret_cast<VMObject*>(f.closure));
    }
}

void VM::gcSweepPhase() {
    VMObject** p = &allObjects_;
    while (*p) {
        if ((*p)->color != GCColor::BLACK) {
            VMObject* dead = *p;
            *p = dead->next;
            gcAllocated_ -= dead->size;  // 先读取size
            dead->~VMObject();
            ::operator delete(dead);
        } else {
            (*p)->setWhite();
            p = &((*p)->next);
        }
    }
}

void VM::gc() {
    if (gcRunning_) return;
    gcRunning_ = true;
    gcMarkRoots();
    gcSweepPhase();
    gcCount_++;
    gcRunning_ = false;
}

bool VM::callNative(VMNativeFunc* nf, int argc, Value* argv, Value* result) {
    if (!nf || !nf->fn) { *result = Value::nil(); return true; }
    VM* savedVM = currentVM_;
    currentVM_ = this;
    try {
        std::vector<Value> args;
        for (int i = 0; i < argc; i++) args.push_back(argv[i]);
        *result = nf->fn(args);
        currentVM_ = savedVM;
        return true;
    } catch (...) {
        currentVM_ = savedVM;
        return false;
    }
}

// ========== 字节码执行 ==========
bool VM::run(ExecContext* ctx) {
    Value* base = ctx->base;

#define RA(r)  (base[(r)])
#define RB(r)  (base[(r)])
#define RC(r)  (base[(r)])
#define RR_a  RA(a)
#define EMIT_ERR(msg) do { error_ = std::string(msg); if (ctx && ctx->func && !ctx->func->lineInfo.empty()) { size_t idx = ctx->pc / 16; if (idx < ctx->func->lineInfo.size()) { int ln = ctx->func->lineInfo[idx]; if (ln > 0) error_ += " (第" + std::to_string(ln) + "行)"; } } return false; } while(0)

    // GC: checked by allocation watermark, not by instruction counter
    size_t lastGcCheckAlloc = gcAllocated_;

    // ── Computed-goto dispatch (GCC/Clang) vs switch fallback ──
// 强制使用 switch-case 模式，避免复杂性
#define USE_COMPUTED_GOTO 0

#if USE_COMPUTED_GOTO
    // Build dispatch table: 256 entries (sparse opcodes, but only 2KB)
    static const void* dispatch_table[256] = {
        /* 0x00 */ &&op_LOADNIL,  /* 0x01 */ &&op_LOADBOOL, /* 0x02 */ &&op_LOADINT,  /* 0x03 */ &&op_LOADFLT,
        /* 0x04 */ &&op_LOADSTR,  /* 0x05 */ &&op_LOADCONST,/* 0x06 */ &&op_MOVE,    /* 0x07 */ &&op_LOADGLOBAL,
        /* 0x08 */ &&op_STOREGLOBAL,
        // 0x09-0x0F → invalid
        /* 0x09-0x0F */ &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid,
        /* 0x10 */ &&op_LOADLOCAL, /* 0x11 */ &&op_STORELOCAL,
        /* 0x12-0x1F */ &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid,
        /* 0x20 */ &&op_ADD,  /* 0x21 */ &&op_SUB,  /* 0x22 */ &&op_MUL,  /* 0x23 */ &&op_DIV,
        /* 0x24 */ &&op_IDIV, /* 0x25 */ &&op_MOD,  /* 0x26 */ &&op_POW,  /* 0x27 */ &&op_NEG,
        /* 0x28 */ &&op_BAND, /* 0x29 */ &&op_BOR,  /* 0x2A */ &&op_BXOR,
        /* 0x2B */ &&op_BSHL, /* 0x2C */ &&op_BSHR, /* 0x2D */ &&op_BNOT,
        /* 0x2E-0x2F */ &&op_invalid, &&op_invalid,
        /* 0x30 */ &&op_CMPEQ, /* 0x31 */ &&op_CMPNE,
        /* 0x32 */ &&op_CMPLT, /* 0x33 */ &&op_CMPLE, /* 0x34 */ &&op_CMPGT, /* 0x35 */ &&op_CMPGE,
        /* 0x36-0x3F */ &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid,
        /* 0x40 */ &&op_JUMP,   /* 0x41 */ &&op_JUMPIF,  /* 0x42 */ &&op_JUMPNIF,
        /* 0x43-0x4F */ &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid,
        /* 0x50 */ &&op_CALL, &&op_invalid, /* 0x52 */ &&op_RETURN,
        /* 0x53-0x5F */ &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid,
        /* 0x60 */ &&op_NEWARRAY, &&op_invalid, /* 0x62 */ &&op_GETELEM, /* 0x63 */ &&op_SETELEM,
        /* 0x64 */ &&op_GETIDX,  /* 0x65 */ &&op_SETIDX,
        /* 0x66-0x67 */ &&op_invalid, &&op_invalid,
        /* 0x68 */ &&op_CONCAT,  /* 0x69 */ &&op_STRLEN,
        /* 0x6A-0x6F */ &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid,
        /* 0x70 */ &&op_TONUM,  /* 0x71 */ &&op_TOSTR,  /* 0x72 */ &&op_TOBool,
        /* 0x73 */ &&op_TYPEOF, /* 0x74 */ &&op_ISNULL,
        /* 0x75-0x7F */ &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid,
        /* 0x80 */ &&op_NEWCLASS, /* 0x81 */ &&op_IMPORT,
        // 0x82-0xFE → invalid
        &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid,
        &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid,
        &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid,
        &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid,
        &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid,
        &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid,
        &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid,
        &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid,
        &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid,
        &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid,
        &&op_invalid, &&op_invalid, &&op_invalid,
        /* 0xFF */ &&op_NOP
    };
#  define DISPATCH_NEXT() do {                                       \
        if (gcAllocated_ > lastGcCheckAlloc + 65536 &&              \
            gcAllocated_ > GC_THRESHOLD && !gcRunning_) {           \
            gc(); lastGcCheckAlloc = gcAllocated_;                  \
        }                                                            \
        UInt8 _op = ctx->code[ctx->pc++];                           \
        a = ctx->code[ctx->pc];                                     \
        b = ctx->code[ctx->pc+1];                                   \
        c = ctx->code[ctx->pc+2];                                   \
        goto *dispatch_table[_op];                                   \
    } while(0)
#else
#  define DISPATCH_NEXT() break
#endif

    Int32 a, b, c;

#if USE_COMPUTED_GOTO
    DISPATCH_NEXT();
#else
    for (;;) {
        if (gcAllocated_ > lastGcCheckAlloc + 65536 && gcAllocated_ > GC_THRESHOLD && !gcRunning_) {
            gc();
            lastGcCheckAlloc = gcAllocated_;
        }
#ifdef CP_DEBUG
        if (ctx->pc >= ctx->codeSize) {
            EMIT_ERR("程序计数器越界");
        }
#endif
        UInt8 op = ctx->code[ctx->pc++];
        a = ctx->code[ctx->pc];
        b = ctx->code[ctx->pc+1];
        c = ctx->code[ctx->pc+2];
        switch (op) {
#endif

case OP_NOP: { ctx->pc += 15; break; }

case OP_LOADNIL: { RA(a) = Value::nil();                 ctx->pc += 15;
            DISPATCH_NEXT(); }

case OP_LOADBOOL: {
                RA(a) = b ? Value::Bool(true) : Value::Bool(false);
                if (c) ctx->pc++;
                ctx->pc += 15;
                break;
            }

case OP_LOADINT: {
                // 16-byte format: [op][a][0][0][imm0][imm1][imm2][imm3][pad8]
                // After op fetch, pc points to a. imm32 starts at pc+3 (byte offset 4 from instruction start)
                Int32 imm = (Int32)ctx->code[ctx->pc+3] |
                           ((Int32)ctx->code[ctx->pc+4] << 8) |
                           ((Int32)ctx->code[ctx->pc+5] << 16) |
                           ((Int32)ctx->code[ctx->pc+6] << 24);

                RA(a) = Value::Int(imm);
                ctx->pc += 15;

                break;
            }

case OP_LOADFLT: {
                // 16-byte format: [op][a][0][0][imm0][imm1][imm2][imm3][pad8]
                // After op fetch, pc points to a. imm32 starts at pc+3
                Int32 idx = (Int32)ctx->code[ctx->pc+3] |
                           ((Int32)ctx->code[ctx->pc+4] << 8) |
                           ((Int32)ctx->code[ctx->pc+5] << 16) |
                           ((Int32)ctx->code[ctx->pc+6] << 24);
                if (idx >= 0 && idx < static_cast<Int32>(ctx->func->constants.size())) {
                    RA(a) = ctx->func->constants[idx];
                } else {
                    RA(a) = Value::Float(idx);
                }
                ctx->pc += 15;
                break;
            }

case OP_LOADSTR: {
                // 16-byte format: [op][a][0][0][imm0][imm1][imm2][imm3][pad8]
                // After op fetch, pc points to a. imm32 starts at pc+3
                Int32 idx = (Int32)ctx->code[ctx->pc+3] |
                           ((Int32)ctx->code[ctx->pc+4] << 8) |
                           ((Int32)ctx->code[ctx->pc+5] << 16) |
                           ((Int32)ctx->code[ctx->pc+6] << 24);
                if (idx >= 0 && idx < static_cast<Int32>(ctx->func->constants.size())) {
                    const Value& kv = ctx->func->constants[idx];
                    if (kv.isString()) {
                        RA(a) = kv;
                    } else {
                        RA(a) = Value::nil();
                    }
                } else {
                    RA(a) = Value::nil();
                }
                ctx->pc += 15;
                break;
            }

case OP_LOADCONST: {
                // 8-byte format: [op][a][b][c][imm0][imm1][imm2][imm3]
                Int32 idx = (Int32)ctx->code[ctx->pc+3] |
                           ((Int32)ctx->code[ctx->pc+4] << 8) |
                           ((Int32)ctx->code[ctx->pc+5] << 16) |
                           ((Int32)ctx->code[ctx->pc+6] << 24);

                if (idx >= 0 && idx < static_cast<Int32>(ctx->func->constants.size())) {
                    const Value& v = ctx->func->constants[idx];

                    RA(a) = v;
                }
                ctx->pc += 15;
                break;
            }

case OP_MOVE: { RA(a) = RB(b);                 ctx->pc += 15;
            DISPATCH_NEXT(); }

case OP_LOADGLOBAL: {
                // 16-byte format: [op][a][0][0][slot0][slot1][slot2][slot3][pad8]
                // After op fetch, pc points to a. imm32 (slot index) starts at pc+3
                // Slot化优化：idx 直接是 slot 索引，无需常量池解析
                Int32 slot = (Int32)ctx->code[ctx->pc+3] |
                            ((Int32)ctx->code[ctx->pc+4] << 8) |
                            ((Int32)ctx->code[ctx->pc+5] << 16) |
                            ((Int32)ctx->code[ctx->pc+6] << 24);
                ctx->pc += 15;
                
                // O(1) 直接数组索引访问，无哈希查找
                if (slot >= 0 && slot < static_cast<Int32>(globalSlots_.size())) {
                    RA(a) = globalSlots_[slot];
                } else {
                    RA(a) = Value::nil();
                }
                break;
            }

case OP_STOREGLOBAL: {
                // 16-byte format: [op][a][0][0][slot0][slot1][slot2][slot3][pad8]
                // After op fetch, pc points to a. imm32 (slot index) starts at pc+3
                // Slot化优化：idx 直接是 slot 索引，无需常量池解析
                Int32 slot = (Int32)ctx->code[ctx->pc+3] |
                            ((Int32)ctx->code[ctx->pc+4] << 8) |
                            ((Int32)ctx->code[ctx->pc+5] << 16) |
                            ((Int32)ctx->code[ctx->pc+6] << 24);
                ctx->pc += 15;
                
                // O(1) 直接数组索引写入，无哈希查找
                if (slot >= 0 && slot < static_cast<Int32>(globalSlots_.size())) {
                    globalSlots_[slot] = RA(a);
                }
                break;
            }

case OP_LOADLOCAL: {

                RA(a) = base[b];                 ctx->pc += 15;
            DISPATCH_NEXT(); }
case OP_STORELOCAL: { base[b] = RA(a);                 ctx->pc += 15;
            DISPATCH_NEXT(); }

case OP_ADD: {
                if (RB(b).isInt() && RC(c).isInt()) {

                    RA(a) = Value::Int(RB(b).asInt() + RC(c).asInt());
                } else {
                    double x = RB(b).isInt() ? RB(b).asInt() : (RB(b).isFloat() ? RB(b).asFloat() : 0);
                    double y = RC(c).isInt() ? RC(c).asInt() : (RC(c).isFloat() ? RC(c).asFloat() : 0);
                    RA(a) = Value::Float(x + y);
                }
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_SUB: {
                if (RB(b).isInt() && RC(c).isInt()) {
                    RA(a) = Value::Int(RB(b).asInt() - RC(c).asInt());
                } else {
                    double x = RB(b).isInt() ? RB(b).asInt() : (RB(b).isFloat() ? RB(b).asFloat() : 0);
                    double y = RC(c).isInt() ? RC(c).asInt() : (RC(c).isFloat() ? RC(c).asFloat() : 0);
                    RA(a) = Value::Float(x - y);
                }
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_MUL: {
                if (RB(b).isInt() && RC(c).isInt()) {
                    RA(a) = Value::Int(RB(b).asInt() * RC(c).asInt());
                } else {
                    double x = RB(b).isInt() ? RB(b).asInt() : (RB(b).isFloat() ? RB(b).asFloat() : 0);
                    double y = RC(c).isInt() ? RC(c).asInt() : (RC(c).isFloat() ? RC(c).asFloat() : 0);
                    RA(a) = Value::Float(x * y);
                }
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_DIV: {
                double x = RB(b).isInt() ? RB(b).asInt() : (RB(b).isFloat() ? RB(b).asFloat() : 0);
                double y = RC(c).isInt() ? RC(c).asInt() : (RC(c).isFloat() ? RC(c).asFloat() : 0);
                if (y == 0) EMIT_ERR("除零错误");
                RA(a) = Value::Float(x / y);
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_IDIV: {
                Int64 y = RC(c).isInt() ? RC(c).asInt() : static_cast<Int64>(RC(c).asFloat());
                if (y == 0) EMIT_ERR("除零错误");
                RA(a) = Value::Int(RB(b).asInt() / y);
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_MOD: {
                Int64 y = RC(c).isInt() ? RC(c).asInt() : static_cast<Int64>(RC(c).asFloat());
                if (y == 0) EMIT_ERR("除零错误");
                RA(a) = Value::Int(RB(b).asInt() % y);
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_POW: {
                double x = RB(b).isInt() ? RB(b).asInt() : (RB(b).isFloat() ? RB(b).asFloat() : 0);
                double y = RC(c).isInt() ? RC(c).asInt() : (RC(c).isFloat() ? RC(c).asFloat() : 0);
                RA(a) = Value::Float(std::pow(x, y));
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_NEG: {
                if (RB(b).isInt()) RA(a) = Value::Int(-RB(b).asInt());
                else if (RB(b).isFloat()) RA(a) = Value::Float(-RB(b).asFloat());
                else RA(a) = Value::nil();
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_BAND: {
                if (RB(b).isInt() && RC(c).isInt()) {
                    RA(a) = Value::Int(RB(b).asInt() & RC(c).asInt());
                } else RA(a) = Value::nil();
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_BOR: {
                if (RB(b).isInt() && RC(c).isInt()) {
                    RA(a) = Value::Int(RB(b).asInt() | RC(c).asInt());
                } else RA(a) = Value::nil();
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_BXOR: {
                if (RB(b).isInt() && RC(c).isInt()) {
                    RA(a) = Value::Int(RB(b).asInt() ^ RC(c).asInt());
                } else RA(a) = Value::nil();
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_BSHL: {
                if (RB(b).isInt() && RC(c).isInt()) {
                    RA(a) = Value::Int(RB(b).asInt() << RC(c).asInt());
                } else RA(a) = Value::nil();
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_BSHR: {
                if (RB(b).isInt() && RC(c).isInt()) {
                    RA(a) = Value::Int(static_cast<UInt64>(RB(b).asInt()) >> RC(c).asInt());
                } else RA(a) = Value::nil();
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_BNOT: {
                if (RB(b).isInt()) RA(a) = Value::Int(~RB(b).asInt());
                else RA(a) = Value::nil();
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_CMPEQ: { RA(a) = Value::Bool(RB(b).equals(RC(c)));                 ctx->pc += 15;
            DISPATCH_NEXT(); }
case OP_CMPNE: { RA(a) = Value::Bool(!RB(b).equals(RC(c)));                 ctx->pc += 15;
            DISPATCH_NEXT(); }

case OP_CMPLT: {
                if (RB(b).isInt() && RC(c).isInt()) RA(a) = Value::Bool(RB(b).asInt() < RC(c).asInt());
                else if (RB(b).isNumber() && RC(c).isNumber()) {
                    double x = RB(b).asFloat(); double y = RC(c).asFloat();
                    RA(a) = Value::Bool(x < y);
                } else RA(a) = Value::Bool(false);
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_CMPLE: {
                if (RB(b).isInt() && RC(c).isInt()) RA(a) = Value::Bool(RB(b).asInt() <= RC(c).asInt());
                else if (RB(b).isNumber() && RC(c).isNumber()) {
                    double x = RB(b).asFloat(); double y = RC(c).asFloat();
                    RA(a) = Value::Bool(x <= y);
                } else RA(a) = Value::Bool(false);
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_CMPGT: {
                if (RB(b).isInt() && RC(c).isInt()) RA(a) = Value::Bool(RB(b).asInt() > RC(c).asInt());
                else if (RB(b).isNumber() && RC(c).isNumber()) {
                    double x = RB(b).asFloat(); double y = RC(c).asFloat();
                    RA(a) = Value::Bool(x > y);
                } else RA(a) = Value::Bool(false);
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_CMPGE: {
                if (RB(b).isInt() && RC(c).isInt()) RA(a) = Value::Bool(RB(b).asInt() >= RC(c).asInt());
                else if (RB(b).isNumber() && RC(c).isNumber()) {
                    double x = RB(b).asFloat(); double y = RC(c).asFloat();
                    RA(a) = Value::Bool(x >= y);
                } else RA(a) = Value::Bool(false);
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_JUMP: {
                // 16-byte format: [op][0][0][0][offset32] (relative offset from next instruction)
                // After reading op, pc points to a (pc+1), so offset is at pc+3
                Int32 offset = (Int32)ctx->code[ctx->pc+3] |
                             ((Int32)ctx->code[ctx->pc+4] << 8) |
                             ((Int32)ctx->code[ctx->pc+5] << 16) |
                             ((Int32)ctx->code[ctx->pc+6] << 24);
                ctx->pc = ctx->pc + 15 + offset;  // pc after opcode + 15 + offset
                break;
            }

case OP_JUMPIF: {
                // 16-byte format: [op][a][0][0][offset32] (relative offset)
                // After reading op, pc points to a (pc+1), so offset is at pc+3
                Int32 offset = (Int32)ctx->code[ctx->pc+3] |
                             ((Int32)ctx->code[ctx->pc+4] << 8) |
                             ((Int32)ctx->code[ctx->pc+5] << 16) |
                             ((Int32)ctx->code[ctx->pc+6] << 24);
                if (RA(a).isTrue()) ctx->pc = ctx->pc + 15 + offset;
                else ctx->pc += 15;
                break;
            }

case OP_JUMPNIF: {
                // 16-byte format: [op][a][0][0][offset32] (relative offset)
                // After reading op, pc points to a (pc+1), so offset is at pc+3
                Int32 offset = (Int32)ctx->code[ctx->pc+3] |
                             ((Int32)ctx->code[ctx->pc+4] << 8) |
                             ((Int32)ctx->code[ctx->pc+5] << 16) |
                             ((Int32)ctx->code[ctx->pc+6] << 24);
                bool cond = RA(a).isTrue();
                if (!cond) ctx->pc = ctx->pc + 15 + offset;
                else ctx->pc += 15;
                break;
            }

case OP_CALL: {
                Int32 calleeReg = b;
                Int32 argc = c;
                Value callee = RA(calleeReg);

                std::vector<Value> args;
                for (Int32 i = 0; i < argc; i++) args.push_back(RA(calleeReg + 1 + i));

                if (callee.isFunction()) {
                    VMFunction* func = callee.func;
                    if (func->typeTag == ObjectHeader::TAG_NATIVE) {
                        VMNativeFunc* nf = reinterpret_cast<VMNativeFunc*>(func);
                        if (nf && nf->fn) {
                            VM* savedVM = currentVM_;
                            try {
                                currentVM_ = this;
                                currentCtx_ = ctx;
                                RA(a) = nf->fn(args);
                                currentVM_ = savedVM;
                            }
                            catch (...) { currentVM_ = savedVM; RA(a) = Value::nil(); }
                        } else {
                            RA(a) = Value::nil();
                        }
                        ctx->pc += 15;
                        break;
                    }
                    // 用户函数
                    CallFrame frame;
                    frame.func = func;
                    frame.closure = nullptr;
                    frame.base = base;
                    frame.savedBase = base;
                    frame.pc = nullptr;
                    Int32 pcAfter = ctx->pc + 15;  // Return to next instruction (16-byte format)
                    frame.returnPcOffset = pcAfter;
                    frame.returnBaseOffset = ctx->baseOffset;
                    frame.resultReg = calleeReg;
                    frames_.push_back(frame);
                    ctx->func = func;
                    ctx->code = func->code.data();
                    ctx->codeSize = func->code.size();
                    ctx->pc = 0;
                    ctx->baseOffset = calleeReg;

                    base = base + calleeReg + 1;

                    ctx->base = base;





                    break;
                } else if (callee.isClosure()) {
                    VMClosure* cl = callee.obj ? static_cast<VMClosure*>(callee.obj) : nullptr;
                    if (!cl) { RA(a) = Value::nil(); ctx->pc += 15; break; }
                    CallFrame frame;
                    frame.func = cl->func;
                    frame.closure = cl;
                    frame.base = base;
                    frame.savedBase = base;
                    frame.pc = nullptr;
                    Int32 pcAfter = ctx->pc + 7;
                    frame.returnPcOffset = pcAfter;
                    frame.returnBaseOffset = ctx->baseOffset;
                    frame.resultReg = calleeReg;
                    frames_.push_back(frame);
                    ctx->func = cl->func;
                    ctx->code = cl->func->code.data();
                    ctx->codeSize = cl->func->code.size();
                    ctx->pc = 0;
                    ctx->baseOffset = calleeReg;
                    base = base + calleeReg + 1;
                    ctx->base = base;
                    break;
                }
                EMIT_ERR("不可调用：该值不是函数");
            }

case OP_RETURN: {
                ctx->pc += 15;  // skip padding bytes (16-byte instruction)
                Value result = RR_a;

                if (frames_.size() <= 1) {
                    return !hasError();
                }

                CallFrame& cur = frames_.back();
                Int32 returnPc = cur.returnPcOffset;
                Int32 returnBaseOff = cur.returnBaseOffset;
                Value* callerBase = cur.savedBase;
                Int32 resultReg = cur.resultReg;

                frames_.pop_back();

                if (!frames_.empty()) {
                    CallFrame& caller = frames_.back();
                    if (caller.func) {
                        ctx->func = caller.func;
                        ctx->code = caller.func->code.data();
                        ctx->codeSize = caller.func->code.size();
                    }
                }
                ctx->pc = returnPc;
                ctx->baseOffset = returnBaseOff;
                base = callerBase ? callerBase : stack_.data() + returnBaseOff;
                ctx->base = base;

                RA(resultReg) = result;

                continue;
            }

case OP_NEWARRAY: {
                RA(a) = makeArrayVal(VMArray::create());
                trackGC(reinterpret_cast<VMObject*>(RA(a).asArray()));
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_GETELEM: {
                Value arr = RB(b);
                Value idx = RC(c);
                if (arr.isArray() && idx.isInt()) {
                    RA(a) = arr.asArray()->get(idx.asInt());
                } else if (arr.obj && arr.obj->typeTag == ObjectHeader::TAG_TABLE) {
                    VMTable* tbl = static_cast<VMTable*>(arr.obj);
                    RA(a) = tbl->get(idx);
                } else {
                    RA(a) = Value::nil();
                }
                ctx->pc += 15;
                break;
            }

case OP_SETELEM: {
                // SETELEM: a=元素值, b=数组, c=索引
                Value elem = RA(a);
                Value arr = RB(b);
                Value idx = RC(c);
                if (arr.isArray() && idx.isInt()) {
                    arr.asArray()->set(idx.asInt(), elem);
                } else if (arr.obj && arr.obj->typeTag == ObjectHeader::TAG_TABLE) {
                    VMTable* tbl = static_cast<VMTable*>(arr.obj);
                    tbl->set(idx, elem);
                }
                ctx->pc += 15;
                break;
            }

case OP_GETIDX: {
                Value obj = RA(b);
                Value key = RA(c);
                if (obj.isArray() && key.isInt()) {
                    RA(a) = obj.asArray()->get(key.asInt());
                } else if (obj.obj && obj.obj->typeTag == ObjectHeader::TAG_TABLE) {
                    VMTable* tbl = static_cast<VMTable*>(obj.obj);
                    RA(a) = tbl->get(key);
                } else {
                    RA(a) = Value::nil();
                }
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_SETIDX: {
                Value obj = RA(a);
                Value key = RA(b);
                if (obj.isArray() && key.isInt()) {
                    obj.asArray()->set(key.asInt(), RA(c));
                } else if (obj.obj && obj.obj->typeTag == ObjectHeader::TAG_TABLE) {
                    VMTable* tbl = static_cast<VMTable*>(obj.obj);
                    tbl->set(key, RA(c));
                }
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_STRLEN: {
                if (RA(b).isString()) {
                    RA(a) = Value::Int(static_cast<Int32>(RA(b).asString()->length));
                } else if (RA(b).isArray()) {
                    RA(a) = Value::Int(static_cast<Int32>(RA(b).asArray()->length()));
                } else {
                    RA(a) = Value::nil();
                }
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_CONCAT: {
                std::string s1 = RB(b).toString();
                std::string s2 = RC(c).toString();
                VMString* r = VMString::create(s1 + s2);
                trackGC(reinterpret_cast<VMObject*>(r));
                RA(a) = makeStringVal(r);
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_TONUM: {
                if (RA(b).isString()) {
                    const char* s = RA(b).asString()->data;
                    if (std::strchr(s, '.')) {
                        RA(a) = Value::Float(std::atof(s));
                    } else {
                        RA(a) = Value::Int(std::atoll(s));
                    }
                } else if (RA(b).isInt() || RA(b).isFloat()) {
                    RA(a) = RA(b);
                } else {
                    RA(a) = Value::nil();
                }
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_TOSTR: {
                VMString* r = VMString::create(RA(b).toString());
                trackGC(reinterpret_cast<VMObject*>(r));
                RA(a) = makeStringVal(r);
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_TOBool: {
                RA(a) = Value::Bool(RA(b).isTrue());
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_TYPEOF: {
                const char* tn = "?";
                if (RA(b).isNil()) tn = "nil";
                else if (RA(b).isBool()) tn = "bool";
                else if (RA(b).isInt()) tn = "int";
                else if (RA(b).isFloat()) tn = "float";
                else if (RA(b).isString()) tn = "string";
                else if (RA(b).isArray()) tn = "array";
                else if (RA(b).obj && RA(b).obj->typeTag == ObjectHeader::TAG_TABLE) tn = "table";
                else if (RA(b).isFunction()) tn = "function";
                else if (RA(b).isClosure()) tn = "function";
                else if (RA(b).isCFunction()) tn = "cfunction";
                else if (RA(b).isClass()) tn = "class";
                else if (RA(b).isInstance()) tn = "instance";
                VMString* r = VMString::create(tn);
                trackGC(reinterpret_cast<VMObject*>(r));
                RA(a) = makeStringVal(r);
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_ISNULL: {
                RA(a) = Value::Bool(RA(b).isNil());
                ctx->pc += 15;
                break;
            }

case OP_NEWCLASS: {
                RA(a) = Value::nil();
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_IMPORT: {
                Int32 modIdx = b;  // b is the 8-bit constant index
                if (modIdx < 0 || modIdx >= static_cast<Int32>(ctx->func->constants.size())) {
                    EMIT_ERR("导入失败：模块索引无效");
                }
                const Value& modVal = ctx->func->constants[modIdx];
                if (!modVal.isString()) EMIT_ERR("导入失败：模块名必须是字符串");
                std::string path(modVal.asString()->data, modVal.asString()->length);

                if (!importCallback) EMIT_ERR("导入失败：导入功能不支持");

                bool ok = importCallback(path);
                VMFunction* moduleFunc = lastImportedFunc_;

                if (!ok || !moduleFunc) {
                    RA(a) = Value::Bool(false);
                    break;
                }

                // 复制模块字节码
                size_t modSz = moduleFunc->code.size();
                UInt8* modBuf = new UInt8[modSz + 256];
                std::memcpy(modBuf, moduleFunc->code.data(), modSz);
                for (size_t i = modSz; i < modSz + 256; i++) modBuf[i] = OP_NOP;

                // 保存主程序上下文
                size_t savedPc = ctx->pc;
                Value* savedBase = base;
                Int32 savedBaseOff = ctx->baseOffset;
                VMFunction* savedFunc = ctx->func;
                UInt8* savedCode = ctx->code;
                size_t savedCodeSize = ctx->codeSize;

                // 设置模块执行上下文
                ctx->pc = 0;
                ctx->func = moduleFunc;
                ctx->code = modBuf;
                ctx->codeSize = modSz + 256;
                ctx->baseOffset = 0;
                base = stack_.data();
                ctx->base = base;
                std::memset(base, 0, MAX_STACK * sizeof(Value));

                // 推入模块帧（保存主程序恢复信息）
                CallFrame mframe;
                mframe.func = moduleFunc; mframe.closure = nullptr;
                mframe.base = base; mframe.savedBase = savedBase;
                mframe.pc = nullptr; mframe.returnPC = nullptr;
                mframe.returnBaseOffset = savedBaseOff;
                mframe.returnPcOffset = savedPc;
                mframe.resultReg = a;
                frames_.push_back(mframe);
                moduleExecDepth_ = frames_.size();

                // 内联执行模块
                while (ctx->pc < ctx->codeSize) {
                    instructionCount_++;
                    if (gcAllocated_ > GC_THRESHOLD && !gcRunning_) gc();

                    UInt8 op2 = ctx->code[ctx->pc++];
                    UInt8 p1 = ctx->code[ctx->pc++];
                    UInt8 p2 = ctx->code[ctx->pc++];
                    UInt8 p3 = ctx->code[ctx->pc++];
                    Value* base2 = ctx->base;

                    switch (op2) {
                        imp_STOREGLOBAL: {
                            Int32 nameIdx = (Int32)ctx->code[ctx->pc] |
                                            ((Int32)ctx->code[ctx->pc+1] << 8) |
                                            ((Int32)ctx->code[ctx->pc+2] << 16) |
                                            ((Int32)ctx->code[ctx->pc+3] << 24);
                            ctx->pc += 15;
                            if (nameIdx >= 0 && nameIdx < static_cast<Int32>(ctx->func->constants.size())) {
                                const Value& keyVal = ctx->func->constants[nameIdx];
                                std::string key;
                                if (keyVal.isString()) {
                                    key = std::string(keyVal.asString()->data, keyVal.asString()->length);
                                } else {
                                    key = keyVal.toString();
                                }

                                globals_[key] = base2[p1];
                            }
                            break;
                        }
                        imp_LOADINT: {
                            // emitInt format: p2 is the 8-bit constant index
                            Int32 idx = p2;
                            if (idx >= 0 && idx < static_cast<Int32>(ctx->func->constants.size())) {
                                base2[p1] = ctx->func->constants[idx];
                            } else {
                                base2[p1] = Value::Int(p2);
                            }
                            break;
                        }
                        imp_LOADCONST: {
                            // emitInt format: p2 is the 8-bit constant index
                            Int32 idx = p2;
                            if (idx >= 0 && idx < static_cast<Int32>(ctx->func->constants.size())) {
                                base2[p1] = ctx->func->constants[idx];
                            }
                            break;
                        }
                        imp_MOVE: {
                            base2[p3] = base2[p2];
                            break;
                        }
                        imp_RETURN: {
                            delete[] modBuf;
                            moduleExecDepth_ = 0;
                            goto import_done;
                        }
                        default:
                            break;
                    }
                }

                delete[] modBuf;
                import_done:
                // 恢复主程序上下文
                ctx->pc = savedPc;
                ctx->func = savedFunc;
                ctx->code = savedCode;
                ctx->codeSize = savedCodeSize;
                ctx->baseOffset = savedBaseOff;
                base = savedBase;
                ctx->base = base;
                RA(a) = Value::Bool(true);
                break;
            }

default: {
                char buf[64];
                std::snprintf(buf, sizeof(buf), "unknown opcode 0x%02X", op);
                EMIT_ERR(buf);
            }
        }
    }
}

bool VM::loadModule(VMFunction* func) {
    if (!func) { error_ = "空函数"; return false; }
    trackGC(reinterpret_cast<VMObject*>(func));
    for (auto& v : func->constants) {
        if (v.isObject()) trackGC(v.asPtr());
    }

    size_t codeSz = func->code.size();
    UInt8* codeBuf = new UInt8[codeSz + 256];
    std::memcpy(codeBuf, func->code.data(), codeSz);
    for (size_t i = codeSz; i < codeSz + 256; i++) codeBuf[i] = OP_NOP;



    for (size_t i = 0; i < 16 && i < codeSz+256; i++) {

    }


    ExecContext ctx;
    ctx.func = func;
    ctx.code = codeBuf;
    ctx.codeSize = codeSz + 256;
    ctx.pc = 0;
    ctx.baseOffset = 0;
    ctx.base = stack_.data();
    std::memset(ctx.base, 0, MAX_STACK * sizeof(Value));
    CallFrame frame;
    frame.func = func; frame.closure = nullptr;
    frame.base = ctx.base; frame.savedBase = ctx.base;
    frame.pc = nullptr; frame.returnPC = nullptr;
    frame.returnBaseOffset = 0; frame.returnPcOffset = 0;
    frame.resultReg = 0;
    frames_.push_back(frame);

    bool ok = run(&ctx);
    delete[] codeBuf;
    return ok;
}

} // namespace cplang



