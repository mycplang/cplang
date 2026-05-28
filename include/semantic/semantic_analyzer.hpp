// CP语言 语义分析器
#pragma once

#include "ast/ast.hpp"
#include "lexer/lexer.hpp"
#include <unordered_map>
#include <set>
#include <string>

namespace cplang {

// 基础类型
enum class BuiltinType {
    UNKNOWN,
    VOID,
    INT,
    FLOAT,
    BOOL,
    STRING,
    CHAR,
    INT8, INT16, INT32, INT64,
    UINT8, UINT16, UINT32, UINT64,
    FLOAT32, FLOAT64,
    OBJECT,
    ARRAY,
    FUNCTION,
    ENUM,
    STRUCT
};

// 类型描述
struct Type {
    BuiltinType kind = BuiltinType::UNKNOWN;
    String name;              // 自定义类型名
    Type* innerType = nullptr; // 数组/指针的元素类型
    std::vector<Type*> generics;  // 泛型参数
    bool isConst = false;
    
    // 构造方法
    static Type* void_() { static Type t{BuiltinType::VOID, "", nullptr, {}, false}; return &t; }
    static Type* int_() { static Type t{BuiltinType::INT64, "", nullptr, {}, false}; return &t; }  // int 是 i64 别名
    static Type* int8_() { static Type t{BuiltinType::INT8, "", nullptr, {}, false}; return &t; }
    static Type* int16_() { static Type t{BuiltinType::INT16, "", nullptr, {}, false}; return &t; }
    static Type* int32_() { static Type t{BuiltinType::INT32, "", nullptr, {}, false}; return &t; }
    static Type* int64_() { static Type t{BuiltinType::INT64, "", nullptr, {}, false}; return &t; }
    static Type* float_() { static Type t{BuiltinType::FLOAT64, "", nullptr, {}, false}; return &t; }  // float 是 f64 别名
    static Type* float32_() { static Type t{BuiltinType::FLOAT32, "", nullptr, {}, false}; return &t; }
    static Type* float64_() { static Type t{BuiltinType::FLOAT64, "", nullptr, {}, false}; return &t; }
    static Type* bool_() { static Type t{BuiltinType::BOOL, "", nullptr, {}, false}; return &t; }
    static Type* string_() { static Type t{BuiltinType::STRING, "", nullptr, {}, false}; return &t; }
    static Type* char_() { static Type t{BuiltinType::CHAR, "", nullptr, {}, false}; return &t; }
    static Type* unknown() { static Type t{BuiltinType::UNKNOWN, "", nullptr, {}, false}; return &t; }
    static Type* table_() { static Type t{BuiltinType::OBJECT, "table", nullptr, {}, false}; return &t; }
    
    static Type* fromBuiltin(BuiltinType k) {
        switch(k) {
            case BuiltinType::VOID: return void_();
            case BuiltinType::INT: return int_();
            case BuiltinType::INT8: return int8_();
            case BuiltinType::INT16: return int16_();
            case BuiltinType::INT32: return int32_();
            case BuiltinType::INT64: return int64_();
            case BuiltinType::FLOAT: return float_();
            case BuiltinType::FLOAT32: return float32_();
            case BuiltinType::FLOAT64: return float64_();
            case BuiltinType::BOOL: return bool_();
            case BuiltinType::STRING: return string_();
            case BuiltinType::CHAR: return char_();
            default: return unknown();
        }
    }
    
    bool equals(Type* other) const {
        if (kind != other->kind) return false;
        if (kind == BuiltinType::ARRAY && innerType && other->innerType)
            return innerType->equals(other->innerType);
        return true;
    }
    
    String toString() const {
        switch (kind) {
            case BuiltinType::VOID: return "void";
            case BuiltinType::INT:
            case BuiltinType::INT64: return "i64";
            case BuiltinType::INT8: return "i8";
            case BuiltinType::INT16: return "i16";
            case BuiltinType::INT32: return "i32";
            case BuiltinType::FLOAT:
            case BuiltinType::FLOAT64: return "f64";
            case BuiltinType::FLOAT32: return "f32";
            case BuiltinType::BOOL: return "bool";
            case BuiltinType::STRING: return "string";
            case BuiltinType::CHAR: return "char";
            case BuiltinType::ARRAY: return innerType ? (innerType->toString() + "[]") : "array";
            case BuiltinType::FUNCTION: return "function";
            case BuiltinType::OBJECT: return name;
            case BuiltinType::STRUCT: return "struct " + name;
            case BuiltinType::ENUM: return "enum " + name;
            default: return name.empty() ? "?" : name;
        }
    }
};

// 类型缓存（避免重复分配）
class TypeRegistry {
public:
    static TypeRegistry& instance() {
        static TypeRegistry reg;
        return reg;
    }
    
    Type* getArrayType(Type* elementType) {
        String key = elementType->toString() + "[]";
        if (cache_.find(key) == cache_.end()) {
            Type* t = new Type();
            t->kind = BuiltinType::ARRAY;
            t->innerType = elementType;
            cache_[key] = t;
        }
        return cache_[key];
    }
    
