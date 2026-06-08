// CP语言 代码生成器实现 - 语句编译
#include "codegen/codegen.hpp"
#include <cstdio>

namespace cplang {

void Codegen::compileStmt(Shared<Stmt> stmt) {
    if (!stmt) return;
    
    // 设置当前源码行号（用于错误定位）
    setLine(stmt->token.line);

    if (auto p = std::dynamic_pointer_cast<PackageStmt>(stmt)) { printf("compileStmt: Package\n"); fflush(stdout); compilePackage(p); }
    else if (auto imp = std::dynamic_pointer_cast<ImportStmt>(stmt)) { printf("compileStmt: Import\n"); fflush(stdout); compileImport(imp); }
    else if (auto v = std::dynamic_pointer_cast<VarDeclStmt>(stmt)) { printf("compileStmt: VarDecl\n"); fflush(stdout); compileVarDecl(v); }
    else if (auto func = std::dynamic_pointer_cast<FuncDeclStmt>(stmt)) { printf("compileStmt: FuncDecl\n"); fflush(stdout); compileFuncDecl(func); printf("compileStmt: FuncDecl done\n"); fflush(stdout); }
    else if (auto cls = std::dynamic_pointer_cast<ClassDeclStmt>(stmt)) { printf("compileStmt: ClassDecl\n"); fflush(stdout); compileClassDecl(cls); }
    else if (auto enm = std::dynamic_pointer_cast<EnumDeclStmt>(stmt)) { printf("compileStmt: EnumDecl\n"); fflush(stdout); compileEnumDecl(enm); }
    else if (auto s = std::dynamic_pointer_cast<StructDeclStmt>(stmt)) { printf("compileStmt: StructDecl\n"); fflush(stdout); compileStructDecl(s); }
    else if (auto blk = std::dynamic_pointer_cast<BlockStmt>(stmt)) { compileBlock(blk); }
    else if (auto ifStmt = std::dynamic_pointer_cast<IfStmt>(stmt)) { compileIf(ifStmt); }
    else if (auto forStmt = std::dynamic_pointer_cast<ForStmt>(stmt)) { printf("compileStmt: For\n"); fflush(stdout); compileFor(forStmt); }
    else if (auto fe = std::dynamic_pointer_cast<ForEachStmt>(stmt)) { printf("compileStmt: ForEach\n"); fflush(stdout); compileForEach(fe); }
    else if (auto w = std::dynamic_pointer_cast<WhileStmt>(stmt)) { printf("compileStmt: While\n"); fflush(stdout); compileWhile(w); }
    else if (auto dw = std::dynamic_pointer_cast<DoWhileStmt>(stmt)) { printf("compileStmt: DoWhile\n"); fflush(stdout); compileDoWhile(dw); }
    else if (auto iface = std::dynamic_pointer_cast<InterfaceDeclStmt>(stmt)) { printf("compileStmt: InterfaceDecl\n"); fflush(stdout); compileInterfaceDecl(iface); }
    else if (auto r = std::dynamic_pointer_cast<ReturnStmt>(stmt)) { printf("compileStmt: Return\n"); fflush(stdout); compileReturn(r); }
    else if (auto brk = std::dynamic_pointer_cast<BreakStmt>(stmt)) { printf("compileStmt: Break\n"); fflush(stdout); compileBreak(brk); }
    else if (auto cont = std::dynamic_pointer_cast<ContinueStmt>(stmt)) { printf("compileStmt: Continue\n"); fflush(stdout); compileContinue(cont); }
    else if (auto tr = std::dynamic_pointer_cast<TryStmt>(stmt)) { printf("compileStmt: Try\n"); fflush(stdout); compileTry(tr); }
    else if (auto th = std::dynamic_pointer_cast<ThrowStmt>(stmt)) { printf("compileStmt: Throw\n"); fflush(stdout); compileThrow(th); }
    else if (auto sw = std::dynamic_pointer_cast<SwitchStmt>(stmt)) { printf("compileStmt: Switch\n"); fflush(stdout); compileSwitch(sw); }
    else if (auto matchStmt = std::dynamic_pointer_cast<MatchStmt>(stmt)) { printf("compileStmt: Match\n"); fflush(stdout); compileMatch(matchStmt); }
    else if (auto e = std::dynamic_pointer_cast<ExprStmt>(stmt)) { printf("compileStmt: ExprStmt\n"); fflush(stdout); compileExprStmt(e); }
    else if (auto d = std::dynamic_pointer_cast<DeferStmt>(stmt)) { printf("compileStmt: Defer\n"); fflush(stdout); compileDefer(d); }
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
        printf("DEBUG: compileFuncDecl: compiling body\n"); fflush(stdout);
        compileBlock(stmt->body);
    }

