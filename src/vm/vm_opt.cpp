// CP语言 高性能虚拟机实现 — 优化版本
#include "vm/vm_opt.hpp"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <cstring>

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  VM对象创建
// ═══════════════════════════════════════════════════════════════════
VMString* VMString::create(const char* s, UInt32 len) {
    VMString* str = (VMString*)std::malloc(sizeof(VMString) + len);
    str->typeTag = T_STRING;
    str->length = len;
    std::memcpy(str->data, s, len);
    str->data[len] = '\0';
    // 简单hash
    UInt32 h = 0;
    for (UInt32 i = 0; i < len; i++) h = h * 31 + (UInt8)s[i];
    str->hash = h;
    return str;
}

VMString* VMString::create(const std::string& s) {
    return create(s.c_str(), static_cast<UInt32>(s.length()));
}

VMArray* VMArray::create(UInt32 cap) {
    VMArray* arr = new VMArray();
    arr->typeTag = T_ARRAY;
    if (cap > 0) arr->data.reserve(cap);
    return arr;
}

Value VMArray::get(Int64 index) {
    if (index < 0 || index >= static_cast<Int64>(data.size())) return Value::nil();
    return data[static_cast<size_t>(index)];
}

void VMArray::set(Int64 index, const Value& v) {
    if (index < 0) return;
    if (index >= static_cast<Int64>(data.size())) {
        data.resize(static_cast<size_t>(index) + 1, Value::nil());
    }
    data[static_cast<size_t>(index)] = v;
}

VMTable* VMTable::create() {
    VMTable* tbl = new VMTable();
    tbl->typeTag = T_TABLE;
    return tbl;
}

Value VMTable::get(const Value& key) {
    for (auto& p : data) {
        if (p.first.equals(key)) return p.second;
    }
    return Value::nil();
}

void VMTable::set(const Value& key, const Value& val) {
    for (auto& p : data) {
        if (p.first.equals(key)) {
            p.second = val;
            return;
        }
    }
    data.push_back({key, val});
}

VMClosure* VMClosure::create(VMFunction* f) {
    VMClosure* c = new VMClosure();
    c->typeTag = T_CLOSURE;
    c->func = f;
    c->upvalues.resize(f->numLocals, nullptr);
    return c;
}

VMUpvalue* VMUpvalue::create(Value* slot) {
    VMUpvalue* uv = new VMUpvalue();
    uv->typeTag = T_UPVALUE;
    uv->location = slot;
    return uv;
}

// ═══════════════════════════════════════════════════════════════════
//  VM实现
// ═══════════════════════════════════════════════════════════════════
VM::VM() {
    stack_.resize(MAX_STACK);
    globalSlots_.resize(MAX_GLOBALS);
}

VM::~VM() {
    // 清理所有对象
    VMObject* obj = allObjects_;
    while (obj) {
        VMObject* next = obj->next;
        delete obj;
        obj = next;
    }
}

void VM::trackGC(VMObject* obj) {
    if (!obj) return;
    obj->next = allObjects_;
    allObjects_ = obj;
    gcAllocated_ += sizeof(*obj);  // 简化计算
}

VMString* VM::internString(const char* s, UInt32 len) {
    std::string key(s, len);
    auto it = stringTable_.find(key);
    if (it != stringTable_.end()) return it->second;
    VMString* str = VMString::create(s, len);
    trackGC(str);
    stringTable_[key] = str;
    return str;
}



void VM::registerGlobal(const char* name, Value val) {
    Int32 slot = getGlobalSlot(name);
    if (slot >= 0) globalSlots_[slot] = val;
}

void VM::registerNative(const char* name, VMNativeFunc::Fn fn) {
    VMNativeFunc* nf = new VMNativeFunc();
    nf->typeTag = T_NATIVE;
    nf->fn = fn;
    nf->name = internString(name, static_cast<UInt32>(std::strlen(name)));
    trackGC(nf);
    registerGlobal(name, Value{.tag=T_NATIVE, .native=nf});
}