    Type* getCustomType(const String& name) {
        if (cache_.find(name) == cache_.end()) {
            Type* t = new Type();
            t->kind = BuiltinType::OBJECT;
            t->name = name;
            cache_[name] = t;
        }
        return cache_[name];
    }

private:
    std::unordered_map<String, Type*> cache_;
};

// 符号
struct Symbol {
    enum Kind { VAR, FUNC, CLASS, PARAM, FIELD, CONST, ENUM_VALUE, TYPE_ALIAS, LABEL };
    Kind kind;
    String name;
    Type* type = nullptr;
    Shared<ASTNode> node;       // 原始 AST 节点
    int scopeLevel = 0;
    bool isConst = false;
    bool isStatic = false;
    bool isPrivate = false;
    bool isPublic = true;
    
    // 函数特有
    std::vector<std::pair<String, Type*>> params;
    Type* returnType = nullptr;
    bool isVariadic = false;
    
    // 类特有
    Type* classType = nullptr;
    Type* baseClass = nullptr;
    
    // 枚举特有
    Int64 enumValue = 0;
};

// 作用域
struct Scope {
    int level = 0;
    String name;
    Scope* parent = nullptr;
    std::unordered_map<String, Symbol*> symbols;
    
    Symbol* find(const String& name_) {
        auto it = symbols.find(name_);
        if (it != symbols.end()) return it->second;
        if (parent) return parent->find(name_);
        return nullptr;
    }
    
    Symbol* findLocal(const String& name_) {
        auto it = symbols.find(name_);
        return it != symbols.end() ? it->second : nullptr;
    }
    
    void define(Symbol* sym) {
        sym->scopeLevel = level;
        symbols[sym->name] = sym;
    }
};

// 语义错误
struct SemanticError {
    enum Level { WARNING, ERROR, FATAL };
    Level level = Level::ERROR;
    int line = 0;
    int column = 0;
    String message;
};

// 类信息
struct ClassInfo {
    String name;
    Type* type = nullptr;
    ClassInfo* baseClass = nullptr;
    std::vector<Symbol*> fields;
    std::vector<Symbol*> methods;
    std::unordered_map<String, Symbol*> methodTable;
    std::unordered_map<String, Symbol*> fieldTable;
};

// 结构体字段信息
struct StructField {
    String name;
    Type* type = nullptr;
    size_t offset = 0;  // 内存偏移
};

// 结构体信息
struct StructInfo {
    String name;
    Type* type = nullptr;
    std::vector<StructField> fields;
    std::unordered_map<String, size_t> fieldIndex;  // name -> index
    size_t size = 0;  // 总大小
    
    // 查找字段
    StructField* findField(const String& name_) {
        auto it = fieldIndex.find(name_);
        if (it != fieldIndex.end() && it->second < fields.size()) {
            return &fields[it->second];
        }
        return nullptr;
    }
};

// 语义分析器
class SemanticAnalyzer {
public:
    explicit SemanticAnalyzer(Lexer* lexer = nullptr);
    ~SemanticAnalyzer();
    
    // 分析整个程序
    bool analyze(Shared<Program> program);
    
    // 错误
    bool hasError() const { return hasError_; }
    const String& errorMessage() const { return errorMsg_; }
    const std::vector<SemanticError>& errors() const { return errors_; }
    void printErrors();
    
    // 符号表访问
    Symbol* lookup(const String& name);
    Scope* currentScope() { return scopeStack_.back(); }
    ClassInfo* currentClass() { return classStack_.empty() ? nullptr : classStack_.back(); }
    
    // 类型查询接口（供代码生成器使用）
    Type* getExprType(Shared<Expr> expr);
    ClassInfo* getClassInfo(const String& name);
    StructInfo* getStructInfo(const String& name);
    const std::vector<Shared<FuncDeclStmt>>& getMonomorphizedFunctions() const {
        return monomorphizedFunctions_;
    }
    
    // 表达式类型缓存（analyzeExpr 后存储，codegen 查询）
    void cacheExprType(Shared<Expr> expr, Type* t);
    void clearExprCache() { exprTypes_.clear(); }

private:
    [[maybe_unused]] Lexer* lexer_ = nullptr;
    Shared<Program> program_;
    
    // 作用域管理
    std::vector<Scope*> scopeStack_;
    std::unordered_map<String, ClassInfo*> classTable_;
    std::unordered_map<String, StructInfo*> structTable_;  // 结构体表
    
    // 类栈（用于处理类内部成员）
    std::vector<ClassInfo*> classStack_;
    
    // 循环栈（用于 break/continue 检查）
    std::vector<String> loopStack_;
    
    // 错误
    bool hasError_ = false;
    String errorMsg_;
    std::vector<SemanticError> errors_;
    
    // 表达式类型缓存 (Expr* raw ptr → Type*)
    std::unordered_map<Expr*, Type*> exprTypes_;
    
