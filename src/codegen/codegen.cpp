// CP语言 代码生成器实现
#include "codegen/codegen.hpp"
#include <cstdio>
#include "parser/parser.hpp"
#include "stdlib/stdlib.hpp"
#include <sstream>
#include <fstream>
#include <iterator>
#include <iostream>

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  构造函数
// ═══════════════════════════════════════════════════════════════════

Codegen::Codegen(VM* vm, SemanticAnalyzer* analyzer) : vm_(vm), analyzer_(analyzer) {
    pushScope(); // 全局作用域
}

Codegen::~Codegen() {
    while (!localScopes_.empty()) popScope();
}

// ═══════════════════════════════════════════════════════════════════
//  主编译入口
// ═══════════════════════════════════════════════════════════════════

VMFunction* Codegen::compile(Shared<Program> program) {
    // 创建主函数（先创建，这样 compileFuncDecl 在注册函数时能正确发射代码）
    func_ = new VMFunction();
    func_->name = nullptr; // 主模块
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

    // === 预编译所有泛型实例化函数 ===
    // 这些函数在语义分析阶段由 monomorphization 创建。
    // 必须在编译主程序体之前编译它们，确保被调用时已注册为全局变量。
    if (analyzer_) {
        for (auto& funcDecl : analyzer_->getMonomorphizedFunctions()) {
            if (hasError_) break;
            // 保存主函数编译状态
            VMFunction* oldMain = func_;
            std::vector<UInt8>* oldMainCode = code_;
            std::vector<Value> oldMainConstants = constants_;
            int oldMainNextReg = nextReg_;
            bool oldMainAllTyped = allTyped_;

            // 编译泛型实例化函数（这会创建 VMFunction 并注册为全局）
            compileFuncDecl(funcDecl);

            // 恢复主函数编译状态
            func_ = oldMain;
            code_ = oldMainCode;
            constants_ = oldMainConstants;
            nextReg_ = oldMainNextReg;
            allTyped_ = oldMainAllTyped;
        }
    }

    // 编译每个语句
    for (size_t si = 0; si < program->statements.size(); si++) {
        if (hasError_) break;
        printf("DEBUG: compiling stmt %zu\n", si); fflush(stdout);
        compileStmt(program->statements[si]);
        releaseTempRegs();
    }

    printf("DEBUG: all statements compiled, adding default return\n"); fflush(stdout);
    printf("DEBUG: code size = %zu, empty = %d\n", func_->code.size(), func_->code.empty()); fflush(stdout);
    // 默认返回nil
    if (!hasError_ && (func_->code.empty() || func_->code.back() != OP_RETURN)) {
        printf("DEBUG: emitting default return\n"); fflush(stdout);
        emit(OP_LOADNIL, 0, 0, 0);
        emit(OP_RETURN, 0, 0, 0);
    }
    printf("DEBUG: setting maxStack\n"); fflush(stdout);

    // 设置最大栈深度
    printf("DEBUG: func_=%p maxReg_=%d\n", (void*)func_, maxReg_); fflush(stdout);
    func_->maxStack = maxReg_ + 32;
    printf("DEBUG: setting constants, size=%zu\n", constants_.size()); fflush(stdout);
    func_->constants = constants_;
    printf("DEBUG: compile done, returning\n"); fflush(stdout);

    return hasError_ ? nullptr : func_;
}

// ═══════════════════════════════════════════════════════════════════
//  指令发射
// ═══════════════════════════════════════════════════════════════════

void Codegen::emit(UInt8 op, UInt8 a, UInt8 b, UInt8 c) {
    // 渐进类型追踪：检查是否 untyped 指令
    if (allTyped_ && op < 0x90) {
        // OP_LOADNIL/LOADINT/LOADBOOL/RETURN/JUMP/CALL etc are structural, not untyped
        if (op >= 0x20 && op < 0x40) allTyped_ = false;  // untyped arithmetic/cmp
    }
    // 记录源文件行号
    func_->lineInfo.push_back(currentLine_);
    
    // 16-byte format: [op][a][b][c][padding12]
    code_->push_back(op);
    code_->push_back(a);
    code_->push_back(b);
    code_->push_back(c);
    for (int i = 0; i < 12; i++) code_->push_back(0);

    if (a < 255 && a >= maxReg_) maxReg_ = a + 1;
    if (b < 255 && b >= maxReg_) maxReg_ = b + 1;
    if (c < 255 && c >= maxReg_) maxReg_ = c + 1;
}

