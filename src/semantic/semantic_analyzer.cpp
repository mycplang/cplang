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
            info->type->kind = BuiltinType::OBJECT;
            info->typeParams = cls->typeParams;
            info->isGeneric = !cls->typeParams.empty();
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
        else if (auto exp = std::dynamic_pointer_cast<ExportStmt>(stmt)) {
            // export 声明：递归收集内部声明
            if (exp->kind == ExportStmt::Kind::DECLARATION && exp->declaration) {
                // 将内部声明作为当前语句重新处理
                auto& innerStmt = exp->declaration;
                if (auto func = std::dynamic_pointer_cast<FuncDeclStmt>(innerStmt)) {
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
                else if (auto cls = std::dynamic_pointer_cast<ClassDeclStmt>(innerStmt)) {
                    auto info = new ClassInfo();
                    info->name = cls->name;
                    info->type = TypeRegistry::instance().getCustomType(cls->name);
                    info->type->kind = BuiltinType::OBJECT;
                    info->typeParams = cls->typeParams;
                    info->isGeneric = !cls->typeParams.empty();
                    if (cls->baseClass.has_value()) {
                        info->baseClass = getClassInfo(*cls->baseClass);
                    }
                    classTable_[cls->name] = info;
                    
                    auto sym = new Symbol();
                    sym->kind = Symbol::CLASS;
                    sym->name = cls->name;
                    sym->node = innerStmt;
                    sym->classType = info->type;
                    defineSymbol(sym);
                }
                else if (auto st = std::dynamic_pointer_cast<StructDeclStmt>(innerStmt)) {
                    auto info = new StructInfo();
                    info->name = st->name;
                    info->type = TypeRegistry::instance().getCustomType(st->name);
                    info->type->kind = BuiltinType::STRUCT;
                    structTable_[st->name] = info;
                    
                    auto sym = new Symbol();
                    sym->kind = Symbol::TYPE_ALIAS;
                    sym->name = st->name;
                    sym->type = info->type;
                    sym->node = innerStmt;
                    defineSymbol(sym);
                }
                // 注意：变量声明不在 PASS 1 中收集，由 PASS 2 的 analyzeVarDecl 处理
            }
        }
        else if (auto dec = std::dynamic_pointer_cast<DecoratorStmt>(stmt)) {
            // 装饰器声明：穿透装饰器，收集内部声明
            if (dec->declaration) {
                auto& innerStmt = dec->declaration;
                if (auto func = std::dynamic_pointer_cast<FuncDeclStmt>(innerStmt)) {
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
                else if (auto cls = std::dynamic_pointer_cast<ClassDeclStmt>(innerStmt)) {
                    auto info = new ClassInfo();
                    info->name = cls->name;
                    info->type = TypeRegistry::instance().getCustomType(cls->name);
                    info->type->kind = BuiltinType::OBJECT;
                    if (cls->baseClass.has_value()) {
                        info->baseClass = getClassInfo(*cls->baseClass);
                    }
                    classTable_[cls->name] = info;
                    
                    auto sym = new Symbol();
                    sym->kind = Symbol::CLASS;
                    sym->name = cls->name;
                    sym->node = innerStmt;
                    sym->classType = info->type;
                    defineSymbol(sym);
                }
                else if (auto st = std::dynamic_pointer_cast<StructDeclStmt>(innerStmt)) {
                    auto info = new StructInfo();
                    info->name = st->name;
                    info->type = TypeRegistry::instance().getCustomType(st->name);
                    info->type->kind = BuiltinType::STRUCT;
                    structTable_[st->name] = info;
                    
                    auto sym = new Symbol();
                    sym->kind = Symbol::TYPE_ALIAS;
                    sym->name = st->name;
                    sym->type = info->type;
                    sym->node = innerStmt;
                    defineSymbol(sym);
                }
            }
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
        else if (auto dd = std::dynamic_pointer_cast<DestructuringDecl>(stmt)) {
            analyzeDestructuringDecl(dd);
        }
        else if (auto imp = std::dynamic_pointer_cast<ImportStmt>(stmt)) {
            // import 分析
        }
        else if (auto exp = std::dynamic_pointer_cast<ExportStmt>(stmt)) {
            // export 分析：递归分析内部声明
            if (exp->kind == ExportStmt::Kind::DECLARATION && exp->declaration) {
                auto& innerStmt = exp->declaration;
                if (auto func = std::dynamic_pointer_cast<FuncDeclStmt>(innerStmt)) {
                    analyzeFuncDecl(func);
                }
                else if (auto cls = std::dynamic_pointer_cast<ClassDeclStmt>(innerStmt)) {
                    analyzeClassDecl(cls);
                }
                else if (auto iface = std::dynamic_pointer_cast<InterfaceDeclStmt>(innerStmt)) {
                    analyzeInterfaceDecl(iface);
                }
                else if (auto enm = std::dynamic_pointer_cast<EnumDeclStmt>(innerStmt)) {
                    analyzeEnumDecl(enm);
                }
                else if (auto st = std::dynamic_pointer_cast<StructDeclStmt>(innerStmt)) {
                    analyzeStructDecl(st);
                }
                else if (auto var = std::dynamic_pointer_cast<VarDeclStmt>(innerStmt)) {
                    analyzeVarDecl(var);
                }
            }
        }
        else if (auto dec = std::dynamic_pointer_cast<DecoratorStmt>(stmt)) {
            // 装饰器分析：递归分析内部声明
            if (dec->declaration) {
                auto& innerStmt = dec->declaration;
                if (auto func = std::dynamic_pointer_cast<FuncDeclStmt>(innerStmt)) {
                    analyzeFuncDecl(func);
                }
                else if (auto cls = std::dynamic_pointer_cast<ClassDeclStmt>(innerStmt)) {
                    analyzeClassDecl(cls);
                }
                else if (auto iface = std::dynamic_pointer_cast<InterfaceDeclStmt>(innerStmt)) {
                    analyzeInterfaceDecl(iface);
                }
                else if (auto enm = std::dynamic_pointer_cast<EnumDeclStmt>(innerStmt)) {
                    analyzeEnumDecl(enm);
                }
                else if (auto st = std::dynamic_pointer_cast<StructDeclStmt>(innerStmt)) {
                    analyzeStructDecl(st);
                }
                else if (auto var = std::dynamic_pointer_cast<VarDeclStmt>(innerStmt)) {
                    analyzeVarDecl(var);
                }
            }
            // 注意：装饰器函数本身的存在性检查在运行时进行（动态语言特性）
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
    
    // 联合类型（最低优先级，先检查）
    if (typeStr.find(" | ") != String::npos) {
        std::vector<String> parts;
        size_t start = 0;
        size_t pos = 0;
        while ((pos = typeStr.find(" | ", start)) != String::npos) {
            parts.push_back(typeStr.substr(start, pos - start));
            start = pos + 3;  // skip " | "
        }
        parts.push_back(typeStr.substr(start));
        
        std::vector<Type*> memberTypes;
        for (auto& p : parts) {
            memberTypes.push_back(getTypeFromString(p));
        }
        return TypeRegistry::instance().getUnionType(memberTypes);
    }
    
    // 可选类型（后缀 ?）
    // 注意：只处理末尾的单个 ?，递归处理嵌套可选
    if (!typeStr.empty() && typeStr.back() == '?') {
        String innerStr = typeStr.substr(0, typeStr.size() - 1);
        Type* innerType = getTypeFromString(innerStr);
        return TypeRegistry::instance().getOptionalType(innerType);
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
    
    // 泛型（如 列表<整数>、字典<字符串, i64>、栈<整数>）
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
            // 检查是否为泛型类，如果是则确保已实例化
            auto classInfo = getClassInfo(baseName);
            if (classInfo && classInfo->isGeneric) {
                ClassInfo* instClass = ensureGenericClassInstantiated(typeStr, baseName, typeArgs);
                if (instClass) {
                    return instClass->type;
                }
            }
        }
        return TypeRegistry::instance().getCustomType(typeStr);
    }
    
    // 可能是自定义类型
    auto sym = lookup(typeStr);
    if (sym && sym->kind == Symbol::CLASS) {
        return sym->classType ? sym->classType : sym->type;
    }
    if (sym && sym->kind == Symbol::TYPE_ALIAS) {
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
    
    // 可选类型赋值：T 可以赋值给 T?
    if (to->kind == BuiltinType::OPTIONAL && to->innerType) {
        if (isAssignableTo(from, to->innerType)) {
            return true;
        }
        // from 也是可选类型，检查内部类型
        if (from->kind == BuiltinType::OPTIONAL && from->innerType) {
            return isAssignableTo(from->innerType, to->innerType);
        }
    }
    
    // 联合类型赋值：
    // - 如果目标是联合类型，检查 from 是否是其中一个成员
    if (to->kind == BuiltinType::UNION) {
        for (auto* member : to->generics) {
            if (isAssignableTo(from, member)) {
                return true;
            }
        }
    }
    // - 如果 from 是联合类型，检查每个成员是否都能赋值给 to
    if (from->kind == BuiltinType::UNION) {
        bool allAssignable = true;
        for (auto* member : from->generics) {
            if (!isAssignableTo(member, to)) {
                allAssignable = false;
                break;
            }
        }
        if (allAssignable) return true;
    }
    
    // 同类型直接相等
    if (from->equals(to)) return true;
    
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
        // 压入异步/生成器上下文标记
        asyncFuncStack_.push_back(func->isAsync);
        generatorFuncStack_.push_back(func->isGenerator);
        
        Type* retType = analyzeBlock(func->body);
        
        // 弹出上下文标记
        asyncFuncStack_.pop_back();
        generatorFuncStack_.pop_back();
        
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
        info->type->kind = BuiltinType::OBJECT;
        info->typeParams = cls->typeParams;
        info->isGeneric = !cls->typeParams.empty();
        info->interfaces = cls->interfaces;  // 填充实现的接口列表
        classTable_[cls->name] = info;
    }
    
    enterClass(info);
    
    // 为泛型类创建类型参数作用域
    if (!cls->typeParams.empty()) {
        pushScope(cls->name + "<type-params>");
        for (const auto& tp : cls->typeParams) {
            auto typeParamSym = new Symbol();
            typeParamSym->kind = Symbol::TYPE_ALIAS;
            typeParamSym->name = tp.name;
            typeParamSym->type = Type::unknown();
            defineSymbol(typeParamSym);
        }
    }
    
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
            fieldSym->isStatic = var->isStatic;
            fieldSym->isPublic = true; // 默认公有
            info->fieldTable[var->name] = fieldSym;
            info->fields.push_back(fieldSym);
            
            // 如果有初始化表达式
            if (var->init) {
                analyzeExpr(var->init);
            }
        }
        else if (auto func = std::dynamic_pointer_cast<FuncDeclStmt>(member)) {
            // 先创建方法符号并注册到作用域（analyzeFuncDecl 中需要通过 lookup 找到它）
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
            defineSymbol(methodSym);
            info->methodTable[func->name] = methodSym;
            info->methods.push_back(methodSym);
            
            // 注入 self 参数（非静态方法），用于方法体内的类型推断
            if (!func->isStatic) {
                func->params.insert(func->params.begin(), {"self", info->name});
            }
            
            // 再分析方法体
            analyzeFuncDecl(func);
            
            // 移除注入的 self 参数（保持 AST 原样）
            if (!func->isStatic) {
                func->params.erase(func->params.begin());
            }
        }
    }
    
    // 弹出泛型类型参数作用域
    if (!cls->typeParams.empty()) {
        popScope();
    }
    
    // 检查接口实现
    checkInterfaceImplementation(info);
    
    leaveClass();
}

