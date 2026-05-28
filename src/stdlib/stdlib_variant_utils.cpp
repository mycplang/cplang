// Optional, Variant, Any, Tuple functions
// #include'd from stdlib.cpp, already inside namespace cplang

void StdLib::registerOptional(VM* vm) {
    registerFunction(vm, "optNone", optional_::optNone);
    registerFunction(vm, "optSome", optional_::optSome);
    registerFunction(vm, "optHas", optional_::optHas);
    registerFunction(vm, "optIsNone", optional_::optIsNone);
    registerFunction(vm, "optGet", optional_::optGet);
    registerFunction(vm, "optOr", optional_::optOr);
    
    registerAlias(vm, "可选空", "optNone");
    registerAlias(vm, "可选值", "optSome");
    registerAlias(vm, "可选有", "optHas");
    registerAlias(vm, "可选为空", "optIsNone");
    registerAlias(vm, "可选取", "optGet");
    registerAlias(vm, "可选或", "optOr");
}

void StdLib::registerVariant(VM* vm) {
    registerFunction(vm, "varNew", variant_::varNew);
    registerFunction(vm, "varType", variant_::varType);
    registerFunction(vm, "varVal", variant_::varVal);
    registerFunction(vm, "varIs", variant_::varIs);
    registerFunction(vm, "varVisit", variant_::varVisit);
    
    registerAlias(vm, "变体新", "varNew");
    registerAlias(vm, "变体类型", "varType");
    registerAlias(vm, "变体值", "varVal");
    registerAlias(vm, "变体是", "varIs");
    registerAlias(vm, "变体访问", "varVisit");
}

void StdLib::registerAny(VM* vm) {
    registerFunction(vm, "anyNew", any_::anyNew);
    registerFunction(vm, "anyHasValue", any_::anyHasValue);
    registerFunction(vm, "anyType", any_::anyType);
    registerFunction(vm, "anyGet", any_::anyGet);
    registerFunction(vm, "anyCast", any_::anyCast);
    registerFunction(vm, "anyReset", any_::anyReset);
    
    registerAlias(vm, "任意新", "anyNew");
    registerAlias(vm, "任意有值", "anyHasValue");
    registerAlias(vm, "任意类型", "anyType");
    registerAlias(vm, "任意取", "anyGet");
    registerAlias(vm, "任意转换", "anyCast");
    registerAlias(vm, "任意重置", "anyReset");
}

void StdLib::registerTuple(VM* vm) {
    registerFunction(vm, "tupMake", tuple_::tupMake);
    registerFunction(vm, "tupGet", tuple_::tupGet);
    registerFunction(vm, "tupSize", tuple_::tupSize);
    registerFunction(vm, "tupCat", tuple_::tupCat);
    registerFunction(vm, "tupSlice", tuple_::tupSlice);
    
    registerAlias(vm, "元组创建", "tupMake");
    registerAlias(vm, "元组取", "tupGet");
    registerAlias(vm, "元组大小", "tupSize");
    registerAlias(vm, "元组拼接", "tupCat");
    registerAlias(vm, "元组切片", "tupSlice");
}

namespace optional_ {

Value optNone(std::vector<Value>& args) {
    auto t = VMTable::create();
    t->set(Value::String(VMString::create("_has")), Value::Bool(false));
    t->set(Value::String(VMString::create("_val")), Value::nil());
    return Value::Table(t);
}

Value optSome(std::vector<Value>& args) {
    auto t = VMTable::create();
    t->set(Value::String(VMString::create("_has")), Value::Bool(true));
    t->set(Value::String(VMString::create("_val")), args.empty() ? Value::nil() : args[0]);
    return Value::Table(t);
}

Value optHas(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::Bool(false);
    VMTable* t = args[0].asTable();
    return Value::Bool(t->get(Value::String(VMString::create("_has"))).isTrue());
}

Value optIsNone(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::Bool(true);
    VMTable* t = args[0].asTable();
    return Value::Bool(!t->get(Value::String(VMString::create("_has"))).isTrue());
}

Value optGet(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::nil();
    VMTable* t = args[0].asTable();
    if (t->get(Value::String(VMString::create("_has"))).isTrue()) {
        return t->get(Value::String(VMString::create("_val")));
    }
    return Value::nil();
}

Value optOr(std::vector<Value>& args) {
    if (args.size() < 2) return Value::nil();
    if (!args[0].isTable()) return args[1];
    VMTable* t = args[0].asTable();
    if (t->get(Value::String(VMString::create("_has"))).isTrue()) {
        return t->get(Value::String(VMString::create("_val")));
    }
    return args[1];
}

} // namespace optional_

