// CP语言类型系统定义
#pragma once
#include "common/types.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace cplang {

// 类型种类
enum class TypeKind {
    Void,       // 无类型
    Int8,       // i8 - 8位有符号整数
    Int16,      // i16 - 16位有符号整数
    Int32,      // i32 - 32位有符号整数
    Int64,      // i64 - 64位有符号整数
    Float32,    // f32 - 32位浮点数
    Float64,    // f64 - 64位浮点数
    Bool,       // bool - 布尔类型
    String,     // string - 字符串
    Array,      // array - 数组
    Table,      // table - 哈希表
    Function,   // func - 函数
    Class,      // class - 类
    Struct,     // struct - 结构体
    Pointer,    // ptr - 原始指针
    Ref,        // & - 不可变引用
    MutRef,     // &可写 - 可变引用
    Any         // any - 任意类型（动态）
};

// 类型描述
struct TypeInfo {
    TypeKind kind;
    String name;
    
    // 复合类型额外信息
    TypeInfo* elemType = nullptr;  // 数组元素类型
    TypeInfo* pointedType = nullptr;  // 指针指向类型
    std::vector<TypeInfo*> paramTypes;  // 函数参数类型
    TypeInfo* returnType = nullptr;  // 函数返回类型
    String structName;  // 结构体/类名
    
    // 大小信息（字节）
    size_t size = 0;
    
    TypeInfo() : kind(TypeKind::Any) {}
    TypeInfo(TypeKind k) : kind(k) {
        name = getDefaultName(k);
        size = getDefaultSize(k);
    }
    
    // 获取类型默认名
    static String getDefaultName(TypeKind k) {
        switch(k) {
            case TypeKind::Void: return "空";
            case TypeKind::Int8: return "i8";
            case TypeKind::Int16: return "i16";
            case TypeKind::Int32: return "i32";
            case TypeKind::Int64: return "i64";
            case TypeKind::Float32: return "f32";
            case TypeKind::Float64: return "f64";
            case TypeKind::Bool: return "布尔";
            case TypeKind::String: return "字符串";
            case TypeKind::Array: return "数组";
            case TypeKind::Table: return "表";
            case TypeKind::Function: return "函数";
            case TypeKind::Class: return "类";
            case TypeKind::Struct: return "结构体";
            case TypeKind::Pointer: return "指针";
            case TypeKind::Ref: return "引用";
            case TypeKind::MutRef: return "可写引用";
            case TypeKind::Any: return "任意";
            default: return "未知";
        }
    }
    
    // 获取类型默认大小（字节）
    static size_t getDefaultSize(TypeKind k) {
        switch(k) {
            case TypeKind::Int8: return 1;
            case TypeKind::Int16: return 2;
            case TypeKind::Int32: return 4;
            case TypeKind::Int64: return 8;
            case TypeKind::Float32: return 4;
            case TypeKind::Float64: return 8;
            case TypeKind::Bool: return 1;
            default: return 8;  // 指针大小或对象引用
        }
    }
    
    bool isInteger() const {
        return kind == TypeKind::Int8 || kind == TypeKind::Int16 || 
               kind == TypeKind::Int32 || kind == TypeKind::Int64;
    }
    
    bool isFloat() const {
        return kind == TypeKind::Float32 || kind == TypeKind::Float64;
    }
    
    bool isNumeric() const {
        return isInteger() || isFloat();
    }
    
    bool isReference() const {
        return kind == TypeKind::Ref || kind == TypeKind::MutRef;
    }
    
    bool isNullable() const {
        return kind == TypeKind::Any || kind == TypeKind::String || 
               kind == TypeKind::Array || kind == TypeKind::Table;
    }
};

// 类型表
class TypeTable {
private:
    std::unordered_map<String, TypeInfo*> types;
    std::vector<TypeInfo*> allocated;
    
public:
    TypeTable() {
        // 初始化内置类型
        addBuiltin(TypeKind::Void);
        addBuiltin(TypeKind::Int8);
        addBuiltin(TypeKind::Int16);
        addBuiltin(TypeKind::Int32);
        addBuiltin(TypeKind::Int64);
        addBuiltin(TypeKind::Float32);
        addBuiltin(TypeKind::Float64);
        addBuiltin(TypeKind::Bool);
        addBuiltin(TypeKind::String);
        addBuiltin(TypeKind::Array);
        addBuiltin(TypeKind::Table);
        addBuiltin(TypeKind::Function);
        addBuiltin(TypeKind::Any);
    }
    
