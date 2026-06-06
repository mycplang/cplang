// CP语言 语义分析器实现
#include "semantic/semantic_analyzer.hpp"
#include <iostream>

namespace cplang {

// === 构造函数 ===
SemanticAnalyzer::SemanticAnalyzer(Lexer* lexer)
    : lexer_(lexer), hasError_(false) {
    // 初始化全局作用域
    pushScope("global");
    
    // 注册内置类型到符号表
    auto voidSym = new Symbol();
    voidSym->kind = Symbol::TYPE_ALIAS;
    voidSym->name = "void";
    voidSym->type = Type::void_();
    defineSymbol(voidSym);
    
    auto intSym = new Symbol();
    intSym->kind = Symbol::TYPE_ALIAS;
    intSym->name = "int";
    intSym->type = Type::int_();
    defineSymbol(intSym);
    
    auto i8Sym = new Symbol();
    i8Sym->kind = Symbol::TYPE_ALIAS;
    i8Sym->name = "i8";
    i8Sym->type = Type::int8_();
    defineSymbol(i8Sym);
    
    auto i16Sym = new Symbol();
    i16Sym->kind = Symbol::TYPE_ALIAS;
    i16Sym->name = "i16";
    i16Sym->type = Type::int16_();
    defineSymbol(i16Sym);
    
    auto i32Sym = new Symbol();
    i32Sym->kind = Symbol::TYPE_ALIAS;
    i32Sym->name = "i32";
    i32Sym->type = Type::int32_();
    defineSymbol(i32Sym);
    
    auto i64Sym = new Symbol();
    i64Sym->kind = Symbol::TYPE_ALIAS;
    i64Sym->name = "i64";
    i64Sym->type = Type::int64_();
    defineSymbol(i64Sym);
    
    auto floatSym = new Symbol();
    floatSym->kind = Symbol::TYPE_ALIAS;
    floatSym->name = "float";
    floatSym->type = Type::float_();
    defineSymbol(floatSym);
    
    auto f32Sym = new Symbol();
    f32Sym->kind = Symbol::TYPE_ALIAS;
    f32Sym->name = "f32";
    f32Sym->type = Type::float32_();
    defineSymbol(f32Sym);
    
    auto f64Sym = new Symbol();
    f64Sym->kind = Symbol::TYPE_ALIAS;
    f64Sym->name = "f64";
    f64Sym->type = Type::float64_();
    defineSymbol(f64Sym);
    
    auto boolSym = new Symbol();
    boolSym->kind = Symbol::TYPE_ALIAS;
    boolSym->name = "bool";
    boolSym->type = Type::bool_();
    defineSymbol(boolSym);
    
    auto strSym = new Symbol();
    strSym->kind = Symbol::TYPE_ALIAS;
    strSym->name = "string";
    strSym->type = Type::string_();
    defineSymbol(strSym);
    
    auto charSym = new Symbol();
    charSym->kind = Symbol::TYPE_ALIAS;
    charSym->name = "char";
    charSym->type = Type::char_();
    defineSymbol(charSym);
}

SemanticAnalyzer::~SemanticAnalyzer() {
    while (!scopeStack_.empty()) {
        popScope();
    }
}

// === 主入口 ===
bool SemanticAnalyzer::analyze(Shared<Program> program) {
    program_ = program;
    
    // === PASS 1: 收集声明 ===
    // 处理 package
    if (program->package.has_value()) {
        // package 只用于组织代码，不影响语义
    }
    
    // 预扫描：收集所有顶级声明（类、函数、枚举、接口）
    for (auto& stmt : program->statements) {
        if (auto func = std::dynamic_pointer_cast<FuncDeclStmt>(stmt)) {
            auto sym = new Symbol();
            sym->kind = Symbol::FUNC;
            sym->name = func->name;
            sym->node = func;
            sym->isStatic = func->isStatic;
            sym->returnType = func->returnType.has_value() 
                ? getTypeFromString(*func->returnType) 
                : Type::void_();
            sym->isVariadic = false;
            defineSymbol(sym);
        }
        else if (auto cls = std::dynamic_pointer_cast<ClassDeclStmt>(stmt)) {
            auto info = new ClassInfo();
            info->name = cls->name;
            info->type = TypeRegistry::instance().getCustomType(cls->name);
            if (cls->baseClass.has_value()) {
                info->baseClass = getClassInfo(*cls->baseClass);
            }
            classTable_[cls->name] = info;
            
            auto sym = new Symbol();
            sym->kind = Symbol::CLASS;
            sym->name = cls->name;
            sym->node = stmt;
            sym->classType = info->type;
            defineSymbol(sym);
        }
        else if (auto iface = std::dynamic_pointer_cast<InterfaceDeclStmt>(stmt)) {
            auto sym = new Symbol();
            sym->kind = Symbol::CLASS;
            sym->name = iface->name;
            sym->node = stmt;
            defineSymbol(sym);
        }
        else if (auto enm = std::dynamic_pointer_cast<EnumDeclStmt>(stmt)) {
            auto sym = new Symbol();
            sym->kind = Symbol::TYPE_ALIAS;
            sym->name = enm->name;
            sym->type = TypeRegistry::instance().getCustomType(enm->name);
            defineSymbol(sym);
        }
        else if (auto st = std::dynamic_pointer_cast<StructDeclStmt>(stmt)) {
            // 预注册结构体类型
            auto info = new StructInfo();
            info->name = st->name;
            info->type = TypeRegistry::instance().getCustomType(st->name);
            info->type->kind = BuiltinType::STRUCT;
            structTable_[st->name] = info;
            
            auto sym = new Symbol();
            sym->kind = Symbol::TYPE_ALIAS;
            sym->name = st->name;
            sym->type = info->type;
            sym->node = stmt;
            defineSymbol(sym);
        }
    }
    
    // === PASS 2: 分析声明和语句 ===
    for (auto& stmt : program->statements) {
        if (auto func = std::dynamic_pointer_cast<FuncDeclStmt>(stmt)) {
            analyzeFuncDecl(func);
        }
        else if (auto cls = std::dynamic_pointer_cast<ClassDeclStmt>(stmt)) {
            analyzeClassDecl(cls);
        }
        else if (auto iface = std::dynamic_pointer_cast<InterfaceDeclStmt>(stmt)) {
            analyzeInterfaceDecl(iface);
        }
        else if (auto enm = std::dynamic_pointer_cast<EnumDeclStmt>(stmt)) {
            analyzeEnumDecl(enm);
        }
        else if (auto st = std::dynamic_pointer_cast<StructDeclStmt>(stmt)) {
            analyzeStructDecl(st);
        }
        else if (auto var = std::dynamic_pointer_cast<VarDeclStmt>(stmt)) {
            analyzeVarDecl(var);
        }
        else if (auto imp = std::dynamic_pointer_cast<ImportStmt>(stmt)) {
            // import 分析
        }
    }

    return !hasError_;
}

// === 作用域管理 ===
void SemanticAnalyzer::pushScope(const String& name) {
    auto scope = new Scope();
    scope->level = static_cast<int>(scopeStack_.size());
    scope->name = name;
    if (!scopeStack_.empty()) {
        scope->parent = scopeStack_.back();
    }
    scopeStack_.push_back(scope);
}

void SemanticAnalyzer::popScope() {
    if (!scopeStack_.empty()) {
        auto scope = scopeStack_.back();
        for (auto& pair : scope->symbols) {
            delete pair.second;
        }
        delete scope;
        scopeStack_.pop_back();
    }
}

void SemanticAnalyzer::defineSymbol(Symbol* sym) {
    auto scope = currentScope();
    auto existing = scope->findLocal(sym->name);
    if (existing) {
        // 从现有符号或新符号的 AST 节点提取行号信息
        int line = 0, col = 0;
        if (sym->node) { line = sym->node->token.line; col = sym->node->token.column; }
        else if (existing->node) { line = existing->node->token.line; col = existing->node->token.column; }
        reportError(line, col, "符号 '" + sym->name + "' 在此作用域中重复定义");
        return;
    }
    scope->define(sym);
}

Symbol* SemanticAnalyzer::lookupInScope(const String& name) {
    return currentScope()->find(name);
}

Symbol* SemanticAnalyzer::lookup(const String& name) {
    auto* scope = currentScope();
    if (!scope) {
        return nullptr;
    }
    return scope->find(name);
}

// === 类型系统 ===
Type* SemanticAnalyzer::getTypeFromString(const String& typeStr) {
    if (typeStr.empty() || typeStr == "auto") {
        return Type::unknown();
    }
    
    // 整数类型
    if (typeStr == "int" || typeStr == "Int" || typeStr == "整数") return Type::int_();
    if (typeStr == "i8" || typeStr == "int8" || typeStr == "Int8") return Type::int8_();
    if (typeStr == "i16" || typeStr == "int16" || typeStr == "Int16") return Type::int16_();
    if (typeStr == "i32" || typeStr == "int32" || typeStr == "Int32") return Type::int32_();
    if (typeStr == "i64" || typeStr == "int64" || typeStr == "Int64") return Type::int64_();
    
    // 浮点类型
    if (typeStr == "float" || typeStr == "Float" || typeStr == "浮点") return Type::float_();
    if (typeStr == "f32" || typeStr == "float32" || typeStr == "Float32") return Type::float32_();
    if (typeStr == "f64" || typeStr == "float64" || typeStr == "Float64") return Type::float64_();
    
    if (typeStr == "bool" || typeStr == "Bool" || typeStr == "布尔") return Type::bool_();
    if (typeStr == "string" || typeStr == "String" || typeStr == "字符串") return Type::string_();
    if (typeStr == "char" || typeStr == "Char" || typeStr == "字符") return Type::char_();
    if (typeStr == "void" || typeStr == "空") return Type::void_();
    
    // 数组类型
    if (typeStr.find("[]") != String::npos) {
        size_t pos = typeStr.find("[]");
        String elemStr = typeStr.substr(0, pos);
        Type* elemType = getTypeFromString(elemStr);
        return TypeRegistry::instance().getArrayType(elemType);
    }
    
    // 泛型（如 列表<整数>、字典<字符串, i64>）
    if (typeStr.find('<') != String::npos) {
        String baseName;
        std::vector<String> typeArgs;
        if (parseGenericTypeName(typeStr, baseName, typeArgs)) {
            // 检查是否为泛型结构体，如果是则确保已实例化
            auto baseInfo = getStructInfo(baseName);
            if (baseInfo) {
                // 查找原始 struct 声明，检查 typeParams
                auto baseSym = lookup(baseName);
                if (baseSym && baseSym->node) {
                    auto stDecl = std::dynamic_pointer_cast<StructDeclStmt>(baseSym->node);
                    if (stDecl && !stDecl->typeParams.empty()) {
                        // 确保泛型结构体已实例化
                        StructInfo* instInfo = ensureGenericStructInstantiated(typeStr, baseName, typeArgs);
                        if (instInfo) {
                            return instInfo->type;
                        }
                    }
                }
            }
        }
        return TypeRegistry::instance().getCustomType(typeStr);
    }
    
    // 可能是自定义类型
    auto sym = lookup(typeStr);
    if (sym && (sym->kind == Symbol::CLASS || sym->kind == Symbol::TYPE_ALIAS)) {
        return sym->type;
    }
    
    return TypeRegistry::instance().getCustomType(typeStr);
}

bool SemanticAnalyzer::isAssignableTo(Type* from, Type* to) {
    if (from == to) return true;
    if (to->kind == BuiltinType::UNKNOWN) return true;
    if (from->kind == BuiltinType::UNKNOWN) return true;
    
    // 检查是否都是数值类型
    bool isFromNumeric = 
        from->kind == BuiltinType::INT || 
        from->kind == BuiltinType::INT8 ||
        from->kind == BuiltinType::INT16 ||
        from->kind == BuiltinType::INT32 ||
        from->kind == BuiltinType::INT64 ||
        from->kind == BuiltinType::FLOAT ||
        from->kind == BuiltinType::FLOAT32 ||
        from->kind == BuiltinType::FLOAT64;
    
    bool isToNumeric = 
        to->kind == BuiltinType::INT || 
        to->kind == BuiltinType::INT8 ||
        to->kind == BuiltinType::INT16 ||
        to->kind == BuiltinType::INT32 ||
        to->kind == BuiltinType::INT64 ||
        to->kind == BuiltinType::FLOAT ||
        to->kind == BuiltinType::FLOAT32 ||
        to->kind == BuiltinType::FLOAT64;
    
    if (isFromNumeric && isToNumeric) {
        return true;
    }
    
    return false;
}

bool SemanticAnalyzer::areTypesEqual(Type* a, Type* b) {
    if (!a || !b) return false;
    return a->equals(b);
}

// === 声明分析 ===
void SemanticAnalyzer::analyzeFuncDecl(Shared<FuncDeclStmt> func) {
    pushScope(func->name);

    // 将泛型类型参数注册为当前作用域中的类型别名
    for (const auto& tp : func->typeParams) {
        auto typeParamSym = new Symbol();
        typeParamSym->kind = Symbol::TYPE_ALIAS;
        typeParamSym->name = tp.name;
        // 泛型类型参数标记为一个未知类型（在使用时由调用点确定具体类型）
        typeParamSym->type = Type::unknown();
        defineSymbol(typeParamSym);
    }

    // 定义函数符号
    auto sym = lookup(func->name);
    if (sym) {
        sym->params.clear();
        for (auto& p : func->params) {
            Type* pt = p.second.has_value() 
                ? getTypeFromString(*p.second) 
                : Type::unknown();
            sym->params.push_back({p.first, pt});
        }
    }
    
    // 分析参数
    for (auto& p : func->params) {
        auto paramSym = new Symbol();
        paramSym->kind = Symbol::PARAM;
        paramSym->name = p.first;
        paramSym->type = p.second.has_value() 
            ? getTypeFromString(*p.second) 
            : Type::unknown();
        defineSymbol(paramSym);
    }
    
    // 分析函数体
    if (func->body) {
        Type* retType = analyzeBlock(func->body);
        
        // 如果没有显式返回类型，从函数体推导
        if (!func->returnType.has_value() && retType->kind != BuiltinType::VOID) {
            sym->returnType = retType;
        }
        
        // 检查返回值
        if (func->returnType.has_value()) {
            Type* expected = getTypeFromString(*func->returnType);
            if (expected->kind != BuiltinType::VOID && retType->kind == BuiltinType::VOID) {
                // 有返回值的函数，但函数体没有返回值
                // 这个检查可以放宽：只要有路径返回值就行
            }
        }
    }
    
    popScope();
}

void SemanticAnalyzer::analyzeClassDecl(Shared<ClassDeclStmt> cls) {
    ClassInfo* info = getClassInfo(cls->name);
    if (!info) {
        info = new ClassInfo();
        info->name = cls->name;
        info->type = TypeRegistry::instance().getCustomType(cls->name);
        classTable_[cls->name] = info;
    }
    
    enterClass(info);
    
    // 分析成员
    for (auto& member : cls->members) {
        if (auto var = std::dynamic_pointer_cast<VarDeclStmt>(member)) {
            auto fieldSym = new Symbol();
            fieldSym->kind = Symbol::FIELD;
            fieldSym->name = var->name;
            fieldSym->type = var->type.has_value() 
                ? getTypeFromString(*var->type) 
                : Type::unknown();
            fieldSym->isConst = var->isConst;
            fieldSym->isStatic = false;
            fieldSym->isPublic = true; // 默认公有
            info->fieldTable[var->name] = fieldSym;
            info->fields.push_back(fieldSym);
            
            // 如果有初始化表达式
            if (var->init) {
                analyzeExpr(var->init);
            }
        }
        else if (auto func = std::dynamic_pointer_cast<FuncDeclStmt>(member)) {
            analyzeFuncDecl(func);
            
            auto methodSym = new Symbol();
            methodSym->kind = Symbol::FUNC;
            methodSym->name = func->name;
            methodSym->isStatic = func->isStatic;
            methodSym->returnType = func->returnType.has_value()
                ? getTypeFromString(*func->returnType)
                : Type::void_();
            for (auto& p : func->params) {
                Type* pt = p.second.has_value()
                    ? getTypeFromString(*p.second)
                    : Type::unknown();
                methodSym->params.push_back({p.first, pt});
            }
            info->methodTable[func->name] = methodSym;
            info->methods.push_back(methodSym);
        }
    }
    
    leaveClass();
}

void SemanticAnalyzer::analyzeInterfaceDecl(Shared<InterfaceDeclStmt> iface) {
    for (auto& method : iface->methods) {
        if (auto func = std::dynamic_pointer_cast<FuncDeclStmt>(method)) {
            // 接口方法不需要分析体
        }
    }
}

void SemanticAnalyzer::analyzeEnumDecl(Shared<EnumDeclStmt> enm) {
    if (enm->isADT) {
        // ADT 风格枚举：注册每个变体名及其字段类型
        Int64 tag = 0;
        for (auto& variant : enm->variants) {
            auto sym = new Symbol();
            sym->kind = Symbol::ENUM_VARIANT;
            sym->name = variant.name;
            sym->enumValue = tag;
            // 创建变体类型：使用枚举名作为类型
            Type* enumType = TypeRegistry::instance().getCustomType(enm->name);
            sym->type = enumType;
            // 存储字段类型信息
            for (auto& field : variant.fields) {
                Type* fieldType = getTypeFromString(field.second);
                sym->params.push_back({field.first, fieldType});
            }
            defineSymbol(sym);
            tag++;
        }
    } else {
        // 简单 C 风格枚举（向后兼容）
        Int64 value = 0;
        for (auto& pair : enm->values) {
            // 注册为枚举变体符号，以便匹配语句使用
            auto sym = new Symbol();
            sym->kind = Symbol::ENUM_VARIANT;
            sym->name = pair.first;
            sym->enumValue = value;
            Type* enumType = TypeRegistry::instance().getCustomType(enm->name);
            sym->type = enumType;
            defineSymbol(sym);
            
            // 枚举值检查
            if (pair.second.has_value()) {
                value = *pair.second;
            }
            pair.second = value;
            value++;
        }
    }
}

void SemanticAnalyzer::analyzeStructDecl(Shared<StructDeclStmt> st) {
    // 检查是否已在 Pass 1 中注册
    StructInfo* info = getStructInfo(st->name);
    if (!info) {
        // 创建结构体信息（Pass 1 未处理的情况）
        info = new StructInfo();
        info->name = st->name;
        info->type = TypeRegistry::instance().getCustomType(st->name);
        info->type->kind = BuiltinType::STRUCT;
        structTable_[st->name] = info;
    }

    // 为泛型结构体创建作用域，注册类型参数
    pushScope(st->name + "<type-params>");
    for (const auto& tp : st->typeParams) {
        auto typeParamSym = new Symbol();
        typeParamSym->kind = Symbol::TYPE_ALIAS;
        typeParamSym->name = tp.name;
        typeParamSym->type = Type::unknown();
        defineSymbol(typeParamSym);
    }

    // 清空之前的字段信息（重新分析）
    info->fields.clear();
    info->fieldIndex.clear();

    // 分析结构体成员
    size_t offset = 0;
    for (auto& member : st->members) {
        if (auto var = std::dynamic_pointer_cast<VarDeclStmt>(member)) {
            StructField field;
            field.name = var->name;
            field.offset = offset;
            
            // 类型推导逻辑：
            // 1. 显式类型：使用声明的类型
            // 2. 有初始值：从初始值推导类型
            // 3. 无类型无初始值：标记为 unknown（待实例化时推导）
            if (var->type.has_value()) {
                // 显式类型
                field.type = getTypeFromString(*var->type);
            } else if (var->init) {
                // 从初始值推导类型
                Type* initType = analyzeExpr(var->init);
                field.type = initType;
            } else {
                // 无类型信息，标记为 unknown
                field.type = Type::unknown();
            }
            
            // 计算字段大小（简化：所有类型都按8字节对齐）
            size_t fieldSize = 8;
            offset += fieldSize;
            
            info->fieldIndex[var->name] = info->fields.size();
            info->fields.push_back(field);
            
            // 检查初始化表达式类型匹配
            if (var->init) {
                Type* initType = getExprType(var->init);
                if (field.type && field.type->kind != BuiltinType::UNKNOWN &&
                    !isAssignableTo(initType, field.type)) {
                    reportError(var->token.line, var->token.column,
                        "结构体字段 '" + var->name + "' 类型不匹配: 无法用 " +
                        initType->toString() + " 初始化 " + field.type->toString());
                }
            }
        }
    }
    info->size = offset;

    // 弹出泛型类型参数作用域
    if (!st->typeParams.empty()) {
        popScope();
    }
}

void SemanticAnalyzer::analyzeVarDecl(Shared<VarDeclStmt> var) {
    // 隐式声明: 如果变量已存在, 作为赋值处理, 不报错
    if (var->isImplicit && currentScope()->findLocal(var->name)) {
        if (var->init) analyzeExpr(var->init);
        return;
    }
    Type* declaredType = nullptr;
    if (var->type.has_value()) {
        declaredType = getTypeFromString(*var->type);
    }
    
    if (var->init) {
        Type* initType = analyzeExpr(var->init);
        
        if (declaredType && initType) {
            if (!isAssignableTo(initType, declaredType)) {
                reportError(var->token.line, var->token.column,
                    "无法用类型 '" + initType->toString() +
                    "' 的值初始化类型 '" + declaredType->toString() + "' 的变量");
            }
        }
        
        // 类型推导
        if (!declaredType) {
            declaredType = initType;
        }
    }
    
    auto sym = new Symbol();
    sym->kind = var->isConst ? Symbol::CONST : Symbol::VAR;
    sym->name = var->name;
    sym->type = declaredType ? declaredType : Type::unknown();
    sym->isConst = var->isConst;
    defineSymbol(sym);
}

// === 语句分析 ===
Type* SemanticAnalyzer::analyzeStmt(Shared<Stmt> stmt) {
    if (!stmt) return Type::void_();
    
    if (auto block = std::dynamic_pointer_cast<BlockStmt>(stmt)) {
        return analyzeBlock(block);
    }
    if (auto ifStmt = std::dynamic_pointer_cast<IfStmt>(stmt)) {
        return analyzeIf(ifStmt);
    }
    if (auto forStmt = std::dynamic_pointer_cast<ForStmt>(stmt)) {
        return analyzeFor(forStmt);
    }
    if (auto whileStmt = std::dynamic_pointer_cast<WhileStmt>(stmt)) {
        return analyzeWhile(whileStmt);
    }
    if (auto ret = std::dynamic_pointer_cast<ReturnStmt>(stmt)) {
        return analyzeReturn(ret);
    }
    if (auto var = std::dynamic_pointer_cast<VarDeclStmt>(stmt)) {
        analyzeVarDecl(var);
        return Type::void_();
    }
    if (auto exprStmt = std::dynamic_pointer_cast<ExprStmt>(stmt)) {
        return analyzeExpr(exprStmt->expr);
    }
    if (auto deferStmt = std::dynamic_pointer_cast<DeferStmt>(stmt)) {
        // defer 语句已在 transformDeferInBlock 中被展开，
        // 如果执行到这里说明不在块内（如全局作用域），提示警告
        reportError(deferStmt->token.line, deferStmt->token.column,
                    "'推迟' 应该在块语句内使用", SemanticError::WARNING);
        if (deferStmt->body) return analyzeStmt(deferStmt->body);
        return Type::void_();
    }
    if (auto matchStmt = std::dynamic_pointer_cast<MatchStmt>(stmt)) {
        return analyzeMatch(matchStmt);
    }
    if (auto brk = std::dynamic_pointer_cast<BreakStmt>(stmt)) {
        if (loopStack_.empty()) {
            reportError(brk->token.line, brk->token.column, "'break' 必须在循环内使用");
        }
        return Type::void_();
    }
    if (auto cont = std::dynamic_pointer_cast<ContinueStmt>(stmt)) {
        if (loopStack_.empty()) {
            reportError(cont->token.line, cont->token.column, "'continue' 必须在循环内使用");
        }
        return Type::void_();
    }
    
    return Type::void_();
}

Type* SemanticAnalyzer::analyzeBlock(Shared<BlockStmt> block) {
    pushScope("block");

    // 展开 defer 语句为 try-finally 模式
    transformDeferInBlock(block);

    Type* lastType = Type::void_();

    for (auto& s : block->statements) {
        lastType = analyzeStmt(s);
    }

    popScope();
    return lastType;
}

static bool isValidConditionType(BuiltinType kind) {
    return kind == BuiltinType::BOOL || kind == BuiltinType::UNKNOWN
        || kind == BuiltinType::INT || kind == BuiltinType::INT8
        || kind == BuiltinType::INT16 || kind == BuiltinType::INT32
        || kind == BuiltinType::INT64
        || kind == BuiltinType::FLOAT || kind == BuiltinType::FLOAT32
        || kind == BuiltinType::FLOAT64;
}

Type* SemanticAnalyzer::analyzeIf(Shared<IfStmt> s) {
    Type* condType = analyzeExpr(s->condition);
    if (!isValidConditionType(condType->kind)) {
        reportError(s->token.line, s->token.column, 
            "条件必须是布尔值，实际为 '" + condType->toString() + "'");
    }
    
    analyzeStmt(s->thenBranch);
    if (s->elseBranch) {
        analyzeStmt(s->elseBranch);
    }
    
    return Type::void_();
}

Type* SemanticAnalyzer::analyzeFor(Shared<ForStmt> s) {
    loopStack_.push_back("for");
    
    pushScope("for");
    
    if (s->init) analyzeStmt(s->init);
    if (s->condition) {
        Type* condType = analyzeExpr(s->condition);
        if (!isValidConditionType(condType->kind)) {
            reportError(s->condition->token.line, s->condition->token.column,
                "for循环条件必须是布尔值");
        }
    }
    if (s->update) analyzeExpr(s->update);
    if (s->body) analyzeStmt(s->body);
    
    popScope();
    loopStack_.pop_back();
    
    return Type::void_();
}

Type* SemanticAnalyzer::analyzeWhile(Shared<WhileStmt> s) {
    loopStack_.push_back("while");
    
    Type* condType = analyzeExpr(s->condition);
    if (!isValidConditionType(condType->kind)) {
        reportError(s->token.line, s->token.column,
            "while循环条件必须是布尔值");
    }
    
    analyzeStmt(s->body);
    
    loopStack_.pop_back();
    return Type::void_();
}

Type* SemanticAnalyzer::analyzeReturn(Shared<ReturnStmt> s) {
    if (s->value) {
        return analyzeExpr(s->value);
    }
    return Type::void_();
}

// ═══════════════════════════════════════════════════════════════════
//  analyzeMatch — 模式匹配语句语义分析
// ═══════════════════════════════════════════════════════════════════
Type* SemanticAnalyzer::analyzeMatch(Shared<MatchStmt> s) {
    // 分析匹配表达式
    Type* matchType = analyzeExpr(s->expr);

    // 查找匹配表达式的类型对应的枚举声明
    Symbol* enumSym = nullptr;
    if (matchType->kind == BuiltinType::OBJECT) {
        // 查找枚举类型符号
        enumSym = lookup(matchType->name);
    }

    // 分析每个分支
    for (auto& mc : s->cases) {
        // 查找变体符号
        Symbol* variantSym = lookup(mc.variantName);
        if (!variantSym || variantSym->kind != Symbol::ENUM_VARIANT) {
            reportError(s->token.line, s->token.column,
                "未定义的枚举变体: '" + mc.variantName + "'");
            continue;
        }

        // 检查绑定数量是否匹配变体字段数量
        if (mc.bindings.size() != variantSym->params.size()) {
            reportError(s->token.line, s->token.column,
                "变体 '" + mc.variantName + "' 需要 " +
                std::to_string(variantSym->params.size()) +
                " 个绑定，但提供了 " + std::to_string(mc.bindings.size()));
        }

        // 在分支作用域中注册绑定变量
        for (size_t i = 0; i < mc.bindings.size() && i < variantSym->params.size(); i++) {
            auto bindSym = new Symbol();
            bindSym->kind = Symbol::VAR;
            bindSym->name = mc.bindings[i];
            bindSym->type = variantSym->params[i].second;
            // 暂时注册到当前作用域（分析分支体时会用到）
            // 注意：在实际实现中，每个分支应该有独立作用域
        }

        // 分析分支体
        if (mc.body) {
            analyzeStmt(mc.body);
        }
    }

    // 分析默认分支
    if (s->defaultCase) {
        analyzeStmt(s->defaultCase);
    }

    return Type::void_();
}

// === 表达式分析 ===
Type* SemanticAnalyzer::analyzeExpr(Shared<Expr> expr) {
    if (!expr) return Type::void_();
    
    Type* result = nullptr;
    
    if (auto lit = std::dynamic_pointer_cast<LiteralExpr>(expr)) {
        result = getExprType(expr);
    }
    else if (auto awaitExpr = std::dynamic_pointer_cast<AwaitExpr>(expr)) {
        // await 表达式类型 = 目标操作的结果类型
        result = analyzeExpr(awaitExpr->target);
    }
    else if (auto thisExpr = std::dynamic_pointer_cast<ThisExpr>(expr)) {
        // this 表达式的类型为当前类的实例类型
        auto* cls = currentClass();
        if (cls && cls->type) {
            result = cls->type;
        } else {
            reportError(expr->token.line, expr->token.column,
                       "'这个' 只能在类方法中使用");
            result = Type::unknown();
        }
    }
    else if (auto superExpr = std::dynamic_pointer_cast<SuperExpr>(expr)) {
        // super 调用：在父类中查找方法
        auto* cls = currentClass();
        if (!cls || !cls->baseClass) {
            reportError(expr->token.line, expr->token.column,
                       "'继承' 只能在有父类的类方法中使用");
            result = Type::unknown();
        } else {
            // 查找父类方法
            for (auto* m : cls->baseClass->methods) {
                if (m->name == superExpr->method) {
                    result = m->returnType;
                    break;
                }
            }
            if (!result || result->kind == TypeKind::UNKNOWN) {
                result = Type::unknown();
            }
        }
    }
    else if (auto id = std::dynamic_pointer_cast<IdentifierExpr>(expr)) {
        auto sym = lookup(id->name);
        if (!sym) {
            result = Type::unknown();
        } else {
            result = sym->type ? sym->type : Type::unknown();
        }
    }
    else if (auto bin = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
        result = analyzeBinaryExpr(bin);
    }
    else if (auto unary = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
        result = analyzeUnaryExpr(unary);
    }
    else if (auto call = std::dynamic_pointer_cast<CallExpr>(expr)) {
        result = analyzeCallExpr(call);
    }
    else if (auto member = std::dynamic_pointer_cast<MemberExpr>(expr)) {
        result = analyzeMemberExpr(member);
    }
    else if (auto index = std::dynamic_pointer_cast<IndexExpr>(expr)) {
        result = analyzeIndexExpr(index);
    }
    else if (auto structLit = std::dynamic_pointer_cast<StructLiteralExpr>(expr)) {
        // 匿名表/对象字面量（如 {} 或 {key: value}），无需结构体定义
        if (structLit->structName.empty()) {
            for (auto& field : structLit->fields) {
                analyzeExpr(field.second);
            }
            result = Type::table_();
        } else {
            // 先检查是否为泛型结构体（如 列表<整数>），触发实例化
            if (structLit->structName.find('<') != String::npos) {
                // 调用 getTypeFromString 会触发泛型结构体实例化
                Type* genericType = getTypeFromString(structLit->structName);
                if (genericType && genericType->kind == BuiltinType::STRUCT) {
                    // 已成功实例化，用实例化后的类型
                    for (auto& field : structLit->fields) {
                        analyzeExpr(field.second);
                    }
                    result = genericType;
                    if (result && expr.get()) exprTypes_[expr.get()] = result;
                    return result;
                }
            }

            auto info = getStructInfo(structLit->structName);
            if (!info) {
                reportError(expr->token.line, expr->token.column,
                    "未定义的结构体类型: " + structLit->structName);
                result = Type::unknown();
            } else {
                for (auto& field : structLit->fields) {
                    analyzeExpr(field.second);
                }
                result = info->type;
            }
        }
    }
    else if (auto lambda = std::dynamic_pointer_cast<LambdaExpr>(expr)) {
        result = analyzeLambda(lambda);
    }
    else {
        result = Type::unknown();
    }
    
    // 缓存结果供 codegen 查询
    if (result && expr.get()) {
        exprTypes_[expr.get()] = result;
    }
    return result;
}

void SemanticAnalyzer::cacheExprType(Shared<Expr> expr, Type* t) {
    if (expr && t) {
        exprTypes_[expr.get()] = t;
    }
}

Type* SemanticAnalyzer::getExprType(Shared<Expr> expr) {
    if (!expr) return Type::void_();
    
    // 1. 查缓存
    auto it = exprTypes_.find(expr.get());
    if (it != exprTypes_.end()) return it->second;
    
    // 2. LiteralExpr
    if (auto lit = std::dynamic_pointer_cast<LiteralExpr>(expr)) {
        if (std::get_if<Int64>(&lit->value)) return Type::int_();
        if (std::get_if<Float64>(&lit->value)) return Type::float_();
        if (auto* v = std::get_if<String>(&lit->value)) {
            if (*v == "null") return Type::unknown();
            return Type::string_();
        }
        if (std::get_if<bool>(&lit->value)) return Type::bool_();
    }
    
    // 3. IdentifierExpr — look up symbol type
    if (auto var = std::dynamic_pointer_cast<IdentifierExpr>(expr)) {
        Symbol* sym = lookup(var->name);
        if (sym && sym->type) return sym->type;
    }
    
    // 4. BinaryExpr — compute from operand types
    if (auto bin = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
        Type* lt = getExprType(bin->left);
        Type* rt = getExprType(bin->right);
        if (lt && rt) {
            // Arithmetic: int+int→int, float→float, string+→string
            if (bin->op == TokenType::OP_PLUS) {
                if (lt->kind == BuiltinType::STRING || rt->kind == BuiltinType::STRING)
                    return Type::string_();
            }
            if (lt->kind == BuiltinType::FLOAT || rt->kind == BuiltinType::FLOAT)
                return Type::float_();
            if (lt->kind != BuiltinType::UNKNOWN && rt->kind != BuiltinType::UNKNOWN)
                return Type::int_();
        }
    }
    
    // 5. UnaryExpr — propagate operand type
    if (auto una = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
        return getExprType(una->operand);
    }
    
    return Type::unknown();
}

Type* SemanticAnalyzer::analyzeBinaryExpr(Shared<BinaryExpr> expr) {
    Type* leftType = analyzeExpr(expr->left);
    Type* rightType = analyzeExpr(expr->right);
    
    // 算术运算符
    if (expr->op == TokenType::OP_PLUS || expr->op == TokenType::OP_MINUS ||
        expr->op == TokenType::OP_MUL || expr->op == TokenType::OP_DIV ||
        expr->op == TokenType::OP_MOD) {
        
        // 字符串支持 +
        if (expr->op == TokenType::OP_PLUS) {
            if (leftType->kind == BuiltinType::STRING || rightType->kind == BuiltinType::STRING) {
                return Type::string_();
            }
        }
        
        // int + float -> float
        if (leftType->kind == BuiltinType::FLOAT || rightType->kind == BuiltinType::FLOAT) {
            return Type::float_();
        }
        return Type::int_();
    }
    
    // 比较/逻辑运算符
    if (expr->op == TokenType::OP_EQ || expr->op == TokenType::OP_NE ||
        expr->op == TokenType::OP_LT || expr->op == TokenType::OP_GT ||
        expr->op == TokenType::OP_LE || expr->op == TokenType::OP_GE ||
        expr->op == TokenType::OP_AND || expr->op == TokenType::OP_OR) {
        return Type::bool_();
    }
    
    // 赋值返回右值类型
    if (expr->op == TokenType::OP_ASSIGN) {
        if (!isAssignableTo(rightType, leftType)) {
            reportError(expr->token.line, expr->token.column,
                "赋值类型不匹配: 无法将 '" + rightType->toString() +
                "' 赋值给 '" + leftType->toString() + "'");
        }
        return rightType;
    }
    
    // 复合赋值
    if (expr->op == TokenType::OP_PLUS_ASSIGN || expr->op == TokenType::OP_MINUS_ASSIGN ||
        expr->op == TokenType::OP_MUL_ASSIGN || expr->op == TokenType::OP_DIV_ASSIGN) {
        return leftType;
    }
    
    // 位运算
    if (expr->op == TokenType::OP_BIT_AND || expr->op == TokenType::OP_BIT_OR ||
        expr->op == TokenType::OP_BIT_XOR || expr->op == TokenType::OP_LSHIFT ||
        expr->op == TokenType::OP_RSHIFT) {
        return Type::int_();
    }
    
    return Type::unknown();
}

Type* SemanticAnalyzer::analyzeUnaryExpr(Shared<UnaryExpr> expr) {
    Type* operandType = analyzeExpr(expr->operand);
    
    if (expr->op == TokenType::OP_NOT) {
        return Type::bool_();
    }
    if (expr->op == TokenType::OP_MINUS) {
        return operandType;
    }
    if (expr->op == TokenType::OP_INC || expr->op == TokenType::OP_DEC) {
        return operandType;
    }
    
    return Type::unknown();
}

Type* SemanticAnalyzer::analyzeCallExpr(Shared<CallExpr> expr) {
    // ════════════════════════════════════════════════════════
    //  泛型调用: 函数名<类型1, 类型2>(参数...)
    // ════════════════════════════════════════════════════════
    if (!expr->typeArgs.empty()) {
        if (auto id = std::dynamic_pointer_cast<IdentifierExpr>(expr->callee)) {
            String mangledName = buildMangledName(id->name, expr->typeArgs);

            // 检查缓存是否已有该实例化版本
            auto it = monomorphCache_.find(mangledName);
            if (it == monomorphCache_.end()) {
                // 查找原始泛型函数定义
                auto sym = lookup(id->name);
                if (!sym || !sym->node) {
                    reportError(expr->token.line, expr->token.column,
                        "未找到泛型函数 '" + id->name + "' 的定义");
                    return Type::unknown();
                }

                auto genericFunc = std::dynamic_pointer_cast<FuncDeclStmt>(sym->node);
                if (!genericFunc || genericFunc->typeParams.empty()) {
                    // 有类型参数但函数不是泛型函数，报错
                    reportError(expr->token.line, expr->token.column,
                        "'" + id->name + "' 不是泛型函数");
                    return Type::unknown();
                }

                // 检查类型参数数量是否匹配
                if (genericFunc->typeParams.size() != expr->typeArgs.size()) {
                    reportError(expr->token.line, expr->token.column,
                        "泛型函数 '" + id->name + "' 需要 " +
                        std::to_string(genericFunc->typeParams.size()) +
                        " 个类型参数，但提供了 " +
                        std::to_string(expr->typeArgs.size()));
                    return Type::unknown();
                }

                // 检查类型参数约束
                checkTypeConstraints(genericFunc->typeParams, expr->typeArgs,
                    id->name, expr->token.line, expr->token.column);

                // 实例化泛型函数
                auto instantiated = instantiateGenericFunc(genericFunc, expr->typeArgs);

                // 存储到独立列表，供代码生成器优先编译
                monomorphizedFunctions_.push_back(instantiated);

                // 注册到符号表（使用 mangled 名）
                auto newSym = new Symbol();
                newSym->kind = Symbol::FUNC;
                newSym->name = mangledName;
                newSym->node = instantiated;
                newSym->returnType = instantiated->returnType.has_value()
                    ? getTypeFromString(*instantiated->returnType)
                    : Type::void_();
                newSym->isStatic = instantiated->isStatic;
                defineSymbol(newSym);

                // 缓存
                monomorphCache_[mangledName] = {mangledName, instantiated};

                // 分析实例化后的函数体（此时类型参数已被替换为具体类型）
                analyzeFuncDecl(instantiated);

                // 更新返回类型（analyzeFuncDecl 可能修改了它）
                auto analyzedSym = lookup(mangledName);
                if (analyzedSym && analyzedSym->returnType) {
                    newSym->returnType = analyzedSym->returnType;
                }

                // 分析参数
                for (auto& arg : expr->arguments) {
                    analyzeExpr(arg);
                }

                return newSym->returnType ? newSym->returnType : Type::void_();
            } else {
                // 已缓存，直接使用
                auto& cached = it->second;
                auto existingSym = lookup(mangledName);
                if (existingSym && existingSym->returnType) {
                    // 分析参数
                    for (auto& arg : expr->arguments) {
                        analyzeExpr(arg);
                    }
                    return existingSym->returnType;
                }
                return Type::void_();
            }
        }
    }

    // ════════════════════════════════════════════════════════
    //  非泛型调用: 原有逻辑
    // ════════════════════════════════════════════════════════
    Type* calleeType = analyzeExpr(expr->callee);

    for (auto& arg : expr->arguments) {
        analyzeExpr(arg);
    }

    // 查找函数符号
    if (auto id = std::dynamic_pointer_cast<IdentifierExpr>(expr->callee)) {
        auto sym = lookup(id->name);
        if (sym && sym->returnType) {
            return sym->returnType;
        }
    }

    return calleeType;
}

Type* SemanticAnalyzer::analyzeMemberExpr(Shared<MemberExpr> expr) {
    Type* objectType = analyzeExpr(expr->object);
    
    // 处理结构体成员访问
    if (objectType->kind == BuiltinType::STRUCT || 
        objectType->kind == BuiltinType::OBJECT) {
        
        StructInfo* structInfo = getStructInfo(objectType->name);
        if (structInfo) {
            StructField* field = structInfo->findField(expr->member);
            if (field) {
                return field->type;
            } else {
                reportError(expr->token.line, expr->token.column,
                    "结构体 '" + objectType->name + "' 没有字段 '" + expr->member + "'");
                return Type::unknown();
            }
        }
        
        // 类成员访问
        ClassInfo* classInfo = getClassInfo(objectType->name);
        if (classInfo) {
            auto it = classInfo->fieldTable.find(expr->member);
            if (it != classInfo->fieldTable.end()) {
                return it->second->type;
            }
            auto mit = classInfo->methodTable.find(expr->member);
            if (mit != classInfo->methodTable.end()) {
                return mit->second->type;
            }
            reportError(expr->token.line, expr->token.column,
                "类 '" + objectType->name + "' 没有成员 '" + expr->member + "'");
            return Type::unknown();
        }
    }
    
    // 数组的 length 属性
    if (objectType->kind == BuiltinType::ARRAY && expr->member == "length") {
        return Type::int_();
    }
    
    // 字符串的 length 属性
    if (objectType->kind == BuiltinType::STRING && expr->member == "length") {
        return Type::int_();
    }
    
    return Type::unknown();
}

Type* SemanticAnalyzer::analyzeIndexExpr(Shared<IndexExpr> expr) {
    Type* arrayType = analyzeExpr(expr->array);
    analyzeExpr(expr->index);
    
    if (arrayType->kind == BuiltinType::ARRAY && arrayType->innerType) {
        return arrayType->innerType;
    }
    return Type::unknown();
}

Type* SemanticAnalyzer::analyzeLambda(Shared<LambdaExpr> expr) {
    // 创建新作用域用于 lambda 参数
    pushScope("lambda");
    
    // 注册参数到当前作用域
    for (auto& p : expr->params) {
        auto paramSym = new Symbol();
        paramSym->kind = Symbol::PARAM;
        paramSym->name = p.first;
        paramSym->type = p.second.has_value()
            ? getTypeFromString(*p.second)
            : Type::unknown();
        defineSymbol(paramSym);
    }
    
    // 分析函数体，收集已定义变量名（用于识别捕获变量）
    std::set<String> lambdaDefinedVars;
    for (auto& p : expr->params) {
        lambdaDefinedVars.insert(p.first);
    }
    
    // 分析函数体
    Type* returnType = Type::void_();
    if (expr->body) {
        returnType = analyzeBlock(expr->body);
    }
    
    // 检测捕获变量：查找函数体中引用的外部变量
    // 简化实现：遍历外层作用域中已定义的变量，对比 lambda 内部定义的变量
    // 实际上更精确的做法需要在 analyzeExpr 中追踪变量引用
    // 这里我们委托给 codegen 来处理捕获逻辑
    // 语义分析器负责标记 captures 列表
    
    // 收集 lambda 体内的标识符引用（在 analyzeBlock 中已分析）
    // captures 在 codegen 阶段通过作用域分析填充
    
    popScope();
    
    // 返回函数类型
    Type* funcType = new Type();
    funcType->kind = BuiltinType::FUNCTION;
    return funcType;
}

// === 工具 ===
void SemanticAnalyzer::reportError(int line, int col, const String& msg, SemanticError::Level level) {
    if (!hasError_) {
        hasError_ = true;
        errorMsg_ = "第" + std::to_string(line) + "行第" +
                     std::to_string(col) + "列: " + msg;
    }
    errors_.push_back({level, line, col, msg});
}

void SemanticAnalyzer::reportError(const String& msg, SemanticError::Level level) {
    if (!hasError_) {
        hasError_ = true;
        errorMsg_ = msg;
    }
    errors_.push_back({level, 0, 0, msg});
}

void SemanticAnalyzer::printErrors() {
    for (auto& err : errors_) {
        std::cout << (err.level == SemanticError::WARNING ? "[警告]" : "[错误]");
        if (err.line > 0) {
            std::cout << " 第" << err.line << "行第" << err.column << "列";
        }
        std::cout << ": " << err.message << std::endl;
    }
}

ClassInfo* SemanticAnalyzer::getClassInfo(const String& name) {
    auto it = classTable_.find(name);
    if (it != classTable_.end()) {
        return it->second;
    }
    return nullptr;
}

void SemanticAnalyzer::enterClass(ClassInfo* cls) {
    classStack_.push_back(cls);
    pushScope(cls->name);
}

void SemanticAnalyzer::leaveClass() {
    popScope();
    if (!classStack_.empty()) {
        classStack_.pop_back();
    }
}

StructInfo* SemanticAnalyzer::getStructInfo(const String& name) {
    auto it = structTable_.find(name);
    if (it != structTable_.end()) {
        return it->second;
    }
    return nullptr;
}

void SemanticAnalyzer::registerStructType(const String& name, StructInfo* info) {
    structTable_[name] = info;
}


// ═══════════════════════════════════════════════════════════════════
//  transformDeferInBlock — 为 defer 做语义准备
//  defer 的实际代码生成在 codegen 层（Go 风格的 defer 栈）
//  这里只做基本的语义检查
// ═══════════════════════════════════════════════════════════════════
void SemanticAnalyzer::transformDeferInBlock(Shared<BlockStmt> block) {
    // defer 的展开由 codegen 负责（compileDefer/compileBlock/compileReturn）
    // 这里只对 defer 体做语义分析
    for (auto& stmt : block->statements) {
        if (auto deferStmt = std::dynamic_pointer_cast<DeferStmt>(stmt)) {
            if (deferStmt->body) {
                analyzeStmt(deferStmt->body);
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
//  泛型约束（trait）检查
// ═══════════════════════════════════════════════════════════════════

bool SemanticAnalyzer::typeSatisfiesTrait(Type* type, const String& traitName) {
    if (!type) return false;

    auto isNumeric = [](BuiltinType k) -> bool {
        return k == BuiltinType::INT || k == BuiltinType::INT8 ||
               k == BuiltinType::INT16 || k == BuiltinType::INT32 ||
               k == BuiltinType::INT64 || k == BuiltinType::FLOAT ||
               k == BuiltinType::FLOAT32 || k == BuiltinType::FLOAT64;
    };

    auto isInteger = [](BuiltinType k) -> bool {
        return k == BuiltinType::INT || k == BuiltinType::INT8 ||
               k == BuiltinType::INT16 || k == BuiltinType::INT32 ||
               k == BuiltinType::INT64;
    };

    BuiltinType k = type->kind;

    if (traitName == "可比较" || traitName == "Comparable") {
        // 数值类型、bool、string、char、枚举可比较
        return isNumeric(k) || k == BuiltinType::BOOL ||
               k == BuiltinType::STRING || k == BuiltinType::CHAR ||
               k == BuiltinType::ENUM;
    }
    if (traitName == "可哈希" || traitName == "Hashable") {
        return isNumeric(k) || k == BuiltinType::BOOL ||
               k == BuiltinType::STRING || k == BuiltinType::CHAR;
    }
    if (traitName == "可字符串化" || traitName == "Stringifiable") {
        // 所有类型都可以字符串化
        return true;
    }
    if (traitName == "可排序" || traitName == "Sortable") {
        return isNumeric(k) || k == BuiltinType::CHAR;
    }
    if (traitName == "可相加" || traitName == "Addable") {
        return isNumeric(k) || k == BuiltinType::STRING;
    }
    if (traitName == "可迭代" || traitName == "Iterable") {
        return k == BuiltinType::ARRAY;
    }
    if (traitName == "可复制" || traitName == "Copyable") {
        // 所有内置类型都是可复制的
        return true;
    }
    if (traitName == "可克隆" || traitName == "Cloneable") {
        return true;
    }

    // 未知 trait — 默认允许（为未来扩展预留）
    return true;
}

bool SemanticAnalyzer::checkTypeConstraints(
    const std::vector<TypeParam>& typeParams,
    const std::vector<String>& typeArgs,
    const String& contextName,
    int line, int col)
{
    bool allOk = true;
    for (size_t i = 0; i < typeParams.size() && i < typeArgs.size(); i++) {
        if (typeParams[i].constraint.has_value()) {
            const String& constraint = typeParams[i].constraint.value();
            const String& concreteTypeStr = typeArgs[i];
            Type* concreteType = getTypeFromString(concreteTypeStr);

            if (!typeSatisfiesTrait(concreteType, constraint)) {
                String typeDisplay = concreteType ? concreteType->toString() : concreteTypeStr;
                reportError(line, col,
                    "类型 '" + typeDisplay + "' 不满足约束 '" + constraint +
                    "' 在泛型 '" + contextName + "' 中");
                allOk = false;
            }
        }
    }
    return allOk;
}

// ═══════════════════════════════════════════════════════════════════
//  泛型实例化（单态化 Monomorphization）
// ═══════════════════════════════════════════════════════════════════

String SemanticAnalyzer::buildMangledName(const String& funcName, const std::vector<String>& typeArgs) {
    // 生成唯一名：排序<整数, 字符串> → 排序$i64$string
    // 支持嵌套：函数<对<整数, 字符串>> → 函数$对$i64$string
    String mangled = funcName;
    for (const auto& arg : typeArgs) {
        // 检查是否是嵌套泛型（如 "对<整数, 字符串>"）
        if (arg.find('<') != String::npos) {
            // 递归解析嵌套泛型
            String baseName;
            std::vector<String> nestedArgs;
            if (parseGenericTypeName(arg, baseName, nestedArgs)) {
                mangled += "$" + buildMangledName(baseName, nestedArgs);
                continue;
            }
        }

        // 将用户写的中文类型名映射为规范名
        String canonical = arg;
        if (arg == "int" || arg == "Int" || arg == "整数" || arg == "i64") canonical = "i64";
        else if (arg == "i32" || arg == "int32" || arg == "Int32") canonical = "i32";
        else if (arg == "i16" || arg == "int16" || arg == "Int16") canonical = "i16";
        else if (arg == "i8" || arg == "int8" || arg == "Int8") canonical = "i8";
        else if (arg == "float" || arg == "Float" || arg == "浮点" || arg == "f64") canonical = "f64";
        else if (arg == "f32" || arg == "float32" || arg == "Float32") canonical = "f32";
        else if (arg == "bool" || arg == "Bool" || arg == "布尔") canonical = "bool";
        else if (arg == "string" || arg == "String" || arg == "字符串") canonical = "string";
        else if (arg == "char" || arg == "Char" || arg == "字符") canonical = "char";
        mangled += "$" + canonical;
    }
    return mangled;
}

Shared<BlockStmt> SemanticAnalyzer::cloneBlock(Shared<BlockStmt> block) {
    if (!block) return nullptr;
    auto newBlock = Shared<BlockStmt>(new BlockStmt());
    for (auto& stmt : block->statements) {
        substituteTypeInStmt(stmt, {}, {});  // 使用空参数仅做克隆（substitute 函数也会克隆）
    }
    // 简化：直接复制 shared_ptr，因为我们只替换类型注解
    newBlock->statements = block->statements;
    return newBlock;
}

void SemanticAnalyzer::substituteTypeInStmt(Shared<Stmt> stmt,
                                            const std::vector<String>& paramNames,
                                            const std::vector<String>& typeArgs) {
    if (!stmt) return;

    // VarDeclStmt: 替换类型注解
    if (auto var = std::dynamic_pointer_cast<VarDeclStmt>(stmt)) {
        if (var->type.has_value()) {
            for (size_t i = 0; i < paramNames.size(); i++) {
                if (var->type.value() == paramNames[i]) {
                    var->type = typeArgs[i];
                    break;
                }
            }
        }
        if (var->init) {
            substituteTypeInExpr(var->init, paramNames, typeArgs);
        }
    }
    // FuncDeclStmt: 替换参数类型和返回类型
    else if (auto func = std::dynamic_pointer_cast<FuncDeclStmt>(stmt)) {
        for (auto& param : func->params) {
            if (param.second.has_value()) {
                for (size_t i = 0; i < paramNames.size(); i++) {
                    if (param.second.value() == paramNames[i]) {
                        param.second = typeArgs[i];
                        break;
                    }
                }
            }
        }
        if (func->returnType.has_value()) {
            for (size_t i = 0; i < paramNames.size(); i++) {
                if (func->returnType.value() == paramNames[i]) {
                    func->returnType = typeArgs[i];
                    break;
                }
            }
        }
        if (func->body) {
            for (auto& s : func->body->statements) {
                substituteTypeInStmt(s, paramNames, typeArgs);
            }
        }
    }
    // IfStmt
    else if (auto ifStmt = std::dynamic_pointer_cast<IfStmt>(stmt)) {
        if (ifStmt->thenBranch) substituteTypeInStmt(ifStmt->thenBranch, paramNames, typeArgs);
        if (ifStmt->elseBranch) substituteTypeInStmt(ifStmt->elseBranch, paramNames, typeArgs);
    }
    // BlockStmt
    else if (auto block = std::dynamic_pointer_cast<BlockStmt>(stmt)) {
        for (auto& s : block->statements) {
            substituteTypeInStmt(s, paramNames, typeArgs);
        }
    }
    // ForStmt
    else if (auto forStmt = std::dynamic_pointer_cast<ForStmt>(stmt)) {
        if (forStmt->init) substituteTypeInStmt(forStmt->init, paramNames, typeArgs);
        if (forStmt->body) substituteTypeInStmt(forStmt->body, paramNames, typeArgs);
    }
    // WhileStmt
    else if (auto whileStmt = std::dynamic_pointer_cast<WhileStmt>(stmt)) {
        if (whileStmt->body) substituteTypeInStmt(whileStmt->body, paramNames, typeArgs);
    }
    // ForEachStmt
    else if (auto fe = std::dynamic_pointer_cast<ForEachStmt>(stmt)) {
        if (fe->body) substituteTypeInStmt(fe->body, paramNames, typeArgs);
    }
    // ReturnStmt
    else if (auto ret = std::dynamic_pointer_cast<ReturnStmt>(stmt)) {
        if (ret->value) substituteTypeInExpr(ret->value, paramNames, typeArgs);
    }
    // ExprStmt
    else if (auto exprStmt = std::dynamic_pointer_cast<ExprStmt>(stmt)) {
        if (exprStmt->expr) substituteTypeInExpr(exprStmt->expr, paramNames, typeArgs);
    }
    // DeferStmt
    else if (auto deferStmt = std::dynamic_pointer_cast<DeferStmt>(stmt)) {
        if (deferStmt->body) substituteTypeInStmt(deferStmt->body, paramNames, typeArgs);
    }
    // TryStmt
    else if (auto tryStmt = std::dynamic_pointer_cast<TryStmt>(stmt)) {
        if (tryStmt->tryBlock) substituteTypeInStmt(tryStmt->tryBlock, paramNames, typeArgs);
        for (auto& catchBlock : tryStmt->catchBlocks) {
            if (catchBlock.second) substituteTypeInStmt(catchBlock.second, paramNames, typeArgs);
        }
        if (tryStmt->finallyBlock) substituteTypeInStmt(tryStmt->finallyBlock, paramNames, typeArgs);
    }
    // SwitchStmt
    else if (auto switchStmt = std::dynamic_pointer_cast<SwitchStmt>(stmt)) {
        for (auto& case_ : switchStmt->cases) {
            if (case_.second) substituteTypeInStmt(case_.second, paramNames, typeArgs);
        }
        if (switchStmt->defaultCase) substituteTypeInStmt(switchStmt->defaultCase, paramNames, typeArgs);
    }
    // DoWhileStmt
    else if (auto dw = std::dynamic_pointer_cast<DoWhileStmt>(stmt)) {
        if (dw->body) substituteTypeInStmt(dw->body, paramNames, typeArgs);
    }
}

void SemanticAnalyzer::substituteTypeInExpr(Shared<Expr> expr,
                                            const std::vector<String>& paramNames,
                                            const std::vector<String>& typeArgs) {
    if (!expr) return;

    // BinaryExpr: 替换左右操作数
    if (auto bin = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
        substituteTypeInExpr(bin->left, paramNames, typeArgs);
        substituteTypeInExpr(bin->right, paramNames, typeArgs);
    }
    // UnaryExpr
    else if (auto unary = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
        substituteTypeInExpr(unary->operand, paramNames, typeArgs);
    }
    // CallExpr: 替换被调用者和参数
    else if (auto call = std::dynamic_pointer_cast<CallExpr>(expr)) {
        substituteTypeInExpr(call->callee, paramNames, typeArgs);
        for (auto& arg : call->arguments) {
            substituteTypeInExpr(arg, paramNames, typeArgs);
        }
    }
    // MemberExpr
    else if (auto member = std::dynamic_pointer_cast<MemberExpr>(expr)) {
        substituteTypeInExpr(member->object, paramNames, typeArgs);
    }
    // IndexExpr
    else if (auto index = std::dynamic_pointer_cast<IndexExpr>(expr)) {
        substituteTypeInExpr(index->array, paramNames, typeArgs);
        substituteTypeInExpr(index->index, paramNames, typeArgs);
    }
    // StructLiteralExpr: 替换字段值
    else if (auto structLit = std::dynamic_pointer_cast<StructLiteralExpr>(expr)) {
        for (auto& field : structLit->fields) {
            substituteTypeInExpr(field.second, paramNames, typeArgs);
        }
    }
    // NewExpr
    else if (auto newExpr = std::dynamic_pointer_cast<NewExpr>(expr)) {
        for (auto& arg : newExpr->args) {
            substituteTypeInExpr(arg, paramNames, typeArgs);
        }
    }
    // ArrayExpr
    else if (auto arr = std::dynamic_pointer_cast<ArrayExpr>(expr)) {
        for (auto& elem : arr->elements) {
            substituteTypeInExpr(elem, paramNames, typeArgs);
        }
    }
    // AwaitExpr
    else if (auto awaitE = std::dynamic_pointer_cast<AwaitExpr>(expr)) {
        substituteTypeInExpr(awaitE->target, paramNames, typeArgs);
    }
    // NewExpr
    else if (auto newE = std::dynamic_pointer_cast<NewExpr>(expr)) {
        for (auto& arg : newE->args) {
            substituteTypeInExpr(arg, paramNames, typeArgs);
        }
    }
    // ThisExpr, SuperExpr — 无子表达式，无需处理
    // 字面量、标识符 — 不包含类型引用，不需要替换
}

Shared<FuncDeclStmt> SemanticAnalyzer::instantiateGenericFunc(
    Shared<FuncDeclStmt> genericFunc,
    const std::vector<String>& typeArgs)
{
    // 检查类型参数约束
    checkTypeConstraints(genericFunc->typeParams, typeArgs, genericFunc->name, 0, 0);

    // 克隆函数声明
    auto instantiated = Shared<FuncDeclStmt>(new FuncDeclStmt());
    instantiated->name = buildMangledName(genericFunc->name, typeArgs);
    instantiated->returnType = genericFunc->returnType;
    instantiated->isStatic = genericFunc->isStatic;
    instantiated->isVirtual = genericFunc->isVirtual;

    // 复制参数（需要替换类型）
    for (auto& param : genericFunc->params) {
        String paramName = param.first;
        Optional<String> paramType = param.second;
        instantiated->params.push_back({paramName, paramType});
    }

    // 复制函数体（共享指针，后续替换类型）
    if (genericFunc->body) {
        auto newBody = Shared<BlockStmt>(new BlockStmt());
        newBody->statements = genericFunc->body->statements;  // 浅拷贝
        instantiated->body = newBody;
    }

    // 在参数类型中替换类型参数
    for (size_t i = 0; i < genericFunc->typeParams.size() && i < typeArgs.size(); i++) {
        const String& paramName = genericFunc->typeParams[i].name;
        const String& concreteType = typeArgs[i];

        // 替换参数类型
        for (auto& param : instantiated->params) {
            if (param.second.has_value() && param.second.value() == paramName) {
                param.second = concreteType;
            }
        }
        // 替换返回类型
        if (instantiated->returnType.has_value() && instantiated->returnType.value() == paramName) {
            instantiated->returnType = concreteType;
        }
        // 替换函数体中的类型注解
        if (instantiated->body) {
            for (auto& stmt : instantiated->body->statements) {
                std::vector<String> pNames = {paramName};
                std::vector<String> cTypes = {concreteType};
                substituteTypeInStmt(stmt, pNames, cTypes);
            }
        }
    }

    // 清空泛型参数（已实例化）
    instantiated->typeParams.clear();

    return instantiated;
}

// ═══════════════════════════════════════════════════════════════════
//  泛型结构体实例化
// ═══════════════════════════════════════════════════════════════════

bool SemanticAnalyzer::parseGenericTypeName(const String& fullName,
                                            String& baseName,
                                            std::vector<String>& typeArgs)
{
    // 解析 "列表<整数, 字符串>" → baseName="列表", typeArgs=["整数","字符串"]
    // 支持嵌套: "列表<对<整数, 字符串>>" → baseName="列表", typeArgs=["对<整数, 字符串>"]
    auto pos = fullName.find('<');
    if (pos == String::npos) return false;

    baseName = fullName.substr(0, pos);
    String argsPart = fullName.substr(pos + 1);

    // 去掉尾部的 '>'（匹配最外层的 <）
    auto endPos = argsPart.find_last_of('>');
    if (endPos == String::npos) return false;
    argsPart = argsPart.substr(0, endPos);

    // 按逗号分割类型参数，感知角括号嵌套深度
    size_t start = 0;
    int depth = 0;
    for (size_t i = 0; i <= argsPart.size(); i++) {
        if (i == argsPart.size() || (argsPart[i] == ',' && depth == 0)) {
            // 截取一个类型参数
            String typeArg = argsPart.substr(start, i - start);
            // 去空格
            while (!typeArg.empty() && typeArg.front() == ' ') typeArg.erase(typeArg.begin());
            while (!typeArg.empty() && typeArg.back() == ' ') typeArg.pop_back();
            if (!typeArg.empty()) {
                typeArgs.push_back(typeArg);
            }
            start = i + 1;
        } else if (argsPart[i] == '<') {
            depth++;
        } else if (argsPart[i] == '>') {
            depth--;
        }
    }

    return !typeArgs.empty();
}

StructInfo* SemanticAnalyzer::ensureGenericStructInstantiated(
    const String& fullName,
    const String& baseName,
    const std::vector<String>& typeArgs)
{
    // 检查缓存
    auto cacheIt = monomorphStructCache_.find(fullName);
    if (cacheIt != monomorphStructCache_.end()) {
        return cacheIt->second;
    }

    // 查找基础结构体信息
    StructInfo* baseInfo = getStructInfo(baseName);
    if (!baseInfo) return nullptr;

    // 查找基础结构体声明
    auto baseSym = lookup(baseName);
    if (!baseSym || !baseSym->node) return nullptr;
    auto stDecl = std::dynamic_pointer_cast<StructDeclStmt>(baseSym->node);
    if (!stDecl) return nullptr;

    // 检查类型参数约束
    checkTypeConstraints(stDecl->typeParams, typeArgs, baseName, 0, 0);

    // 创建实例化后的 StructInfo
    StructInfo* instInfo = new StructInfo();
    instInfo->name = fullName;
    instInfo->type = TypeRegistry::instance().getCustomType(fullName);
    instInfo->type->kind = BuiltinType::STRUCT;

    // 遍历基础结构体的字段，替换类型参数
    size_t offset = 0;
    for (auto& member : stDecl->members) {
        if (auto var = std::dynamic_pointer_cast<VarDeclStmt>(member)) {
            StructField field;
            field.name = var->name;
            field.offset = offset;

            // 确定字段类型（替换类型参数）
            String fieldTypeStr;
            if (var->type.has_value()) {
                fieldTypeStr = var->type.value();
                // 替换类型参数
                for (size_t i = 0; i < typeArgs.size(); i++) {
                    if (fieldTypeStr == stDecl->typeParams[i].name) {
                        fieldTypeStr = typeArgs[i];
                        break;
                    }
                }
                field.type = getTypeFromString(fieldTypeStr);
            } else {
                field.type = Type::unknown();
            }

            // 注册字段
            instInfo->fieldIndex[var->name] = instInfo->fields.size();
            instInfo->fields.push_back(field);

            // 更新偏移（简化：所有类型按8字节对齐）
            offset += 8;
        }
    }
    instInfo->size = offset;

    // 注册到类型表
    structTable_[fullName] = instInfo;
    monomorphStructCache_[fullName] = instInfo;

    return instInfo;
}

} // namespace cplang