    printf("DEBUG: compileFuncDecl: adding default return\n"); fflush(stdout);

    // 默认返回 nil
    if (func_->code.empty() || func_->code.back() != OP_RETURN) {
        emit(OP_LOADNIL, 0, 0, 0);
        emit(OP_RETURN, 0, 0, 0);
    }

    printf("DEBUG: compileFuncDecl: popScope\n"); fflush(stdout);
    // 弹出函数作用域
    popScope();

    printf("DEBUG: compileFuncDecl: setting properties\n"); fflush(stdout);
    // 设置新函数的属性
    newFunc->maxStack = maxReg_ + 16;
    newFunc->constants = constants_;  // 复制常量表
    newFunc->isTyped = allTyped_;     // 渐进类型标记
    
    printf("DEBUG: compileFuncDecl: restoring context\n"); fflush(stdout);
    lastCompiledFunc_ = func_;
    // 恢复外层编译状态
    func_ = oldFunc;
    code_ = oldCode;
    nextReg_ = oldNextReg;
    constants_ = oldConstants;  // 恢复外层常量表
    
    // 重置 typed 追踪
    allTyped_ = true;

    printf("DEBUG: compileFuncDecl: adding function to constant pool\n"); fflush(stdout);
    // 添加新函数到外层的常量池
    Value funcVal = makeFunctionVal(reinterpret_cast<VMFunction*>(newFunc));
    Int32 funcIdx = addConstant(funcVal);

    printf("DEBUG: compileFuncDecl: creating name string\n"); fflush(stdout);
    // 全局注册函数名
    VMString* nameStr = VMString::create(stmt->name);
    Int32 nameIdx = addConstant(makeStringVal(nameStr));

