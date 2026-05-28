// Misc small modules for CP stdlib: MathConst, Format, Result, Functional, Span, Charconv, SourceLoc, Memory
// #include'd from stdlib.cpp, already inside namespace cplang

// ==================== MathConst ====================
namespace mathconst_ns {
Value tau_(std::vector<Value>&) { return Value::Float(6.283185307179586); }
Value sqrt2_(std::vector<Value>&) { return Value::Float(1.4142135623730951); }
Value goldenRatio_(std::vector<Value>&) { return Value::Float(1.618033988749895); }
} // namespace mathconst_ns

void StdLib::registerMathConst(VM* vm) {
    using namespace mathconst_ns;
    registerFunction(vm, "tau",           tau_);
    registerFunction(vm, "sqrt2",         sqrt2_);
    registerFunction(vm, "goldenRatio",   goldenRatio_);
    registerAlias(vm, "圆周率两倍",       "tau");
    registerAlias(vm, "根号2",            "sqrt2");
    registerAlias(vm, "黄金比例",         "goldenRatio");
}

// ==================== Format ====================
namespace format_ns {
Value fmt_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return makeStringVal(VMString::create(""));
    std::string tmpl(args[0].asString()->data, args[0].asString()->length);
    std::string result;
    size_t pos = 0;
    for (size_t i = 0; i < tmpl.size(); i++) {
        if (tmpl[i] == '{' && i + 1 < tmpl.size() && tmpl[i+1] >= '0' && tmpl[i+1] <= '9') {
            int argIdx = 0;
            i++; // skip {
            while (i < tmpl.size() && tmpl[i] >= '0' && tmpl[i] <= '9') {
                argIdx = argIdx * 10 + (tmpl[i] - '0'); i++;
            }
            // Check for format spec (e.g., :.2f)
            bool hasSpec = false;
            if (i < tmpl.size() && tmpl[i] == ':') {
                hasSpec = true;
                i++; // skip :
                while (i < tmpl.size() && tmpl[i] != '}') {
                    // Parse format spec - simplified: just skip to }
                    i++;
                }
            }
            // Expect }
            if (i < tmpl.size() && tmpl[i] == '}') {
                if (argIdx + 1 < (int)args.size()) {
                    Value& v = args[argIdx + 1];
                    char buf[128];
                    if (v.isFloat()) {
                        snprintf(buf, sizeof(buf), "%.6g", v.asFloat());
                        result += buf;
                    } else if (v.isInt()) {
                        snprintf(buf, sizeof(buf), "%lld", (long long)v.asInt());
                        result += buf;
                    } else if (v.isString()) {
                        result += std::string(v.asString()->data, v.asString()->length);
                    } else if (v.isBool()) {
                        result += v.isTrue() ? "true" : "false";
                    } else {
                        result += "[?]";
                    }
                }
            }
        } else {
            result += tmpl[i];
        }
    }
    return makeStringVal(VMString::create(result));
}
} // namespace format_ns

void StdLib::registerFormat(VM* vm) {
    using namespace format_ns;
    registerFunction(vm, "fmt",     fmt_);
    registerAlias(vm, "格式化",     "fmt");
}

// ==================== Result ====================
namespace result_ns {
Value resOk_(std::vector<Value>& args) {
    VMTable* t = VMTable::create();
    t->set(makeStringVal(VMString::create("_ok")), Value::Bool(true));
    if (!args.empty()) t->set(makeStringVal(VMString::create("_val")), args[0]);
    return makeTableVal(t);
}
Value resErr_(std::vector<Value>& args) {
    VMTable* t = VMTable::create();
    t->set(makeStringVal(VMString::create("_ok")), Value::Bool(false));
    if (!args.empty()) t->set(makeStringVal(VMString::create("_val")), args[0]);
    return makeTableVal(t);
}
Value resIsOk_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::Bool(false);
    return args[0].asTable()->get(makeStringVal(VMString::create("_ok")));
}
Value resIsErr_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::Bool(false);
    Value ok = args[0].asTable()->get(makeStringVal(VMString::create("_ok")));
    return Value::Bool(!ok.isTrue());
}
Value resUnwrap_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::nil();
    Value ok = args[0].asTable()->get(makeStringVal(VMString::create("_ok")));
    if (!ok.isTrue()) return Value::nil();
    return args[0].asTable()->get(makeStringVal(VMString::create("_val")));
}
Value resOr_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::nil();
    if (!args[0].isTable()) return args[1];
    Value ok = args[0].asTable()->get(makeStringVal(VMString::create("_ok")));
    if (ok.isTrue()) return args[0].asTable()->get(makeStringVal(VMString::create("_val")));
    return args[1];
}
} // namespace result_ns