    ~TypeTable() {
        for (auto t : allocated) delete t;
    }
    
    // 获取内置类型
    TypeInfo* get(TypeKind kind) {
        return types[TypeInfo::getDefaultName(kind)];
    }
    
    // 根据名称查找类型
    TypeInfo* find(const String& name) {
        auto it = types.find(name);
        if (it != types.end()) return it->second;
        return nullptr;
    }
    
    // 从类型注解字符串解析类型
    TypeInfo* parseType(const String& typeStr) {
        if (typeStr.empty() || typeStr == "任意") return get(TypeKind::Any);
        if (typeStr == "空" || typeStr == "void") return get(TypeKind::Void);
        if (typeStr == "i8" || typeStr == "int8") return get(TypeKind::Int8);
        if (typeStr == "i16" || typeStr == "int16") return get(TypeKind::Int16);
        if (typeStr == "i32" || typeStr == "int32" || typeStr == "int") return get(TypeKind::Int32);
        if (typeStr == "i64" || typeStr == "int64") return get(TypeKind::Int64);
        if (typeStr == "f32" || typeStr == "float32" || typeStr == "float") return get(TypeKind::Float32);
        if (typeStr == "f64" || typeStr == "float64" || typeStr == "double") return get(TypeKind::Float64);
        if (typeStr == "布尔" || typeStr == "bool") return get(TypeKind::Bool);
        if (typeStr == "字符串" || typeStr == "string") return get(TypeKind::String);
        if (typeStr == "数组" || typeStr == "array") return get(TypeKind::Array);
        if (typeStr == "表" || typeStr == "table") return get(TypeKind::Table);
        
        // 引用类型 &T 或 &可写 T
        if (typeStr.substr(0, 2) == "&可写") {
            TypeInfo* base = parseType(typeStr.substr(2));
            TypeInfo* ref = new TypeInfo(TypeKind::MutRef);
            ref->pointedType = base;
            ref->name = "&可写 " + base->name;
            allocated.push_back(ref);
            return ref;
        }
        if (typeStr[0] == '&') {
            TypeInfo* base = parseType(typeStr.substr(1));
            TypeInfo* ref = new TypeInfo(TypeKind::Ref);
            ref->pointedType = base;
            ref->name = "&" + base->name;
            allocated.push_back(ref);
            return ref;
        }
        
        // 尝试查找自定义类型（类/结构体）
        return find(typeStr);
    }
    
    // 数字类型提升（用于类型化算术）
    TypeInfo* promote(TypeInfo* a, TypeInfo* b) {
        if (!a->isNumeric() || !b->isNumeric()) return nullptr;
        
        // 浮点优先
        if (a->isFloat() || b->isFloat()) {
            if (a->kind == TypeKind::Float64 || b->kind == TypeKind::Float64)
                return get(TypeKind::Float64);
            return get(TypeKind::Float32);
        }
        
        // 整数提升
        int aRank = typeRank(a);
        int bRank = typeRank(b);
        
        if (aRank >= bRank) return a;
        return b;
    }
    
    // 检查类型是否兼容
    bool isCompatible(TypeInfo* from, TypeInfo* to) {
        if (from == to) return true;
        if (to->kind == TypeKind::Any) return true;
        
        // 数值类型转换
        if (from->isNumeric() && to->isNumeric()) return true;
        
        // 引用兼容性
        if (from->isReference() && to->isReference()) {
            if (to->kind == TypeKind::Ref) {
                // &T 可以转换到 &T 或 &可写 T 可以隐式转换为 &T
                return from->pointedType == to->pointedType;
            }
        }
        
        return false;
    }
    
private:
    void addBuiltin(TypeKind kind) {
        TypeInfo* t = new TypeInfo(kind);
        types[t->name] = t;
        allocated.push_back(t);
    }
    
    int typeRank(TypeInfo* t) {
        switch(t->kind) {
            case TypeKind::Int8: return 1;
            case TypeKind::Int16: return 2;
            case TypeKind::Int32: return 3;
            case TypeKind::Int64: return 4;
            case TypeKind::Float32: return 5;
            case TypeKind::Float64: return 6;
            default: return 0;
        }
    }
};

} // namespace cplang
