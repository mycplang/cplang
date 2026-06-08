// CP语言 代码生成器实现 - 表达式编译与常量折叠
#include "codegen/codegen.hpp"
#include <cstdio>

namespace cplang {

int Codegen::compileExpr(Shared<Expr> expr) {
    if (!expr) {
        int r = allocReg();
        emit(OP_LOADNIL, r, 0, 0);
        return r;
    }

    // 字面量
    if (auto lit = std::dynamic_pointer_cast<LiteralExpr>(expr))
        return compileLiteral(lit);

    // 数组字面量
    if (auto arr = std::dynamic_pointer_cast<ArrayExpr>(expr))
        return compileArray(arr);

    // 标识符
    // await / 等待 — 提取异步操作结果（同步实现：直接求值目标）
    if (auto awaitExpr = std::dynamic_pointer_cast<AwaitExpr>(expr)) {
        return compileExpr(awaitExpr->target);
    }

    // this / 这个 — 加载 self（类方法的第一个参数）
    if (auto thisExpr = std::dynamic_pointer_cast<ThisExpr>(expr)) {
        int r = allocReg();
        emit(OP_LOADLOCAL, (UInt8)r, 0, 0);  // self 在局部变量槽位 0
        return r;
    }

    // super / 继承 — 调用父类方法
    if (auto superExpr = std::dynamic_pointer_cast<SuperExpr>(expr)) {
        int baseReg = allocReg();
        for (size_t i = 0; i < superExpr->arguments.size(); i++) {
            int argReg = compileExpr(superExpr->arguments[i]);
            emit(OP_MOVE, (UInt8)(baseReg + 1 + (int)i), (UInt8)argReg, 0);
        }
        // self 在局部变量槽位 0，作为第一个参数
        emit(OP_MOVE, (UInt8)(baseReg + 1), 0, 0);
        emit(OP_CALL, (UInt8)baseReg, (UInt8)(baseReg + 1), (UInt8)(superExpr->arguments.size() + 1));
        return baseReg;
    }

    if (auto id = std::dynamic_pointer_cast<IdentifierExpr>(expr))
        return compileIdentifier(id);

    // 二元表达式
    if (auto bin = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
        // 赋值?
        if (bin->op == TokenType::OP_ASSIGN)
            return compileAssign(bin);
        // 三元运算符
        if (bin->op == TokenType::OP_QUESTION)
            return compileTernary(bin);
        return compileBinary(bin);
    }

    // 一元表达式
    if (auto unary = std::dynamic_pointer_cast<UnaryExpr>(expr))
        return compileUnary(unary);

    // 函数调用
    if (auto call = std::dynamic_pointer_cast<CallExpr>(expr)) {
        // Special-case: import("path") 直接生成 OP_IMPORT
        if (auto calleeId = std::dynamic_pointer_cast<IdentifierExpr>(call->callee)) {
            if (calleeId->name == "import" || calleeId->name == u8"导入") {
                if (call->arguments.size() >= 1) {
                    int argReg = compileExpr(call->arguments[0]);
                    int r = allocReg();
                    emit(OP_IMPORT, r, argReg, 0);
                    return r;
                }
            }
        }
        return compileCall(call);
    }

    // new 表达式
    if (auto n = std::dynamic_pointer_cast<NewExpr>(expr))
        return compileNew(n);

    // 成员访问
    if (auto mem = std::dynamic_pointer_cast<MemberExpr>(expr)) {
        return compileMember(mem);
    }

    // 索引访问
    if (auto idx = std::dynamic_pointer_cast<IndexExpr>(expr))
        return compileIndex(idx);

    // 结构体字面量
    if (auto structLit = std::dynamic_pointer_cast<StructLiteralExpr>(expr))
        return compileStructLiteral(structLit);

    // Lambda表达式
    if (auto lambda = std::dynamic_pointer_cast<LambdaExpr>(expr))
        return compileLambda(lambda);

    // 默认管道表达式
    if (auto pipe = std::dynamic_pointer_cast<PipeExpr>(expr))
        return compilePipe(pipe);

    // 默认
    int r = allocReg();
    emit(OP_LOADNIL, r, 0, 0);
    return r;
}

int Codegen::compileLiteral(Shared<LiteralExpr> expr) {
    int r = allocReg();

    if (auto* vInt = std::get_if<Int64>(&expr->value)) {
        // Write integer value directly to bytecode (not constant index)
        emitInt(OP_LOADINT, r, static_cast<Int32>(*vInt));
    }
    else if (auto* vFloat = std::get_if<Float64>(&expr->value)) {
        Int32 idx = addConstant(Value::Float(*vFloat));
        emitInt(OP_LOADFLT, r, idx);
    }
    else if (auto* vStr = std::get_if<String>(&expr->value)) {
        VMString* str = VMString::create(*vStr);
        Int32 idx = addConstant(makeStringVal(str));
        emitInt(OP_LOADSTR, r, idx);
    }
    else if (auto* vBool = std::get_if<bool>(&expr->value)) {
        emit(OP_LOADBOOL, r, *vBool ? 1 : 0, 0);
    }
    else {
        emit(OP_LOADNIL, r, 0, 0);
    }

    return r;
}

int Codegen::compileArray(Shared<ArrayExpr> expr) {
    // 创建数组
    int arrReg = allocReg();
    int count = static_cast<int>(expr->elements.size());
    emit(OP_NEWARRAY, arrReg, count, 0);

    // 填充元素: SETELEM a=值, b=数组, c=索引
    for (int i = 0; i < count; i++) {
        int elemReg = compileExpr(expr->elements[i]);
        int idxReg = allocReg();
        emitInt(OP_LOADINT, idxReg, i);  // 直接写入索引值
        emit(OP_SETELEM, elemReg, arrReg, idxReg);
    }

    return arrReg;
}

int Codegen::compileIdentifier(Shared<IdentifierExpr> expr) {
    // 查找局部变量
    int reg = getLocalReg(expr->name);
    if (reg >= 0) {
        int r = allocReg();
        emit(OP_MOVE, r, reg, 0);
        return r;
    }

    // 查找全局 - 使用Slot化优化
    int r = allocReg();
    if (vm_) {
        Int32 slot = vm_->getOrCreateGlobalSlot(expr->name.c_str());
        emitInt(OP_LOADGLOBAL, r, slot);
    } else {
        // Fallback: use legacy string-based instruction
        VMString* name = VMString::create(expr->name);
        Int32 idx = addConstant(makeStringVal(name));
        emitInt(OP_LOADGLOBAL, r, idx);
    }
    return r;
}

int Codegen::compileBinary(Shared<BinaryExpr> expr) {
    // Short-circuit logical AND (&&)
    if (expr->op == TokenType::OP_AND) {
        int left = compileExpr(expr->left);
        int ra = allocReg();
        emit(OP_MOVE, ra, left, 0);  // result = left (default if falsy)
        int skipJump = emitJumpPlaceholder(OP_JUMPNIF, static_cast<UInt8>(left));
        int right = compileExpr(expr->right);
        emit(OP_MOVE, ra, right, 0);  // result = right (if left was truthy)
        patchJump(skipJump, static_cast<int>(code_->size()));
        return ra;
    }

    // Short-circuit logical OR (||)
    if (expr->op == TokenType::OP_OR) {
        int left = compileExpr(expr->left);
        int ra = allocReg();
        emit(OP_MOVE, ra, left, 0);  // result = left (default if truthy)
        int skipJump = emitJumpPlaceholder(OP_JUMPIF, static_cast<UInt8>(left));
        int right = compileExpr(expr->right);
        emit(OP_MOVE, ra, right, 0);  // result = right (if left was falsy)
        patchJump(skipJump, static_cast<int>(code_->size()));
        return ra;
    }

    // Compound assignment: += -= *= /= %=
    if (expr->op == TokenType::OP_PLUS_ASSIGN || expr->op == TokenType::OP_MINUS_ASSIGN ||
        expr->op == TokenType::OP_MUL_ASSIGN  || expr->op == TokenType::OP_DIV_ASSIGN ||
        expr->op == TokenType::OP_MOD_ASSIGN) {
        int rb = compileExpr(expr->left);
        int rc = compileExpr(expr->right);
        int ra = allocReg();
        // Map compound op to binary op
        TokenType binOp;
        switch (expr->op) {
            case TokenType::OP_PLUS_ASSIGN: binOp = TokenType::OP_PLUS; break;
            case TokenType::OP_MINUS_ASSIGN:binOp = TokenType::OP_MINUS;break;
            case TokenType::OP_MUL_ASSIGN:  binOp = TokenType::OP_MUL;  break;
            case TokenType::OP_DIV_ASSIGN:  binOp = TokenType::OP_DIV;  break;
            case TokenType::OP_MOD_ASSIGN:  binOp = TokenType::OP_MOD;  break;
            default: binOp = TokenType::OP_PLUS; break;
        }
        compileBinaryOp(binOp, ra, rb, rc);
        // Store back to left-hand side
        if (auto id = std::dynamic_pointer_cast<IdentifierExpr>(expr->left)) {
            int lr = getLocalReg(id->name);
            if (lr >= 0) {
                emit(OP_MOVE, lr, ra, 0);
            } else if (vm_) {
                Int32 slot = vm_->getOrCreateGlobalSlot(id->name.c_str());
                emitInt(OP_STOREGLOBAL, ra, slot);
            } else {
                VMString* name = VMString::create(id->name);
                Int32 idx = addConstant(makeStringVal(name));
                emitInt(OP_STOREGLOBAL, ra, idx);
            }
        }
        return ra;
    }

    // Regular binary operators
    int rb = compileExpr(expr->left);
    int rc = compileExpr(expr->right);
    int ra = allocReg();

    // 类型推断优化：已知类型时使用类型化指令
    if (analyzer_) {
        Type* lt = analyzer_->getExprType(expr->left);
        Type* rt = analyzer_->getExprType(expr->right);
        if (lt && rt && lt->kind != BuiltinType::UNKNOWN && rt->kind != BuiltinType::UNKNOWN) {
            if (lt->kind == BuiltinType::INT && rt->kind == BuiltinType::INT) {
                switch (expr->op) {
                    case TokenType::OP_PLUS:  emit(OP_IADD, ra, rb, rc); return ra;
                    case TokenType::OP_MINUS: emit(OP_ISUB, ra, rb, rc); return ra;
                    case TokenType::OP_MUL:   emit(OP_IMUL, ra, rb, rc); return ra;
                    case TokenType::OP_DIV:   emit(OP_IDIV2, ra, rb, rc); return ra;
                    case TokenType::OP_MOD:   emit(OP_IMOD, ra, rb, rc); return ra;
                    case TokenType::OP_EQ:    emit(OP_ICMPEQ, ra, rb, rc); return ra;
                    case TokenType::OP_NE:    emit(OP_ICMPNE, ra, rb, rc); return ra;
                    case TokenType::OP_LT:    emit(OP_ICMPLT, ra, rb, rc); return ra;
                    case TokenType::OP_LE:    emit(OP_ICMPLE, ra, rb, rc); return ra;
                    case TokenType::OP_GT:    emit(OP_ICMPGT, ra, rb, rc); return ra;
                    case TokenType::OP_GE:    emit(OP_ICMPGE, ra, rb, rc); return ra;
                    default: break;
                }
            }
            else if (lt->kind == BuiltinType::FLOAT || rt->kind == BuiltinType::FLOAT) {
                switch (expr->op) {
                    case TokenType::OP_PLUS:  emit(OP_FADD, ra, rb, rc); return ra;
                    case TokenType::OP_MINUS: emit(OP_FSUB, ra, rb, rc); return ra;
                    case TokenType::OP_MUL:   emit(OP_FMUL, ra, rb, rc); return ra;
                    case TokenType::OP_DIV:   emit(OP_FDIV, ra, rb, rc); return ra;
                    case TokenType::OP_EQ:    emit(OP_FCMPEQ, ra, rb, rc); return ra;
                    case TokenType::OP_NE:    emit(OP_FCMPNE, ra, rb, rc); return ra;
                    case TokenType::OP_LT:    emit(OP_FCMPLT, ra, rb, rc); return ra;
                    case TokenType::OP_LE:    emit(OP_FCMPLE, ra, rb, rc); return ra;
                    case TokenType::OP_GT:    emit(OP_FCMPGT, ra, rb, rc); return ra;
                    case TokenType::OP_GE:    emit(OP_FCMPGE, ra, rb, rc); return ra;
                    default: break;
                }
            }
        }
    }

    return compileBinaryOp(expr->op, ra, rb, rc);
}

int Codegen::compileNew(Shared<NewExpr> n) {
    // 创建实例：调用 table() 建空表，然后设字段
    // 1. 调用 table() 创建空表
    int tblReg = allocReg();
    if (vm_) {
        Int32 slot = vm_->getOrCreateGlobalSlot("table");
        emitInt(OP_LOADGLOBAL, tblReg, slot);
    } else {
        VMString* name = VMString::create("table");
        Int32 idx = addConstant(makeStringVal(name));
        emitInt(OP_LOADGLOBAL, tblReg, idx);
    }
    emit(OP_CALL, tblReg, tblReg, 0);  // tblReg = table()
    
    // 2. 设 _class 字段
    VMString* classField = VMString::create("_class");
    Int32 classIdx = addConstant(makeStringVal(classField));
    int keyReg = allocReg();
    emitInt(OP_LOADSTR, keyReg, classIdx);
    VMString* cname = VMString::create(n->className);
    Int32 cnameIdx = addConstant(makeStringVal(cname));
    int valReg = allocReg();
    emitInt(OP_LOADSTR, valReg, cnameIdx);
    emit(OP_SETIDX, valReg, tblReg, keyReg);  // tblReg._class = className
    
    // 3. 按字段顺序设置构造参数
    auto it = classMeta_.find(n->className);
    if (it != classMeta_.end()) {
        for (size_t i = 0; i < it->second.fieldNames.size() && i < n->args.size(); i++) {
            VMString* fname = VMString::create(it->second.fieldNames[i]);
            Int32 fnameIdx = addConstant(makeStringVal(fname));
            int fkeyReg = allocReg();
            emitInt(OP_LOADSTR, fkeyReg, fnameIdx);
            int fvalReg = compileExpr(n->args[i]);
            emit(OP_SETIDX, fvalReg, tblReg, fkeyReg);  // tblReg.fieldName = argValue
        }
    }
    
    return tblReg;
}

int Codegen::compilePipe(Shared<PipeExpr> expr) {
    // a |> f → f(a)
    // First compile the function (right side) to get the callee register
    int funcReg = compileExpr(expr->right);

    // The argument must be in funcReg + 1 per calling convention
    nextReg_ = funcReg + 1;
    int argReg = compileExpr(expr->left);
    if (argReg != funcReg + 1) {
        emit(OP_MOVE, funcReg + 1, argReg, 0);
    }

    // Call: result in funcReg, function at funcReg, 1 argument
    emit(OP_CALL, funcReg, funcReg, 1);
    return funcReg;
}

int Codegen::compileLambda(Shared<LambdaExpr> expr) {
    // Lambda 编译策略:
    // 1. 将 lambda 体编译为独立的 VMFunction
    // 2. 将该 VMFunction 添加到常量池
    // 3. 生成 OP_MAKECLOSURE 指令在运行时创建闭包并捕获变量
    
    // 保存当前编译状态
    VMFunction* oldFunc = func_;
    std::vector<UInt8>* oldCode = code_;
    int oldNextReg = nextReg_;
    std::vector<Value> oldConstants = constants_;
    bool oldAllTyped = allTyped_;
    
    // 清空常量表，为 lambda 函数准备
    constants_.clear();
    
    // 创建 lambda 的 VMFunction
    VMFunction* lambdaFunc = new VMFunction();
    lambdaFunc->name = nullptr;  // 匿名函数
    lambdaFunc->maxStack = 256;
    lambdaFunc->numParams = static_cast<UInt32>(expr->params.size());
    lambdaFunc->numLocals = 0;
    lambdaFunc->isVararg = false;
    lambdaFunc->hasSlots = (vm_ != nullptr);
    lambdaFunc->sourceFile = sourceFile_;
    
    // 切换到 lambda 函数的编译上下文
    func_ = lambdaFunc;
    code_ = &lambdaFunc->code;
    nextReg_ = 0;
    maxReg_ = 0;
    allTyped_ = true;
    
    // 创建新作用域
    pushScope();
    
    // 注册参数到局部变量
    for (size_t p = 0; p < expr->params.size(); p++) {
        LocalVar lv{static_cast<int>(p), nullptr};
        localScopes_.back()[expr->params[p].first] = lv;
        if (static_cast<int>(p) >= nextReg_) nextReg_ = static_cast<int>(p) + 1;
    }
    
    // 编译 lambda 体
    if (expr->body) {
        compileBlock(expr->body);
    }
    
    // 确保有返回指令
    if (lambdaFunc->code.empty() || lambdaFunc->code[lambdaFunc->code.size() - 16] != OP_RETURN) {
        emit(OP_LOADNIL, 0, 0, 0);
        emit(OP_RETURN, 0, 0, 0);
    }
    
    // 弹出 lambda 作用域
    popScope();
    
    // 设置 lambda 函数属性
    lambdaFunc->maxStack = maxReg_ + 16;
    lambdaFunc->constants = constants_;
    lambdaFunc->isTyped = allTyped_;
    
    // 恢复外层编译状态
    func_ = oldFunc;
    code_ = oldCode;
    nextReg_ = oldNextReg;
    constants_ = oldConstants;
    allTyped_ = oldAllTyped;
    
    // 将 lambda 的 VMFunction 添加到外层常量池
    Value funcVal = makeFunctionVal(lambdaFunc);
    Int32 funcIdx = addConstant(funcVal);
    
    // 收集需要捕获的变量
    // 简化实现：扫描 lambda 体中使用的外部局部变量
    // 通过对比 lambda 作用域内的变量和外层作用域的变量来识别
    std::vector<String> captureNames;
    
    // 使用 expr->captures 如果语义分析器已经填充
    if (!expr->captures.empty()) {
        captureNames = expr->captures;
    }
    // 否则，codegen 自动检测：检查外层作用域中在 lambda 体内被引用的变量
    // 简化：暂时依赖语义分析器填充 captures
    
    // 分配结果寄存器
    int resultReg = allocReg();
    
    if (captureNames.empty()) {
        // 无捕获变量：直接创建闭包（仍然需要 OP_MAKECLOSURE 以包装为闭包对象）
        // OP_MAKECLOSURE: a=result, b=funcConstIdx(low8), c=captureCount
        emit(OP_MAKECLOSURE, static_cast<UInt8>(resultReg),
             static_cast<UInt8>(funcIdx & 0xFF), 0);
    } else {
        // 有捕获变量：先加载每个捕获变量到连续寄存器，然后创建闭包
        int captureBaseReg = resultReg + 1;
        for (size_t i = 0; i < captureNames.size(); i++) {
            int capReg = captureBaseReg + static_cast<int>(i);
            int localReg = getLocalReg(captureNames[i]);
            if (localReg >= 0) {
                emit(OP_MOVE, capReg, localReg, 0);
            } else {
                // 捕获变量未找到，加载 nil 作为占位
                emit(OP_LOADNIL, capReg, 0, 0);
            }
        }
        // OP_MAKECLOSURE: a=result, b=funcConstIdx(low8), c=captureCount
        emit(OP_MAKECLOSURE, static_cast<UInt8>(resultReg),
             static_cast<UInt8>(funcIdx & 0xFF),
             static_cast<UInt8>(captureNames.size()));
    }
    
    return resultReg;
}

int Codegen::compileBinaryOp(TokenType op, int ra, int rb, int rc) {
    switch (op) {
        case TokenType::OP_PLUS:      emit(OP_ADD, ra, rb, rc); break;
        case TokenType::OP_MINUS:     emit(OP_SUB, ra, rb, rc); break;
        case TokenType::OP_MUL:       emit(OP_MUL, ra, rb, rc); break;
        case TokenType::OP_DIV:       emit(OP_DIV, ra, rb, rc); break;
        case TokenType::OP_MOD:       emit(OP_MOD, ra, rb, rc); break;
        case TokenType::OP_LSHIFT:  emit(OP_BSHL, ra, rb, rc); break;
        case TokenType::OP_RSHIFT:  emit(OP_BSHR, ra, rb, rc); break;
        case TokenType::OP_EQ:        emit(OP_CMPEQ, ra, rb, rc); break;
        case TokenType::OP_NE:        emit(OP_CMPNE, ra, rb, rc); break;
        case TokenType::OP_LT:        emit(OP_CMPLT, ra, rb, rc); break;
        case TokenType::OP_LE:        emit(OP_CMPLE, ra, rb, rc); break;
        case TokenType::OP_GT:        emit(OP_CMPGT, ra, rb, rc); break;
        case TokenType::OP_GE:        emit(OP_CMPGE, ra, rb, rc); break;
        default: emit(OP_MOVE, ra, rb, 0);
    }
    return ra;
}

int Codegen::compileAssign(Shared<BinaryExpr> expr) {
    int valueReg = compileExpr(expr->right);

    if (auto id = std::dynamic_pointer_cast<IdentifierExpr>(expr->left)) {
        int localReg = getLocalReg(id->name);
        if (localReg >= 0) {
            emit(OP_MOVE, localReg, valueReg, 0);
            return localReg;
        }
        // 全局 - 使用Slot化优化
        if (vm_) {
            Int32 slot = vm_->getOrCreateGlobalSlot(id->name.c_str());
            emitInt(OP_STOREGLOBAL, valueReg, slot);
        } else {
            VMString* name = VMString::create(id->name);
            Int32 idx = addConstant(makeStringVal(name));
            emitInt(OP_STOREGLOBAL, valueReg, idx);
        }
        return valueReg;
    }

    // 数组索引赋值: arr[idx] = value
    if (auto idxExpr = std::dynamic_pointer_cast<IndexExpr>(expr->left)) {
        int arrReg = compileExpr(idxExpr->array);
        int indexReg = compileExpr(idxExpr->index);
        // SETELEM: a=值, b=数组, c=索引
        emit(OP_SETELEM, valueReg, arrReg, indexReg);
        return valueReg;
    }

    // 成员赋值: obj.member = value
    if (auto member = std::dynamic_pointer_cast<MemberExpr>(expr->left)) {
        int objReg = compileExpr(member->object);

        // 首先尝试通过结构体信息找字段偏移
        if (analyzer_) {
            Type* objType = analyzer_->getExprType(member->object);
            if (objType && objType->kind == BuiltinType::STRUCT) {
                StructInfo* structInfo = analyzer_->getStructInfo(objType->name);
                if (structInfo) {
                    StructField* field = structInfo->findField(member->member);
                    if (field) {
                        emit(OP_SETFIELD, valueReg, objReg,
                             static_cast<UInt8>(field->offset / 8));
                        return valueReg;
                    }
                }
            }
        }

        // 通用字符串键赋值: obj[fieldStr] = value
        VMString* fieldName = VMString::create(member->member);
        Int32 nameIdx = addConstant(makeStringVal(fieldName));
        int nameReg = allocReg();
        emitInt(OP_LOADSTR, nameReg, nameIdx);
        emit(OP_SETIDX, valueReg, objReg, nameReg);
        return valueReg;
    }

    return valueReg;
}

int Codegen::compileTernary(Shared<BinaryExpr> expr) {
    // AST structure: result = { left: cond, right: { left: thenExpr, right: elseExpr } }
    int condReg = compileExpr(expr->left);
    auto ternary = std::dynamic_pointer_cast<BinaryExpr>(expr->right);

    int elseJump = emitJumpPlaceholder(OP_JUMPNIF, static_cast<UInt8>(condReg));
    int resultReg = allocReg();

    // Then branch
    int thenReg = compileExpr(ternary->left);
    emit(OP_MOVE, resultReg, thenReg, 0);

    int endJump = emitJumpPlaceholder(OP_JUMP);
    patchJump(elseJump, static_cast<int>(code_->size()));

    // Else branch
    int elseReg = compileExpr(ternary->right);
    emit(OP_MOVE, resultReg, elseReg, 0);

    patchJump(endJump, static_cast<int>(code_->size()));
    return resultReg;
}

int Codegen::compileUnary(Shared<UnaryExpr> expr) {
    int rb = compileExpr(expr->operand);
    int ra = allocReg();

    switch (expr->op) {
        case TokenType::OP_MINUS:
            emit(OP_NEG, ra, rb, 0);
            break;
        case TokenType::OP_NOT:
            emit(OP_NOT, ra, rb, 0);
            break;
        case TokenType::OP_INC:
        {
            // ++x / x++: ra = rb + 1, store back to variable
            int oneReg = allocReg();
            emitInt(OP_LOADINT, oneReg, 1);
            emit(OP_ADD, ra, rb, oneReg);
            // Store back
            if (auto id = std::dynamic_pointer_cast<IdentifierExpr>(expr->operand)) {
                int lr = getLocalReg(id->name);
                if (lr >= 0) {
                    emit(OP_MOVE, lr, ra, 0);
                } else if (vm_) {
                    Int32 slot = vm_->getOrCreateGlobalSlot(id->name.c_str());
                    emitInt(OP_STOREGLOBAL, ra, slot);
                } else {
                    VMString* name = VMString::create(id->name);
                    Int32 idx = addConstant(makeStringVal(name));
                    emitInt(OP_STOREGLOBAL, ra, idx);
                }
            }
            // Postfix: return old value, not new value
            if (expr->isPostfix) {
                emit(OP_MOVE, ra, rb, 0);
            }
            break;
        }
        case TokenType::OP_DEC:
        {
            // --x / x--: ra = rb - 1, store back to variable
            int oneReg = allocReg();
            emitInt(OP_LOADINT, oneReg, 1);
            emit(OP_SUB, ra, rb, oneReg);
            if (auto id = std::dynamic_pointer_cast<IdentifierExpr>(expr->operand)) {
                int lr = getLocalReg(id->name);
                if (lr >= 0) {
                    emit(OP_MOVE, lr, ra, 0);
                } else if (vm_) {
                    Int32 slot = vm_->getOrCreateGlobalSlot(id->name.c_str());
                    emitInt(OP_STOREGLOBAL, ra, slot);
                } else {
                    VMString* name = VMString::create(id->name);
                    Int32 idx = addConstant(makeStringVal(name));
                    emitInt(OP_STOREGLOBAL, ra, idx);
                }
            }
            // Postfix: return old value, not new value
            if (expr->isPostfix) {
                emit(OP_MOVE, ra, rb, 0);
            }
            break;
        }
        default:
            emit(OP_MOVE, ra, rb, 0);
    }

    return ra;
}

int Codegen::compileCall(Shared<CallExpr> expr) {
    // ════════════════════════════════════════════════════════
    //  泛型调用处理: 使用 mangled name 查找实例化版本
    // ════════════════════════════════════════════════════════
    if (!expr->typeArgs.empty()) {
        if (auto id = std::dynamic_pointer_cast<IdentifierExpr>(expr->callee)) {
            // 构建 mangled name (与语义分析器一致)
            String mangled = id->name;
            for (const auto& arg : expr->typeArgs) {
                String canonical = arg;
                if (arg == "int" || arg == "Int" || arg == "整数" || arg == "i64") canonical = "i64";
                else if (arg == "i32" || arg == "int32") canonical = "i32";
                else if (arg == "i16" || arg == "int16") canonical = "i16";
                else if (arg == "i8"  || arg == "int8")  canonical = "i8";
                else if (arg == "float" || arg == "Float" || arg == "浮点" || arg == "f64") canonical = "f64";
                else if (arg == "f32" || arg == "float32") canonical = "f32";
                else if (arg == "bool" || arg == "Bool" || arg == "布尔") canonical = "bool";
                else if (arg == "string" || arg == "String" || arg == "字符串") canonical = "string";
                else if (arg == "char" || arg == "Char" || arg == "字符") canonical = "char";
                mangled += "$" + canonical;
            }

            // 加载 mangled 函数
            int calleeReg = allocReg();
            if (vm_) {
                Int32 slot = vm_->getOrCreateGlobalSlot(mangled.c_str());
                emitInt(OP_LOADGLOBAL, calleeReg, slot);
            } else {
                VMString* name = VMString::create(mangled);
                Int32 idx = addConstant(makeStringVal(name));
                emitInt(OP_LOADGLOBAL, calleeReg, idx);
            }

            // 参数
            nextReg_ = calleeReg + 1;
            for (size_t i = 0; i < expr->arguments.size(); i++) {
                int expectedSlot = calleeReg + 1 + static_cast<int>(i);
                int argReg = compileExpr(expr->arguments[i]);
                if (argReg != expectedSlot) {
                    emit(OP_MOVE, expectedSlot, argReg, 0);
                }
            }

            emit(OP_CALL, calleeReg, calleeReg, static_cast<UInt8>(expr->arguments.size()));
            return calleeReg;
        }
    }

    // ════════════════════════════════════════════════════════
    //  普通函数调用
    // ════════════════════════════════════════════════════════
    int calleeReg = compileExpr(expr->callee);

    // Force argument registers to be consecutive starting right after callee
    nextReg_ = calleeReg + 1;

    // arguments (each compileExpr will allocReg from calleeReg+1, +2, ...)
    // If the result register doesn't match the expected slot, emit OP_MOVE
    for (size_t i = 0; i < expr->arguments.size(); i++) {
        int expectedSlot = calleeReg + 1 + static_cast<int>(i);
        int argReg = compileExpr(expr->arguments[i]);
        if (argReg != expectedSlot) {
            emit(OP_MOVE, expectedSlot, argReg, 0);
        }
    }

    if (std::dynamic_pointer_cast<MemberExpr>(expr->callee) && lastObjReg_ >= 0) {
        emit(OP_CALLMETHOD, calleeReg, lastObjReg_, static_cast<UInt8>(expr->arguments.size()));
    } else {
        emit(OP_CALL, calleeReg, calleeReg, static_cast<UInt8>(expr->arguments.size()));
    }
    // Return value is in calleeReg
    return calleeReg;
}

int Codegen::compileMember(Shared<MemberExpr> expr) {
    int objReg = compileExpr(expr->object);
    lastObjReg_ = objReg;
    int ra = allocReg();

    // 查找对象类型信息
    Type* objType = nullptr;
    if (analyzer_) {
        // 尝试从表达式推断类型
        objType = analyzer_->getExprType(expr->object);
    }

    // 处理数组/字符串的 length 属性
    if (expr->member == "length") {
        if (objType && (objType->kind == BuiltinType::ARRAY ||
                       objType->kind == BuiltinType::STRING)) {
            emit(OP_GETLEN, ra, objReg, 0);
            return ra;
        }
    }

    // 处理结构体成员访问
    if (objType && objType->kind == BuiltinType::STRUCT) {
        StructInfo* structInfo = analyzer_->getStructInfo(objType->name);
        if (structInfo) {
            StructField* field = structInfo->findField(expr->member);
            if (field) {
                // 使用字段偏移访问结构体成员
                emit(OP_GETFIELD, ra, objReg, static_cast<UInt8>(field->offset / 8));
                return ra;
            }
        }
    }

    // 处理类成员访问
    if (objType && objType->kind == BuiltinType::OBJECT) {
        ClassInfo* classInfo = analyzer_->getClassInfo(objType->name);
        if (classInfo) {
            auto it = classInfo->fieldTable.find(expr->member);
            if (it != classInfo->fieldTable.end()) {
                // GETFIELD 访问类字段
                emit(OP_GETFIELD, ra, objReg, 0);
                return ra;
            }
        }
    }

    // 通用对象属性访问(使用字符串键)
    VMString* fieldName = VMString::create(expr->member);
    Int32 nameIdx = addConstant(makeStringVal(fieldName));
    int nameReg = allocReg();
    emitInt(OP_LOADSTR, nameReg, nameIdx);
    emit(OP_GETIDX, ra, objReg, nameReg);
    return ra;
}

int Codegen::compileIndex(Shared<IndexExpr> expr) {
    int arrReg = compileExpr(expr->array);
    int idxReg = compileExpr(expr->index);
    int ra = allocReg();
    emit(OP_GETELEM, ra, arrReg, idxReg);
    return ra;
}

int Codegen::compileStructLiteral(Shared<StructLiteralExpr> expr) {
    // 查找结构体信息
    if (!analyzer_) {
        return compileArray(Shared<ArrayExpr>(new ArrayExpr())); // 降级为数组
    }

    // 匿名表字面量: {} 或 {key: val, ...}
    if (expr->structName.empty() && !analyzer_->getStructInfo(expr->structName)) {
        int fieldCount = static_cast<int>(expr->fields.size());
        int structReg = allocReg();
        emit(OP_NEWSTRUCT, structReg, fieldCount, 0);

        // 按字段顺序填充值（匿名表字段顺序即定义顺序）
        for (int i = 0; i < fieldCount; i++) {
            int valueReg = compileExpr(expr->fields[i].second);
            // 使用字符串键存储，支持点号访问
            VMString* fieldNameStr = VMString::create(expr->fields[i].first);
            Int32 nameIdx = addConstant(makeStringVal(fieldNameStr));
            int nameReg = allocReg();
            emitInt(OP_LOADSTR, nameReg, nameIdx);
            emit(OP_SETIDX, valueReg, structReg, nameReg);
        }
        return structReg;
    }

    StructInfo* info = analyzer_->getStructInfo(expr->structName);
    if (!info) {
        // 结构体未定义，降级为空数组
        return compileArray(Shared<ArrayExpr>(new ArrayExpr()));
    }

    // 创建结构体实例（使用Table存储字段）
    int structReg = allocReg();
    int fieldCount = static_cast<int>(info->fields.size());
    emit(OP_NEWSTRUCT, structReg, fieldCount, 0);

    // 按字段顺序填充值
    for (int i = 0; i < fieldCount; i++) {
        const String& fieldName = info->fields[i].name;

        // 查找初始化表达式
        Shared<Expr> initExpr;
        for (auto& f : expr->fields) {
            if (f.first == fieldName) {
                initExpr = f.second;
                break;
            }
        }

        // 编译字段值(未提供的字段使用nil)
        int valueReg;
        if (initExpr) {
            valueReg = compileExpr(initExpr);
        } else {
            valueReg = allocReg();
            emit(OP_LOADNIL, valueReg, 0, 0);
        }

        // 设置字段: SETFIELD valueReg, structReg, fieldIndex
        emit(OP_SETFIELD, valueReg, structReg, static_cast<UInt8>(i));
        
        // 同时用字符串键存储，支持动态/未知类型访问
        VMString* fieldNameStr = VMString::create(fieldName);
        Int32 nameIdx = addConstant(makeStringVal(fieldNameStr));
        int nameReg = allocReg();
        emitInt(OP_LOADSTR, nameReg, nameIdx);
        emit(OP_SETIDX, valueReg, structReg, nameReg);
    }

    return structReg;
}

// ═══════════════════════════════════════════════════════════════════
//  常量折叠
// ═══════════════════════════════════════════════════════════════════

bool Codegen::canFold(Shared<Expr> expr) const {
    // 检查表达式是否可以常量折叠
    if (auto lit = std::dynamic_pointer_cast<LiteralExpr>(expr)) {
        return true;
    }
    if (auto bin = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
        return canFold(bin->left) && canFold(bin->right);
    }
    if (auto unary = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
        return canFold(unary->operand);
    }
    return false;
}

Value Codegen::foldConstant(Shared<Expr> expr) const {
    // 常量折叠求值
    if (auto lit = std::dynamic_pointer_cast<LiteralExpr>(expr)) {
        if (std::holds_alternative<Int64>(lit->value)) {
            return Value::Int(std::get<Int64>(lit->value));
        }
        if (std::holds_alternative<Float64>(lit->value)) {
            return Value::Float(std::get<Float64>(lit->value));
        }
        if (std::holds_alternative<bool>(lit->value)) {
            return Value::Bool(std::get<bool>(lit->value));
        }
        if (std::holds_alternative<String>(lit->value)) {
            VMString* str = VMString::create(std::get<String>(lit->value));
            return makeStringVal(str);
        }
        return Value::nil();
    }

    if (auto bin = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
        Value left = foldConstant(bin->left);
        Value right = foldConstant(bin->right);

        // 执行二元运算
        if (left.isInt() && right.isInt()) {
            Int64 li = left.asInt();
            Int64 ri = right.asInt();
            switch (bin->op) {
                case TokenType::OP_PLUS:  return Value::Int(li + ri);
                case TokenType::OP_MINUS: return Value::Int(li - ri);
                case TokenType::OP_MUL:   return Value::Int(li * ri);
                case TokenType::OP_DIV:   return ri != 0 ? Value::Int(li / ri) : Value::nil();
                case TokenType::OP_MOD:   return ri != 0 ? Value::Int(li % ri) : Value::nil();
                case TokenType::OP_EQ:    return Value::Bool(li == ri);
                case TokenType::OP_NE:    return Value::Bool(li != ri);
                case TokenType::OP_LT:    return Value::Bool(li < ri);
                case TokenType::OP_LE:    return Value::Bool(li <= ri);
                case TokenType::OP_GT:    return Value::Bool(li > ri);
                case TokenType::OP_GE:    return Value::Bool(li >= ri);
                default: return Value::nil();
            }
        }
    }

    if (auto unary = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
        Value operand = foldConstant(unary->operand);
        if (operand.isInt()) {
            Int64 v = operand.asInt();
            switch (unary->op) {
                case TokenType::OP_MINUS: return Value::Int(-v);
                case TokenType::OP_NOT:   return Value::Bool(v == 0);
                default: return Value::nil();
            }
        }
    }

    return Value::nil();
}

// ═══════════════════════════════════════════════════════════════════
//  错误处理
// ═══════════════════════════════════════════════════════════════════

void Codegen::reportError(const String& msg) {
    if (!hasError_) {
        hasError_ = true;
        errorMsg_ = msg;
    }
}

// ═══════════════════════════════════════════════════════════════════
//  Compiler 整合入口
// ═══════════════════════════════════════════════════════════════════

} // namespace cplang