void StdLib::registerResult(VM* vm) {
    using namespace result_ns;
    registerFunction(vm, "resOk",       resOk_);
    registerFunction(vm, "resErr",      resErr_);
    registerFunction(vm, "resIsOk",     resIsOk_);
    registerFunction(vm, "resIsErr",    resIsErr_);
    registerFunction(vm, "resUnwrap",   resUnwrap_);
    registerFunction(vm, "resOr",       resOr_);
    registerAlias(vm, "结果成功",       "resOk");
    registerAlias(vm, "结果失败",       "resErr");
    registerAlias(vm, "结果是否成功",   "resIsOk");
    registerAlias(vm, "结果是否失败",   "resIsErr");
    registerAlias(vm, "结果解包",       "resUnwrap");
    registerAlias(vm, "结果或",         "resOr");
}

// ==================== Functional ====================
namespace functional_ns {
Value identity_(std::vector<Value>& args) {
    return args.empty() ? Value::nil() : args[0];
}
Value compare_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Int(0);
    return Value::Int(args[0].asInt() - args[1].asInt());
}
} // namespace functional_ns

void StdLib::registerFunctional(VM* vm) {
    using namespace functional_ns;
    registerFunction(vm, "identity",    identity_);
    registerFunction(vm, "compare",     compare_);
    registerAlias(vm, "恒等",           "identity");
    registerAlias(vm, "比较",           "compare");
}

// ==================== Span ====================
namespace span_ns {
Value spanNew_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isTable()) return Value::nil();
    // Span is just a VMTable with reference + start + len
    VMTable* t = VMTable::create();
    t->set(makeStringVal(VMString::create("_src")), args[0]);
    t->set(makeStringVal(VMString::create("_start")), args[1]);
    Int64 len = -1;
    if (args.size() >= 3) len = args[2].asInt();
    t->set(makeStringVal(VMString::create("_len")), Value::Int(len));
    return makeTableVal(t);
}
Value spanLen_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::Int(0);
    VMTable* t = args[0].asTable();
    Value len = t->get(makeStringVal(VMString::create("_len")));
    if (len.asInt() >= 0) return Value::Int(len.asInt());
    // Compute from source
    VMTable* src = t->get(makeStringVal(VMString::create("_src"))).asTable();
    Value start = t->get(makeStringVal(VMString::create("_start")));
    return Value::Int(Int64(src->data.size()) - start.asInt());
}
Value spanGet_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isTable()) return Value::nil();
    VMTable* t = args[0].asTable();
    VMTable* src = t->get(makeStringVal(VMString::create("_src"))).asTable();
    Int64 start = t->get(makeStringVal(VMString::create("_start"))).asInt();
    Int64 idx = args[1].asInt() + start;
    if (idx >= 0 && (size_t)idx < src->data.size())
        return src->data[(size_t)idx].second;
    return Value::nil();
}
} // namespace span_ns

void StdLib::registerSpan(VM* vm) {
    using namespace span_ns;
    registerFunction(vm, "spanNew",     spanNew_);
    registerFunction(vm, "spanLen",     spanLen_);
    registerFunction(vm, "spanGet",     spanGet_);
    registerAlias(vm, "切分",           "spanNew");
    registerAlias(vm, "切分长度",       "spanLen");
    registerAlias(vm, "切分获取",       "spanGet");
}

