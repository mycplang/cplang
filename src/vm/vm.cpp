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
    youngObjects_ = nullptr;
    gcAllocated_ = 0;
    youngAllocated_ = 0;
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
    // 清理新生代
    VMObject* yp = youngObjects_;
    while (yp) {
        VMObject* next = yp->next;
        delete yp;
        yp = next;
    }
    youngObjects_ = nullptr;
    // 清理老年代
    VMObject* p = allObjects_;
    while (p) {
        VMObject* next = p->next;
        delete p;
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

void VM::setGlobal(const char* name, Value v) {
    globals_[std::string(name)] = v;
    Int32 slot = getOrCreateGlobalSlot(name);
    if (slot >= 0) {
        if (slot >= static_cast<Int32>(globalSlots_.size()))
            globalSlots_.resize(slot + 1);
        globalSlots_[slot] = v;
    }
}

void VM::raiseError(const std::string& msg) {
    error_ = msg;
}

void VM::raiseError(const char* msg) {
    error_ = msg;
}

void VM::resetExecutionState() {
    // 清空执行栈和调用帧，保留全局变量表
    stack_.clear();
    top_ = nullptr;
    frames_.clear();
    handlerStack_.clear();
    error_.clear();
    moduleExecDepth_ = 0;
    lastImportedFunc_ = nullptr;
    debugPaused_ = false;
    debugStepMode_ = false;
    debugStop_ = false;
    debugStepDepth_ = 0;
}

// ═══════════════════════════════════════════════════════════════
//  分代GC：trackGC + 写屏障
// ═══════════════════════════════════════════════════════════════

bool VM::isPointerToYoung(const Value& v) const {
    if (!v.isPtr() || !v.asPtr()) return false;
    return v.asPtr()->isYoung();
}

void VM::trackGC(VMObject* obj) {
    if (!obj) return;
    // 检查已在新生代链表中
    VMObject* p = youngObjects_;
    while (p) {
        if (p == obj) return;
        p = p->next;
    }
    // 检查已在老年代链表中
    p = allObjects_;
    while (p) {
        if (p == obj) return;
        p = p->next;
    }
    // 新对象放入新生代
    obj->next = youngObjects_;
    youngObjects_ = obj;
    youngAllocated_ += obj->size;
    // 新生代阈值触发 minor GC
    if (youngAllocated_ > NURSERY_THRESHOLD && !gcRunning_) gcMinor();
    // 老年代阈值触发 major GC
    if (gcAllocated_ > GC_THRESHOLD && !gcRunning_) gcMajor();
}

void VM::gcWriteBarrier(VMObject* container, const Value& newVal) {
    if (!container || !container->isOld()) return; // 只关心old→young引用
    if (!isPointerToYoung(newVal)) return;
    rememberedSet_.insert(container);
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
            gcMarkValue(kv.first);
            gcMarkValue(kv.second);
        }
    } else if (obj->typeTag == ObjectHeader::TAG_UNORDERED_SET) {
        auto us = static_cast<VMUnorderedSet*>(obj);
        for (auto& v : us->data) gcMarkValue(v);
    } else if (obj->typeTag == ObjectHeader::TAG_UNORDERED_MULTISET) {
        auto ums = static_cast<VMUnorderedMultiSet*>(obj);
        for (auto& v : ums->data) gcMarkValue(v);
    } else if (obj->typeTag == ObjectHeader::TAG_UNORDERED_MAP) {
        auto um = static_cast<VMUnorderedMap*>(obj);
        for (auto& kv : um->data) {
            gcMarkValue(kv.first);
            gcMarkValue(kv.second);
        }
    } else if (obj->typeTag == ObjectHeader::TAG_UNORDERED_MULTIMAP) {
        auto umm = static_cast<VMUnorderedMultiMap*>(obj);
        for (auto& kv : umm->data) {
            gcMarkValue(kv.first);
            gcMarkValue(kv.second);
        }
    // VMTable: 标记所有键值对
    } else if (obj->typeTag == ObjectHeader::TAG_TABLE) {
        auto tbl = static_cast<VMTable*>(obj);
        for (auto& kv : tbl->data) {
            gcMarkValue(kv.first);
            gcMarkValue(kv.second);
        }
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
            gcMarkValue(kv.first);
            gcMarkValue(kv.second);
        }
    } else if (obj->typeTag == ObjectHeader::TAG_ORDERED_SET) {
        auto os = static_cast<VMOrderedSet*>(obj);
        for (auto& v : os->data) gcMarkValue(v);
    } else if (obj->typeTag == ObjectHeader::TAG_ORDERED_MAP) {
        auto om = static_cast<VMOrderedMap*>(obj);
        for (auto& kv : om->data) {
            gcMarkValue(kv.first);
            gcMarkValue(kv.second);
        }
    } else if (obj->typeTag == ObjectHeader::TAG_BOXED_INT64) {
        // BoxedInt64: no Value children, only raw Int64
        return;
    } else if (obj->typeTag == ObjectHeader::TAG_BOXED_FLOAT) {
        // BoxedFloat: no Value children, only raw Float64
        return;
    } else if (obj->typeTag == ObjectHeader::TAG_GENERATOR) {
        // 生成器对象：标记栈中的值、函数、闭包、上值
        auto gen = static_cast<VMGenerator*>(obj);
        for (auto& v : gen->stack) gcMarkValue(v);
        gcMarkObject(reinterpret_cast<VMObject*>(gen->func));
        gcMarkObject(reinterpret_cast<VMObject*>(gen->closure));
        for (auto uv : gen->upvalues) gcMarkObject(reinterpret_cast<VMObject*>(uv));
    } else if (obj->typeTag == ObjectHeader::TAG_PROMISE) {
        // 承诺对象：标记结果、回调、等待的协程
        auto prom = static_cast<VMPromise*>(obj);
        gcMarkValue(prom->result);
        for (auto& cb : prom->thenCallbacks) gcMarkValue(cb);
        for (auto& cb : prom->catchCallbacks) gcMarkValue(cb);
        for (auto* coro : prom->waitingCoroutines) {
            gcMarkObject(reinterpret_cast<VMObject*>(coro));
        }
    } else if (obj->typeTag == ObjectHeader::TAG_BYTEARRAY) {
        auto ba = static_cast<VMByteArray*>(obj);
        if (ba->parent) gcMarkObject(reinterpret_cast<VMObject*>(ba->parent));
    }
    // Raylib types: no CP Value children to mark, GC cleanup via destructors
}

