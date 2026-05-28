// Iterator module for CP stdlib
// #include'd from stdlib.cpp, already inside namespace cplang

namespace iter_ns {

// Detect container type and return string tag
static const char* detectType(const Value& v) {
    if (v.isArray())        return "array";
    if (v.isString())       return "string";
    if (v.isTable())        return "table";
    if (v.isSet())          return "set";
    if (v.isStack())        return "stack";
    if (v.isQueue())        return "queue";
    if (v.isDeque())        return "deque";
    if (v.isLinkedList())   return "linkedlist";
    return nullptr;
}

// Create iterator (state stored in VMTable)
Value iter_(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    const char* type = detectType(args[0]);
    if (!type) return Value::nil();
    VMTable* t = VMTable::create();
    t->set(makeStringVal(VMString::create("_container")), args[0]);
    t->set(makeStringVal(VMString::create("_type")), makeStringVal(VMString::create(type)));
    t->set(makeStringVal(VMString::create("_index")), Value::Int(0));
    t->set(makeStringVal(VMString::create("_total")), Value::Int(-1)); // lazy
    return makeTableVal(t);
}

static Int64 calcTotal(const Value& container, const std::string& type) {
    if (type == "array") return (Int64)container.asArray()->data.size();
    if (type == "string") return (Int64)(container.asString() ? container.asString()->length : 0);
    if (type == "table") return (Int64)container.asTable()->data.size();
    if (type == "set") return (Int64)container.asSet()->data.size();
    if (type == "stack") return (Int64)container.asStack()->data.size();
    if (type == "queue") return (Int64)container.asQueue()->data.size();
    if (type == "deque") return (Int64)container.asDeque()->data.size();
    if (type == "linkedlist") return (Int64)container.asLinkedList()->data.size();
    return 0;
}

static Value getAt(const Value& container, const std::string& type, Int64 idx) {
    if (type == "array") {
        auto& d = container.asArray()->data;
        return (idx >= 0 && (size_t)idx < d.size()) ? d[(size_t)idx] : Value::nil();
    }
    if (type == "string") {
        auto* s = container.asString();
        if (s && idx >= 0 && (size_t)idx < s->length) {
            char buf[2] = {s->data[idx], 0};
            return makeStringVal(VMString::create(std::string(buf)));
        }
        return Value::nil();
    }
    if (type == "table") {
        auto& d = container.asTable()->data;
        return (idx >= 0 && (size_t)idx < d.size()) ? d[(size_t)idx].first : Value::nil();
    }
    if (type == "set") {
        auto& d = container.asSet()->data;
        return (idx >= 0 && (size_t)idx < d.size()) ? d[(size_t)idx] : Value::nil();
    }
    if (type == "stack") {
        auto& d = container.asStack()->data;
        if (idx >= 0 && (size_t)idx < d.size()) {
            return d[(size_t)idx];
        }
        return Value::nil();
    }
    if (type == "queue") {
        auto& d = container.asQueue()->data;
        return (idx >= 0 && (size_t)idx < d.size()) ? d[(size_t)idx] : Value::nil();
    }
    if (type == "deque") {
        auto& d = container.asDeque()->data;
        if (idx >= 0 && (size_t)idx < d.size()) {
            auto it = d.begin(); std::advance(it, (size_t)idx);
            return *it;
        }
        return Value::nil();
    }
    if (type == "linkedlist") {
        auto& d = container.asLinkedList()->data;
        if (idx >= 0 && (size_t)idx < d.size()) {
            auto it = d.begin(); std::advance(it, (size_t)idx);
            return *it;
        }
        return Value::nil();
    }
    return Value::nil();
}

Value iterHasNext_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::Bool(false);
    VMTable* t = args[0].asTable();
    VMString* ts = t->get(makeStringVal(VMString::create("_type"))).asString();
    std::string type(ts->data, ts->length);
    
    // Range iterator
    if (type == "range") {
        Int64 cur = t->get(makeStringVal(VMString::create("_cur"))).asInt();
        Int64 end = t->get(makeStringVal(VMString::create("_end"))).asInt();
        Int64 step = t->get(makeStringVal(VMString::create("_step"))).asInt();
        if (step > 0) return Value::Bool(cur < end);
        else return Value::Bool(cur > end);
    }
    