// ==================== Charconv ====================
namespace charconv_ns {
Value intToStr_(std::vector<Value>& args) {
    if (args.empty()) return makeStringVal(VMString::create("0"));
    int base = 10;
    if (args.size() >= 2) base = (int)args[1].asInt();
    if (base < 2 || base > 36) base = 10;
    Int64 val = args[0].asInt();
    static const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    char buf[65]; int pos = 64; buf[pos] = 0;
    bool neg = false;
    unsigned long long uval;
    if (val < 0) { neg = true; uval = (unsigned long long)(-val); }
    else uval = (unsigned long long)val;
    do { buf[--pos] = digits[uval % base]; uval /= base; } while (uval > 0);
    if (neg) buf[--pos] = '-';
    return makeStringVal(VMString::create(std::string(buf + pos)));
}
Value strToInt_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Int(0);
    int base = 10;
    if (args.size() >= 2) base = (int)args[1].asInt();
    if (base < 2 || base > 36) base = 10;
    std::string s(args[0].asString()->data, args[0].asString()->length);
    return Value::Int(std::stoll(s, nullptr, base));
}
} // namespace charconv_ns

void StdLib::registerCharconv(VM* vm) {
    using namespace charconv_ns;
    registerFunction(vm, "intToStr",    intToStr_);
    registerFunction(vm, "strToInt",    strToInt_);
    registerAlias(vm, "整数转字符串",   "intToStr");
    registerAlias(vm, "字符串转整数",   "strToInt");
}

// ==================== SourceLoc ====================
// Real implementation using VM current execution context
namespace sourceloc_ns {
Value sourceFile_(std::vector<Value>&) {
    VM* vm = VM::current();
    if (!vm) return makeStringVal(VMString::create("<unknown>"));
    std::string file = vm->getCurrentSourceFile();
    return makeStringVal(VMString::create(file));
}
Value sourceLine_(std::vector<Value>&) {
    VM* vm = VM::current();
    if (!vm) return Value::Int(0);
    return Value::Int(vm->getCurrentLine());
}
Value sourceFunc_(std::vector<Value>&) {
    VM* vm = VM::current();
    if (!vm) return makeStringVal(VMString::create("<unknown>"));
    std::string func = vm->getCurrentFunction();
    return makeStringVal(VMString::create(func));
}
} // namespace sourceloc_ns

void StdLib::registerSourceLoc(VM* vm) {
    using namespace sourceloc_ns;
    registerFunction(vm, "sourceFile",  sourceFile_);
    registerFunction(vm, "sourceLine",  sourceLine_);
    registerFunction(vm, "sourceFunc",  sourceFunc_);
    registerAlias(vm, "源文件",         "sourceFile");
    registerAlias(vm, "源行号",         "sourceLine");
    registerAlias(vm, "源函数",         "sourceFunc");
}

// ==================== Memory (Box/Rc) ====================
namespace memory_ns {
Value boxNew_(std::vector<Value>& args) {
    VMTable* t = VMTable::create();
    if (!args.empty()) t->set(makeStringVal(VMString::create("_val")), args[0]);
    t->set(makeStringVal(VMString::create("_type")), makeStringVal(VMString::create("box")));
    return makeTableVal(t);
}
Value boxGet_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::nil();
    return args[0].asTable()->get(makeStringVal(VMString::create("_val")));
}
Value boxSet_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isTable()) return Value::nil();
    args[0].asTable()->set(makeStringVal(VMString::create("_val")), args[1]);
    return Value::nil();
}
Value rcNew_(std::vector<Value>& args) {
    VMTable* t = VMTable::create();
    t->set(makeStringVal(VMString::create("_type")), makeStringVal(VMString::create("rc")));
    t->set(makeStringVal(VMString::create("_count")), Value::Int(1));
    if (!args.empty()) t->set(makeStringVal(VMString::create("_val")), args[0]);
    return makeTableVal(t);
}
Value rcCount_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::Int(0);
    return args[0].asTable()->get(makeStringVal(VMString::create("_count")));
}
Value rcGet_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::nil();
    return args[0].asTable()->get(makeStringVal(VMString::create("_val")));
}
} // namespace memory_ns

void StdLib::registerMemory(VM* vm) {
    using namespace memory_ns;
    registerFunction(vm, "boxNew",      boxNew_);
    registerFunction(vm, "boxGet",      boxGet_);
    registerFunction(vm, "boxSet",      boxSet_);
    registerFunction(vm, "rcNew",       rcNew_);
    registerFunction(vm, "rcCount",     rcCount_);
    registerFunction(vm, "rcGet",       rcGet_);
    registerAlias(vm, "包装",           "boxNew");
    registerAlias(vm, "解包",           "boxGet");
    registerAlias(vm, "引用计数创建",   "rcNew");
    registerAlias(vm, "引用计数值",     "rcCount");
}