    printf("DEBUG: compileFuncDecl: emitting LOADCONST\n"); fflush(stdout);
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
    if (stmt->isADT) {
        // ADT 风格枚举：为每个变体创建构造器函数并注册 tag 常量
        if (!vm_) return;

        Int64 tag = 0;
        for (auto& variant : stmt->variants) {
            // 注册变体 tag 常量: EnumName::VariantName = tag
            String tagName = stmt->name + "::" + variant.name;
            vm_->registerGlobal(tagName.c_str(), Value::Int(tag));

            // 创建构造器函数: make_EnumName_VariantName(field1, field2, ...)
            VMFunction* ctor = new VMFunction();
            String ctorName = "make_" + stmt->name + "_" + variant.name;
            ctor->name = VMString::create(ctorName);
            ctor->maxStack = 256;
            ctor->numParams = static_cast<UInt32>(variant.fields.size());
            ctor->numLocals = 0;
            ctor->isVararg = false;
            ctor->hasSlots = (vm_ != nullptr);
            ctor->sourceFile = sourceFile_;

            // 构造器体：创建变体表 {tag, field0, field1, ...}
            // NEWVARIANT ra, tag
            // SETFIELD(value, obj, 1+N) for each field
            int resultReg = 0;
            ctor->code.push_back(OP_NEWVARIANT);
            ctor->code.push_back(static_cast<UInt8>(resultReg));  // a
            ctor->code.push_back(static_cast<UInt8>(tag));         // b = tag
            ctor->code.push_back(0);  // c
            for (int i = 0; i < 12; i++) ctor->code.push_back(0); // padding to 16 bytes

            // 存储每个字段到变体
            for (size_t i = 0; i < variant.fields.size(); i++) {
                // SETFIELD ra, rb, c: rb.fields[c] = ra
                // field 参数在寄存器 0, 1, 2, ...; resultReg 是 resultReg
                // 字段索引从 1 开始（0 是 tag）
                int fieldReg = static_cast<int>(i);  // 参数寄存器
                ctor->code.push_back(OP_SETFIELD);
                ctor->code.push_back(static_cast<UInt8>(fieldReg));  // a = value
                ctor->code.push_back(static_cast<UInt8>(resultReg));  // b = obj
                ctor->code.push_back(static_cast<UInt8>(i + 1));      // c = field index (1-based, 0=tag)
                for (int j = 0; j < 12; j++) ctor->code.push_back(0); // padding
            }

            // RETURN resultReg
            ctor->code.push_back(OP_RETURN);
            ctor->code.push_back(static_cast<UInt8>(resultReg));
            ctor->code.push_back(0);
            ctor->code.push_back(0);
            for (int i = 0; i < 12; i++) ctor->code.push_back(0);

            // 注册构造器函数
            vm_->registerGlobal(ctorName.c_str(), makeFunctionVal(ctor));

            tag++;
        }
    } else {
        // 简单 C 风格枚举（向后兼容）
        if (!vm_) return;
        Int64 val = 0;
        for (auto& [name, init] : stmt->values) {
            if (init.has_value()) {
                val = init.value();
            }
            // 注册无前缀版本（向后兼容）
            vm_->registerGlobal(name.c_str(), Value::Int(val));
            // 也注册带枚举名前缀的版本（支持 颜色.红 语法）
            String prefixed = stmt->name + "::" + name;
            vm_->registerGlobal(prefixed.c_str(), Value::Int(val));
            val++;
        }
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

void Codegen::compileInterfaceDecl(Shared<InterfaceDeclStmt> stmt) {
    // 接口声明：注册类型名，方法签名暂不强制检查
    // 为每个方法创建存根函数
    if (!vm_) return;

    for (auto& method : stmt->methods) {
        if (auto func = std::dynamic_pointer_cast<FuncDeclStmt>(method)) {
            // 注册接口方法为全局函数(存根)
            VMString* nameStr = VMString::create(stmt->name + "_" + func->name);
            Int32 nameIdx = addConstant(makeStringVal(nameStr));
            // 创建一个空函数作为存根
            VMFunction* stub = new VMFunction();
            stub->name = nameStr;
            stub->maxStack = 16;
            stub->numParams = static_cast<UInt32>(func->params.size());
            stub->numLocals = 0;
            stub->isVararg = false;
            stub->hasSlots = (vm_ != nullptr);
            // 存根返回 nil
            stub->code.push_back(OP_LOADNIL);
            stub->code.push_back(0); stub->code.push_back(0); stub->code.push_back(0);
            for (int i = 0; i < 12; i++) stub->code.push_back(0);
            stub->code.push_back(OP_RETURN);
            stub->code.push_back(0); stub->code.push_back(0); stub->code.push_back(0);
            for (int i = 0; i < 12; i++) stub->code.push_back(0);

            Value funcVal = makeFunctionVal(reinterpret_cast<VMFunction*>(stub));
            Int32 funcIdx = addConstant(funcVal);
            int r = allocReg();
            emitInt(OP_LOADCONST, r, funcIdx);
            if (vm_) {
                Int32 slot = vm_->getOrCreateGlobalSlot((stmt->name + "_" + func->name).c_str());
                emitInt(OP_STOREGLOBAL, r, slot);
            }
        }
    }
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

void Codegen::compileDoWhile(Shared<DoWhileStmt> s) {
    size_t loopIdx = loopStack_.size();
    loopStack_.push_back(LoopContext{});
    int loopStart = static_cast<int>(code_->size());
    loopStack_[loopIdx].continueTarget = loopStart;

    // body
    compileStmt(s->body);

    // patch pending continues: jump to condition check
    int continuePos = static_cast<int>(code_->size());
    for (int jmpPos : loopStack_[loopIdx].pendingContinues) {
        patchJump(jmpPos, continuePos);
    }

    // condition
    int condReg = compileExpr(s->condition);
    int loopJump = emitJumpPlaceholder(OP_JUMPIF, static_cast<UInt8>(condReg));
    patchJump(loopJump, loopStart);

    // exit: patch all pending break jumps
    int exitTarget = static_cast<int>(code_->size());
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
// compileMatch moved to codegen_match.cpp
} // namespace cplang