void SemanticAnalyzer::analyzeInterfaceDecl(Shared<InterfaceDeclStmt> iface) {
    // 创建接口信息
    InterfaceInfo* info = getInterfaceInfo(iface->name);
    if (!info) {
        info = new InterfaceInfo();
        info->name = iface->name;
        info->typeParams = iface->typeParams;
        info->isGeneric = !iface->typeParams.empty();
        info->baseInterfaces = iface->baseInterfaces;  // 填充继承的接口
        interfaceTable_[iface->name] = info;
    }
    
    // 注册继承的接口方法（接口继承）
    for (auto& baseName : iface->baseInterfaces) {
        auto baseIface = getInterfaceInfo(baseName);
        if (baseIface) {
            // 继承基接口的所有方法
            for (auto& method : baseIface->methods) {
                if (info->methodTable.find(method->name) == info->methodTable.end()) {
                    auto sym = new Symbol(*method);  // 复制方法符号
                    info->methods.push_back(sym);
                    info->methodTable[method->name] = sym;
                }
            }
        }
    }
    
    // 注册接口方法
    for (auto& method : iface->methods) {
        if (auto func = std::dynamic_pointer_cast<FuncDeclStmt>(method)) {
            auto sym = new Symbol();
            sym->kind = Symbol::FUNC;  // METHOD not in Kind enum
            sym->name = func->name;
            sym->type = Type::void_();
            
            // 参数类型
            for (auto& p : func->params) {
                Type* pt = p.second.has_value() 
                    ? getTypeFromString(*p.second) 
                    : Type::unknown();
                sym->params.push_back({p.first, pt});
            }
            
            // 返回类型
            if (func->returnType.has_value()) {
                sym->returnType = getTypeFromString(*func->returnType);
            }
            
            // 如果方法已存在（从基接口继承），则覆盖
            auto it = info->methodTable.find(func->name);
            if (it != info->methodTable.end()) {
                // 找到并替换
                for (auto it2 = info->methods.begin(); it2 != info->methods.end(); ++it2) {
                    if ((*it2)->name == func->name) {
                        delete *it2;
                        *it2 = sym;
                        break;
                    }
                }
                info->methodTable[func->name] = sym;
            } else {
                info->methods.push_back(sym);
                info->methodTable[func->name] = sym;
            }
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

void SemanticAnalyzer::analyzeDestructuringDecl(Shared<DestructuringDecl> dd) {
    // 分析右侧表达式
    Type* initType = nullptr;
    if (dd->init) {
        initType = analyzeExpr(dd->init);
    }
    
    // 为每个变量注册符号
    for (const auto& name : dd->names) {
        auto sym = new Symbol();
        sym->kind = dd->isConst ? Symbol::CONST : Symbol::VAR;
        sym->name = name;
        // 如果是数组解构且知道元素类型，使用元素类型
        if (initType && initType->kind == BuiltinType::ARRAY && initType->innerType) {
            sym->type = initType->innerType;
        } else {
            sym->type = Type::unknown();
        }
        sym->isConst = dd->isConst;
        defineSymbol(sym);
    }
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
    if (auto typeAlias = std::dynamic_pointer_cast<TypeAliasStmt>(stmt)) {
        // 类型别名声明
        Type* targetType = getTypeFromString(typeAlias->targetType);
        auto sym = new Symbol();
        sym->kind = Symbol::TYPE_ALIAS;
        sym->name = typeAlias->name;
        sym->type = targetType;
        sym->node = typeAlias;
        defineSymbol(sym);
        return Type::void_();
    }
    if (auto tryStmt = std::dynamic_pointer_cast<TryStmt>(stmt)) {
        // 分析 try 块
        if (tryStmt->tryBlock) {
            analyzeStmt(tryStmt->tryBlock);
        }
        // B1: 分析 catch 块（支持类型化catch）
        for (auto& cb : tryStmt->catchBlocks) {
            pushScope();
            if (!cb.varName.empty()) {
                auto sym = new Symbol();
                sym->kind = Symbol::VAR;
                sym->name = cb.varName;
                // 如果有异常类型，尝试获取该类型
                if (!cb.exceptionType.empty()) {
                    Type* exType = getTypeFromString(cb.exceptionType);
                    sym->type = exType ? exType : Type::unknown();
                } else {
                    sym->type = Type::unknown();
                }
                sym->isConst = true;
                defineSymbol(sym);
            }
            if (cb.body) {
                analyzeStmt(cb.body);
            }
            popScope();
        }
        // 分析 finally 块
        if (tryStmt->finallyBlock) {
            analyzeStmt(tryStmt->finallyBlock);
        }
        return Type::void_();
    }
    if (auto withStmt = std::dynamic_pointer_cast<WithStmt>(stmt)) {
        // 分析资源表达式
        if (withStmt->expr) {
            analyzeExpr(withStmt->expr);
        }
        // with 体在新作用域中分析，资源变量在该作用域内可见
        pushScope();
        auto sym = new Symbol();
        sym->kind = Symbol::VAR;
        sym->name = withStmt->varName;
        sym->type = Type::unknown();
        sym->isConst = false;
        defineSymbol(sym);
        if (withStmt->body) {
            analyzeStmt(withStmt->body);
        }
        popScope();
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
    if (auto yld = std::dynamic_pointer_cast<YieldStmt>(stmt)) {
        // 检查：yield 只能在生成器函数内使用
        if (!isInGeneratorFunc()) {
            reportError(yld->token.line, yld->token.column,
                "'产出' / 'yield' 只能在生成器函数内使用");
        }
        // 分析 yield 的值表达式
        if (yld->value) {
            analyzeExpr(yld->value);
        }
        return Type::void_();
    }
    if (auto go = std::dynamic_pointer_cast<GoStmt>(stmt)) {
        // 分析 go 后面的表达式（通常是函数调用）
        if (go->expr) {
            analyzeExpr(go->expr);
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
        // defer 已在 transformDeferInBlock 中处理，跳过
        if (std::dynamic_pointer_cast<DeferStmt>(s)) continue;
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
    
    // ═══════════════════════════════════════════════════
    //  类型窄化：if (x 属于 T) { ... }
    // ═══════════════════════════════════════════════════
    String narrowedVarName;
    Type* narrowedVarOrigType = nullptr;
    Symbol* narrowedSymbol = nullptr;
    Type* narrowedType = nullptr;
    bool didNarrow = false;
    
    // 检查是否是类型守卫表达式 (is / 属于)
    if (auto isExpr = std::dynamic_pointer_cast<IsExpr>(s->condition)) {
        // 检查左侧是否是标识符
        if (auto idExpr = std::dynamic_pointer_cast<IdentifierExpr>(isExpr->expr)) {
            narrowedVarName = idExpr->name;
            narrowedSymbol = lookup(narrowedVarName);
            if (narrowedSymbol && narrowedSymbol->type) {
                narrowedVarOrigType = narrowedSymbol->type;
                // 解析要窄化到的目标类型
                narrowedType = getTypeFromString(isExpr->checkType);
                
                if (narrowedType) {
                    // 执行类型窄化
                    narrowedSymbol->type = narrowedType;
                    didNarrow = true;
                }
            }
        }
    }
    
    Type* thenType = analyzeStmt(s->thenBranch);
    
    // 恢复窄化前的类型
    if (didNarrow && narrowedSymbol) {
        narrowedSymbol->type = narrowedVarOrigType;
        didNarrow = false;
    }
    
    Type* elseType = nullptr;
    if (s->elseBranch) {
        // else 分支的反向窄化：如果原类型是联合类型，排除检查的类型
        if (narrowedSymbol && narrowedVarOrigType && narrowedType &&
            narrowedVarOrigType->kind == BuiltinType::UNION) {
            // 从联合类型中移除检查的类型
            std::vector<Type*> remaining;
            for (auto* member : narrowedVarOrigType->generics) {
                if (!member->equals(narrowedType)) {
                    remaining.push_back(member);
                }
            }
            if (!remaining.empty()) {
                Type* elseNarrowedType = nullptr;
                if (remaining.size() == 1) {
                    elseNarrowedType = remaining[0];
                } else {
                    elseNarrowedType = TypeRegistry::instance().getUnionType(remaining);
                }
                narrowedSymbol->type = elseNarrowedType;
                didNarrow = true;
            }
        }
        
        elseType = analyzeStmt(s->elseBranch);
        
        // 恢复窄化前的类型
        if (didNarrow && narrowedSymbol) {
            narrowedSymbol->type = narrowedVarOrigType;
        }
    }
    
    // if-else 类型推导：如果有 else 分支，推导为两个分支的统一类型
    if (s->elseBranch && elseType) {
        // 如果两个分支类型相同，返回该类型
        if (thenType && elseType && thenType->equals(elseType)) {
            return thenType;
        }
        // 如果其中一个是 void，返回另一个（处理只有一个分支有值的情况）
        if (thenType && thenType->kind != BuiltinType::VOID && 
            elseType->kind == BuiltinType::VOID) {
            return thenType;
        }
        if (elseType && elseType->kind != BuiltinType::VOID && 
            thenType->kind == BuiltinType::VOID) {
            return elseType;
        }
        // 类型不同，返回 unknown
        if (thenType && elseType && thenType->kind != BuiltinType::VOID &&
            elseType->kind != BuiltinType::VOID) {
            return Type::unknown();
        }
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

    // 用于穷尽性检查：收集已覆盖的变体
    std::unordered_map<String, bool> coveredVariants;
    bool hasWildcard = false;
    bool hasDefault = (s->defaultCase != nullptr);
    
    // 用于类型推导：收集所有分支的返回类型
    Type* resultType = nullptr;
    bool allSameType = true;
    bool hasBodyType = false;

    // 分析每个分支
    for (auto& mc : s->cases) {
        Type* branchType = Type::void_();
        
        if (mc.kind == PatternKind::PATTERN_VARIANT) {
            // 变体模式
            // 查找变体符号
            Symbol* variantSym = lookup(mc.variantName);
            
            // A1.1: 如果不是枚举变体且没有绑定参数，则降级为绑定模式
            if ((!variantSym || variantSym->kind != Symbol::ENUM_VARIANT) && mc.bindings.empty()) {
                // 降级为绑定模式
                mc.kind = PatternKind::PATTERN_BINDING;
                mc.bindingName = mc.variantName;
                mc.variantName = "";
                
                // 按绑定模式处理
                hasWildcard = true;
                
                // A1.3: 注册绑定变量到作用域
                pushScope();
                auto bindSym = new Symbol();
                bindSym->kind = Symbol::VAR;
                bindSym->name = mc.bindingName;
                bindSym->type = matchType;
                defineSymbol(bindSym);
                
                // A2.2: 分析守卫条件（可以引用绑定变量）
                if (mc.guard) {
                    Type* guardType = analyzeExpr(mc.guard);
                    if (guardType && guardType->kind != BuiltinType::BOOL) {
                        reportError(s->token.line, s->token.column,
                            "守卫条件必须是布尔类型");
                    }
                }
                
                // 分析分支体
                if (mc.body) {
                    branchType = analyzeStmt(mc.body);
                }
                popScope();
            }
            else if (variantSym && variantSym->kind == Symbol::ENUM_VARIANT) {
                // 真正的变体模式
                // 标记已覆盖
                coveredVariants[mc.variantName] = true;

                // 检查绑定数量是否匹配变体字段数量
                if (mc.bindings.size() != variantSym->params.size()) {
                    reportError(s->token.line, s->token.column,
                        "变体 '" + mc.variantName + "' 需要 " +
                        std::to_string(variantSym->params.size()) +
                        " 个绑定，但提供了 " + std::to_string(mc.bindings.size()));
                }

                // A1.3: 在分支作用域中注册绑定变量
                pushScope();
                for (size_t i = 0; i < mc.bindings.size() && i < variantSym->params.size(); i++) {
                    auto bindSym = new Symbol();
                    bindSym->kind = Symbol::VAR;
                    bindSym->name = mc.bindings[i];
                    bindSym->type = variantSym->params[i].second;
                    defineSymbol(bindSym);
                }

                // A2.2: 分析守卫条件（可以引用绑定变量）
                if (mc.guard) {
                    Type* guardType = analyzeExpr(mc.guard);
                    if (guardType && guardType->kind != BuiltinType::BOOL) {
                        reportError(s->token.line, s->token.column,
                            "守卫条件必须是布尔类型");
                    }
                }

                // 分析分支体
                if (mc.body) {
                    branchType = analyzeStmt(mc.body);
                }
                popScope();
            }
            else {
                // 有绑定参数但不是变体，报错
                reportError(s->token.line, s->token.column,
                    "未定义的枚举变体: '" + mc.variantName + "'");
                continue;
            }
        }
        else if (mc.kind == PatternKind::PATTERN_LITERAL) {
            // 字面量模式：直接分析分支体
            hasWildcard = true;  // 字面量模式也视为覆盖了可能的值
            
            pushScope();
            
            // A2.2: 分析守卫条件
            if (mc.guard) {
                Type* guardType = analyzeExpr(mc.guard);
                if (guardType && guardType->kind != BuiltinType::BOOL) {
                    reportError(s->token.line, s->token.column,
                        "守卫条件必须是布尔类型");
                }
            }
            
            if (mc.body) {
                branchType = analyzeStmt(mc.body);
            }
            popScope();
        }
        else if (mc.kind == PatternKind::PATTERN_WILDCARD) {
            // 通配符模式：匹配所有值
            hasWildcard = true;
            
            pushScope();
            
            // A2.2: 分析守卫条件
            if (mc.guard) {
                Type* guardType = analyzeExpr(mc.guard);
                if (guardType && guardType->kind != BuiltinType::BOOL) {
                    reportError(s->token.line, s->token.column,
                        "守卫条件必须是布尔类型");
                }
            }
            
            if (mc.body) {
                branchType = analyzeStmt(mc.body);
            }
            popScope();
        }
        else if (mc.kind == PatternKind::PATTERN_BINDING) {
            // 绑定模式
            hasWildcard = true;
            // A1.3: 注册绑定变量到新作用域
            pushScope();
            auto bindSym = new Symbol();
            bindSym->kind = Symbol::VAR;
            bindSym->name = mc.bindingName;
            bindSym->type = matchType;
            defineSymbol(bindSym);
            
            // A2.2: 分析守卫条件（可以引用绑定变量）
            if (mc.guard) {
                Type* guardType = analyzeExpr(mc.guard);
                if (guardType && guardType->kind != BuiltinType::BOOL) {
                    reportError(s->token.line, s->token.column,
                        "守卫条件必须是布尔类型");
                }
            }
            
            // 分析分支体
            if (mc.body) {
                branchType = analyzeStmt(mc.body);
            }
            popScope();
        }
        else if (mc.kind == PatternKind::PATTERN_ARRAY) {
            // A3.1: 数组/元组模式
            hasWildcard = true;
            
            pushScope();
            
            // 注册数组元素绑定变量
            // 数组元素类型：如果匹配类型是数组，则用元素类型；否则用动态类型
            Type* elemType = Type::int_();  // 默认，实际运行时确定
            if (matchType->kind == BuiltinType::ARRAY) {
                // 数组类型：获取元素类型
                // elemType = matchType->elementType; // elementType not in Type
            }
            
            for (auto& bindingName : mc.arrayBindings) {
                if (bindingName != "_") {  // _ 是通配符，不绑定
                    auto bindSym = new Symbol();
                    bindSym->kind = Symbol::VAR;
                    bindSym->name = bindingName;
                    bindSym->type = elemType;
                    defineSymbol(bindSym);
                }
            }
            
            // A2.2: 分析守卫条件
            if (mc.guard) {
                Type* guardType = analyzeExpr(mc.guard);
                if (guardType && guardType->kind != BuiltinType::BOOL) {
                    reportError(s->token.line, s->token.column,
                        "守卫条件必须是布尔类型");
                }
            }
            
            // 分析分支体
            if (mc.body) {
                branchType = analyzeStmt(mc.body);
            }
            popScope();
        }
        
        // 收集分支类型用于类型推导
        if (branchType && branchType->kind != BuiltinType::VOID) {
            if (!resultType) {
                resultType = branchType;
                hasBodyType = true;
            } else if (!resultType->equals(branchType)) {
                allSameType = false;
            }
        }
    }

    // 分析默认分支
    Type* defaultType = Type::void_();
    if (s->defaultCase) {
        defaultType = analyzeStmt(s->defaultCase);
    }
    
    // 收集默认分支类型
    if (defaultType && defaultType->kind != BuiltinType::VOID) {
        if (!resultType) {
            resultType = defaultType;
            hasBodyType = true;
        } else if (!resultType->equals(defaultType)) {
            allSameType = false;
        }
    }

    // 穷尽性检查（仅针对枚举类型）
    if (enumSym && !hasWildcard && !hasDefault) {
        // 检查是否覆盖了所有变体
        // 获取枚举的所有变体
        auto enumDecl = std::dynamic_pointer_cast<EnumDeclStmt>(enumSym->node);
        if (enumDecl && enumDecl->isADT) {
            for (const auto& variant : enumDecl->variants) {
                if (coveredVariants.find(variant.name) == coveredVariants.end()) {
                    reportError(s->token.line, s->token.column,
                        "匹配不完整：未覆盖变体 '" + variant.name + "'");
                }
            }
        }
    }

    // match 语句类型推导：如果所有分支类型相同且有默认分支或穷尽覆盖，推导为该类型
    if (hasBodyType && allSameType && (hasDefault || hasWildcard)) {
        return resultType;
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
        // 检查：await 只能在 async 函数内使用
        if (!isInAsyncFunc()) {
            reportError(awaitExpr->token.line, awaitExpr->token.column,
                "'等待' / 'await' 只能在异步函数内使用");
        }
        // await 表达式类型 = 目标操作的结果类型
        result = analyzeExpr(awaitExpr->target);
    }
    else if (auto isExpr = std::dynamic_pointer_cast<IsExpr>(expr)) {
        // is / 属于 类型守卫：返回布尔类型
        analyzeExpr(isExpr->expr);
        // 验证目标类型是否有效
        getTypeFromString(isExpr->checkType);
        result = Type::bool_();
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
        // super 调用：在父类继承链中查找方法
        auto* cls = currentClass();
        if (!cls || !cls->baseClass) {
            reportError(expr->token.line, expr->token.column,
                       "'继承' 只能在有父类的类方法中使用");
            result = Type::unknown();
        } else {
            // 沿继承链向上查找方法（从父类开始，不包含当前类）
            Symbol* methodSym = nullptr;
            ClassInfo* cur = cls->baseClass;
            while (cur) {
                auto it = cur->methodTable.find(superExpr->method);
                if (it != cur->methodTable.end()) {
                    methodSym = it->second;
                    break;
                }
                cur = cur->baseClass;
            }
            
            if (!methodSym) {
                reportError(expr->token.line, expr->token.column,
                           "父类中找不到方法 '" + superExpr->method + "'");
                result = Type::unknown();
            } else {
                result = methodSym->returnType ? methodSym->returnType : Type::unknown();
                
                // 检查参数数量（methodSym->params 不包含 self 参数）
                // 临时禁用参数检查，先验证 super 基本功能
                // size_t expectedArgs = methodSym->params.size();
                // size_t actualArgs = superExpr->arguments.size();
                // if (actualArgs != expectedArgs) {
                //     String errMsg = "方法 '" + superExpr->method + "' 需要 " + 
                //                    std::to_string(expectedArgs) + " 个参数，但提供了 " + 
                //                    std::to_string(actualArgs) + " 个";
                //     reportError(expr->token.line, expr->token.column, errMsg);
                // }
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
    else if (auto arr = std::dynamic_pointer_cast<ArrayExpr>(expr)) {
        // 数组字面量类型推导：
        // 1. 如果数组为空，返回 unknown[]
        // 2. 如果所有元素类型相同，返回该类型的数组
        // 3. 如果元素类型不同，返回 unknown[]
        Type* elemType = nullptr;
        bool allSame = true;
        
        for (auto& elem : arr->elements) {
            Type* t = analyzeExpr(elem);
            if (!elemType) {
                elemType = t;
            } else if (t && elemType && !elemType->equals(t)) {
                allSame = false;
            }
        }
        
        if (!elemType) {
            // 空数组
            result = TypeRegistry::instance().getArrayType(Type::unknown());
        } else if (allSame) {
            result = TypeRegistry::instance().getArrayType(elemType);
        } else {
            // 混合类型数组，降级为 unknown[]
            result = TypeRegistry::instance().getArrayType(Type::unknown());
        }
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
    else if (auto newExpr = std::dynamic_pointer_cast<NewExpr>(expr)) {
        // new 类名(参数)：返回类的实例类型
        for (auto& arg : newExpr->args) {
            analyzeExpr(arg);
        }
        
        // 如果是泛型类实例化，确保已实例化
        if (!newExpr->typeArgs.empty()) {
            // 构建完整的泛型类名
            String fullName = newExpr->className + "<";
            for (size_t i = 0; i < newExpr->typeArgs.size(); i++) {
                if (i > 0) fullName += ", ";
                fullName += newExpr->typeArgs[i];
            }
            fullName += ">";
            
            // 确保泛型类已实例化
            auto baseCls = getClassInfo(newExpr->className);
            if (baseCls && baseCls->isGeneric) {
                ClassInfo* instCls = ensureGenericClassInstantiated(
                    fullName, newExpr->className, newExpr->typeArgs);
                if (instCls && instCls->type) {
                    result = instCls->type;
                } else {
                    result = Type::unknown();
                }
            } else {
                result = Type::unknown();
            }
        } else {
            ClassInfo* cls = getClassInfo(newExpr->className);
            if (cls && cls->type) {
                result = cls->type;
            } else {
                result = Type::unknown();
            }
        }
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
        if (std::get_if<std::monostate>(&lit->value)) return Type::unknown();
        if (std::get_if<Int64>(&lit->value)) return Type::int_();
        if (std::get_if<Float64>(&lit->value)) return Type::float_();
        if (std::get_if<String>(&lit->value)) return Type::string_();
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
    
    // 6. NewExpr — 返回类的实例类型
    if (auto newExpr = std::dynamic_pointer_cast<NewExpr>(expr)) {
        if (!newExpr->typeArgs.empty()) {
            // 泛型类：构建完整类名并查找
            String fullName = newExpr->className + "<";
            for (size_t i = 0; i < newExpr->typeArgs.size(); i++) {
                if (i > 0) fullName += ", ";
                fullName += newExpr->typeArgs[i];
            }
            fullName += ">";
            ClassInfo* cls = getClassInfo(fullName);
            if (cls && cls->type) return cls->type;
        } else {
            ClassInfo* cls = getClassInfo(newExpr->className);
            if (cls && cls->type) return cls->type;
        }
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
    //  非泛型调用: 原有逻辑 + 类型推断
    // ════════════════════════════════════════════════════════
    Type* calleeType = analyzeExpr(expr->callee);

    for (auto& arg : expr->arguments) {
        analyzeExpr(arg);
    }

    // 查找函数符号
    if (auto id = std::dynamic_pointer_cast<IdentifierExpr>(expr->callee)) {
        auto sym = lookup(id->name);
        if (sym && sym->returnType) {
            // 检查是否是泛型函数，如果是，尝试类型推断
            if (sym->node) {
                auto genericFunc = std::dynamic_pointer_cast<FuncDeclStmt>(sym->node);
                if (genericFunc && !genericFunc->typeParams.empty()) {
                    // 尝试从参数推断类型参数
                    std::vector<String> inferredArgs;
                    if (inferTypeArgs(genericFunc, expr->arguments, inferredArgs)) {
                        // 类型推断成功，使用推断的类型参数进行实例化
                        String mangledName = buildMangledName(id->name, inferredArgs);
                        
                        // 检查缓存
                        auto it = monomorphCache_.find(mangledName);
                        if (it == monomorphCache_.end()) {
                            // 检查类型参数约束
                            checkTypeConstraints(genericFunc->typeParams, inferredArgs,
                                id->name, expr->token.line, expr->token.column);
                            
                            // 实例化泛型函数
                            auto instantiated = instantiateGenericFunc(genericFunc, inferredArgs);
                            monomorphizedFunctions_.push_back(instantiated);
                            
                            auto newSym = new Symbol();
                            newSym->kind = Symbol::FUNC;
                            newSym->name = mangledName;
                            newSym->node = instantiated;
                            newSym->returnType = instantiated->returnType.has_value()
                                ? getTypeFromString(*instantiated->returnType)
                                : Type::void_();
                            newSym->isStatic = instantiated->isStatic;
                            defineSymbol(newSym);
                            
                            monomorphCache_[mangledName] = {mangledName, instantiated};
                            
                            analyzeFuncDecl(instantiated);
                            
                            auto analyzedSym = lookup(mangledName);
                            if (analyzedSym && analyzedSym->returnType) {
                                newSym->returnType = analyzedSym->returnType;
                            }
                        }
                        
                        auto& mf = monomorphCache_[mangledName];
                        if (mf.funcDecl && mf.funcDecl->returnType.has_value())
                            return getTypeFromString(*mf.funcDecl->returnType);
                        return sym->returnType;
                    }
                }
            }
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
        
        // 类成员访问（沿继承链向上查找）
        ClassInfo* classInfo = getClassInfo(objectType->name);
        if (classInfo) {
            ClassInfo* cur = classInfo;
            while (cur) {
                auto it = cur->fieldTable.find(expr->member);
                if (it != cur->fieldTable.end()) {
                    return it->second->type;
                }
                auto mit = cur->methodTable.find(expr->member);
                if (mit != cur->methodTable.end()) {
                    return mit->second->returnType;
                }
                cur = cur->baseClass;
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

InterfaceInfo* SemanticAnalyzer::getInterfaceInfo(const String& name) {
    auto it = interfaceTable_.find(name);
    if (it != interfaceTable_.end()) {
        return it->second;
    }
    return nullptr;
}

void SemanticAnalyzer::checkInterfaceImplementation(ClassInfo* cls) {
    if (!cls) return;
    
    // 收集类的所有方法（包括继承的）
    std::unordered_map<String, Symbol*> allMethods;
    ClassInfo* current = cls;
    while (current) {
        for (auto& method : current->methods) {
            if (allMethods.find(method->name) == allMethods.end()) {
                allMethods[method->name] = method;
            }
        }
        current = current->baseClass;
    }
    
    // 检查每个接口
    for (auto& ifaceName : cls->interfaces) {
        auto iface = getInterfaceInfo(ifaceName);
        if (!iface) {
            // 接口不存在，可能是前向引用，暂时跳过
            continue;
        }
        
        // 检查接口的每个方法是否在类中实现
        for (auto& ifaceMethod : iface->methods) {
            auto it = allMethods.find(ifaceMethod->name);
            if (it == allMethods.end()) {
                // 方法未实现
                reportError("Class '" + cls->name + "' does not implement interface method '" 
                      + ifaceName + "::" + ifaceMethod->name + "'");
                continue;
            }
            
            // 检查方法签名是否匹配（参数数量和类型）
            auto clsMethod = it->second;
            if (clsMethod->params.size() != ifaceMethod->params.size()) {
                reportError("Class '" + cls->name + "' method '" + ifaceMethod->name 
                      + "' parameter count mismatch with interface '" + ifaceName + "'");
                continue;
            }
        }
    }
}

bool SemanticAnalyzer::interfaceImplementsTrait(const String& ifaceName, const String& traitName) {
    if (ifaceName == traitName) return true;
    
    auto iface = getInterfaceInfo(ifaceName);
    if (!iface) return false;
    
    // 递归检查基接口
    for (auto& baseIface : iface->baseInterfaces) {
        if (interfaceImplementsTrait(baseIface, traitName)) {
            return true;
        }
    }
    
    return false;
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

    // 自定义接口/trait 检查：对于对象类型，检查是否实现了该接口
    // v0.5.0 注：此段需要将 typeSatisfiesTrait 改为非静态才能访问 getClassInfo。
    // 当前内置 trait（可比较/可哈希等）检查已完整覆盖。
    // TODO: 将 typeSatisfiesTrait 改为非静态，或通过 SemanticAnalyzer* 参数传递实例
#if 0
    if (k == BuiltinType::OBJECT) {
        String typeName = type->name;
        if (!typeName.empty()) {
            ClassInfo* cls = getClassInfo(typeName);
            if (cls) {
                for (auto& iface : cls->interfaces) {
                    if (interfaceImplementsTrait(iface, traitName)) return true;
                }
                if (cls->baseClass) {
                    if (typeSatisfiesTrait(cls->baseClass->type, traitName)) return true;
                }
            }
        }
    }
#endif

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
        const String& concreteTypeStr = typeArgs[i];
        Type* concreteType = getTypeFromString(concreteTypeStr);
        
        // 检查单约束（简写形式）
        if (typeParams[i].constraint.has_value()) {
            const String& constraint = typeParams[i].constraint.value();
            if (!typeSatisfiesTrait(concreteType, constraint)) {
                String typeDisplay = concreteType ? concreteType->toString() : concreteTypeStr;
                reportError(line, col,
                    "类型 '" + typeDisplay + "' 不满足约束 '" + constraint +
                    "' 在泛型 '" + contextName + "' 中");
                allOk = false;
            }
        }
        
        // 检查多约束（where子句形式）
        for (auto& constraint : typeParams[i].constraints) {
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

bool SemanticAnalyzer::inferTypeArgs(Shared<FuncDeclStmt> genericFunc,
                                     const std::vector<Shared<Expr>>& arguments,
                                     std::vector<String>& inferredArgs) {
    if (genericFunc->typeParams.empty()) return false;
    
    size_t numParams = genericFunc->typeParams.size();
    inferredArgs.assign(numParams, "");
    
    bool allInferred = true;
    
    // 遍历函数参数，尝试推断类型参数
    for (size_t i = 0; i < genericFunc->params.size() && i < arguments.size(); i++) {
        auto& param = genericFunc->params[i];
        auto& arg = arguments[i];
        
        // 参数类型名
        String paramTypeStr = param.second.has_value() ? *param.second : "";
        if (paramTypeStr.empty()) continue;
        
        // 实际参数类型
        Type* argType = getExprType(arg);
        if (!argType || argType->kind == BuiltinType::UNKNOWN) continue;
        
        // 检查参数类型是否是类型参数本身（如 T）
        for (size_t j = 0; j < numParams; j++) {
            if (paramTypeStr == genericFunc->typeParams[j].name) {
                // 找到匹配的类型参数，用实参类型填充
                String argTypeName = argType->name;
                if (!argTypeName.empty()) {
                    if (inferredArgs[j].empty()) {
                        inferredArgs[j] = argTypeName;
                    } else if (inferredArgs[j] != argTypeName) {
                        // 类型冲突，推断失败
                        return false;
                    }
                }
                break;
            }
        }
    }
    
    // 检查是否所有类型参数都被推断出来了
    for (auto& arg : inferredArgs) {
        if (arg.empty()) {
            allInferred = false;
            break;
        }
    }
    
    return allInferred;
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
    // DestructuringDecl: 替换 init 表达式中的类型
    else if (auto dd = std::dynamic_pointer_cast<DestructuringDecl>(stmt)) {
        if (dd->init) {
            substituteTypeInExpr(dd->init, paramNames, typeArgs);
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
        for (auto& cb : tryStmt->catchBlocks) {
            if (cb.body) substituteTypeInStmt(cb.body, paramNames, typeArgs);
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
        // 替换类名中的类型参数（如果类名本身就是类型参数）
        for (size_t i = 0; i < paramNames.size(); i++) {
            if (newExpr->className == paramNames[i]) {
                newExpr->className = typeArgs[i];
                break;
            }
        }
        // 替换类型参数中的类型参数
        for (auto& ta : newExpr->typeArgs) {
            for (size_t i = 0; i < paramNames.size(); i++) {
                if (ta == paramNames[i]) {
                    ta = typeArgs[i];
                    break;
                }
            }
        }
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
    // IsExpr - 类型守卫表达式
    else if (auto isE = std::dynamic_pointer_cast<IsExpr>(expr)) {
        substituteTypeInExpr(isE->expr, paramNames, typeArgs);
        // 检查类型字符串中是否包含泛型参数，如果有则替换
        for (size_t i = 0; i < paramNames.size(); i++) {
            if (isE->checkType == paramNames[i]) {
                isE->checkType = typeArgs[i];
                break;
            }
        }
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

ClassInfo* SemanticAnalyzer::ensureGenericClassInstantiated(
    const String& fullName,
    const String& baseName,
    const std::vector<String>& typeArgs)
{
    // 检查缓存
    auto cacheIt = monomorphClassCache_.find(fullName);
    if (cacheIt != monomorphClassCache_.end()) {
        return cacheIt->second;
    }

    // 查找基础类信息
    ClassInfo* baseInfo = getClassInfo(baseName);
    if (!baseInfo) return nullptr;

    // 查找基础类声明
    auto baseSym = lookup(baseName);
    if (!baseSym || !baseSym->node) return nullptr;
    auto clsDecl = std::dynamic_pointer_cast<ClassDeclStmt>(baseSym->node);
    if (!clsDecl) return nullptr;

    // 检查类型参数约束
    checkTypeConstraints(clsDecl->typeParams, typeArgs, baseName, 0, 0);

    // 创建实例化后的 ClassInfo
    ClassInfo* instInfo = new ClassInfo();
    instInfo->name = fullName;
    instInfo->type = TypeRegistry::instance().getCustomType(fullName);
    instInfo->type->kind = BuiltinType::OBJECT;
    instInfo->isGeneric = false;  // 实例化后不再是泛型

    // 处理基类（如果基类也是泛型，需要实例化）
    if (baseInfo->baseClass) {
        // 简化：直接复用基类信息（复杂情况暂不处理）
        instInfo->baseClass = baseInfo->baseClass;
    }

    // 收集类型参数名列表，用于替换
    std::vector<String> paramNames;
    for (const auto& tp : clsDecl->typeParams) {
        paramNames.push_back(tp.name);
    }

    // 遍历成员，替换类型参数
    for (auto& member : clsDecl->members) {
        if (auto var = std::dynamic_pointer_cast<VarDeclStmt>(member)) {
            // 字段：替换类型
            auto fieldSym = new Symbol();
            fieldSym->kind = Symbol::FIELD;
            fieldSym->name = var->name;
            fieldSym->isConst = var->isConst;
            fieldSym->isStatic = var->isStatic;
            fieldSym->isPublic = true;

            if (var->type.has_value()) {
                String fieldTypeStr = var->type.value();
                // 替换类型参数（精确匹配）
                for (size_t i = 0; i < typeArgs.size(); i++) {
                    if (fieldTypeStr == paramNames[i]) {
                        fieldTypeStr = typeArgs[i];
                        break;
                    }
                }
                fieldSym->type = getTypeFromString(fieldTypeStr);
            } else {
                fieldSym->type = Type::unknown();
            }

            instInfo->fieldTable[var->name] = fieldSym;
            instInfo->fields.push_back(fieldSym);
        }
        else if (auto func = std::dynamic_pointer_cast<FuncDeclStmt>(member)) {
            // 方法：替换参数类型和返回类型中的类型参数
            auto methodSym = new Symbol();
            methodSym->kind = Symbol::FUNC;
            methodSym->name = func->name;
            methodSym->isStatic = func->isStatic;

            // 替换返回类型
            if (func->returnType.has_value()) {
                String retTypeStr = *func->returnType;
                for (size_t i = 0; i < typeArgs.size(); i++) {
                    if (retTypeStr == paramNames[i]) {
                        retTypeStr = typeArgs[i];
                        break;
                    }
                }
                methodSym->returnType = getTypeFromString(retTypeStr);
            } else {
                methodSym->returnType = Type::void_();
            }

            // 替换参数类型
            for (auto& p : func->params) {
                Type* pt = Type::unknown();
                if (p.second.has_value()) {
                    String paramTypeStr = *p.second;
                    for (size_t i = 0; i < typeArgs.size(); i++) {
                        if (paramTypeStr == paramNames[i]) {
                            paramTypeStr = typeArgs[i];
                            break;
                        }
                    }
                    pt = getTypeFromString(paramTypeStr);
                }
                methodSym->params.push_back({p.first, pt});
            }

            instInfo->methodTable[func->name] = methodSym;
            instInfo->methods.push_back(methodSym);

            // 如果方法有函数体，需要实例化函数体（替换类型参数）
            // 简化版：先只处理签名，函数体在调用时再处理
            // 完整实现需要 clone 函数体并 substituteTypeInStmt
            if (func->body && !func->body->statements.empty()) {
                // 创建实例化后的方法并加入 monomorphizedFunctions_
                auto instFunc = std::make_shared<FuncDeclStmt>();
                instFunc->name = fullName + "::" + func->name;
                instFunc->params = func->params;
                instFunc->returnType = func->returnType;
                instFunc->body = cloneBlock(func->body);
                instFunc->isStatic = func->isStatic;
                instFunc->access = func->access;

                // 替换参数类型中的类型参数
                for (auto& p : instFunc->params) {
                    if (p.second.has_value()) {
                        String ptStr = *p.second;
                        for (size_t i = 0; i < typeArgs.size(); i++) {
                            if (ptStr == paramNames[i]) {
                                p.second = typeArgs[i];
                                break;
                            }
                        }
                    }
                }

                // 替换返回类型
                if (instFunc->returnType.has_value()) {
                    String rtStr = *instFunc->returnType;
                    for (size_t i = 0; i < typeArgs.size(); i++) {
                        if (rtStr == paramNames[i]) {
                            instFunc->returnType = typeArgs[i];
                            break;
                        }
                    }
                }

                // 替换函数体内的类型参数
                substituteTypeInStmt(instFunc->body, paramNames, typeArgs);

                monomorphizedFunctions_.push_back(instFunc);
            }
        }
    }

    // 注册到类表和缓存
    classTable_[fullName] = instInfo;
    monomorphClassCache_[fullName] = instInfo;

    return instInfo;
}

} // namespace cplang