void VM::raiseError(const char* msg) {
    error_ = msg;
}

// ═══════════════════════════════════════════════════════════════════
//  核心执行循环 — Direct Threading实现
// ═══════════════════════════════════════════════════════════════════

// 指令解码宏（4字节格式）
#define GET_OP() (pc_[0])
#define GET_A()  (pc_[1])
#define GET_B()  (pc_[2])
#define GET_C()  (pc_[3])
#define GET_sBx() (static_cast<Int16>((pc_[2]) | ((pc_[3]) << 8)))

// 寄存器访问
#define RA() (base_ + GET_A())
#define RB() (base_ + GET_B())
#define RC() (base_ + GET_C())
#define R(n) (base_ + (n))

// 下一条指令
#define NEXT() do { pc_ += 4; goto dispatch; } while(0)

// 跳转
#define JUMP() do { pc_ += 4 + GET_sBx() * 4; goto dispatch; } while(0)

bool VM::call(VMFunction* func, int argc, Value* argv) {
    if (!func) { raiseError("null function"); return false; }
    
    // 初始化执行上下文
    func_ = func;
    pc_ = func->code.data();
    base_ = stack_.data();
    frameCount_ = 0;
    instructionCount_ = 0;
    error_.clear();
    
    // 设置参数
    for (int i = 0; i < argc && i < static_cast<int>(func->numParams); i++) {
        base_[i] = argv[i];
    }
    
    return run();
}