void Codegen::emitInt(UInt8 op, UInt8 a, Int32 imm) {
    // 记录源文件行号
    func_->lineInfo.push_back(currentLine_);
    
    // 16-byte format: [op][a][b][c][imm32]
    code_->push_back(op);
    code_->push_back(a);
    code_->push_back(0); code_->push_back(0);  // b, c reserved
    // write 4-byte little-endian imm
    code_->push_back(static_cast<UInt8>(imm & 0xFF));
    code_->push_back(static_cast<UInt8>((imm >> 8) & 0xFF));
    code_->push_back(static_cast<UInt8>((imm >> 16) & 0xFF));
    code_->push_back(static_cast<UInt8>((imm >> 24) & 0xFF));
    // Pad to 16 bytes
    code_->push_back(0); code_->push_back(0); code_->push_back(0); code_->push_back(0);
    code_->push_back(0); code_->push_back(0); code_->push_back(0); code_->push_back(0);
    if (a < 255 && a >= maxReg_) maxReg_ = a + 1;
}

void Codegen::emitJump(UInt8 op, int offset) {
    // 16-byte format: [op][0][0][0][offset32]
    code_->push_back(op);
    code_->push_back(0); code_->push_back(0); code_->push_back(0);
    code_->push_back(static_cast<UInt8>(offset & 0xFF));
    code_->push_back(static_cast<UInt8>((offset >> 8) & 0xFF));
    code_->push_back(static_cast<UInt8>((offset >> 16) & 0xFF));
    code_->push_back(static_cast<UInt8>((offset >> 24) & 0xFF));
}

int Codegen::emitJumpPlaceholder(UInt8 op, UInt8 a) {
    int pos = static_cast<int>(code_->size());
    // 16-byte format: [op][a][0][0][offset32=0][padding8]
    code_->push_back(op);
    code_->push_back(a);
    code_->push_back(0); code_->push_back(0);  // b, c
    code_->push_back(0); code_->push_back(0); code_->push_back(0); code_->push_back(0);  // offset32=0
    for (int i = 0; i < 8; i++) code_->push_back(0);  // 8 bytes padding to reach 16
    return pos;
}

void Codegen::patchJump(int pos, int target) {
    Int32 offset = target - (pos + 16);  // 16-byte instruction
    (*code_)[pos + 4] = (offset >> 0) & 0xFF;
    (*code_)[pos + 5] = (offset >> 8) & 0xFF;
    (*code_)[pos + 6] = (offset >> 16) & 0xFF;
    (*code_)[pos + 7] = (offset >> 24) & 0xFF;
}

int Codegen::allocReg() {
    return nextReg_++;
}

// ═══════════════════════════════════════════════════════════════════
//  常量池
// ═══════════════════════════════════════════════════════════════════

Int32 Codegen::addConstant(const Value& v) {
    // 查找是否已存在
    for (Int32 i = 0; i < static_cast<Int32>(constants_.size()); i++) {
        if (constants_[i].equals(v)) return i;
    }
    Int32 idx = static_cast<Int32>(constants_.size());
    constants_.push_back(v);
    return idx;
}

// ═══════════════════════════════════════════════════════════════════
//  局部变量
// ═══════════════════════════════════════════════════════════════════

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

// ═══════════════════════════════════════════════════════════════════
//  作用域
// ═══════════════════════════════════════════════════════════════════

void Codegen::freeRegs(int n) {
    // 释放 n 个寄存器(简化实现:仅调整 nextReg_)
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
    // 类型转换(简化实现,暂不支持)
    (void)from;
    (void)to;
}