void VM::gcMarkRoots() {
    for (auto& g : globals_) gcMarkValue(g.second);
    // 标记全局槽位中的值（Slot优化存储的全局变量）
    for (auto& s : globalSlots_) gcMarkValue(s);
    // 标记新生代对象（minor GC时需要将young对象作为根）
    VMObject* yp = youngObjects_;
    while (yp) {
        if (yp->color != GCColor::BLACK) gcMarkObject(yp);
        yp = yp->next;
    }
    
    // 标记栈上的所有值
    // 由于 top_ 可能未正确维护，我们通过当前上下文和调用帧来计算栈顶
    Value* stackBottom = stack_.data();
    Value* stackTop = stackBottom;
    
    if (currentCtx_ && currentCtx_->base && currentCtx_->func) {
        // 当前函数的栈顶 = base + maxStack
        stackTop = currentCtx_->base + currentCtx_->func->maxStack;
    } else if (!frames_.empty()) {
        // 回退：使用最后一个调用帧的 base
        stackTop = frames_.back().base + 256; // 保守估计
    }
    
    // 确保不越界
    Value* stackEnd = stackBottom + stack_.size();
    if (stackTop > stackEnd) stackTop = stackEnd;
    if (stackTop < stackBottom) stackTop = stackBottom;
    
    for (Value* p = stackBottom; p < stackTop; p++) {
        gcMarkValue(*p);
    }
    
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

// ═══════════════════════════════════════════════════════════════
//  Minor GC：仅回收新生代（快速路径）
// ═══════════════════════════════════════════════════════════════

void VM::gcMinor() {
    if (gcRunning_) return;
    if (!youngObjects_) return;
    gcRunning_ = true;
    
    // 1. 标记所有根
    gcMarkRoots();
    
    // 2. 从remembered set（old→young引用）继续标记
    for (auto* oldObj : rememberedSet_) {
        if (oldObj) gcMarkObject(oldObj);
    }
    
    // 3. 仅清扫新生代，幸存者晋升
    gcSweepYoungPhase();
    
    rememberedSet_.clear();
    gcCount_++;
    gcRunning_ = false;
}

// ═══════════════════════════════════════════════════════════════
//  Major GC：全堆回收
// ═══════════════════════════════════════════════════════════════

void VM::gcMajor() {
    if (gcRunning_) return;
    gcRunning_ = true;
    
    // 先做一次minor清理新生代垃圾
    if (youngObjects_) {
        gcMarkRoots();
        for (auto* oldObj : rememberedSet_) {
            if (oldObj) gcMarkObject(oldObj);
        }
        gcSweepYoungPhase();
        rememberedSet_.clear();
    }
    
    // 全堆标记（old gen + 剩余young）
    gcMarkRoots();
    
    // 标记所有老年代对象
    VMObject* p = allObjects_;
    while (p) {
        gcMarkObject(p);
        p = p->next;
    }
    
    // 全堆清扫
    gcSweepPhase();
    gcCount_++;
    gcRunning_ = false;
}

// ═══════════════════════════════════════════════════════════════
//  仅清扫新生代（晋升幸存者，回收死者）
// ═══════════════════════════════════════════════════════════════

void VM::gcSweepYoungPhase() {
    VMObject** p = &youngObjects_;
    while (*p) {
        if ((*p)->color != GCColor::BLACK) {
            // 死者：回收
            VMObject* dead = *p;
            *p = dead->next;
            youngAllocated_ -= dead->size;
            dead->~VMObject();
            ::operator delete(dead);
        } else {
            // 幸存者
            (*p)->incSurvivals();
            if ((*p)->survivals() >= PROMOTE_AFTER) {
                // 晋升到老年代
                VMObject* promoted = *p;
                *p = promoted->next;
                youngAllocated_ -= promoted->size;
                promoted->promote();
                promoted->next = allObjects_;
                allObjects_ = promoted;
                gcAllocated_ += promoted->size;
            } else {
                // 留在新生代
                (*p)->setWhite();
                p = &((*p)->next);
            }
        }
    }
}

void VM::gc() {
    // gc() now delegates to generational collection
    // Minor GC is preferred (faster, less disruptive)
    gcMinor();
    // If old gen is also pressured, do major
    if (gcAllocated_ > GC_THRESHOLD) gcMajor();
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

// ========== 生成器支持（P9.1） ==========
Value VM::resumeGenerator(VMGenerator* gen, const Value& sendValue) {
    if (!gen || gen->isDone || !gen->func) {
        return Value::nil();
    }
    
    VMFunction* func = gen->func;
    if (func->code.empty()) return Value::nil();
    
    // 使用生成器自己的栈
    std::vector<Value>& genStack = gen->stack;
    if (genStack.empty()) {
        genStack.resize(func->maxStack, Value::nil());
    }
    
    // 非首次调用：将发送值作为 yield 表达式的返回值，写入 yield 寄存器
    if (gen->pcOffset > 0) {
        genStack[gen->yieldReg] = sendValue;
    }
    
    ExecContext ctx;
    ctx.func = func;
    ctx.code = const_cast<UInt8*>(func->code.data());
    ctx.codeSize = func->code.size();
    ctx.pc = gen->pcOffset;
    ctx.baseOffset = gen->baseOffset;
    ctx.base = genStack.data();
    
    // 推入临时调用帧（标记为生成器帧）
    Int32 savedCallDepth = callDepth_;
    Int32 entryDepth = (Int32)frames_.size();
    callDepth_ = entryDepth;
    CallFrame frame;
    frame.func = func;
    frame.closure = gen->closure;
    frame.generator = gen;  // 标记为生成器帧
    frame.base = genStack.data();
    frame.savedBase = genStack.data();
    frame.baseOffset = gen->baseOffset;
    frames_.push_back(frame);
    
    bool ok = run(&ctx);
    
    callDepth_ = savedCallDepth;
    
    // 清理帧（如果还没被弹出）
    if ((Int32)frames_.size() > entryDepth) {
        frames_.pop_back();
    }
    
    // 执行完毕（遇到 OP_RETURN），标记为完成
    if (ok && gen->pcOffset >= (Int32)func->code.size()) {
        gen->isDone = true;
    }
    
    // 返回产出值（栈顶或寄存器0）
    // 简化：返回栈[0]，OP_YIELD 和 OP_RETURN 都会把值放在那里
    return ok ? genStack[0] : Value::nil();
}

// ========== 协程/异步 支持（P9.3） ==========

VMPromise* VM::createPromise() {
    VMPromise* p = VMPromise::create();
    trackGC(reinterpret_cast<VMObject*>(p));
    return p;
}

void VM::resolvePromise(VMPromise* promise, const Value& value) {
    if (!promise || promise->state != VMPromise::PENDING) return;
    
    promise->resolve(value);
    
    // 调度 then 回调
    for (auto& cb : promise->thenCallbacks) {
        enqueueMicrotask(cb, value);
    }
    promise->thenCallbacks.clear();
    
    // 恢复等待此承诺的协程
    for (auto* coro : promise->waitingCoroutines) {
        // 将协程恢复作为微任务
        // 注意：这里我们需要一个特殊的"恢复协程"回调
        // 简化实现：直接恢复（同步），完整实现应该加入调度队列
        // 为了简单，我们使用微任务机制
        // 但恢复协程需要 VM 的 resumeGenerator，所以我们用一个特殊方式
        // 先存起来，runMicrotasks 里处理
        microtaskQueue_.emplace_back(Value::nil(), Value::nil());  // placeholder
        // 实际实现中，我们需要一个更好的方式
        // 这里先简化：同步恢复
        (void)coro;  // 暂时忽略，后面用回调方式
    }
    promise->waitingCoroutines.clear();
    
    // 立即运行微任务（类似 Promise 的 microtask 队列）
    runMicrotasks();
}

void VM::rejectPromise(VMPromise* promise, const Value& reason) {
    if (!promise || promise->state != VMPromise::PENDING) return;
    
    promise->reject(reason);
    
    // 调度 catch 回调
    for (auto& cb : promise->catchCallbacks) {
        enqueueMicrotask(cb, reason);
    }
    promise->catchCallbacks.clear();
    
    runMicrotasks();
}

Value VM::awaitPromise(VMPromise* promise) {
    if (!promise) return Value::nil();
    
    // 如果承诺已经完成，直接返回结果
    if (promise->state == VMPromise::FULFILLED) {
        return promise->result;
    }
    if (promise->state == VMPromise::REJECTED) {
        // 简化：返回 nil，完整实现应该抛出异常
        return Value::nil();
    }
    
    // 如果有当前协程，暂停它
    if (currentCoroutine_) {
        promise->waitingCoroutines.push_back(currentCoroutine_);
        // TODO: 暂停当前协程
        // 这需要在 OP_AWAIT 层面处理，类似 yield
    }
    
    return Value::nil();
}

void VM::enqueueMicrotask(const Value& callback, const Value& arg) {
    microtaskQueue_.emplace_back(callback, arg);
}

void VM::runMicrotasks() {
    while (!microtaskQueue_.empty()) {
        auto task = microtaskQueue_.front();
        microtaskQueue_.erase(microtaskQueue_.begin());
        
        Value callback = task.first;
        Value arg = task.second;
        
        // 跳过 placeholder
        if (callback.isNil()) continue;
        
        // 执行回调
        std::vector<Value> args = {arg};
        callFunction(callback, args);
    }
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

// ═══════════════════════════════════════════════════════════════
//  事件循环
// ═══════════════════════════════════════════════════════════════

EventLoop* VM::getEventLoop() {
    if (!eventLoop_) {
        eventLoop_ = std::make_unique<EventLoop>();
    }
    return eventLoop_.get();
}

void VM::startEventLoop() {
    if (!eventLoop_) eventLoop_ = std::make_unique<EventLoop>();
    if (!eventLoop_->isRunning()) eventLoop_->start();
}

void VM::stopEventLoop() {
    if (eventLoop_) eventLoop_->stop();
}

} // namespace cplang