bool VM::run() {
    // Direct Threading: 使用标签地址表
    static const void* dispatchTable[] = {
        &&OP_LOADNIL, &&OP_LOADBOOL, &&OP_LOADINT, &&OP_LOADFLT,
        &&OP_LOADSTR, &&OP_LOADCONST, &&OP_MOVE,
        &&OP_LOADGLOBAL, &&OP_STOREGLOBAL,
        &&OP_UNUSED, &&OP_UNUSED, &&OP_UNUSED, &&OP_UNUSED,
        &&OP_UNUSED, &&OP_UNUSED, &&OP_UNUSED,  // 0x09-0x0F
        &&OP_LOADLOCAL, &&OP_STORELOCAL,        // 0x10-0x11
        &&OP_UNUSED, &&OP_UNUSED, &&OP_UNUSED, &&OP_UNUSED,
        &&OP_UNUSED, &&OP_UNUSED, &&OP_UNUSED, &&OP_UNUSED,
        &&OP_UNUSED, &&OP_UNUSED, &&OP_UNUSED, &&OP_UNUSED,
        &&OP_UNUSED, &&OP_UNUSED,               // 0x12-0x1F
        &&OP_ADD, &&OP_SUB, &&OP_MUL, &&OP_DIV, // 0x20-0x23
        &&OP_IDIV, &&OP_MOD, &&OP_POW, &&OP_NEG,
        &&OP_BAND, &&OP_BOR, &&OP_BXOR,
        &&OP_BSHL, &&OP_BSHR, &&OP_BNOT,        // 0x24-0x2D
        &&OP_UNUSED, &&OP_UNUSED,               // 0x2E-0x2F
        &&OP_CMPEQ, &&OP_CMPNE,                 // 0x30-0x31
        &&OP_CMPLT, &&OP_CMPLE, &&OP_CMPGT, &&OP_CMPGE,
        &&OP_UNUSED, &&OP_UNUSED, &&OP_UNUSED, &&OP_UNUSED,
        &&OP_UNUSED, &&OP_UNUSED, &&OP_UNUSED, &&OP_UNUSED,
        &&OP_UNUSED, &&OP_UNUSED, &&OP_UNUSED, &&OP_UNUSED,  // 0x36-0x3F
        &&OP_JUMP, &&OP_JUMPIF, &&OP_JUMPNIF,   // 0x40-0x42
        &&OP_UNUSED, &&OP_UNUSED, &&OP_UNUSED, &&OP_UNUSED,
        &&OP_UNUSED, &&OP_UNUSED, &&OP_UNUSED, &&OP_UNUSED,
        &&OP_UNUSED, &&OP_UNUSED, &&OP_UNUSED, &&OP_UNUSED,
        &&OP_UNUSED, &&OP_UNUSED,               // 0x43-0x4F
        &&OP_CALL, &&OP_UNUSED, &&OP_RETURN,    // 0x50-0x52
    };
    
    #define DISPATCH() goto dispatch
    
    dispatch:
        goto *dispatchTable[GET_OP()];
    
    // ═══════════════════════════════════════════════════════════════
    //  加载指令
    // ═══════════════════════════════════════════════════════════════
    OP_LOADNIL:
        RA()->tag = T_NIL;
        RA()->i = 0;
        NEXT();
    
    OP_LOADBOOL:
        RA()->tag = T_BOOL;
        RA()->i = GET_B();
        NEXT();
    
    OP_LOADINT:
        RA()->tag = T_INT;
        RA()->i = GET_sBx();
        NEXT();
    
    OP_LOADFLT:
        // 浮点数从常量池加载
        {
            Int16 idx = GET_sBx();
            if (idx >= 0 && idx < static_cast<Int16>(func_->constants.size())) {
                *RA() = func_->constants[idx];
            }
        }
        NEXT();
    
    OP_LOADSTR:
        {
            Int16 idx = GET_sBx();
            if (idx >= 0 && idx < static_cast<Int16>(func_->constants.size())) {
                *RA() = func_->constants[idx];
            }
        }
        NEXT();
    
    OP_LOADCONST:
        {
            Int16 idx = GET_sBx();
            if (idx >= 0 && idx < static_cast<Int16>(func_->constants.size())) {
                *RA() = func_->constants[idx];
            }
        }
        NEXT();
    
    OP_MOVE:
        *RA() = *RB();
        NEXT();
    
    // ═══════════════════════════════════════════════════════════════
    //  全局变量（Slot化，O(1)访问）
    // ═══════════════════════════════════════════════════════════════
    OP_LOADGLOBAL:
        {
            UInt16 slot = static_cast<UInt16>(GET_sBx());
            if (slot < globalSlots_.size()) {
                *RA() = globalSlots_[slot];
            }
        }
        NEXT();
    
    OP_STOREGLOBAL:
        {
            UInt16 slot = static_cast<UInt16>(GET_sBx());
            if (slot < globalSlots_.size()) {
                globalSlots_[slot] = *RA();
            }
        }
        NEXT();
    
    // ═══════════════════════════════════════════════════════════════
    //  局部变量（基于base的偏移）
    // ═══════════════════════════════════════════════════════════════
    OP_LOADLOCAL:
        *RA() = *RB();  // B是局部变量偏移
        NEXT();
    
    OP_STORELOCAL:
        *RB() = *RA();  // B是局部变量偏移
        NEXT();
    
    // ═══════════════════════════════════════════════════════════════
    //  算术运算
    // ═══════════════════════════════════════════════════════════════
    OP_ADD:
        {
            Value* ra = RA(); Value* rb = RB(); Value* rc = RC();
            if (rb->isInt() && rc->isInt()) {
                ra->tag = T_INT;
                ra->i = rb->i + rc->i;
            } else {
                ra->tag = T_FLOAT;
                ra->f = rb->toNumber() + rc->toNumber();
            }
        }
        NEXT();
    
    OP_SUB:
        {
            Value* ra = RA(); Value* rb = RB(); Value* rc = RC();
            if (rb->isInt() && rc->isInt()) {
                ra->tag = T_INT;
                ra->i = rb->i - rc->i;
            } else {
                ra->tag = T_FLOAT;
                ra->f = rb->toNumber() - rc->toNumber();
            }
        }
        NEXT();
    
    OP_MUL:
        {
            Value* ra = RA(); Value* rb = RB(); Value* rc = RC();
            if (rb->isInt() && rc->isInt()) {
                ra->tag = T_INT;
                ra->i = rb->i * rc->i;
            } else {
                ra->tag = T_FLOAT;
                ra->f = rb->toNumber() * rc->toNumber();
            }
        }
        NEXT();
    
    OP_DIV:
        {
            Value* ra = RA(); Value* rb = RB(); Value* rc = RC();
            ra->tag = T_FLOAT;
            double divisor = rc->toNumber();
            if (divisor == 0.0) { raiseError("division by zero"); return false; }
            ra->f = rb->toNumber() / divisor;
        }
        NEXT();
    
    OP_IDIV:
        {
            Value* ra = RA(); Value* rb = RB(); Value* rc = RC();
            ra->tag = T_INT;
            Int64 divisor = rc->asInt();
            if (divisor == 0) { raiseError("division by zero"); return false; }
            ra->i = rb->asInt() / divisor;
        }
        NEXT();
    
    OP_MOD:
        {
            Value* ra = RA(); Value* rb = RB(); Value* rc = RC();
            ra->tag = T_INT;
            Int64 divisor = rc->asInt();
            if (divisor == 0) { raiseError("division by zero"); return false; }
            ra->i = rb->asInt() % divisor;
        }
        NEXT();
    
    OP_POW:
        {
            Value* ra = RA(); Value* rb = RB(); Value* rc = RC();
            ra->tag = T_FLOAT;
            ra->f = std::pow(rb->toNumber(), rc->toNumber());
        }
        NEXT();
    
    OP_NEG:
        {
            Value* ra = RA(); Value* rb = RB();
            if (rb->isInt()) {
                ra->tag = T_INT;
                ra->i = -rb->i;
            } else {
                ra->tag = T_FLOAT;
                ra->f = -rb->toNumber();
            }
        }
        NEXT();
    
    // ═══════════════════════════════════════════════════════════════
    //  位运算
    // ═══════════════════════════════════════════════════════════════
    OP_BAND:
        RA()->tag = T_INT;
        RA()->i = RB()->asInt() & RC()->asInt();
        NEXT();
    
    OP_BOR:
        RA()->tag = T_INT;
        RA()->i = RB()->asInt() | RC()->asInt();
        NEXT();
    
    OP_BXOR:
        RA()->tag = T_INT;
        RA()->i = RB()->asInt() ^ RC()->asInt();
        NEXT();
    
    OP_BSHL:
        RA()->tag = T_INT;
        RA()->i = RB()->asInt() << (RC()->asInt() & 0x3F);
        NEXT();
    
    OP_BSHR:
        RA()->tag = T_INT;
        RA()->i = RB()->asInt() >> (RC()->asInt() & 0x3F);
        NEXT();
    
    OP_BNOT:
        RA()->tag = T_INT;
        RA()->i = ~RB()->asInt();
        NEXT();
    
    // ═══════════════════════════════════════════════════════════════
    //  比较运算
    // ═══════════════════════════════════════════════════════════════
    OP_CMPEQ:
        RA()->tag = T_BOOL;
        RA()->i = RB()->equals(*RC()) ? 1 : 0;
        NEXT();
    
    OP_CMPNE:
        RA()->tag = T_BOOL;
        RA()->i = RB()->equals(*RC()) ? 0 : 1;
        NEXT();
    
    OP_CMPLT:
        RA()->tag = T_BOOL;
        RA()->i = (RB()->toNumber() < RC()->toNumber()) ? 1 : 0;
        NEXT();
    
    OP_CMPLE:
        RA()->tag = T_BOOL;
        RA()->i = (RB()->toNumber() <= RC()->toNumber()) ? 1 : 0;
        NEXT();
    
    OP_CMPGT:
        RA()->tag = T_BOOL;
        RA()->i = (RB()->toNumber() > RC()->toNumber()) ? 1 : 0;
        NEXT();
    
    OP_CMPGE:
        RA()->tag = T_BOOL;
        RA()->i = (RB()->toNumber() >= RC()->toNumber()) ? 1 : 0;
        NEXT();
    
    // ═══════════════════════════════════════════════════════════════
    //  跳转指令
    // ═══════════════════════════════════════════════════════════════
    OP_JUMP:
        JUMP();
    
    OP_JUMPIF:
        if (RA()->isTrue()) {
            JUMP();
        } else {
            pc_ += 4;
            goto dispatch;
        }
    
    OP_JUMPNIF:
        if (!RA()->isTrue()) {
            JUMP();
        } else {
            pc_ += 4;
            goto dispatch;
        }
    
    // ═══════════════════════════════════════════════════════════════
    //  函数调用（优化版）
    // ═══════════════════════════════════════════════════════════════
    OP_CALL:
        {
            int calleeReg = GET_A();
            int argc = GET_B();
            Value* callee = R(calleeReg);
            
            if (callee->isNative()) {
                // 原生函数调用（零拷贝）
                VMNativeFunc* nf = callee->native;
                Value result = nf->fn(argc, R(calleeReg + 1));
                *R(calleeReg) = result;
                NEXT();
            }
            
            if (!callee->isFunction()) {
                raiseError("call on non-function");
                return false;
            }
            
            // 保存当前帧
            if (frameCount_ >= 64) {
                raiseError("call stack overflow");
                return false;
            }
            
            CallFrame* frame = &frames_[frameCount_++];
            frame->func = func_;
            frame->pc = pc_;
            frame->base = base_;
            frame->retReg = calleeReg;
            
            // 切换到新函数
            VMFunction* newFunc = callee->func;
            func_ = newFunc;
            pc_ = newFunc->code.data();
            base_ = stack_.data() + calleeReg;  // 参数就在calleeReg之后
            
            // 初始化局部变量
            for (UInt32 i = argc; i < newFunc->numLocals; i++) {
                base_[i].tag = T_NIL;
                base_[i].i = 0;
            }
        }
        goto dispatch;
    
    OP_RETURN:
        {
            int retReg = GET_A();
            Value retVal = *R(retReg);
            
            if (frameCount_ == 0) {
                // 顶层返回
                stack_[0] = retVal;
                return true;
            }
            
            // 恢复调用者帧
            CallFrame* frame = &frames_[--frameCount_];
            int resultReg = frame->retReg;
            func_ = frame->func;
            pc_ = frame->pc + 4;  // 跳过CALL指令
            base_ = frame->base;
            
            // 写入返回值
            base_[resultReg] = retVal;
        }
        goto dispatch;
    
    OP_UNUSED:
        raiseError("unknown opcode");
        return false;
    
    #undef DISPATCH
    #undef NEXT
    #undef JUMP
    #undef RA
    #undef RB
    #undef RC
    #undef R
    #undef GET_OP
    #undef GET_A
    #undef GET_B
    #undef GET_C
    #undef GET_sBx
}

