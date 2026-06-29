// CP璇█ 浠ｇ爜鐢熸垚鍣ㄥ疄鐜?
#include "codegen/codegen.hpp"
#include <cstdio>
#include "parser/parser.hpp"
#include "stdlib/stdlib.hpp"
#include <sstream>
#include <fstream>
#include <iterator>
#include <iostream>

namespace cplang {

// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?
//  鏋勯€犲嚱鏁?
// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?

Codegen::Codegen(VM* vm, SemanticAnalyzer* analyzer) : vm_(vm), analyzer_(analyzer) {
    pushScope(); // 鍏ㄥ眬浣滅敤鍩?
}

Codegen::~Codegen() {
    while (!localScopes_.empty()) popScope();
}

// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?
//  涓荤紪璇戝叆鍙?
// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?

VMFunction* Codegen::compile(Shared<Program> program) {
    // 鍒涘缓涓诲嚱鏁帮紙鍏堝垱寤猴紝杩欐牱 compileFuncDecl 鍦ㄦ敞鍐屽嚱鏁版椂鑳芥纭彂灏勪唬鐮侊級
    func_ = new VMFunction();
    func_->name = nullptr; // 涓绘ā鍧?
    func_->maxStack = 256;
    func_->numParams = 0;
    func_->numLocals = 0;
    func_->isVararg = false;
    func_->hasSlots = (vm_ != nullptr);  // main=true, module=false
    func_->sourceFile = sourceFile_;
    code_ = &func_->code;

    nextReg_ = 0;
    maxReg_ = 0;
    maxStack_ = 256;

    // === 棰勭紪璇戞墍鏈夋硾鍨嬪疄渚嬪寲鍑芥暟 ===
    // 杩欎簺鍑芥暟鍦ㄨ涔夊垎鏋愰樁娈电敱 monomorphization 鍒涘缓銆?
    // 蹇呴』鍦ㄧ紪璇戜富绋嬪簭浣撲箣鍓嶇紪璇戝畠浠紝纭繚琚皟鐢ㄦ椂宸叉敞鍐屼负鍏ㄥ眬鍙橀噺銆?
    if (analyzer_) {
        for (auto& funcDecl : analyzer_->getMonomorphizedFunctions()) {
            if (hasError_) break;
            // 淇濆瓨涓诲嚱鏁扮紪璇戠姸鎬?
            VMFunction* oldMain = func_;
            std::vector<UInt8>* oldMainCode = code_;
            std::vector<Value> oldMainConstants = constants_;
            int oldMainNextReg = nextReg_;
            bool oldMainAllTyped = allTyped_;

            // 缂栬瘧娉涘瀷瀹炰緥鍖栧嚱鏁帮紙杩欎細鍒涘缓 VMFunction 骞舵敞鍐屼负鍏ㄥ眬锛?
            compileFuncDecl(funcDecl);

            // 鎭㈠涓诲嚱鏁扮紪璇戠姸鎬?
            func_ = oldMain;
            code_ = oldMainCode;
            constants_ = oldMainConstants;
            nextReg_ = oldMainNextReg;
            allTyped_ = oldMainAllTyped;
        }
    }

    // 缂栬瘧姣忎釜璇彞
    for (size_t si = 0; si < program->statements.size(); si++) {
        if (hasError_) break;
        compileStmt(program->statements[si]);
        releaseTempRegs();
    }

    // 榛樿杩斿洖nil
    if (!hasError_ && (func_->code.size() < 4 || func_->code[func_->code.size()-4] != OP_RETURN)) {
        emit(OP_LOADNIL, 0, 0, 0);
        emit(OP_RETURN, 0, 0, 0);
    }

    // 璁剧疆鏈€澶ф爤娣卞害
    func_->maxStack = maxReg_ + 32;
    func_->constants = constants_;

    return hasError_ ? nullptr : func_;
}

// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?
//  鎸囦护鍙戝皠
// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?

void Codegen::emit(UInt8 op, UInt8 a, UInt8 b, UInt8 c) {
    // 娓愯繘绫诲瀷杩借釜锛氭鏌ユ槸鍚?untyped 鎸囦护
    if (allTyped_ && op < 0x90) {
        // OP_LOADNIL/LOADINT/LOADBOOL/RETURN/JUMP/CALL etc are structural, not untyped
        if (op >= 0x20 && op < 0x40) allTyped_ = false;  // untyped arithmetic/cmp
    }
    // 璁板綍婧愭枃浠惰鍙?
    func_->lineInfo.push_back(currentLine_);
    
    // 16-byte format: [op][a][b][c][padding12]
    code_->push_back(op);
    code_->push_back(a);
    code_->push_back(b);
    code_->push_back(c);

    if (a < 255 && a >= maxReg_) maxReg_ = a + 1;
    if (b < 255 && b >= maxReg_) maxReg_ = b + 1;
    if (c < 255 && c >= maxReg_) maxReg_ = c + 1;
}

void Codegen::emitInt(UInt8 op, UInt8 a, Int32 imm) {
    func_->lineInfo.push_back(currentLine_);
    
    Int32 idx;
    if (op == OP_LOADGLOBAL || op == OP_STOREGLOBAL || op == OP_LOADCONST) {
        idx = imm;
    } else {
        idx = addConstant(Value::Int(imm));
    }
    if (idx > 65535) idx = 65535;
    code_->push_back(op);
    code_->push_back(a);
    code_->push_back(static_cast<UInt8>((idx >> 8) & 0xFF));
    code_->push_back(static_cast<UInt8>(idx & 0xFF));
    if (a < 255 && a >= maxReg_) maxReg_ = a + 1;
}

void Codegen::emitJump(UInt8 op, int offset) {
    Int32 off = offset;
    if (off > 32767) off = 32767;
    if (off < -32768) off = -32768;
    code_->push_back(op);
    code_->push_back(static_cast<UInt8>(off & 0xFF));
    code_->push_back(static_cast<UInt8>((off >> 8) & 0xFF));
    code_->push_back(0);
}

int Codegen::emitJumpPlaceholder(UInt8 op, UInt8 a) {
    int pos = static_cast<int>(code_->size());
    code_->push_back(op);
    code_->push_back(a);
    code_->push_back(0); code_->push_back(0);
    return pos;
}

void Codegen::patchJump(int pos, int target) {
    Int32 offset = target - (pos + 4);
    if (offset > 32767) offset = 32767;
    if (offset < -32768) offset = -32768;
    (*code_)[pos + 2] = (offset >> 0) & 0xFF;
    (*code_)[pos + 3] = (offset >> 8) & 0xFF;
}

int Codegen::allocReg() {
    return nextReg_++;
}

// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?
//  甯搁噺姹?
// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?

Int32 Codegen::addConstant(const Value& v) {
    // 鏌ユ壘鏄惁宸插瓨鍦?
    for (Int32 i = 0; i < static_cast<Int32>(constants_.size()); i++) {
        if (constants_[i].equals(v)) return i;
    }
    Int32 idx = static_cast<Int32>(constants_.size());
    constants_.push_back(v);
    return idx;
}

// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?
//  灞€閮ㄥ彉閲?
// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?

Codegen::LocalVar* Codegen::findLocal(const String& name) {
    for (int i = static_cast<int>(localScopes_.size()) - 1; i >= 0; i--) {
        auto it = localScopes_[i].find(name);
        if (it != localScopes_[i].end()) {
            return &it->second;
        }
    }
    return nullptr;
}

int Codegen::getLocalReg(const String& name) {
    auto* lv = findLocal(name);
    if (lv) return lv->reg;
    return -1;
}

// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?
//  浣滅敤鍩?
// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?

void Codegen::freeRegs(int n) {
    // 閲婃斁 n 涓瘎瀛樺櫒(绠€鍖栧疄鐜?浠呰皟鏁?nextReg_)
    if (nextReg_ >= n) {
        nextReg_ -= n;
    } else {
        nextReg_ = 0;
    }
}

void Codegen::pushScope() {
    localScopes_.push_back({});
}

void Codegen::popScope() {
    if (!localScopes_.empty()) {
        localScopes_.pop_back();
    }
}

int Codegen::getScopeHighWater() {
    int maxReg = 0;
    for (auto& scope : localScopes_) {
        for (auto& [name, lv] : scope) {
            if (lv.reg + 1 > maxReg) maxReg = lv.reg + 1;
        }
    }
    return maxReg;
}

void Codegen::releaseTempRegs() {
    nextReg_ = getScopeHighWater();
}

void Codegen::emitConversion(const Value& from, const Value& to) {
    // 绫诲瀷杞崲(绠€鍖栧疄鐜?鏆備笉鏀寔)
    (void)from;
    (void)to;
}

void Codegen::compileComparison(TokenType op, int ra, int rb, int rc) {
    // 姣旇緝鎿嶄綔缂栬瘧(浣跨敤 compileBinaryOp 涓殑閫昏緫)
    switch (op) {
        case TokenType::OP_EQ:  emit(OP_CMPEQ, ra, rb, rc); break;
        case TokenType::OP_NE:  emit(OP_CMPNE, ra, rb, rc); break;
        case TokenType::OP_LT:  emit(OP_CMPLT, ra, rb, rc); break;
        case TokenType::OP_LE:  emit(OP_CMPLE, ra, rb, rc); break;
        case TokenType::OP_GT:  emit(OP_CMPGT, ra, rb, rc); break;
        case TokenType::OP_GE:  emit(OP_CMPGE, ra, rb, rc); break;
        default: emit(OP_MOVE, ra, rb, 0); break;
    }
}

// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?
//  璇彞缂栬瘧
// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?

Compiler::Compiler(bool useSlotOpt) : useSlotOpt_(useSlotOpt) {
    if (useSlotOpt_) {
        vm_ = std::make_unique<VM>();
        // 蹇呴』鍦╟odegen鍓嶆敞鍐宻tdlib,纭繚global slot缂栧彿涓€鑷?
        StdLib::registerAll(vm_.get());
        vm_->importCallback = [this](const std::string& filename) -> bool {
            // @cp/ 模块导入: 直接注册，不编译文件
            if (filename.find("@cp/") == 0 || filename.find("cp/") == 0) {
                std::vector<std::string> modules = {filename};
                StdLib::registerModules(vm_.get(), modules);
                return true;
            }
            // 常规文件导入
            VMFunction* f = compileFile(filename);
            if (!f) { vm_->raiseError(errorMsg_); return false; }
            vm_->setLastImportedFunc(f);
            return true;
        };
    }
}

void Codegen::compileTry(Shared<TryStmt> s) {
    // 鍒嗛厤寮傚父鍊煎瘎瀛樺櫒
    UInt8 excReg = (UInt8)allocReg();

    // OP_TRY 鍗犱綅: [op][a][0][0][catchPC_imm32][padding8]
    int tryPos = static_cast<int>(code_->size());
    code_->push_back(OP_TRY);
    code_->push_back(excReg);
    code_->push_back(0); code_->push_back(0);  // b, c
    for (int i = 0; i < 4; i++) code_->push_back(0);  // imm32 placeholder
    for (int i = 0; i < 8; i++) code_->push_back(0);  // padding to 16

    // 缂栬瘧 try 浣?
    compileStmt(s->tryBlock);

    // OP_ENDTRY: 姝ｅ父璺緞寮瑰嚭handler
    emit(OP_ENDTRY);

    // 璺宠繃catch鍧楋紙姝ｅ父瀹屾垚璺緞锛?
    int skipJump = emitJumpPlaceholder(OP_JUMP);

    // 璁板綍catch浣嶇疆锛屽洖濉玂P_TRY涓殑catchPC
    int catchPos = static_cast<int>(code_->size());
    (*code_)[tryPos + 4] = (catchPos >> 0) & 0xFF;
    (*code_)[tryPos + 5] = (catchPos >> 8) & 0xFF;
    (*code_)[tryPos + 6] = (catchPos >> 16) & 0xFF;
    (*code_)[tryPos + 7] = (catchPos >> 24) & 0xFF;

    // 缂栬瘧鎵€鏈?catch 鍧楋紙鏆備笉鍖哄垎寮傚父绫诲瀷锛屼緷娆℃墽琛屾墍鏈夊鐞嗗潡锛?
    for (size_t i = 0; i < s->catchBlocks.size(); i++) {
        pushScope();
        String exName = s->catchBlocks[i].exceptionType;
        if (!exName.empty()) {
            localScopes_.back()[exName] = { (int)excReg, nullptr };
        }
        compileStmt(s->catchBlocks[i].body);
        popScope();
    }

    // 鍥炲～璺宠繃璺宠浆
    patchJump(skipJump, static_cast<int>(code_->size()));

    freeRegs(1);
}

void Codegen::compileThrow(Shared<ThrowStmt> s) {
    int reg = compileExpr(s->exception);
    emit(OP_THROW, (UInt8)reg);
    // 娉ㄦ剰: 涓嶅湪杩欓噷freeReg锛屽洜涓篛P_THROW鎺у埗娴佷笉浼氳繑鍥?
    // 瀵勫瓨鍣ㄧ敱澶栧眰compileStmt缁熶竴绠＄悊
}

void Codegen::compileSwitch(Shared<SwitchStmt> s) {
    int exprReg = compileExpr(s->expr);
    int exitPos = 0;
    int defaultJump = -1;

    // Phase 1: emit case comparisons and conditional jumps
    struct CaseInfo {
        int jumpHere;
        Shared<Stmt> body;
    };
    std::vector<CaseInfo> caseList;
    std::vector<int> exitJumps;

    for (size_t i = 0; i < s->cases.size(); i++) {
        Shared<Expr> caseExpr = *s->cases[i].first;
        int caseValReg = compileExpr(caseExpr);
        int cmpReg = allocReg();
        emit(OP_CMPEQ, cmpReg, exprReg, caseValReg);
        // If equal, jump to this case body
        int matchJump = emitJumpPlaceholder(OP_JUMPIF, static_cast<UInt8>(cmpReg));
        caseList.push_back({matchJump, s->cases[i].second});
    }

    // If no case matched, jump to default or exit
    if (s->defaultCase) {
        defaultJump = emitJumpPlaceholder(OP_JUMP);
    } else {
        exitJumps.push_back(emitJumpPlaceholder(OP_JUMP));
    }

    // Phase 2: compile case bodies (each followed by jump to exit)
    int savedNextReg = nextReg_;
    for (size_t i = 0; i < caseList.size(); i++) {
        nextReg_ = savedNextReg;
        patchJump(caseList[i].jumpHere, static_cast<int>(code_->size()));
        compileStmt(caseList[i].body);
        exitJumps.push_back(emitJumpPlaceholder(OP_JUMP));
    }

    // Phase 3: default case
    if (s->defaultCase && defaultJump >= 0) {
        nextReg_ = savedNextReg;
        patchJump(defaultJump, static_cast<int>(code_->size()));
        compileStmt(s->defaultCase);
    }

    // Phase 4: patch all exit jumps
    exitPos = static_cast<int>(code_->size());
    for (int jump : exitJumps) {
        patchJump(jump, exitPos);
    }
}

Compiler::~Compiler() {}

VMFunction* Compiler::compile(const String& source, const String& sourceFile) {
    return compileInternal(source, sourceFile);
}

VMFunction* Compiler::compileFile(const String& filename) {
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        hasError_ = true;
        errorMsg_ = "鏃犳硶鎵撳紑鏂囦欢: " + filename;
        return nullptr;
    }
    String source((std::istreambuf_iterator<char>(ifs)),
                   std::istreambuf_iterator<char>());
    return compileInternal(source, filename);
}