    // Container iterator
    Int64 idx = t->get(makeStringVal(VMString::create("_index"))).asInt();
    // Check for reverse step
    Int64 step = t->get(makeStringVal(VMString::create("_step"))).asInt();
    if (step == 0) step = 1; // default forward
    Value totalV = t->get(makeStringVal(VMString::create("_total")));
    Int64 total = totalV.asInt();
    if (total < 0) {
        total = calcTotal(t->get(makeStringVal(VMString::create("_container"))), type);
        t->set(makeStringVal(VMString::create("_total")), Value::Int(total));
    }
    if (step < 0) return Value::Bool(idx >= 0);
    return Value::Bool(idx < total);
}

Value iterNext_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::nil();
    VMTable* t = args[0].asTable();
    VMString* ts = t->get(makeStringVal(VMString::create("_type"))).asString();
    std::string type(ts->data, ts->length);
    
    // Range iterator
    if (type == "range") {
        Int64 cur = t->get(makeStringVal(VMString::create("_cur"))).asInt();
        Int64 step = t->get(makeStringVal(VMString::create("_step"))).asInt();
        t->set(makeStringVal(VMString::create("_cur")), Value::Int(cur + step));
        Int64 idx = t->get(makeStringVal(VMString::create("_index"))).asInt();
        t->set(makeStringVal(VMString::create("_index")), Value::Int(idx + 1));
        return Value::Int(cur);
    }
    
    // Container iterator
    Value cont = t->get(makeStringVal(VMString::create("_container")));
    Int64 idx = t->get(makeStringVal(VMString::create("_index"))).asInt();
    Int64 step = t->get(makeStringVal(VMString::create("_step"))).asInt();
    if (step == 0) step = 1;
    Value v = getAt(cont, type, idx);
    t->set(makeStringVal(VMString::create("_index")), Value::Int(idx + step));
    return v;
}

Value iterReset_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::nil();
    args[0].asTable()->set(makeStringVal(VMString::create("_index")), Value::Int(0));
    return Value::nil();
}

Value iterCount_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::Int(0);
    VMTable* t = args[0].asTable();
    Value cont = t->get(makeStringVal(VMString::create("_container")));
    std::string type(t->get(makeStringVal(VMString::create("_type"))).asString()->data,
                     t->get(makeStringVal(VMString::create("_type"))).asString()->length);
    Value totalV = t->get(makeStringVal(VMString::create("_total")));
    Int64 total = totalV.asInt();
    if (total < 0) { total = calcTotal(cont, type); t->set(makeStringVal(VMString::create("_total")), Value::Int(total)); }
    return Value::Int(total);
}

// ── iter_ext ──
Value iterRange_(std::vector<Value>& args) {
    Int64 start = 0, end = 0, step = 1;
    if (args.size() >= 2) { start = args[0].asInt(); end = args[1].asInt(); }
    if (args.size() >= 3) step = args[2].asInt();
    if (step == 0) return Value::nil();
    VMTable* t = VMTable::create();
    t->set(makeStringVal(VMString::create("_container")), Value::nil());
    t->set(makeStringVal(VMString::create("_type")), makeStringVal(VMString::create("range")));
    t->set(makeStringVal(VMString::create("_start")), Value::Int(start));
    t->set(makeStringVal(VMString::create("_cur")), Value::Int(start));
    t->set(makeStringVal(VMString::create("_end")), Value::Int(end));
    t->set(makeStringVal(VMString::create("_step")), Value::Int(step));
    Int64 total = (step > 0) ? std::max(Int64(0), (end - start + step - 1) / step)
                             : std::max(Int64(0), (start - end - step - 1) / (-step));
    t->set(makeStringVal(VMString::create("_total")), Value::Int(total));
    t->set(makeStringVal(VMString::create("_index")), Value::Int(0));
    return makeTableVal(t);
}