// ═══════════════════════════════════════════════════════════════════
//  GC实现（分代版）
// ═══════════════════════════════════════════════════════════════════
void VM::gc() {
    // 旧GC接口兼容：调用全堆GC
    majorGc();
}

void VM::minorGc() {
    if (gcRunning_) return;
    gcRunning_ = true;
    
    // 标记所有根对象（会同时标记新生代和老生代的存活对象）
    gcMarkRoots();
    
    // 晋升存活的新生代对象到老生代
    gcPromoteSurvivors();
    
    // 清理新生代所有未存活对象
    gcSweepNewGen();
    
    // 重置新生代分配计数
    newGenAllocated_ = 0;
    minorGcCount_++;
    gcCount_++;
    gcRunning_ = false;
}

void VM::majorGc() {
    if (gcRunning_) return;
    gcRunning_ = true;
    
    // 标记所有根对象
    gcMarkRoots();
    
    // 清理新生代和老生代所有未存活对象
    gcSweepNewGen();
    gcSweepOldGen();
    
    // 重置分配计数
    gcAllocated_ = 0;
    newGenAllocated_ = 0;
    majorGcCount_++;
    gcCount_++;
    gcRunning_ = false;
}

void VM::gcPromoteSurvivors() {
    VMObject** current = &newGenObjects_;
    while (*current) {
        VMObject* obj = *current;
        if (obj->color == GCColor::BLACK) { // 对象存活
            obj->age++;
            // 年龄达到晋升阈值，晋升到老生代
            if (obj->age >= GC_PROMOTION_AGE) {
                // 从新生代移除
                *current = obj->next;
                // 添加到老生代
                obj->isOldGen = true;
                obj->next = oldGenObjects_;
                oldGenObjects_ = obj;
                // 重置颜色为白色
                obj->setWhite();
            } else {
                // 还没到晋升年龄，留在新生代，重置颜色
                obj->setWhite();
                current = &obj->next;
            }
        } else { // 对象未存活，留在链表等待sweep清理
            current = &obj->next;
        }
    }
}