VMFunction* Compiler::compileInternal(const String& source, const String& sourceFile) {
    // 璇嶆硶鍒嗘瀽
    Lexer lexer(source);

    // 璇硶鍒嗘瀽
    Parser parser(&lexer);
    auto program = parser.parse();
    if (!program) {
        hasError_ = true;
        errorMsg_ = "璇硶鍒嗘瀽: 绋嬪簭涓虹┖";
        return nullptr;
    }
    if (parser.hasError()) {
        hasError_ = true;
        errorMsg_ = "璇硶鍒嗘瀽: " + parser.errorMessage();
        return nullptr;
    }

    // 璇箟鍒嗘瀽
    SemanticAnalyzer analyzer;
    if (!analyzer.analyze(program)) {
        hasError_ = true;
        errorMsg_ = "璇箟鍒嗘瀽: " + analyzer.errorMessage();
        return nullptr;
    }

    // 浠ｇ爜鐢熸垚
    Codegen codegen(useSlotOpt_ ? vm_.get() : nullptr, &analyzer);
    codegen.setSourceFile(sourceFile);
    VMFunction* func = codegen.compile(program);
    if (codegen.hasError()) {
        hasError_ = true;
        errorMsg_ = "浠ｇ爜鐢熸垚: " + codegen.errorMessage();
        return nullptr;
    }

    // 璁剧疆婧愭枃浠跺悕锛堜緵 source_location 鏍囧噯搴撲娇鐢級
    if (func) func->sourceFile = sourceFile;

    return func;
}

void Compiler::setTraceVM(bool v) {
    // 璁剧疆 VM 璺熻釜鏍囧織(鐢ㄤ簬璋冭瘯)
    (void)v;
}

} // namespace cplang