// ═══════════════════════════════════════════════════════════════════
//  Variant 变体实现（Table 存储：_type + _val）
// ═══════════════════════════════════════════════════════════════════

namespace variant_ {

static const char* valueTypeTag(Value v) {
    if (v.isNil()) return "nil";
    if (v.isBool()) return "bool";
    if (v.isInt()) return "int";
    if (v.isFloat()) return "float";
    if (v.isString()) return "string";
    if (v.isArray()) return "array";
    if (v.isTable()) return "table";
    if (v.isFunction()) return "function";
    return "object";
}

Value varNew(std::vector<Value>& args) {
    auto t = VMTable::create();
    Value val = args.empty() ? Value::nil() : args[0];
    t->set(Value::String(VMString::create("_val")), val);
    const char* tn = args.size() >= 2 && args[1].isString()
        ? args[1].asString()->c_str() : valueTypeTag(val);
    t->set(Value::String(VMString::create("_type")), Value::String(VMString::create(tn)));
    return Value::Table(t);
}

Value varType(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::String(VMString::create("?"));
    return args[0].asTable()->get(Value::String(VMString::create("_type")));
}

Value varVal(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::nil();
    return args[0].asTable()->get(Value::String(VMString::create("_val")));
}

Value varIs(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isTable() || !args[1].isString()) return Value::Bool(false);
    Value typeV = args[0].asTable()->get(Value::String(VMString::create("_type")));
    return Value::Bool(typeV.isString() && strcmp(typeV.asString()->c_str(), args[1].asString()->c_str()) == 0);
}

Value varVisit(std::vector<Value>& args) {
    // varVisit(variant, mappingTable) — 从 mappingTable 中按 _type 查找映射值
    if (args.size() < 2 || !args[0].isTable()) return Value::nil();
    Value val = args[0].asTable()->get(Value::String(VMString::create("_val")));
    Value typeV = args[0].asTable()->get(Value::String(VMString::create("_type")));
    if (!typeV.isString()) return val;
    if (args[1].isTable()) {
        Value result = args[1].asTable()->get(typeV);
        if (!result.isNil()) return result;
    }
    return val;
}

} // namespace variant_

// ═══════════════════════════════════════════════════════════════════
//  Any 任意值实现（Table 存储：_has + _type + _val）
// ═══════════════════════════════════════════════════════════════════

namespace any_ {

Value anyNew(std::vector<Value>& args) {
    auto t = VMTable::create();
    Value val = args.empty() ? Value::nil() : args[0];
    t->set(Value::String(VMString::create("_has")), Value::Bool(true));
    t->set(Value::String(VMString::create("_val")), val);
    const char* tn = "nil";
    if (val.isNil()) tn = "nil";
    else if (val.isBool()) tn = "bool";
    else if (val.isInt()) tn = "int";
    else if (val.isFloat()) tn = "float";
    else if (val.isString()) tn = "string";
    else if (val.isArray()) tn = "array";
    else if (val.isTable()) tn = "table";
    else if (val.isFunction()) tn = "function";
    else tn = "object";
    t->set(Value::String(VMString::create("_type")), Value::String(VMString::create(tn)));
    return Value::Table(t);
}

Value anyHasValue(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::Bool(false);
    return args[0].asTable()->get(Value::String(VMString::create("_has")));
}

Value anyType(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::String(VMString::create("?"));
    return args[0].asTable()->get(Value::String(VMString::create("_type")));
}

Value anyGet(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::nil();
    VMTable* t = args[0].asTable();
    return t->get(Value::String(VMString::create("_has"))).isTrue()
        ? t->get(Value::String(VMString::create("_val"))) : Value::nil();
}

Value anyCast(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isTable() || !args[1].isString()) return Value::Bool(false);
    VMTable* t = args[0].asTable();
    if (!t->get(Value::String(VMString::create("_has"))).isTrue()) return Value::Bool(false);
    Value typeV = t->get(Value::String(VMString::create("_type")));
    return Value::Bool(typeV.isString() && strcmp(typeV.asString()->c_str(), args[1].asString()->c_str()) == 0);
}