void VM::gcSweepNewGen() {
    VMObject** current = &newGenObjects_;
    size_t freedSize = 0;
    
    while (*current) {
        VMObject* obj = *current;
        if (obj->color == GCColor::WHITE) { // 未存活，回收
            *current = obj->next;
            freedSize += obj->size;
            delete obj;
        } else {
            // 重置颜色为白色，下次GC使用
            obj->setWhite();
            current = &obj->next;
        }
    }
    
    gcAllocated_ -= freedSize;
}

void VM::gcSweepOldGen() {
    VMObject** current = &oldGenObjects_;
    size_t freedSize = 0;
    
    while (*current) {
        VMObject* obj = *current;
        if (obj->color == GCColor::WHITE) { // 未存活，回收
            *current = obj->next;
            freedSize += obj->size;
            delete obj;
        } else {
            // 重置颜色为白色，下次GC使用
            obj->setWhite();
            current = &obj->next;
        }
    }
    
    gcAllocated_ -= freedSize;
}

void VM::gcMarkRoots() {
    // 标记栈
    for (auto& v : stack_) {
        if (v.isObject()) gcMarkObject(v.asPtr());
    }
    
    // 标记全局变量
    for (auto& v : globalSlots_) {
        if (v.isObject()) gcMarkObject(v.asPtr());
    }
}

