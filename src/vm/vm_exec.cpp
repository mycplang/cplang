// CP语言 虚拟机实现 - 字节码执行
#include "vm/vm.hpp"
#include "jit/jit_dispatch.hpp"

namespace cplang {

// ========== 字节码执行 ==========
bool VM::run(ExecContext* ctx) {
    currentCtx_ = ctx;
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
        /* 0x2E */ &&op_NOT, /* 0x2F */ &&op_invalid,
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
        /* 0x73 */ &&op_TYPEOF, /* 0x74 */ &&op_ISNULL, &&op_GETLEN,
        /* 0x76-0x7F */ &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid,
        /* 0x80-0x8F */ &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid,
        /* 0x90 */ &&op_IADD,  /* 0x91 */ &&op_ISUB,  /* 0x92 */ &&op_IMUL,  /* 0x93 */ &&op_IDIV2,
        /* 0x94 */ &&op_IMOD,  /* 0x95 */ &&op_INEG,
        /* 0x96 */ &&op_ICMPEQ, /* 0x97 */ &&op_ICMPNE, /* 0x98 */ &&op_ICMPLT, /* 0x99 */ &&op_ICMPLE,
        /* 0x9A */ &&op_ICMPGT, /* 0x9B */ &&op_ICMPGE,
        /* 0x9C-0x9F */ &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid,
        /* 0xA0 */ &&op_FADD,  /* 0xA1 */ &&op_FSUB,  /* 0xA2 */ &&op_FMUL,  /* 0xA3 */ &&op_FDIV,
        /* 0xA4 */ &&op_FNEG,
        /* 0xA5 */ &&op_FCMPEQ, /* 0xA6 */ &&op_FCMPNE, /* 0xA7 */ &&op_FCMPLT, /* 0xA8 */ &&op_FCMPLE,
        /* 0xA9 */ &&op_FCMPGT, /* 0xAA */ &&op_FCMPGE,
        /* 0xAB-0xAF */ &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid,
        /* 0xB0 */ &&op_I8ADD, /* 0xB1 */ &&op_I8SUB, /* 0xB2 */ &&op_I8MUL, /* 0xB3 */ &&op_I8DIV,
        /* 0xB4 */ &&op_I8MOD, /* 0xB5 */ &&op_I8NEG,
        /* 0xB6 */ &&op_I8CMPEQ, /* 0xB7 */ &&op_I8CMPNE, /* 0xB8 */ &&op_I8CMPLT, /* 0xB9 */ &&op_I8CMPLE,
        /* 0xBA */ &&op_I8CMPGT, /* 0xBB */ &&op_I8CMPGE,
        /* 0xBC-0xBF */ &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid,
        /* 0xC0 */ &&op_I16ADD, /* 0xC1 */ &&op_I16SUB, /* 0xC2 */ &&op_I16MUL, /* 0xC3 */ &&op_I16DIV,
        /* 0xC4 */ &&op_I16MOD, /* 0xC5 */ &&op_I16NEG,
        /* 0xC6 */ &&op_I16CMPEQ, /* 0xC7 */ &&op_I16CMPNE, /* 0xC8 */ &&op_I16CMPLT, /* 0xC9 */ &&op_I16CMPLE,
        /* 0xCA */ &&op_I16CMPGT, /* 0xCB */ &&op_I16CMPGE,
        /* 0xCC-0xCF */ &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid,
        /* 0xD0 */ &&op_F32ADD, /* 0xD1 */ &&op_F32SUB, /* 0xD2 */ &&op_F32MUL, /* 0xD3 */ &&op_F32DIV,
        /* 0xD4 */ &&op_F32NEG,
        /* 0xD5 */ &&op_F32CMPEQ, /* 0xD6 */ &&op_F32CMPNE, /* 0xD7 */ &&op_F32CMPLT, /* 0xD8 */ &&op_F32CMPLE,
        /* 0xD9 */ &&op_F32CMPGT, /* 0xDA */ &&op_F32CMPGE,
        /* 0xDB-0xDF */ &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid,
        /* 0xE0 */ &&op_NEWCLASS, /* 0xE1 */ &&op_IMPORT, /* 0xE2 */ &&op_NEWSTRUCT,
        /* 0xE3 */ &&op_GETFIELD, /* 0xE4 */ &&op_SETFIELD,
        /* 0xE5 */ &&op_NEWVARIANT, /* 0xE6 */ &&op_GETVARIANTTAG, /* 0xE7 */ &&op_GETVARIANTFIELD,
        // 0xE8-0xFE → except 0xE8 = MAKECLOSURE
        &&op_MAKECLOSURE, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid,
        &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid, &&op_invalid,
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
        instructionCount_++;

        // workaround: avoid Value::isNumber() which suffers ODR conflict between value.hpp and vm_opt.hpp
        auto isNum = [](const Value& v) { return v.isDouble() || v.isInt() || v.isFloat32() || v.isInt64(); };

        // ── 调试器断点检查 ──
        if (!breakpoints_.empty() || debugStepMode_) {
            size_t instrIdx = (ctx->pc - 1) / 16;
            if (instrIdx < ctx->func->lineInfo.size()) {
                debugCurrentLine_ = ctx->func->lineInfo[instrIdx];
                if (debugStop_) { debugPaused_ = false; debugStop_ = false; return false; }
                bool hitBp = breakpoints_.count(debugCurrentLine_) > 0;
                bool stepHit = debugStepMode_ && callDepth_ <= debugStepDepth_;
                if (hitBp || stepHit) {
                    if (debugStepMode_) { debugStepMode_ = false; }
                    if (!debugPaused_) { debugPaused_ = true; return true; }
                }
            }
        }
        // DEBUG: 打印当前执行的指令
        printf("VM_OP: 0x%02X pc=%zu a=%d b=%d c=%d\n", op, ctx->pc - 1, a, b, c);
        switch (op) {
#endif

case OP_NOP: { ctx->pc += 15; break; }

case OP_LOADNIL: { RA(a) = Value::nil();                 ctx->pc += 15;
            DISPATCH_NEXT(); }

case OP_LOADBOOL: {
                RA(a) = b ? Value::Bool(true) : Value::Bool(false);
                ctx->pc += 15;
                if (c) ctx->pc += 16;  // skip next 16-byte instruction
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
                    RA(a) = Value::fromFloat(idx);
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
                Int32 idx = (Int32)ctx->code[ctx->pc+3] |
                            ((Int32)ctx->code[ctx->pc+4] << 8) |
                            ((Int32)ctx->code[ctx->pc+5] << 16) |
                            ((Int32)ctx->code[ctx->pc+6] << 24);
                ctx->pc += 15;
                
                if (ctx->func->hasSlots) {
                    // Slot 优化：idx 直接是 slot 编号
                    if (idx >= 0 && idx < static_cast<Int32>(globalSlots_.size())) {
                        RA(a) = globalSlots_[idx];
                    } else {
                        RA(a) = Value::nil();
                    }
                } else {
                    // 非 slot 模式：idx 是常量池索引，需通过名字查找
                    if (idx >= 0 && idx < static_cast<Int32>(ctx->func->constants.size())) {
                        const Value& nameVal = ctx->func->constants[idx];
                        if (nameVal.isString()) {
                            std::string name(nameVal.asString()->data, nameVal.asString()->length);
                            auto it = globalNameToSlot_.find(name);
                            if (it != globalNameToSlot_.end()) {
                                Int32 slot = it->second;
                                if (slot >= 0 && slot < static_cast<Int32>(globalSlots_.size())) {
                                    RA(a) = globalSlots_[slot];
                                } else {
                                    RA(a) = Value::nil();
                                }
                            } else {
                                RA(a) = Value::nil();
                            }
                        } else {
                            RA(a) = Value::nil();
                        }
                    } else {
                        RA(a) = Value::nil();
                    }
                }
                break;
            }

case OP_STOREGLOBAL: {
                // 16-byte format: [op][a][0][0][slot0][slot1][slot2][slot3][pad8]
                Int32 idx = (Int32)ctx->code[ctx->pc+3] |
                            ((Int32)ctx->code[ctx->pc+4] << 8) |
                            ((Int32)ctx->code[ctx->pc+5] << 16) |
                            ((Int32)ctx->code[ctx->pc+6] << 24);
                ctx->pc += 15;

                if (ctx->func->hasSlots) {
                    // Slot 优化：idx 直接是 slot 编号
                    if (idx >= 0) {
                        if (idx >= static_cast<Int32>(globalSlots_.size()))
                            globalSlots_.resize(idx + 1);
                        globalSlots_[idx] = RA(a);
                    }
                } else {
                    // 非 slot 模式：idx 是常量池索引，需通过名字查找/创建slot
                    if (idx >= 0 && idx < static_cast<Int32>(ctx->func->constants.size())) {
                        const Value& nameVal = ctx->func->constants[idx];
                        if (nameVal.isString()) {
                            std::string name(nameVal.asString()->data, nameVal.asString()->length);
                            Int32 slot = getOrCreateGlobalSlot(name.c_str());
                            // [已修复] 移除了遗留的 DBG STORE_NOSLOT 调试输出
                            if (slot >= 0) {
                                if (slot >= static_cast<Int32>(globalSlots_.size())) {
                                    globalSlots_.resize(slot + 1);
                                }
                                globalSlots_[slot] = RA(a);
                            }
                        }
                    }
                }
                break;
            }

case OP_LOADLOCAL: {

                RA(a) = base[b];                 ctx->pc += 15;
            DISPATCH_NEXT(); }
case OP_STORELOCAL: { base[b] = RA(a);                 ctx->pc += 15;
            DISPATCH_NEXT(); }

// ═══════════════════════════════════════════════════════════════════
//  类型化算术指令（零运行时开销，编译期保证类型正确）
// ═══════════════════════════════════════════════════════════════════

case OP_IADD: { RA(a) = Value::Int(RB(b).asInt() + RC(c).asInt()); ctx->pc += 15; break; }
case OP_ISUB: { RA(a) = Value::Int(RB(b).asInt() - RC(c).asInt()); ctx->pc += 15; break; }
case OP_IMUL: { RA(a) = Value::Int(RB(b).asInt() * RC(c).asInt()); ctx->pc += 15; break; }
case OP_IDIV2:{ Int64 d = RC(c).asInt(); if (d==0) { EMIT_ERR("除零错误"); break; } RA(a) = Value::Int(RB(b).asInt() / d); ctx->pc += 15; break; }
case OP_IMOD: { Int64 d = RC(c).asInt(); if (d==0) { EMIT_ERR("除零错误"); break; } RA(a) = Value::Int(RB(b).asInt() % d); ctx->pc += 15; break; }
case OP_INEG: { RA(a) = Value::Int(-RB(b).asInt()); ctx->pc += 15; break; }

case OP_FADD: { RA(a) = Value::fromFloat(RB(b).asFloat() + RC(c).asFloat()); ctx->pc += 15; break; }
case OP_FSUB: { RA(a) = Value::fromFloat(RB(b).asFloat() - RC(c).asFloat()); ctx->pc += 15; break; }
case OP_FMUL: { RA(a) = Value::fromFloat(RB(b).asFloat() * RC(c).asFloat()); ctx->pc += 15; break; }
case OP_FDIV: { RA(a) = Value::fromFloat(RB(b).asFloat() / RC(c).asFloat()); ctx->pc += 15; break; }
case OP_FNEG: { RA(a) = Value::fromFloat(-RB(b).asFloat()); ctx->pc += 15; break; }

case OP_ICMPEQ: { RA(a) = Value::Bool(RB(b).asInt() == RC(c).asInt()); ctx->pc += 15; break; }
case OP_ICMPNE: { RA(a) = Value::Bool(RB(b).asInt() != RC(c).asInt()); ctx->pc += 15; break; }
case OP_ICMPLT: { RA(a) = Value::Bool(RB(b).asInt() <  RC(c).asInt()); ctx->pc += 15; break; }
case OP_ICMPLE: { RA(a) = Value::Bool(RB(b).asInt() <= RC(c).asInt()); ctx->pc += 15; break; }
case OP_ICMPGT: { RA(a) = Value::Bool(RB(b).asInt() >  RC(c).asInt()); ctx->pc += 15; break; }
case OP_ICMPGE: { RA(a) = Value::Bool(RB(b).asInt() >= RC(c).asInt()); ctx->pc += 15; break; }

case OP_FCMPEQ: { RA(a) = Value::Bool(RB(b).asFloat() == RC(c).asFloat()); ctx->pc += 15; break; }
case OP_FCMPNE: { RA(a) = Value::Bool(RB(b).asFloat() != RC(c).asFloat()); ctx->pc += 15; break; }
case OP_FCMPLT: { RA(a) = Value::Bool(RB(b).asFloat() <  RC(c).asFloat()); ctx->pc += 15; break; }
case OP_FCMPLE: { RA(a) = Value::Bool(RB(b).asFloat() <= RC(c).asFloat()); ctx->pc += 15; break; }
case OP_FCMPGT: { RA(a) = Value::Bool(RB(b).asFloat() >  RC(c).asFloat()); ctx->pc += 15; break; }
case OP_FCMPGE: { RA(a) = Value::Bool(RB(b).asFloat() >= RC(c).asFloat()); ctx->pc += 15; break; }

// ═══════════════════════════════════════════════════════════════════
case OP_I8ADD: case OP_I16ADD: case OP_F32ADD:
case OP_ADD: {
                Value left = RB(b);
                Value right = RC(c);
                // 字符串拼接
                if (left.isString() || right.isString()) {
                    String lstr, rstr;
                    if (left.isString()) lstr = String(left.asString()->data, left.asString()->length);
                    else if (left.isInt() || left.isInt64()) lstr = std::to_string(left.asInt());
                    else if (left.isFloat()) lstr = std::to_string(left.asFloat());
                    else if (left.isBool()) lstr = left.asInt() ? "true" : "false";
                    else if (left.isNil()) lstr = "nil";
                    else if (left.isArray()) {
                        lstr = "["; auto* arr = left.asArray();
                        for (size_t i = 0; i < arr->data.size(); i++) {
                            if (i > 0) lstr += ", ";
                            auto& v = arr->data[i];
                            if (v.isString()) lstr += String(v.asString()->data, v.asString()->length);
                            else if (v.isInt() || v.isInt64()) lstr += std::to_string(v.asInt());
                            else if (v.isFloat()) lstr += std::to_string(v.asFloat());
                            else if (v.isBool()) lstr += v.asInt() ? "true" : "false";
                            else if (v.isNil()) lstr += "nil";
                            else if (v.isArray()) lstr += "[array]";
                            else lstr += "[object]";
                        }
                        lstr += "]";
                    }
                    else lstr = "[object]";

                    if (right.isString()) rstr = String(right.asString()->data, right.asString()->length);
                    else if (right.isInt() || right.isInt64()) rstr = std::to_string(right.asInt());
                    else if (right.isFloat()) rstr = std::to_string(right.asFloat());
                    else if (right.isBool()) rstr = right.asInt() ? "true" : "false";
                    else if (right.isNil()) rstr = "nil";
                    else if (right.isArray()) {
                        rstr = "["; auto* arr = right.asArray();
                        for (size_t i = 0; i < arr->data.size(); i++) {
                            if (i > 0) rstr += ", ";
                            auto& v = arr->data[i];
                            if (v.isString()) rstr += String(v.asString()->data, v.asString()->length);
                            else if (v.isInt() || v.isInt64()) rstr += std::to_string(v.asInt());
                            else if (v.isFloat()) rstr += std::to_string(v.asFloat());
                            else if (v.isBool()) rstr += v.asInt() ? "true" : "false";
                            else if (v.isNil()) rstr += "nil";
                            else if (v.isArray()) rstr += "[array]";
                            else rstr += "[object]";
                        }
                        rstr += "]";
                    }
                    else rstr = "[object]";
                    
                    RA(a) = makeStringVal(VMString::create(lstr + rstr));
                } else if (left.isArray() && right.isArray()) {
                    // 数组拼接
                    auto* result = VMArray::create();
                    auto* leftArr = left.asArray();
                    auto* rightArr = right.asArray();
                    for (Int64 i = 0; i < leftArr->length(); i++) result->set(i, leftArr->get(i));
                    for (Int64 i = 0; i < rightArr->length(); i++) result->set(leftArr->length() + i, rightArr->get(i));
                    RA(a) = makeArrayVal(result);
                } else if (left.isArray()) {
                    // 数组 + 元素（追加）
                    auto* result = VMArray::create();
                    auto* arr = left.asArray();
                    for (Int64 i = 0; i < arr->length(); i++) result->set(i, arr->get(i));
                    result->set(arr->length(), right);
                    RA(a) = makeArrayVal(result);
                } else if (right.isArray()) {
                    // 元素 + 数组（前置）
                    auto* result = VMArray::create();
                    result->set(0, left);
                    auto* arr = right.asArray();
                    for (Int64 i = 0; i < arr->length(); i++) result->set(i + 1, arr->get(i));
                    RA(a) = makeArrayVal(result);
                } else if ((left.isInt() || left.isInt64()) && (right.isInt() || right.isInt64())) {
                    RA(a) = Value::Int(left.asInt() + right.asInt());
                } else {
                    double x = isNum(left) ? left.asFloat() : 0;
                    double y = isNum(right) ? right.asFloat() : 0;
                    RA(a) = Value::fromFloat(x + y);
                }
                ctx->pc += 15;
                DISPATCH_NEXT();
            }

case OP_I8SUB: case OP_I16SUB: case OP_F32SUB:
case OP_SUB: {
                if ((RB(b).isInt() || RB(b).isInt64()) && (RC(c).isInt() || RC(c).isInt64())) {
                    RA(a) = Value::Int(RB(b).asInt() - RC(c).asInt());
                } else {
                    double x = isNum(RB(b)) ? RB(b).asFloat() : 0;
                    double y = isNum(RC(c)) ? RC(c).asFloat() : 0;
                    RA(a) = Value::fromFloat(x - y);
                }
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_I8MUL: case OP_I16MUL: case OP_F32MUL:
case OP_MUL: {
                if ((RB(b).isInt() || RB(b).isInt64()) && (RC(c).isInt() || RC(c).isInt64())) {
                    RA(a) = Value::Int(RB(b).asInt() * RC(c).asInt());
                } else {
                    double x = isNum(RB(b)) ? RB(b).asFloat() : 0;
                    double y = isNum(RC(c)) ? RC(c).asFloat() : 0;
                    RA(a) = Value::fromFloat(x * y);
                }
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_I8DIV: case OP_I16DIV: case OP_F32DIV:
	case OP_DIV: {
	                // NB: avoid Value::isNumber() — ODR conflict between NaN-boxing and old struct Value types
	                auto isNum = [](const Value& v) { return v.isDouble() || v.isInt() || v.isFloat32() || v.isInt64(); };
	                double x = isNum(RB(b)) ? RB(b).asFloat() : 0;
	                double y = isNum(RC(c)) ? RC(c).asFloat() : 0;
	                if (y == 0) EMIT_ERR("除零错误");
	                RA(a) = Value::fromFloat(x / y);
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

	case OP_IDIV: {
	                auto isNum = [](const Value& v) { return v.isDouble() || v.isInt() || v.isFloat32() || v.isInt64(); };
	                Int64 y = isNum(RC(c)) ? RC(c).asInt() : 0;
	                if (y == 0) EMIT_ERR("除零错误");
	                RA(a) = Value::Int(RB(b).asInt() / y);
                ctx->pc += 15;
                DISPATCH_NEXT();
            }

case OP_I8MOD: case OP_I16MOD:
case OP_MOD: {
	                auto isNum = [](const Value& v) { return v.isDouble() || v.isInt() || v.isFloat32() || v.isInt64(); };
	                Int64 y = isNum(RC(c)) ? RC(c).asInt() : 0;
	                if (y == 0) EMIT_ERR("除零错误");
	                RA(a) = Value::Int(RB(b).asInt() % y);
                ctx->pc += 15;
                DISPATCH_NEXT();
            }

case OP_POW: {
                auto isNum = [](const Value& v) { return v.isDouble() || v.isInt() || v.isFloat32() || v.isInt64(); };
                double x = isNum(RB(b)) ? RB(b).asFloat() : 0;
                double y = isNum(RC(c)) ? RC(c).asFloat() : 0;
                RA(a) = Value::fromFloat(std::pow(x, y));
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_I8NEG: case OP_I16NEG: case OP_F32NEG:
case OP_NEG: {
                if (RB(b).isInt() || RB(b).isInt64()) RA(a) = Value::Int(-RB(b).asInt());
                else if (RB(b).isFloat()) RA(a) = Value::fromFloat(-RB(b).asFloat());
                else RA(a) = Value::nil();
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_BAND: {
                if ((RB(b).isInt() || RB(b).isInt64()) && (RC(c).isInt() || RC(c).isInt64())) {
                    RA(a) = Value::Int(RB(b).asInt() & RC(c).asInt());
                } else RA(a) = Value::nil();
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_BOR: {
                if ((RB(b).isInt() || RB(b).isInt64()) && (RC(c).isInt() || RC(c).isInt64())) {
                    RA(a) = Value::Int(RB(b).asInt() | RC(c).asInt());
                } else RA(a) = Value::nil();
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_BXOR: {
                if ((RB(b).isInt() || RB(b).isInt64()) && (RC(c).isInt() || RC(c).isInt64())) {
                    RA(a) = Value::Int(RB(b).asInt() ^ RC(c).asInt());
                } else RA(a) = Value::nil();
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_BSHL: {
                if ((RB(b).isInt() || RB(b).isInt64()) && (RC(c).isInt() || RC(c).isInt64())) {
                    RA(a) = Value::Int(RB(b).asInt() << RC(c).asInt());
                } else RA(a) = Value::nil();
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_BSHR: {
                if ((RB(b).isInt() || RB(b).isInt64()) && (RC(c).isInt() || RC(c).isInt64())) {
                    RA(a) = Value::Int(static_cast<UInt64>(RB(b).asInt()) >> RC(c).asInt());
                } else RA(a) = Value::nil();
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_BNOT: {
                if (RB(b).isInt() || RB(b).isInt64()) RA(a) = Value::Int(~RB(b).asInt());
                else RA(a) = Value::nil();
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_NOT: {
                RA(a) = Value::Bool(!RB(b).isTrue());
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_I8CMPEQ: case OP_I16CMPEQ: case OP_F32CMPEQ:
case OP_CMPEQ: { RA(a) = Value::Bool(RB(b).equals(RC(c)));                 ctx->pc += 15;
            DISPATCH_NEXT(); }
case OP_I8CMPNE: case OP_I16CMPNE: case OP_F32CMPNE:
case OP_CMPNE: { RA(a) = Value::Bool(!RB(b).equals(RC(c)));                 ctx->pc += 15;
            DISPATCH_NEXT(); }

case OP_I8CMPLT: case OP_I16CMPLT: case OP_F32CMPLT:
case OP_CMPLT: {
                if (RB(b).isInt() && RC(c).isInt()) RA(a) = Value::Bool(RB(b).asInt() < RC(c).asInt());
                else if (isNum(RB(b)) && isNum(RC(c))) {
                    double x = RB(b).asFloat(); double y = RC(c).asFloat();
                    RA(a) = Value::Bool(x < y);
                } else RA(a) = Value::Bool(false);
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_I8CMPLE: case OP_I16CMPLE: case OP_F32CMPLE:
case OP_CMPLE: {
                if (RB(b).isInt() && RC(c).isInt()) RA(a) = Value::Bool(RB(b).asInt() <= RC(c).asInt());
                else if (isNum(RB(b)) && isNum(RC(c))) {
                    double x = RB(b).asFloat(); double y = RC(c).asFloat();
                    RA(a) = Value::Bool(x <= y);
                } else RA(a) = Value::Bool(false);
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_I8CMPGT: case OP_I16CMPGT: case OP_F32CMPGT:
case OP_CMPGT: {
                if (RB(b).isInt() && RC(c).isInt()) RA(a) = Value::Bool(RB(b).asInt() > RC(c).asInt());
                else if (isNum(RB(b)) && isNum(RC(c))) {
                    double x = RB(b).asFloat(); double y = RC(c).asFloat();
                    RA(a) = Value::Bool(x > y);
                } else RA(a) = Value::Bool(false);
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_I8CMPGE: case OP_I16CMPGE: case OP_F32CMPGE:
case OP_CMPGE: {
                if (RB(b).isInt() && RC(c).isInt()) RA(a) = Value::Bool(RB(b).asInt() >= RC(c).asInt());
                else if (isNum(RB(b)) && isNum(RC(c))) {
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

case OP_CALLMETHOD: {
                // 方法调用: a=calleeReg, b=objReg, c=argc
                // 自动把 RA(b) (表/self) 插入 args[0]
                Value callee = RA(a);
                Value obj = RA(b);
                std::vector<Value> args;
                args.push_back(obj);
                for (Int32 i = 0; i < c; i++) args.push_back(RA(a + 1 + i));
                if (callee.isFunction() || callee.isCFunction()) {
                    VMFunction* func = callee.asFunction();
                    if (func->typeTag == ObjectHeader::TAG_NATIVE) {
                        VMNativeFunc* nf = reinterpret_cast<VMNativeFunc*>(func);
                        RA(a) = nf ? nf->fn(args) : Value::nil();
                        ctx->pc += 15; break;
                    }
                    RA(a) = callFunction(callee, args);
                    ctx->pc += 15; break;
                }
                EMIT_ERR("不可调用：该值不是函数");
            }
            case OP_CALL: {
                Int32 calleeReg = b;
                Int32 argc = c;
                Value callee = RA(calleeReg);

                std::vector<Value> args;
                for (Int32 i = 0; i < argc; i++) args.push_back(RA(calleeReg + 1 + i));

                // 表作为可调用对象（闭包基础）
                if (isTableVal(callee)) {
                    VMTable* tbl = asTableVal(callee);
                    Value callMethod = tbl->get(makeStringVal(VMString::create("调用")));
                    if (callMethod.isFunction() || callMethod.isCFunction()) {
                        args.insert(args.begin(), callee);
                        RA(a) = callFunction(callMethod, args);
                        ctx->pc += 15; break;
                    }
                }

                if (callee.isFunction() || callee.isCFunction()) {
                    VMFunction* func = callee.asFunction();
                    if (func->typeTag == ObjectHeader::TAG_NATIVE) {
                        VMNativeFunc* nf = reinterpret_cast<VMNativeFunc*>(func);
                        if (nf && nf->fn) {
                            VM* savedVM = currentVM_;
                            try {
                                currentVM_ = this;
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
                    
                    // ── JIT 分派（委托给独立模块） ──
                    {
                        Value jitResult;
                        if (jitTryCallDispatch(this, func, argc, args.data(), jitResult)) {
                            RA(a) = jitResult;
                            ctx->pc += 15;
                            break;
                        }
                    }
                    
                    // 用户函数字节码调用
                    CallFrame frame;
                    frame.func = func;
                    frame.closure = nullptr;
                    frame.base = base;
                    frame.savedBase = base;
                    frame.pc = nullptr;
                    Int32 pcAfter = (Int32)(ctx->pc + 15);  // Return to next instruction (16-byte format)
                    frame.returnPcOffset = pcAfter;
                    frame.returnBaseOffset = (Int32)ctx->baseOffset;
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
                    VMClosure* cl = callee.asPtr() ? static_cast<VMClosure*>(callee.asPtr()) : nullptr;
                    if (!cl) { RA(a) = Value::nil(); ctx->pc += 15; break; }
                    CallFrame frame;
                    frame.func = cl->func;
                    frame.closure = cl;
                    frame.base = base;
                    frame.savedBase = base;
                    frame.pc = nullptr;
                    Int32 pcAfter = (Int32)(ctx->pc + 7);
                    frame.returnPcOffset = pcAfter;
                    frame.returnBaseOffset = (Int32)ctx->baseOffset;
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

                // 写返回值到基址[0]供 callFunction 读取
                base[0] = result;
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
                base = callerBase;
                ctx->base = base;
                // 写入返回值到调用者的resultReg
                RA(resultReg) = result;
                
                // callFunction嵌套调用：入口帧深度时退出
                if ((Int32)frames_.size() <= callDepth_) {
                    return true;
                }
                break;
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
                } else if (arr.isPtr() && arr.asPtr() && arr.asPtr()->typeTag == ObjectHeader::TAG_TABLE) {
                    VMTable* tbl = static_cast<VMTable*>(arr.asPtr());
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
                } else if (arr.isPtr() && arr.asPtr() && arr.asPtr()->typeTag == ObjectHeader::TAG_TABLE) {
                    VMTable* tbl = static_cast<VMTable*>(arr.asPtr());
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
                } else if (obj.asPtr() && obj.asPtr()->typeTag == ObjectHeader::TAG_TABLE) {
                    VMTable* tbl = static_cast<VMTable*>(obj.asPtr());
                    RA(a) = tbl->get(key);
                } else {
                    RA(a) = Value::nil();
                }
                                ctx->pc += 15;
            DISPATCH_NEXT();
            }

case OP_SETIDX: {
                Value val = RA(a);
                Value obj = RA(b);
                Value key = RA(c);
                if (obj.isArray() && key.isInt()) {
                    obj.asArray()->set(key.asInt(), val);
                } else if (obj.asPtr() && obj.asPtr()->typeTag == ObjectHeader::TAG_TABLE) {
                    VMTable* tbl = static_cast<VMTable*>(obj.asPtr());
                    tbl->set(key, val);
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
                        RA(a) = Value::fromFloat(std::atof(s));
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
                else if (RA(b).asPtr() && RA(b).asPtr()->typeTag == ObjectHeader::TAG_TABLE) tn = "table";
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

                if (!importCallback) EMIT_ERR("导入功能不支持");

                bool ok = importCallback(path);
                VMFunction* moduleFunc = lastImportedFunc_;

                if (!ok) {
                    RA(a) = Value::Bool(false);
                    break;
                }
                
                // 如果模块函数存在，执行它
                if (moduleFunc) {
                    // 保存主程序上下文
                    size_t savedPc = ctx->pc;
                    VMFunction* savedFunc = ctx->func;
                    UInt8* savedCode = ctx->code;
                    size_t savedCodeSize = ctx->codeSize;
                    
                    // 复制模块字节码
                    size_t modSz = moduleFunc->code.size();
                    UInt8* modBuf = new UInt8[modSz + 256];
                    std::memcpy(modBuf, moduleFunc->code.data(), modSz);
                    for (size_t i = modSz; i < modSz + 256; i++) modBuf[i] = OP_NOP;
                    
                    // 设置模块执行上下文
                    ctx->pc = 0;
                    ctx->func = moduleFunc;
                    ctx->code = modBuf;
                    ctx->codeSize = modSz + 256;
                    
                    // 模块执行栈
                    Value modStack[16];
                    std::memset(modStack, 0, sizeof(modStack));
                    
                    // 执行模块字节码 - 16字节对齐格式
                    
                    while (ctx->pc < modSz) {
                        // 16字节对齐
                        ctx->pc = ((ctx->pc + 15) / 16) * 16;
                        if (ctx->pc >= modSz) break;
                        
                        size_t startPc = ctx->pc;
                        UInt8 op2 = ctx->code[ctx->pc++];
                        UInt8 opa = ctx->code[ctx->pc++];
                        UInt8 opb = ctx->code[ctx->pc++];
                        UInt8 opc = ctx->code[ctx->pc++];
                        
                        switch (op2) {
                            case OP_RETURN:
                                goto module_done;
                                
                            case OP_LOADINT: {
                                // emitInt格式: [op][a][0][0][imm32(4)][padding(8)]
                                // 立即数在startPc+4位置
                                Int32 imm = (Int32)ctx->code[startPc+4] |
                                           ((Int32)ctx->code[startPc+5] << 8) |
                                           ((Int32)ctx->code[startPc+6] << 16) |
                                           ((Int32)ctx->code[startPc+7] << 24);
                                modStack[a] = Value::Int(imm);
                                break;
                            }
                                
                            case OP_LOADFLT: {
                                // emitInt格式: [op][a][0][0][imm32(4)][padding(8)]
                                Int32 idx = (Int32)ctx->code[startPc+4] |
                                           ((Int32)ctx->code[startPc+5] << 8) |
                                           ((Int32)ctx->code[startPc+6] << 16) |
                                           ((Int32)ctx->code[startPc+7] << 24);
                                if (idx >= 0 && idx < static_cast<Int32>(ctx->func->constants.size())) {
                                    modStack[a] = ctx->func->constants[idx];
                                } else {
                                    modStack[a] = Value::fromFloat(static_cast<Float64>(idx));
                                }
                                break;
                            }
                                
                            case OP_LOADCONST: {
                                Int32 idx = (Int32)ctx->code[startPc+4] |
                                           ((Int32)ctx->code[startPc+5] << 8) |
                                           ((Int32)ctx->code[startPc+6] << 16) |
                                           ((Int32)ctx->code[startPc+7] << 24);
                                if (idx >= 0 && idx < static_cast<Int32>(ctx->func->constants.size())) {
                                    modStack[a] = ctx->func->constants[idx];
                                }
                                break;
                            }
                            
                            case OP_LOADNIL:
                                modStack[a] = Value::nil();
                                break;
                                
                            case OP_LOADBOOL:
                                modStack[a] = Value::Bool(b != 0);
                                break;
                                
                            case OP_STOREGLOBAL: {
                                Int32 idx = (Int32)ctx->code[startPc+4] |
                                               ((Int32)ctx->code[startPc+5] << 8) |
                                               ((Int32)ctx->code[startPc+6] << 16) |
                                               ((Int32)ctx->code[startPc+7] << 24);
                                if (ctx->func->hasSlots) {
                                    // Slot 模式：idx 直接是 slot 编号
                                    if (idx >= 0) {
                                        if (idx >= static_cast<Int32>(globalSlots_.size()))
                                            globalSlots_.resize(idx + 1);
                                        globalSlots_[idx] = modStack[a];
                                    }
                                } else {
                                    // 非slot模式：idx 是常数池索引，需通过名字查找/创建slot
                                    if (idx >= 0 && idx < static_cast<Int32>(ctx->func->constants.size())) {
                                        const Value& nameVal = ctx->func->constants[idx];
                                        if (nameVal.isString()) {
                                            std::string name(nameVal.asString()->data, nameVal.asString()->length);
                                            Int32 slot = getOrCreateGlobalSlot(name.c_str());
                                            if (slot >= 0) {
                                                if (slot >= static_cast<Int32>(globalSlots_.size()))
                                                    globalSlots_.resize(slot + 1);
                                                globalSlots_[slot] = modStack[a];
                                            }
                                        }
                                    }
                                }
                                break;
                            }
                            
                            case OP_LOADGLOBAL: {
                                Int32 idx = (Int32)ctx->code[startPc+4] |
                                               ((Int32)ctx->code[startPc+5] << 8) |
                                               ((Int32)ctx->code[startPc+6] << 16) |
                                               ((Int32)ctx->code[startPc+7] << 24);
                                if (ctx->func->hasSlots) {
                                    // Slot 模式：idx 直接是 slot 编号
                                    if (idx >= 0 && idx < static_cast<Int32>(globalSlots_.size()))
                                        modStack[a] = globalSlots_[idx];
                                    else
                                        modStack[a] = Value::nil();
                                } else {
                                    // 非slot模式：通过名字查找slot
                                    if (idx >= 0 && idx < static_cast<Int32>(ctx->func->constants.size())) {
                                        const Value& nameVal = ctx->func->constants[idx];
                                        if (nameVal.isString()) {
                                            std::string name(nameVal.asString()->data, nameVal.asString()->length);
                                            auto it = globalNameToSlot_.find(name);
                                            if (it != globalNameToSlot_.end()) {
                                                Int32 slot = it->second;
                                                if (slot >= 0 && slot < static_cast<Int32>(globalSlots_.size()))
                                                    modStack[a] = globalSlots_[slot];
                                                else modStack[a] = Value::nil();
                                            } else modStack[a] = Value::nil();
                                        } else modStack[a] = Value::nil();
                                    } else modStack[a] = Value::nil();
                                }
                                break;
                            }
                            
                            case OP_MOVE:
                                modStack[a] = modStack[b];
                                break;
                                
                            case OP_ADD:
                                if (modStack[b].isInt() && modStack[c].isInt()) {
                                    modStack[a] = Value::Int(modStack[b].asInt() + modStack[c].asInt());
                                }
                                break;
                                
                            case OP_NOP:
                            default:
                                break;
                        }
                    }
                    module_done:
                    

                    
                    delete[] modBuf;
                    
                    // 恢复主程序上下文
                    ctx->pc = savedPc;
                    ctx->func = savedFunc;
                    ctx->code = savedCode;
                    ctx->codeSize = savedCodeSize;
                }
                
                RA(a) = Value::Bool(true);
                ctx->pc += 15;
                break;
            }

case OP_GETLEN: {
                // GETLEN ra, rb: ra = length(rb)
                Value obj = RB(b);
                if (obj.isArray()) {
                    RA(a) = Value::Int(obj.asArray()->length());
                } else if (obj.isString()) {
                    RA(a) = Value::Int(static_cast<Int64>(obj.asString()->length));
                } else {
                    RA(a) = Value::Int(0);
                }
                ctx->pc += 15;
                break;
            }

case OP_NEWSTRUCT: {
                // NEWSTRUCT ra, n: 创建有n个字段的结构体
                // 简化实现：使用类似Table的结构存储字段
                VMTable* tbl = VMTable::create();
                trackGC(reinterpret_cast<VMObject*>(tbl));
                RA(a) = makePtrVal(reinterpret_cast<VMObject*>(tbl));
                ctx->pc += 15;
                break;
            }

case OP_NEWVARIANT: {
                // NEWVARIANT ra, tag: 创建变体表 {0: tag, 1: nil, 2: nil, ...}
                VMTable* tbl = VMTable::create();
                trackGC(reinterpret_cast<VMObject*>(tbl));
                // 设置 tag 在字段 0
                tbl->set(Value::Int(0), Value::Int(b));
                RA(a) = makePtrVal(reinterpret_cast<VMObject*>(tbl));
                ctx->pc += 15;
                break;
            }

case OP_GETVARIANTTAG: {
                // GETVARIANTTAG ra, rb: ra = rb.fields[0] (the tag)
                // 对于ADT枚举(表对象)，提取第0字段作为tag
                // 对于简单枚举(整数)，值本身就是tag
                Value obj = RB(b);
                if (obj.asPtr() && obj.asPtr()->typeTag == ObjectHeader::TAG_TABLE) {
                    VMTable* tbl = static_cast<VMTable*>(obj.asPtr());
                    RA(a) = tbl->get(Value::Int(0));
                } else if (obj.isInt()) {
                    RA(a) = obj;  // 简单枚举：值本身就是tag
                } else {
                    RA(a) = Value::Int(0);
                }
                ctx->pc += 15;
                break;
            }

case OP_GETVARIANTFIELD: {
                // GETVARIANTFIELD ra, rb, c: ra = rb.fields[c] (c is field index)
                Value obj = RB(b);
                Int32 fieldIdx = c;
                if (obj.asPtr() && obj.asPtr()->typeTag == ObjectHeader::TAG_TABLE) {
                    VMTable* tbl = static_cast<VMTable*>(obj.asPtr());
                    RA(a) = tbl->get(Value::Int(fieldIdx));
                } else {
                    RA(a) = Value::nil();
                }
                ctx->pc += 15;
                break;
            }

case OP_GETFIELD: {
                // GETFIELD ra, rb, c: ra = rb.fields[c]
                Value obj = RB(b);
                Int32 fieldIdx = c;
                
                if (obj.asPtr() && obj.asPtr()->typeTag == ObjectHeader::TAG_INSTANCE) {
                    VMInstance* inst = static_cast<VMInstance*>(obj.asPtr());
                    RA(a) = inst->getField(fieldIdx);
                } else if (obj.asPtr() && obj.asPtr()->typeTag == ObjectHeader::TAG_TABLE) {
                    VMTable* tbl = static_cast<VMTable*>(obj.asPtr());
                    RA(a) = tbl->get(Value::Int(fieldIdx));
                } else {
                    RA(a) = Value::nil();
                }
                ctx->pc += 15;
                break;
            }

case OP_SETFIELD: {
                // SETFIELD ra, rb, c: rb.fields[c] = ra
                Value obj = RB(b);
                Int32 fieldIdx = c;
                Value val = RA(a);
                
                if (obj.asPtr() && obj.asPtr()->typeTag == ObjectHeader::TAG_INSTANCE) {
                    VMInstance* inst = static_cast<VMInstance*>(obj.asPtr());
                    inst->setField(fieldIdx, val);
                } else if (obj.asPtr() && obj.asPtr()->typeTag == ObjectHeader::TAG_TABLE) {
                    VMTable* tbl = static_cast<VMTable*>(obj.asPtr());
                    tbl->set(Value::Int(fieldIdx), val);
                }
                ctx->pc += 15;
                break;
            }

case OP_MAKECLOSURE: {
                // OP_MAKECLOSURE: a=resultReg, b=funcConstIdx(low8), c=captureCount
                // 从常量池获取 VMFunction，创建 VMClosure 包装它
                // 捕获的变量从 RA(a+1) 开始的连续寄存器中读取
                Int32 funcIdx = b;
                Int32 captureCount = c;
                
                VMFunction* targetFunc = nullptr;
                if (funcIdx >= 0 && funcIdx < static_cast<Int32>(ctx->func->constants.size())) {
                    const Value& funcVal = ctx->func->constants[funcIdx];
                    if (funcVal.isFunction()) {
                        targetFunc = funcVal.asFunction();
                    }
                }
                
                if (!targetFunc) {
                    RA(a) = Value::nil();
                    ctx->pc += 15;
                    break;
                }
                
                // 创建闭包对象
                VMClosure* closure = VMClosure::create(targetFunc);
                
                // 捕获变量：从 RA(a+1) 开始的连续寄存器
                if (captureCount > 0) {
                    closure->upvalues.resize(captureCount);
                    for (Int32 i = 0; i < captureCount; i++) {
                        // 复制捕获的变量值到闭包的 upvalue
                        VMUpvalue* upval = VMUpvalue::create(&RA(a + 1 + i));
                        upval->closed = RA(a + 1 + i);  // 立即关闭（捕获值拷贝）
                        closure->upvalues[i] = upval;
                    }
                }
                
                // 将闭包包装为 Value
                RA(a) = Value::Ptr(reinterpret_cast<VMObject*>(closure));
                ctx->pc += 15;
                break;
            }

case OP_TRY: {
                // OP_TRY a, imm32: push handler frame with absolute catch PC
                Int32 catchPC = (Int32)ctx->code[ctx->pc+3] |
                               ((Int32)ctx->code[ctx->pc+4] << 8) |
                               ((Int32)ctx->code[ctx->pc+5] << 16) |
                               ((Int32)ctx->code[ctx->pc+6] << 24);
                HandlerFrame hf;
                hf.catchPC = catchPC;
                hf.savedBase = base;
                hf.resultReg = a;
                handlerStack_.push_back(hf);
                ctx->pc += 15;
                break;
            }

case OP_ENDTRY: {
                // 正常路径：弹出异常处理帧
                if (!handlerStack_.empty()) handlerStack_.pop_back();
                ctx->pc += 15;
                break;
            }

case OP_THROW: {
                // 抛出异常值 RA(a)：沿着handlerStack向上查找catch
                Value ex = RA(a);
                if (handlerStack_.empty()) {
                    error_ = "未捕获的异常: " + ex.toString();
                    return false;
                }
                // 弹出一个handler帧
                HandlerFrame hf = handlerStack_.back();
                handlerStack_.pop_back();
                
                // 恢复栈到handler的位置
                base = hf.savedBase;
                ctx->base = base;
                // 把异常值写入结果寄存器
                RA(hf.resultReg) = ex;
                // 跳转到catch处理器
                ctx->pc = hf.catchPC;
                break;
            }

default: {
                char buf[64];
                std::snprintf(buf, sizeof(buf), "未知操作码 0x%02X", op);
                EMIT_ERR(buf);
            }
        }
    }
}

bool VM::loadModule(VMFunction* func) {
    // 调试器恢复执行
    if (debugPaused_ && savedDebugCtx_.func) {
        debugPaused_ = false;
        return run(&savedDebugCtx_);
    }
    if (!func) { error_ = "空函数"; return false; }
    // 注意：func 及其常量的生命周期由创建它们的 Compiler 管理
    // 这里不再重复追踪，避免双 VM 析构时的重复释放问题

    size_t codeSz = func->code.size();
    UInt8* codeBuf = new UInt8[codeSz + 256];
    std::memcpy(codeBuf, func->code.data(), codeSz);
    for (size_t i = codeSz; i < codeSz + 256; i++) codeBuf[i] = OP_NOP;




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
    
    // 如果因断点暂停，保存上下文（不删codeBuf）
    if (debugPaused_) {
        savedDebugCtx_ = ctx;
        return true;
    }
    
    delete[] codeBuf;
    return ok;
}

// ── 调试器 API 实现 ──

std::string VM::debugCallStack() const {
    std::string result;
    for (size_t i = 0; i < frames_.size(); i++) {
        if (i > 0) result += " <- ";
        if (frames_[i].func && frames_[i].func->name)
            result += std::string(frames_[i].func->name->data, frames_[i].func->name->length);
        else result += "<main>";
    }
    return result.empty() ? "<main>" : result;
}
std::string VM::debugLocals() const {
    return "{}";
}
Value VM::debugGetVariable(const std::string& name) const {
    auto it = globals_.find(name);
    if (it != globals_.end()) return it->second;
    return Value::nil();
}
} // namespace cplang