Value anyReset(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::nil();
    VMTable* t = args[0].asTable();
    t->set(Value::String(VMString::create("_has")), Value::Bool(false));
    t->set(Value::String(VMString::create("_val")), Value::nil());
    return Value::nil();
}

} // namespace any_

// ═══════════════════════════════════════════════════════════════════
//  Tuple 元组实现（Table 存储：_size + _0.._n 键）
// ═══════════════════════════════════════════════════════════════════

namespace tuple_ {

Value tupMake(std::vector<Value>& args) {
    auto t = VMTable::create();
    Int32 n = static_cast<Int32>(args.size());
    t->set(Value::String(VMString::create("_size")), Value::Int(n));
    char buf[32];
    for (Int32 i = 0; i < n; i++) {
        snprintf(buf, sizeof(buf), "_%d", i);
        t->set(Value::String(VMString::create(buf)), args[i]);
    }
    return Value::Table(t);
}

Value tupGet(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isTable() || !args[1].isInt()) return Value::nil();
    VMTable* t = args[0].asTable();
    Int32 idx = args[1].asInt();
    Int32 n = 0;
    Value sizeV = t->get(Value::String(VMString::create("_size")));
    if (sizeV.isInt()) n = sizeV.asInt();
    if (idx < 0) idx += n;
    if (idx < 0 || idx >= n) return Value::nil();
    char buf[32];
    snprintf(buf, sizeof(buf), "_%d", idx);
    return t->get(Value::String(VMString::create(buf)));
}

Value tupSize(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::Int(0);
    return args[0].asTable()->get(Value::String(VMString::create("_size")));
}

Value tupCat(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isTable() || !args[1].isTable()) return Value::nil();
    VMTable* t1 = args[0].asTable();
    VMTable* t2 = args[1].asTable();
    Int32 n1 = 0, n2 = 0;
    Value sv = t1->get(Value::String(VMString::create("_size")));
    if (sv.isInt()) n1 = sv.asInt();
    sv = t2->get(Value::String(VMString::create("_size")));
    if (sv.isInt()) n2 = sv.asInt();
    
    auto t = VMTable::create();
    Int32 total = n1 + n2;
    t->set(Value::String(VMString::create("_size")), Value::Int(total));
    char buf[32];
    for (Int32 i = 0; i < n1; i++) {
        snprintf(buf, sizeof(buf), "_%d", i);
        t->set(Value::String(VMString::create(buf)),
            t1->get(Value::String(VMString::create(buf))));
    }
    for (Int32 i = 0; i < n2; i++) {
        snprintf(buf, sizeof(buf), "_%d", i);
        Value v = t2->get(Value::String(VMString::create(buf)));
        snprintf(buf, sizeof(buf), "_%d", n1 + i);
        t->set(Value::String(VMString::create(buf)), v);
    }
    return Value::Table(t);
}

Value tupSlice(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::nil();
    VMTable* src = args[0].asTable();
    Int32 n = 0;
    Value sv = src->get(Value::String(VMString::create("_size")));
    if (sv.isInt()) n = sv.asInt();
    
    Int32 start = 0, end = n;
    if (args.size() >= 2 && args[1].isInt()) start = args[1].asInt();
    if (args.size() >= 3 && args[2].isInt()) end = args[2].asInt();
    if (start < 0) start += n;
    if (end < 0) end += n;
    if (start < 0) start = 0;
    if (end > n) end = n;
    if (start >= end) start = end = 0;
    
    auto t = VMTable::create();
    Int32 sz = end - start;
    t->set(Value::String(VMString::create("_size")), Value::Int(sz));
    char buf[32];
    for (Int32 i = 0; i < sz; i++) {
        snprintf(buf, sizeof(buf), "_%d", start + i);
        Value v = src->get(Value::String(VMString::create(buf)));
        snprintf(buf, sizeof(buf), "_%d", i);
        t->set(Value::String(VMString::create(buf)), v);
    }
    return Value::Table(t);
}

} // namespace tuple_

// ═══════════════════════════════════════════════════════════════════
//  TCP/UDP 套接字实现（Windows Winsock API）
// ═══════════════════════════════════════════════════════════════════