void Codegen::compileComparison(TokenType op, int ra, int rb, int rc) {
    // 比较操作编译(使用 compileBinaryOp 中的逻辑)
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

// ═══════════════════════════════════════════════════════════════════
//  语句编译
// ═══════════════════════════════════════════════════════════════════

Compiler::Compiler(bool useSlotOpt) : useSlotOpt_(useSlotOpt) {
    if (useSlotOpt_) {
        vm_ = std::make_unique<VM>();
        // 必须在codegen前注册stdlib,确保global slot编号一致
        StdLib::registerAll(vm_.get());
        vm_->importCallback = [this](const std::string& filename) -> bool {
            VMFunction* f = compileFile(filename);
            if (!f) { vm_->raiseError(errorMsg_); return false; }
            vm_->setLastImportedFunc(f);
            return true;
        };
    }
}

void Codegen::compileTry(Shared<TryStmt> s) {
    // 分配异常值寄存器
    UInt8 excReg = (UInt8)allocReg();

    // OP_TRY 占位: [op][a][0][0][catchPC_imm32][padding8]
    int tryPos = static_cast<int>(code_->size());
    code_->push_back(OP_TRY);
    code_->push_back(excReg);
    code_->push_back(0); code_->push_back(0);  // b, c
    for (int i = 0; i < 4; i++) code_->push_back(0);  // imm32 placeholder
    for (int i = 0; i < 8; i++) code_->push_back(0);  // padding to 16

    // 编译 try 体
    compileStmt(s->tryBlock);

    // OP_ENDTRY: 正常路径弹出handler
    emit(OP_ENDTRY);

    // 跳过catch块（正常完成路径）
    int skipJump = emitJumpPlaceholder(OP_JUMP);

    // 记录catch位置，回填OP_TRY中的catchPC
    int catchPos = static_cast<int>(code_->size());
    (*code_)[tryPos + 4] = (catchPos >> 0) & 0xFF;
    (*code_)[tryPos + 5] = (catchPos >> 8) & 0xFF;
    (*code_)[tryPos + 6] = (catchPos >> 16) & 0xFF;
    (*code_)[tryPos + 7] = (catchPos >> 24) & 0xFF;

    // 编译所有 catch 块（暂不区分异常类型，依次执行所有处理块）
    for (size_t i = 0; i < s->catchBlocks.size(); i++) {
        pushScope();
        String exName = s->catchBlocks[i].first;
        if (!exName.empty()) {
            localScopes_.back()[exName] = { (int)excReg, nullptr };
        }
        compileStmt(s->catchBlocks[i].second);
        popScope();
    }

    // 回填跳过跳转
    patchJump(skipJump, static_cast<int>(code_->size()));

    freeRegs(1);
}

void Codegen::compileThrow(Shared<ThrowStmt> s) {
    int reg = compileExpr(s->exception);
    emit(OP_THROW, (UInt8)reg);
    // 注意: 不在这里freeReg，因为OP_THROW控制流不会返回
    // 寄存器由外层compileStmt统一管理
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
        errorMsg_ = "无法打开文件: " + filename;
        return nullptr;
    }
    String source((std::istreambuf_iterator<char>(ifs)),
                   std::istreambuf_iterator<char>());
    return compileInternal(source, filename);
}

VMFunction* Compiler::compileInternal(const String& source, const String& sourceFile) {
    printf("DEBUG: compileInternal start\n"); fflush(stdout);
    // 词法分析
    Lexer lexer(source);

    // 语法分析
    printf("DEBUG: parsing...\n"); fflush(stdout);
    Parser parser(&lexer);
    auto program = parser.parse();
    if (!program) {
        hasError_ = true;
        errorMsg_ = "语法分析: 程序为空";
        return nullptr;
    }
    if (parser.hasError()) {
        hasError_ = true;
        errorMsg_ = "语法分析: " + parser.errorMessage();
        return nullptr;
    }

    printf("DEBUG: parsing done, semantic...\n"); fflush(stdout);
    // 语义分析
    SemanticAnalyzer analyzer;
    if (!analyzer.analyze(program)) {
        hasError_ = true;
        errorMsg_ = "语义分析: " + analyzer.errorMessage();
        return nullptr;
    }

    printf("DEBUG: semantic done, codegen...\n"); fflush(stdout);
    // 代码生成
    Codegen codegen(useSlotOpt_ ? vm_.get() : nullptr, &analyzer);
    codegen.setSourceFile(sourceFile);
    printf("DEBUG: codegen created, compiling...\n"); fflush(stdout);
    VMFunction* func = codegen.compile(program);
    printf("DEBUG: compile returned, func=%p\n", (void*)func); fflush(stdout);
    if (codegen.hasError()) {
        hasError_ = true;
        errorMsg_ = "代码生成: " + codegen.errorMessage();
        return nullptr;
    }

    // 设置源文件名（供 source_location 标准库使用）
    if (func) func->sourceFile = sourceFile;

    return func;
}

void Compiler::setTraceVM(bool v) {
    // 设置 VM 跟踪标志(用于调试)
    (void)v;
}

} // namespace cplang