    // ── 所有权/借用追踪 ──
    struct BorrowState { int mutableCount = 0; int immutableCount = 0; };
    std::unordered_map<String, BorrowState> borrowCounts_;
    std::unordered_set<String> movedVars_;  // 已移动的变量
    int  trustDepth_ = 0;  // 可信 { } 嵌套深度，>0 时跳过借用检查
    
    void checkBorrow(const String& varName, bool isMutable, int line, int col);
    void releaseBorrow(const String& varName, bool isMutable);
    void releaseAllBorrows();
    
    void moveVariable(const String& varName, int line, int col);
    void dropVariable(const String& varName, int line, int col);
    void checkVariableMoved(const String& varName, int line, int col);
    
    // === 作用域管理 ===
    void pushScope(const String& name = "");
    void popScope();
    void defineSymbol(Symbol* sym);
    Symbol* lookupInScope(const String& name);
    
    // === 类型系统 ===
    Type* getTypeFromString(const String& typeStr);
    bool isAssignableTo(Type* from, Type* to);
    bool areTypesEqual(Type* a, Type* b);
    
    // === 声明分析 ===
    void analyzeFuncDecl(Shared<FuncDeclStmt> func);
    void analyzeClassDecl(Shared<ClassDeclStmt> cls);
    void analyzeInterfaceDecl(Shared<InterfaceDeclStmt> iface);
    void analyzeEnumDecl(Shared<EnumDeclStmt> enm);
    void analyzeStructDecl(Shared<StructDeclStmt> st);
    void analyzeVarDecl(Shared<VarDeclStmt> var);
    
    // === 语句分析 ===
    Type* analyzeStmt(Shared<Stmt> stmt);
    Type* analyzeBlock(Shared<BlockStmt> block);
    Type* analyzeIf(Shared<IfStmt> s);
    Type* analyzeFor(Shared<ForStmt> s);
    Type* analyzeWhile(Shared<WhileStmt> s);
    Type* analyzeReturn(Shared<ReturnStmt> s);
    
    // === 表达式分析 ===
    Type* analyzeExpr(Shared<Expr> expr);
    Type* analyzeBinaryExpr(Shared<BinaryExpr> expr);
    Type* analyzeUnaryExpr(Shared<UnaryExpr> expr);
    Type* analyzeCallExpr(Shared<CallExpr> expr);
    Type* analyzeMemberExpr(Shared<MemberExpr> expr);
    Type* analyzeIndexExpr(Shared<IndexExpr> expr);
    
    // === AST 变换 ===
    // 将块内的 defer 语句展开为 try-finally 模式
    void transformDeferInBlock(Shared<BlockStmt> block);

    // === 工具 ===
    void reportError(int line, int col, const String& msg, SemanticError::Level level = SemanticError::ERROR);
    void reportError(const String& msg, SemanticError::Level level = SemanticError::ERROR);
    
    void enterClass(ClassInfo* cls);
    void leaveClass();
    
    // 结构体相关
    void registerStructType(const String& name, StructInfo* info);

    // === 泛型约束（trait）检查 ===
    // 检查具体类型是否满足 trait 约束
    static bool typeSatisfiesTrait(Type* type, const String& traitName);
    // 检查类型参数约束并报告错误
    bool checkTypeConstraints(const std::vector<TypeParam>& typeParams,
                              const std::vector<String>& typeArgs,
                              const String& contextName,
                              int line, int col);

    // === 泛型实例化（单态化） ===
    // 存储已实例化的泛型函数（供代码生成器优先编译）
    std::vector<Shared<FuncDeclStmt>> monomorphizedFunctions_;
    // 结构体实例化缓存
    std::unordered_map<String, StructInfo*> monomorphStructCache_;

    // 确保泛型结构体已被实例化（如 列表<整数>）
    StructInfo* ensureGenericStructInstantiated(const String& fullName,
                                                const String& baseName,
                                                const std::vector<String>& typeArgs);
    // 解析 "列表<整数, 字符串>" → {base="列表", args=["整数","字符串"]}
    static bool parseGenericTypeName(const String& fullName,
                                     String& baseName,
                                     std::vector<String>& typeArgs);
    struct MonomorphizedFunc {
        String mangledName;
        Shared<FuncDeclStmt> funcDecl;
    };
    std::unordered_map<String, MonomorphizedFunc> monomorphCache_;

    String buildMangledName(const String& funcName, const std::vector<String>& typeArgs);
    Shared<FuncDeclStmt> instantiateGenericFunc(Shared<FuncDeclStmt> genericFunc,
                                                const std::vector<String>& typeArgs);
    Shared<BlockStmt> cloneBlock(Shared<BlockStmt> block);
    void substituteTypeInStmt(Shared<Stmt> stmt,
                              const std::vector<String>& paramNames,
                              const std::vector<String>& typeArgs);
    void substituteTypeInExpr(Shared<Expr> expr,
                              const std::vector<String>& paramNames,
                              const std::vector<String>& typeArgs);
};

} // namespace cplang