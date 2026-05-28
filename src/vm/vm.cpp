// ============================================================
// vm.cpp — Threaded-code VM 主实现（活跃版本）
//
// 这是 CP 语言的活跃 VM 实现。
// 历史存档见 vm_switch.cpp（switch-dispatch 风格，未编译）。
//
// 如果未来需要提取共享工厂方法到 vm_objects.cpp，
// 请确保同时移除 vm_switch.cpp 中的重复定义。
// ============================================================

#include "vm/vm.hpp"
#include "jit/orc_jit.hpp"
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
thread_local VM* VM::currentVM_ = nullptr;
}

namespace cplang {

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
    for (auto& v : globalSlots_) { v = Value::nil(); }

    // 初始化随机种子
    srand((unsigned int)::time(nullptr));
}

VM::~VM() {
    // 统一使用 delete 清理所有对象（VMString/VMNativeFunc 都改用 new 分配）
    VMObject* p = allObjects_;
    while (p) {
        VMObject* next = p->next;
        delete p;  // 虚析构会自动调用派生类析构
        p = next;
    }
    allObjects_ = nullptr;
}

void VM::registerGlobal(const char* name, Value val) {
    // Slot化优化：同时存入slot表和兼容表
    Int32 slot = getOrCreateGlobalSlot(name);
    if (slot >= 0) {
        if (slot >= static_cast<Int32>(globalSlots_.size())) {
            globalSlots_.resize(slot + 1);
        }
        globalSlots_[slot] = val;
    }
    
    // 保留兼容
    globals_[std::string(name)] = val;
}

Int32 VM::getOrCreateGlobalSlot(const char* name) {
    auto it = globalNameToSlot_.find(name);
    if (it != globalNameToSlot_.end()) {
        return it->second;
    }

    if (nextGlobalSlot_ >= MAX_GLOBAL_SLOTS) return -1;

    UInt16 slot = nextGlobalSlot_++;
    globalNameToSlot_[name] = slot;
    return slot;
}

