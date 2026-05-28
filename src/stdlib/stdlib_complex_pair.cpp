// Complex and Pair functions
// #include'd from stdlib.cpp, already inside namespace cplang

void StdLib::registerComplex(VM* vm) {
    registerFunction(vm, "complex", complex::create);
    registerFunction(vm, "complexAdd", complex::add);
    registerFunction(vm, "complexSub", complex::sub);
    registerFunction(vm, "complexMul", complex::mul);
    registerFunction(vm, "complexDiv", complex::div);
    registerFunction(vm, "complexAbs", complex::abs_);
    registerFunction(vm, "complexArg", complex::arg);
    registerFunction(vm, "complexConj", complex::conj);
    registerFunction(vm, "complexReal", complex::real_);
    registerFunction(vm, "complexImag", complex::imag);
    
    // 中文别名
    registerAlias(vm, "复数新建", "complex");
    registerAlias(vm, "复数加", "complexAdd");
    registerAlias(vm, "复数减", "complexSub");
    registerAlias(vm, "复数乘", "complexMul");
    registerAlias(vm, "复数除", "complexDiv");
    registerAlias(vm, "复数模", "complexAbs");
    registerAlias(vm, "复数辐角", "complexArg");
    registerAlias(vm, "复数共轭", "complexConj");
    registerAlias(vm, "复数实部", "complexReal");
    registerAlias(vm, "复数虚部", "complexImag");
}

namespace complex {
// 内部辅助：用两个 double 创建复数表
static Value makeComplex(double r, double i) {
    Value tbl = Value::Table(VMTable::create());
    tbl.asTable()->set(Value::String(VMString::create("real")), Value::Float(r));
    tbl.asTable()->set(Value::String(VMString::create("imag")), Value::Float(i));
    return tbl;
}

Value create(std::vector<Value>& args) {
    double r = 0.0, i = 0.0;
    if (args.size() >= 1) r = args[0].isInt() ? static_cast<double>(args[0].asInt()) : args[0].asFloat();
    if (args.size() >= 2) i = args[1].isInt() ? static_cast<double>(args[1].asInt()) : args[1].asFloat();
    return makeComplex(r, i);
}

void getParts(const Value& c, double& r, double& i) {
    if (c.isTable()) {
        auto t = c.asTable();
        Value rv = t->get(Value::String(VMString::create("real")));
        Value iv = t->get(Value::String(VMString::create("imag")));
        r = rv.isInt() ? static_cast<double>(rv.asInt()) : (rv.isFloat() ? rv.asFloat() : 0.0);
        i = iv.isInt() ? static_cast<double>(iv.asInt()) : (iv.isFloat() ? iv.asFloat() : 0.0);
    }
}

Value add(std::vector<Value>& args) {
    if (args.size() < 2) return Value::nil();
    double r1, i1, r2, i2;
    getParts(args[0], r1, i1);
    getParts(args[1], r2, i2);
    return makeComplex(r1 + r2, i1 + i2);
}

Value sub(std::vector<Value>& args) {
    if (args.size() < 2) return Value::nil();
    double r1, i1, r2, i2;
    getParts(args[0], r1, i1);
    getParts(args[1], r2, i2);
    return makeComplex(r1 - r2, i1 - i2);
}

Value mul(std::vector<Value>& args) {
    if (args.size() < 2) return Value::nil();
    double r1, i1, r2, i2;
    getParts(args[0], r1, i1);
    getParts(args[1], r2, i2);
    // (r1+i1)(r2+i2) = (r1r2-i1i2) + (r1i2+r2i1)i
    return makeComplex(r1 * r2 - i1 * i2, r1 * i2 + r2 * i1);
}

Value div(std::vector<Value>& args) {
    if (args.size() < 2) return Value::nil();
    double r1, i1, r2, i2;
    getParts(args[0], r1, i1);
    getParts(args[1], r2, i2);
    // (r1+i1)/(r2+i2) = ((r1r2+i1i2) + (i1r2-r1i2)i) / (r2^2+i2^2)
    double denom = r2 * r2 + i2 * i2;
    if (denom == 0.0) return Value::nil();
    return makeComplex((r1 * r2 + i1 * i2) / denom,
                       (i1 * r2 - r1 * i2) / denom);
}

Value abs_(std::vector<Value>& args) {
    if (args.empty()) return Value::Float(0.0);
    double r, i;
    getParts(args[0], r, i);
    return Value::Float(std::sqrt(r * r + i * i));
}

Value arg(std::vector<Value>& args) {
    if (args.empty()) return Value::Float(0.0);
    double r, i;
    getParts(args[0], r, i);
    return Value::Float(std::atan2(i, r));
}

Value conj(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    double r, i;
    getParts(args[0], r, i);
    return makeComplex(r, -i);
}

Value real_(std::vector<Value>& args) {
    if (args.empty()) return Value::Float(0.0);
    double r, i;
    getParts(args[0], r, i);
    return Value::Float(r);
}

Value imag(std::vector<Value>& args) {
    if (args.empty()) return Value::Float(0.0);
    double r, i;
    getParts(args[0], r, i);
    return Value::Float(i);
}
} // namespace complex

// ═══════════════════════════════════════════════════════════════════
//  对组（对标 C++ std::pair）
//  用表存储 {first, second}
// ═══════════════════════════════════════════════════════════════════

void StdLib::registerPair(VM* vm) {
    registerFunction(vm, "pair", pair_::create);
    registerFunction(vm, "pairFirst", pair_::first);
    registerFunction(vm, "pairSecond", pair_::second);
    registerFunction(vm, "pairSwap", pair_::swap_);
    
    // 中文别名
    registerAlias(vm, "对组", "pair");
    registerAlias(vm, "对组第一", "pairFirst");
    registerAlias(vm, "对组第二", "pairSecond");
    registerAlias(vm, "对组交换", "pairSwap");
}

namespace pair_ {
Value create(std::vector<Value>& args) {
    Value tbl = Value::Table(VMTable::create());
    if (args.size() >= 1)
        tbl.asTable()->set(Value::String(VMString::create("first")), args[0]);
    else
        tbl.asTable()->set(Value::String(VMString::create("first")), Value::nil());
    if (args.size() >= 2)
        tbl.asTable()->set(Value::String(VMString::create("second")), args[1]);
    else
        tbl.asTable()->set(Value::String(VMString::create("second")), Value::nil());
    return tbl;
}

Value first(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::nil();
    return args[0].asTable()->get(Value::String(VMString::create("first")));
}

Value second(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::nil();
    return args[0].asTable()->get(Value::String(VMString::create("second")));
}

Value swap_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isTable() || !args[1].isTable()) return Value::nil();
    auto t0 = args[0].asTable();
    auto t1 = args[1].asTable();
    // 全字段交换
    Value tmp0 = t0->get(Value::String(VMString::create("first")));
    Value tmp1 = t0->get(Value::String(VMString::create("second")));
    t0->set(Value::String(VMString::create("first")), t1->get(Value::String(VMString::create("first"))));
    t0->set(Value::String(VMString::create("second")), t1->get(Value::String(VMString::create("second"))));
    t1->set(Value::String(VMString::create("first")), tmp0);
    t1->set(Value::String(VMString::create("second")), tmp1);
    return Value::nil();
}
} // namespace pair_

// ═══════════════════════════════════════════════════════════════════
//  位运算实现（对标 C++ <bit>）
// ═══════════════════════════════════════════════════════════════════