void VM::gcMarkObject(VMObject* obj) {
    if (!obj || obj->color == GCColor::BLACK) return;
    obj->setBlack();
    
    // 递归标记引用的对象
    switch (obj->typeTag) {
        case ObjectHeader::TAG_CLOSURE: {
            VMClosure* c = (VMClosure*)obj;
            gcMarkObject(c->func);
            for (auto* uv : c->upvalues) gcMarkObject(uv);
            break;
        }
        case ObjectHeader::TAG_ARRAY: {
            VMArray* a = (VMArray*)obj;
            for (auto& v : a->data) gcMarkValue(v);
            break;
        }
        case ObjectHeader::TAG_TABLE: {
            VMTable* t = (VMTable*)obj;
            for (auto& p : t->buckets) {
                if (p.occupied) {
                    gcMarkValue(p.key);
                    gcMarkValue(p.value);
                }
            }
            break;
        }
        case ObjectHeader::TAG_UPVALUE:
            gcMarkValue(*((VMUpvalue*)obj)->location);
            break;
        case ObjectHeader::TAG_FUNCTION: {
            VMFunction* func = (VMFunction*)obj;
            for (auto& c : func->constants) gcMarkValue(c);
            break;
        }
        default:
            break;
    }
}

void VM::gcMarkValue(const Value& v) {
    if (v.isObject()) gcMarkObject(v.asPtr());
}

} // namespace cplang
