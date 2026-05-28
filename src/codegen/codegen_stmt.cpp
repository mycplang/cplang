// CP语言 代码生成器实现 - 语句编译
#include "codegen/codegen.hpp"

namespace cplang {

void Codegen::compileStmt(Shared<Stmt> stmt) {
    if (!stmt) return;
    
    // 设置当前源码行号（用于错误定位）
    setLine(stmt->token.line);

    if (auto p = std::dynamic_pointer_cast<PackageStmt>(stmt)) compilePackage(p);
    else if (auto imp = std::dynamic_pointer_cast<ImportStmt>(stmt)) compileImport(imp);
    else if (auto v = std::dynamic_pointer_cast<VarDeclStmt>(stmt)) compileVarDecl(v);
    else if (auto func = std::dynamic_pointer_cast<FuncDeclStmt>(stmt)) compileFuncDecl(func);
    else if (auto cls = std::dynamic_pointer_cast<ClassDeclStmt>(stmt)) compileClassDecl(cls);
    else if (auto enm = std::dynamic_pointer_cast<EnumDeclStmt>(stmt)) compileEnumDecl(enm);
    else if (auto s = std::dynamic_pointer_cast<StructDeclStmt>(stmt)) compileStructDecl(s);
    else if (auto blk = std::dynamic_pointer_cast<BlockStmt>(stmt)) compileBlock(blk);
    else if (auto trust = std::dynamic_pointer_cast<TrustBlockStmt>(stmt)) {
        if (trust->body) compileBlock(trust->body);  // 可信块目前就是普通块
    }
    else if (auto ifStmt = std::dynamic_pointer_cast<IfStmt>(stmt)) compileIf(ifStmt);
    else if (auto forStmt = std::dynamic_pointer_cast<ForStmt>(stmt)) compileFor(forStmt);
    else if (auto fe = std::dynamic_pointer_cast<ForEachStmt>(stmt)) compileForEach(fe);
    else if (auto w = std::dynamic_pointer_cast<WhileStmt>(stmt)) compileWhile(w);
    else if (auto r = std::dynamic_pointer_cast<ReturnStmt>(stmt)) compileReturn(r);
    else if (auto brk = std::dynamic_pointer_cast<BreakStmt>(stmt)) compileBreak(brk);
    else if (auto cont = std::dynamic_pointer_cast<ContinueStmt>(stmt)) compileContinue(cont);
    else if (auto tr = std::dynamic_pointer_cast<TryStmt>(stmt)) compileTry(tr);
    else if (auto th = std::dynamic_pointer_cast<ThrowStmt>(stmt)) compileThrow(th);
    else if (auto sw = std::dynamic_pointer_cast<SwitchStmt>(stmt)) compileSwitch(sw);
    else if (auto e = std::dynamic_pointer_cast<ExprStmt>(stmt)) compileExprStmt(e);
    else if (auto d = std::dynamic_pointer_cast<DeferStmt>(stmt)) compileDefer(d);
}

void Codegen::compilePackage(Shared<PackageStmt> /*stmt*/) {
    // package 声明不生成代码
}

void Codegen::compileImport(Shared<ImportStmt> stmt) {
    // 将模块名添加到常量池
    VMString* modNameStr = VMString::create(stmt->moduleName.c_str(),
                                             static_cast<UInt32>(stmt->moduleName.length()));
    Int32 modNameIdx = addConstant(makeStringVal(modNameStr));

    // 分配结果寄存器
    int resultReg = allocReg();

    // 生成 OP_IMPORT ra, moduleNameIdx
    // 格式: OP(8) | A(8) | B(8) | C(8)
    // B 是常量池索引的低8位
    emit(OP_IMPORT, static_cast<UInt8>(resultReg), static_cast<UInt8>(modNameIdx & 0xFF), 0);

    // 释放结果寄存器(import结果通常不需要使用)
    nextReg_ = resultReg;  // 回退寄存器分配
}

void Codegen::compileVarDecl(Shared<VarDeclStmt> stmt) {
    if (stmt->isImplicit) {
        int existingReg = getLocalReg(stmt->name);
        if (existingReg >= 0) {
            if (stmt->init) { int srcReg = compileExpr(stmt->init); emit(OP_MOVE, existingReg, srcReg, 0); }
            return;
        }
        // Check if this is an existing global variable (or create slot for it)
        // Use getOrCreateGlobalSlot (non-inline, avoids MSVC inline expansion crash across TUs)
        if (vm_) {
            Int32 slot = vm_->getOrCreateGlobalSlot(stmt->name.c_str());
            if (slot >= 0) {
                int srcReg = stmt->init ? compileExpr(stmt->init) : 0;
                if (!stmt->init) { srcReg = allocReg(); emit(OP_LOADNIL, srcReg, 0, 0); }
                emitInt(OP_STOREGLOBAL, srcReg, slot);
                return;
            }
        }
    }
    bool isGlobal = (localScopes_.size() == 1);

    if (isGlobal) {
        // Global variable: store directly
        int valReg = 0;
        if (stmt->init) {
            valReg = compileExpr(stmt->init);
        } else {
            valReg = allocReg();
            emit(OP_LOADNIL, valReg, 0, 0);
        }
        // Store to global using Slot化优化
        if (vm_) {
            Int32 slot = vm_->getOrCreateGlobalSlot(stmt->name.c_str());
            emitInt(OP_STOREGLOBAL, valReg, slot);
        } else {
            // Fallback: use legacy string-based instruction
            VMString* name = VMString::create(stmt->name);
            Int32 idx = addConstant(makeStringVal(name));
            emitInt(OP_STOREGLOBAL, valReg, idx);
        }
    } else {
        // Local variable: store in register
        int resultReg = allocReg();
        if (stmt->init) {
            int srcReg = compileExpr(stmt->init);
            emit(OP_MOVE, resultReg, srcReg, 0);
        } else {
            emit(OP_LOADNIL, resultReg, 0, 0);
        }
        // Register to local variable table
        LocalVar lv{resultReg, nullptr};
        localScopes_.back()[stmt->name] = lv;
    }
}

void Codegen::compileFuncDecl(Shared<FuncDeclStmt> stmt) {
    // 保存外层编译状态
    VMFunction* oldFunc = func_;
    std::vector<UInt8>* oldCode = code_;
    int oldNextReg = nextReg_;
    std::vector<Value> oldConstants = constants_;  // 保存外层常量表

    // 清空常量表,为新函数准备
    constants_.clear();

    // 创建新函数
    VMFunction* newFunc = new VMFunction();
    newFunc->name = VMString::create(stmt->name);
    newFunc->maxStack = 256;
    newFunc->numParams = static_cast<UInt32>(stmt->params.size());
    newFunc->numLocals = 0;
    newFunc->isVararg = false;
    newFunc->hasSlots = (vm_ != nullptr);
    newFunc->sourceFile = sourceFile_;
    
    // 检查是否有显式类型标注
    for (auto& p : stmt->params) {
        if (p.second.has_value()) { newFunc->hasExplicitTypes = true; break; }
    }
    if (stmt->returnType.has_value()) newFunc->hasExplicitTypes = true;

    // 切换到新函数的编译上下文
    func_ = newFunc;
    code_ = &newFunc->code;
    nextReg_ = 0;
    maxReg_ = 0;

    // 创建函数作用域
    pushScope();

    // 注册参数到局部变量(参数占用寄存器 0, 1, 2, ...)
    for (size_t p = 0; p < stmt->params.size(); p++) {
        LocalVar lv{static_cast<int>(p), nullptr};
        localScopes_.back()[stmt->params[p].first] = lv;
        if (static_cast<int>(p) >= nextReg_) nextReg_ = static_cast<int>(p) + 1;
    }

    // 编译函数体
    if (stmt->body) {
        compileBlock(stmt->body);
    }

    // 默认返回 nil
    if (func_->code.empty() || func_->code.back() != OP_RETURN) {
        emit(OP_LOADNIL, 0, 0, 0);
        emit(OP_RETURN, 0, 0, 0);
    }

    // 弹出函数作用域
    popScope();

    // 设置新函数的属性
    newFunc->maxStack = maxReg_ + 16;
    newFunc->constants = constants_;  // 复制常量表
    newFunc->isTyped = allTyped_;     // 渐进类型标记
    
    lastCompiledFunc_ = func_;
    // 恢复外层编译状态
    func_ = oldFunc;
    code_ = oldCode;
    nextReg_ = oldNextReg;
    constants_ = oldConstants;  // 恢复外层常量表
    
    // 重置 typed 追踪
    allTyped_ = true;

    // 添加新函数到外层的常量池
    Value funcVal = makeFunctionVal(reinterpret_cast<VMFunction*>(newFunc));
    Int32 funcIdx = addConstant(funcVal);

    // 全局注册函数名
    VMString* nameStr = VMString::create(stmt->name);
    Int32 nameIdx = addConstant(makeStringVal(nameStr));

    // 生成代码:加载函数引用,存入全局(使用Slot化优化)
    int r = allocReg();
    emitInt(OP_LOADCONST, r, funcIdx);
    if (vm_) {
        Int32 slot = vm_->getOrCreateGlobalSlot(stmt->name.c_str());
        emitInt(OP_STOREGLOBAL, r, slot);
        // 自动调用main函数(入口点)
        if (stmt->name == "main" || stmt->name == "主") {
            int callReg = allocReg();
            emitInt(OP_LOADGLOBAL, callReg, slot);
            emit(OP_CALL, callReg, callReg, 0);
        }
    } else {
        emitInt(OP_STOREGLOBAL, r, nameIdx);
    }
}

void Codegen::compileEnumDecl(Shared<EnumDeclStmt> stmt) {
    // 将枚举值注册为全局常量
    if (!vm_) return;
    Int64 val = 0;
    for (auto& [name, init] : stmt->values) {
        if (init.has_value()) {
            val = init.value();
        }
        vm_->registerGlobal(name.c_str(), Value::Int(val));
        val++;
    }
}

void Codegen::compileClassDecl(Shared<ClassDeclStmt> stmt) {
    ClassMeta meta;
    meta.name = stmt->name;
    for (auto& m : stmt->members) {
        if (auto f = std::dynamic_pointer_cast<FuncDeclStmt>(m)) {
            // 注入 self 为隐式第一参数
            f->params.insert(f->params.begin(), {"self", std::nullopt});
            compileFuncDecl(f);
            f->params.erase(f->params.begin());
            if (lastCompiledFunc_) {
                meta.methods.push_back({f->name, lastCompiledFunc_});
                lastCompiledFunc_ = nullptr;
            }
        } else if (auto v = std::dynamic_pointer_cast<VarDeclStmt>(m)) {
            meta.fieldNames.push_back(v->name);
        }
    }
    classMeta_[stmt->name] = meta;
}

void Codegen::compileStructDecl(Shared<StructDeclStmt> stmt) {
    // 结构体声明在语义分析阶段已处理
    // 这里注册结构体类型信息供运行时和代码生成使用

    if (!analyzer_) return;

    // 获取结构体信息
    StructInfo* info = analyzer_->getStructInfo(stmt->name);
    if (!info) return;

    // 创建结构体描述符并添加到常量池
    // 简化处理:结构体类型已在语义分析阶段注册
    // 这里可以为 VM 创建结构体元数据对象

    // 如果 VM 支持结构体元数据,可以创建 VMStruct 对象
    if (vm_) {
        // 注册结构体类型到 VM 的类型表
        // 实际实现取决于 VM 的结构体支持程度
    }
}

void Codegen::compileBlock(Shared<BlockStmt> block) {
    pushScope();
    deferStack_.push_back({});  // 新作用域：新的 defer 层
    for (auto& s : block->statements) {
        compileStmt(s);
        releaseTempRegs();
    }
    // 离开作用域：逆序执行所有本层的 defer
    if (!deferStack_.empty()) {
        auto& defers = deferStack_.back();
        for (int i = static_cast<int>(defers.size()) - 1; i >= 0; i--) {
            compileStmt(defers[i]);
        }
        deferStack_.pop_back();
    }
    popScope();
}

void Codegen::compileIf(Shared<IfStmt> s) {
    // condition
    int condReg = compileExpr(s->condition);

    // if !cond jump to else/elif
    int elseJump = emitJumpPlaceholder(OP_JUMPNIF, static_cast<UInt8>(condReg));

    // then branch
    compileStmt(s->thenBranch);

    if (s->elseBranch) {
        // jump over else after then
        int endJump = emitJumpPlaceholder(OP_JUMP);

        // patch else jump to here (start of else)
        patchJump(elseJump, static_cast<int>(code_->size()));

        // else branch
        compileStmt(s->elseBranch);

        // patch end jump to here (after else)
        patchJump(endJump, static_cast<int>(code_->size()));
    } else {
        // patch to end (no else)
        patchJump(elseJump, static_cast<int>(code_->size()));
    }
}

void Codegen::compileFor(Shared<ForStmt> s) {
    size_t loopIdx = loopStack_.size();
    loopStack_.push_back(LoopContext{});
    int loopStart = static_cast<int>(code_->size());
    loopStack_[loopIdx].continueTarget = loopStart;

    // init
    if (s->init) compileStmt(s->init);

    // condition check
    int condStart = static_cast<int>(code_->size());
    loopStack_[loopIdx].continueTarget = condStart;  // fallback (while-style)

    int condReg = 0;

    if (s->condition) {
        condReg = compileExpr(s->condition);
    } else {
        // always true
        condReg = allocReg();
        emit(OP_LOADBOOL, condReg, 1, 0);
    }

    // jump out if false
    int exitJump = emitJumpPlaceholder(OP_JUMPNIF, static_cast<UInt8>(condReg));

    // body
    compileStmt(s->body);

    // continue target: after body, before update (C-style for)
    int continuePos = static_cast<int>(code_->size());
    for (int jmpPos : loopStack_[loopIdx].pendingContinues) {
        patchJump(jmpPos, continuePos);
    }
    // update
    if (s->update) compileExpr(s->update);

    // jump back to condition check
    int backJump = emitJumpPlaceholder(OP_JUMP);
    patchJump(backJump, condStart);

    // patch exit
    int exitTarget = static_cast<int>(code_->size());
    patchJump(exitJump, exitTarget);

    // patch all pending break jumps to exit point
    for (int jmpPos : loopStack_[loopIdx].pendingBreaks) {
        patchJump(jmpPos, exitTarget);
    }

    loopStack_.pop_back();
}

void Codegen::compileForEach(Shared<ForEachStmt> s) {
    size_t loopIdx = loopStack_.size();
    loopStack_.push_back(LoopContext{});

    // 编译可迭代对象
    int iterReg = compileExpr(s->iterable);

    // 保存到临时全局槽位，循环体内每轮重新加载
    Int32 iterSlot = vm_ ? vm_->getOrCreateGlobalSlot("__for_tmp_iter") : 0;
    emitInt(OP_STOREGLOBAL, iterReg, iterSlot);

    // 分配索引变量寄存器
    int idxReg = allocReg();
    emit(OP_LOADINT, idxReg, 0, 0);  // idx = 0

    // 循环开始：每轮重新加载可迭代对象
    int loopStart = static_cast<int>(code_->size());
    loopStack_[loopIdx].continueTarget = loopStart;
    emitInt(OP_LOADGLOBAL, iterReg, iterSlot);

    // 分配元素变量寄存器
    int elemReg = allocReg();

    // 获取元素: GETELEM elemReg, iterReg, idxReg
    emit(OP_GETELEM, elemReg, iterReg, idxReg);

    // 检查是否为 nil(数组越界)- 使用 JUMPIF (如果 elem == nil 则跳出)
    int condReg = allocReg();
    emit(OP_ISNULL, condReg, elemReg, 0);  // cond = isnull(elem)

    // 如果为 nil,跳出循环
    int exitJump = emitJumpPlaceholder(OP_JUMPIF, static_cast<UInt8>(condReg));

    // 存储到全局变量 — 使用正确的global slot
    Int32 varSlot = vm_ ? vm_->getOrCreateGlobalSlot(s->varName.c_str()) : 0;
    emitInt(OP_STOREGLOBAL, elemReg, varSlot);

    // 循环体
    compileStmt(s->body);

    // patch pending continues: jump before index increment
    int continuePos = static_cast<int>(code_->size());
    for (int jmpPos : loopStack_[loopIdx].pendingContinues) {
        patchJump(jmpPos, continuePos);
    }

    // 索引递增 - 使用 emitInt 加载立即数1到临时寄存器
    int oneReg = allocReg();
    emitInt(OP_LOADINT, static_cast<UInt8>(oneReg), 1);
    emit(OP_ADD, static_cast<UInt8>(idxReg), static_cast<UInt8>(idxReg), static_cast<UInt8>(oneReg));

    // 跳回循环开始
    int backJump = emitJumpPlaceholder(OP_JUMP);
    patchJump(backJump, loopStart);

    // 修补退出跳转
    int exitTarget = static_cast<int>(code_->size());
    patchJump(exitJump, exitTarget);

    // patch all pending break jumps to exit point
    for (int jmpPos : loopStack_[loopIdx].pendingBreaks) {
        patchJump(jmpPos, exitTarget);
    }

    loopStack_.pop_back();
}

void Codegen::compileWhile(Shared<WhileStmt> s) {
    size_t loopIdx = loopStack_.size();
    loopStack_.push_back(LoopContext{});
    int loopStart = static_cast<int>(code_->size());
    loopStack_[loopIdx].continueTarget = loopStart;

    // condition
    int condReg = compileExpr(s->condition);
    int exitJump = emitJumpPlaceholder(OP_JUMPNIF, static_cast<UInt8>(condReg));

    // body
    compileStmt(s->body);

    // patch pending continues: jump to condition check (while-style)
    for (int jmpPos : loopStack_[loopIdx].pendingContinues) {
        patchJump(jmpPos, loopStart);
    }
    // jump back to loop start
    int backJump = emitJumpPlaceholder(OP_JUMP);
    patchJump(backJump, loopStart);

    // exit: patch the JUMPNIF to jump here
    int exitTarget = static_cast<int>(code_->size());
    patchJump(exitJump, exitTarget);

    // patch all pending break jumps to exit point
    for (int jmpPos : loopStack_[loopIdx].pendingBreaks) {
        patchJump(jmpPos, exitTarget);
    }

    loopStack_.pop_back();
}

void Codegen::compileReturn(Shared<ReturnStmt> s) {
    // 在执行 return 之前，逆序执行所有 pending 的 defer
    for (auto it = deferStack_.rbegin(); it != deferStack_.rend(); ++it) {
        auto& defers = *it;
        for (int i = static_cast<int>(defers.size()) - 1; i >= 0; i--) {
            compileStmt(defers[i]);
        }
    }

    int retReg = 0;
    if (s->value) {
        retReg = compileExpr(s->value);
    } else {
        retReg = allocReg();
        emit(OP_LOADNIL, retReg, 0, 0);
    }
    emit(OP_RETURN, retReg, 0, 0);
}

void Codegen::compileBreak(Shared<BreakStmt> /*s*/) {
    if (!loopStack_.empty()) {
        int jmpPos = emitJumpPlaceholder(OP_JUMP);
        loopStack_.back().pendingBreaks.push_back(jmpPos);
    }
}

void Codegen::compileContinue(Shared<ContinueStmt> /*s*/) {
    if (!loopStack_.empty()) {
        int jmpPos = emitJumpPlaceholder(OP_JUMP);
        loopStack_.back().pendingContinues.push_back(jmpPos);
    }
}

void Codegen::compileExprStmt(Shared<ExprStmt> s) {
    compileExpr(s->expr);
}

// ═══════════════════════════════════════════════════════════════════
//  defer: 推迟执行
//  将语句加入当前作用域的 defer 栈，离开作用域时逆序执行
//  在 return 之前也会执行所有 pending 的 defer
// ═══════════════════════════════════════════════════════════════════
void Codegen::compileDefer(Shared<DeferStmt> stmt) {
    if (!deferStack_.empty()) {
        deferStack_.back().push_back(stmt->body);
    } else {
        // 没有块上下文（全局作用域），直接编译
        if (stmt->body) compileStmt(stmt->body);
    }
}

void Codegen::emitDeferCleanup() {
    for (auto it = deferStack_.rbegin(); it != deferStack_.rend(); ++it) {
        auto& defers = *it;
        for (int i = static_cast<int>(defers.size()) - 1; i >= 0; i--) {
            compileStmt(defers[i]);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
//  表达式编译
// ═══════════════════════════════════════════════════════════════════

} // namespace cplang