Value iterReverse_(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    const char* type = detectType(args[0]);
    if (!type) return Value::nil();
    VMTable* t = VMTable::create();
    Int64 total = calcTotal(args[0], type);
    t->set(makeStringVal(VMString::create("_container")), args[0]);
    t->set(makeStringVal(VMString::create("_type")), makeStringVal(VMString::create(type)));
    t->set(makeStringVal(VMString::create("_index")), Value::Int(total - 1));
    t->set(makeStringVal(VMString::create("_step")), Value::Int(-1));
    t->set(makeStringVal(VMString::create("_total")), Value::Int(total));
    return makeTableVal(t);
}
// Override iterHasNext/iterNext for reverse case
// NOTE: iterHasNext and iterNext above handle general case.
// For reverse iterators, step is -1 and index should go down.
// The existing iterNext increments index, so we need a special case.
// Let's handle it inline in the general functions.

// Override: special reverse handling in iterHasNext & iterNext

Value iterPos_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::Int(0);
    VMTable* t = args[0].asTable();
    Value idx = t->get(makeStringVal(VMString::create("_index")));
    return idx;
}

Value iterRemaining_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::Int(0);
    VMTable* t = args[0].asTable();
    VMString* typeStr = t->get(makeStringVal(VMString::create("_type"))).asString();
    std::string type(typeStr->data, typeStr->length);
    Int64 idx = t->get(makeStringVal(VMString::create("_index"))).asInt();
    Value totalV = t->get(makeStringVal(VMString::create("_total")));
    Int64 total = totalV.asInt();
    if (type == "range" || total >= 0) {
        Int64 step = t->get(makeStringVal(VMString::create("_step"))).asInt();
        if (step < 0) return Value::Int(idx + 1);
        return Value::Int(total - idx);
    }
    if (total < 0) { total = calcTotal(t->get(makeStringVal(VMString::create("_container"))), type); }
    return Value::Int(total - idx);
}

Value iterSkip_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isTable()) return Value::nil();
    VMTable* t = args[0].asTable();
    Int64 idx = t->get(makeStringVal(VMString::create("_index"))).asInt();
    Int64 skip = args[1].asInt();
    t->set(makeStringVal(VMString::create("_index")), Value::Int(idx + skip));
    return Value::nil();
}

Value iterPeek_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTable()) return Value::nil();
    VMTable* t = args[0].asTable();
    Value cont = t->get(makeStringVal(VMString::create("_container")));
    VMString* typeStr = t->get(makeStringVal(VMString::create("_type"))).asString();
    std::string type(typeStr->data, typeStr->length);
    Int64 idx = t->get(makeStringVal(VMString::create("_index"))).asInt();
    if (type == "range") {
        Int64 cur = t->get(makeStringVal(VMString::create("_cur"))).asInt();
        return Value::Int(cur);
    }
    return getAt(cont, type, idx);
}

} // namespace iter_ns

void StdLib::registerIterator(VM* vm) {
    using namespace iter_ns;
    registerFunction(vm, "iter",           iter_);
    registerFunction(vm, "iterHasNext",    iterHasNext_);
    registerFunction(vm, "iterNext",       iterNext_);
    registerFunction(vm, "iterReset",      iterReset_);
    registerFunction(vm, "iterCount",      iterCount_);
    registerFunction(vm, "iterRange",      iterRange_);
    registerFunction(vm, "iterReverse",    iterReverse_);
    registerFunction(vm, "iterPos",        iterPos_);
    registerFunction(vm, "iterRemaining",  iterRemaining_);
    registerFunction(vm, "iterSkip",       iterSkip_);
    registerFunction(vm, "iterPeek",       iterPeek_);
    registerAlias(vm, "迭代器",            "iter");
    registerAlias(vm, "迭代有下一个",      "iterHasNext");
    registerAlias(vm, "迭代下一个",        "iterNext");
    registerAlias(vm, "迭代重置",          "iterReset");
    registerAlias(vm, "迭代计数",          "iterCount");
    registerAlias(vm, "迭代范围",          "iterRange");
    registerAlias(vm, "迭代反向",          "iterReverse");
    registerAlias(vm, "迭代位置",          "iterPos");
    registerAlias(vm, "迭代剩余",          "iterRemaining");
    registerAlias(vm, "迭代跳过",          "iterSkip");
    registerAlias(vm, "迭代预览",          "iterPeek");
}