std::vector<std::string> VM::getGlobalSlotNames() const {
    std::vector<std::string> names;
    names.reserve(globalNameToSlot_.size() + globals_.size());
    for (const auto& pair : globalNameToSlot_) {
        names.push_back(pair.first);
    }
    for (const auto& pair : globals_) {
        if (globalNameToSlot_.find(pair.first) == globalNameToSlot_.end()) {
            names.push_back(pair.first);
        }
    }
    return names;
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
    Value v = makeFunctionVal(reinterpret_cast<VMFunction*>(nf));
    
    // Slot化：同时存入slot表
    Int32 slot = getOrCreateGlobalSlot(name);
    if (slot >= 0) {
        if (slot >= static_cast<Int32>(globalSlots_.size())) {
            globalSlots_.resize(slot + 1);
        }
        globalSlots_[slot] = v;
    }
    
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

void VM::raiseError(const char* msg) {
    error_ = msg;
}

void VM::trackGC(VMObject* obj) {
    if (!obj) return;
    // Check if already tracked to avoid double-free
    VMObject* p = allObjects_;
    while (p) {
        if (p == obj) return;  // Already tracked
        p = p->next;
    }
    obj->next = allObjects_;
    allObjects_ = obj;
    gcAllocated_ += obj->size;
    if (gcAllocated_ > GC_THRESHOLD && !gcRunning_) gc();
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
    } else if (obj->typeTag == ObjectHeader::TAG_SET) {
        auto s = static_cast<VMSet*>(obj);
        for (auto& v : s->data) gcMarkValue(v);
    } else if (obj->typeTag == ObjectHeader::TAG_STACK) {
        auto s = static_cast<VMStack*>(obj);
        for (auto& v : s->data) gcMarkValue(v);
    } else if (obj->typeTag == ObjectHeader::TAG_QUEUE) {
        auto q = static_cast<VMQueue*>(obj);
        for (auto& v : q->data) gcMarkValue(v);
    } else if (obj->typeTag == ObjectHeader::TAG_DEQUE) {
        auto d = static_cast<VMDeque*>(obj);
        for (auto& v : d->data) gcMarkValue(v);
    } else if (obj->typeTag == ObjectHeader::TAG_PRIORITY_QUEUE) {
        auto pq = static_cast<VMPriorityQueue*>(obj);
        for (auto& e : pq->heap) gcMarkValue(e.value);
    } else if (obj->typeTag == ObjectHeader::TAG_LINKEDLIST) {
        auto l = static_cast<VMLinkedList*>(obj);
        for (auto& v : l->data) gcMarkValue(v);
    } else if (obj->typeTag == ObjectHeader::TAG_SLINKEDLIST) {
        auto sl = static_cast<VMSLinkedList*>(obj);
        for (auto& v : sl->data) gcMarkValue(v);
    } else if (obj->typeTag == ObjectHeader::TAG_MULTISET) {
        auto ms = static_cast<VMMultiSet*>(obj);
        for (auto& v : ms->data) gcMarkValue(v);
    } else if (obj->typeTag == ObjectHeader::TAG_MULTIMAP) {
        auto mm = static_cast<VMMultiMap*>(obj);
        for (auto& kv : mm->data) {
            gcMarkValue(const_cast<Value&>(kv.first));
            gcMarkValue(kv.second);
        }
    } else if (obj->typeTag == ObjectHeader::TAG_UNORDERED_SET) {
        auto us = static_cast<VMUnorderedSet*>(obj);
        for (auto& v : us->data) gcMarkValue(const_cast<Value&>(v));
    } else if (obj->typeTag == ObjectHeader::TAG_UNORDERED_MULTISET) {
        auto ums = static_cast<VMUnorderedMultiSet*>(obj);
        for (auto& v : ums->data) gcMarkValue(const_cast<Value&>(v));
    } else if (obj->typeTag == ObjectHeader::TAG_UNORDERED_MAP) {
        auto um = static_cast<VMUnorderedMap*>(obj);
        for (auto& kv : um->data) {
            gcMarkValue(const_cast<Value&>(kv.first));
            gcMarkValue(kv.second);
        }
    } else if (obj->typeTag == ObjectHeader::TAG_UNORDERED_MULTIMAP) {
        auto umm = static_cast<VMUnorderedMultiMap*>(obj);
        for (auto& kv : umm->data) {
            gcMarkValue(const_cast<Value&>(kv.first));
            gcMarkValue(kv.second);
        }
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
    } else if (obj->typeTag == ObjectHeader::TAG_MAP) {
        auto mp = static_cast<VMMap*>(obj);
        for (auto& kv : mp->data) {
            gcMarkValue(const_cast<Value&>(kv.first));
            gcMarkValue(kv.second);
        }
    } else if (obj->typeTag == ObjectHeader::TAG_ORDERED_SET) {
        auto os = static_cast<VMOrderedSet*>(obj);
        for (auto& v : os->data) gcMarkValue(const_cast<Value&>(v));
    } else if (obj->typeTag == ObjectHeader::TAG_ORDERED_MAP) {
        auto om = static_cast<VMOrderedMap*>(obj);
        for (auto& kv : om->data) {
            gcMarkValue(const_cast<Value&>(kv.first));
            gcMarkValue(kv.second);
        }
    } else if (obj->typeTag == ObjectHeader::TAG_BOXED_INT64) {
        // BoxedInt64: no Value children, only raw Int64
        return;
    } else if (obj->typeTag == ObjectHeader::TAG_BOXED_FLOAT) {
        // BoxedFloat: no Value children, only raw Float64
        return;
    }
    // Raylib types: no CP Value children to mark, GC cleanup via destructors
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
    currentVM_ = this;  // 允许原生函数通过VM::current()回调
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

Value VM::callFunction(Value callee, std::vector<Value>& args) {
    // 原生函数直接调用
    if (callee.asPtr() && callee.asPtr()->typeTag == ObjectHeader::TAG_NATIVE) {
        VMNativeFunc* nf = reinterpret_cast<VMNativeFunc*>(callee.asPtr());
        if (nf && nf->fn) return nf->fn(args);
        return Value::nil();
    }
    
    // CP 用户定义函数
    VMFunction* func = nullptr;
    if (callee.isFunction() && callee.asPtr() && callee.asPtr()->typeTag == ObjectHeader::TAG_FUNCTION) {
        func = callee.asFunction();
    }
    if (!func || func->code.empty()) return Value::nil();
    
    // 用独立栈避免与主执行栈冲突（简单函数不会触发GC）
    std::vector<Value> funcStack(256, Value::nil());
    for (size_t i = 0; i < args.size() && i < funcStack.size(); i++) {
        funcStack[i] = args[i];
    }
    
    ExecContext ctx;
    ctx.func = func;
    ctx.code = const_cast<UInt8*>(func->code.data());
    ctx.codeSize = func->code.size();
    ctx.pc = 0;
    ctx.baseOffset = 0;
    ctx.base = funcStack.data();
    
    // 推入临时调用帧
    Int32 savedCallDepth = callDepth_;
    Int32 entryDepth = (Int32)frames_.size();
    callDepth_ = entryDepth;  // 记录入口帧深度
    CallFrame frame;
    frame.func = func;
    frame.closure = nullptr;
    frame.base = funcStack.data();
    frame.savedBase = funcStack.data();
    frame.baseOffset = 0;
    frames_.push_back(frame);

    bool ok = run(&ctx);

    // 恢复 callDepth_（防止影响外层 run() 的 OP_RETURN 判断）
    callDepth_ = savedCallDepth;

    // 仅在OP_RETURN未弹出帧时才清理（正常路径已弹出）
    if ((Int32)frames_.size() > entryDepth) {
        frames_.pop_back();
    }

    return ok ? funcStack[0] : Value::nil();
}

int VM::getCurrentLine() const {
    if (!currentCtx_ || !currentCtx_->func) return 0;
    size_t idx = currentCtx_->pc / 16;
    if (idx < currentCtx_->func->lineInfo.size()) {
        return currentCtx_->func->lineInfo[idx];
    }
    return 0;
}

std::string VM::getCurrentSourceFile() const {
    if (!currentCtx_ || !currentCtx_->func) return "<unknown>";
    return currentCtx_->func->sourceFile.empty() ? "<unknown>" : currentCtx_->func->sourceFile;
}

std::string VM::getCurrentFunction() const {
    if (!currentCtx_ || !currentCtx_->func) return "<unknown>";
    auto* name = currentCtx_->func->name;
    if (name && name->data) {
        return std::string(name->data, name->length);
    }
    return "<top-level>";
}

} // namespace cplang