// Ordered Map (std::map<Value, Value>) module
// #include'd from stdlib.cpp, already inside namespace cplang

namespace map_ns {

// Helper: extract key type
static bool isValidKey(const Value& k) {
    return k.isInt() || k.isFloat() || k.isString();
}

Value mapNew_(std::vector<Value>&) {
    VMMap* m = VMMap::create();
    VM::current()->trackGC(m);
    return makePtrVal(m);
}

Value mapInsert_(std::vector<Value>& args) {
    if (args.size() < 3) return Value::Bool(false);
    VMMap* m = dynamic_cast<VMMap*>(args[0].isUserData() ? args[0].obj : nullptr);
    if (!m) return Value::Bool(false);
    m->data[args[1]] = args[2]; // insert or update
    return Value::Bool(true);
}

Value mapFind_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::nil();
    VMMap* m = dynamic_cast<VMMap*>(args[0].isUserData() ? args[0].obj : nullptr);
    if (!m) return Value::nil();
    auto it = m->data.find(args[1]);
    if (it != m->data.end()) return it->second;
    return Value::nil();
}

Value mapContains_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Bool(false);
    VMMap* m = dynamic_cast<VMMap*>(args[0].isUserData() ? args[0].obj : nullptr);
    if (!m) return Value::Bool(false);
    return Value::Bool(m->data.find(args[1]) != m->data.end());
}

Value mapErase_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Bool(false);
    VMMap* m = dynamic_cast<VMMap*>(args[0].isUserData() ? args[0].obj : nullptr);
    if (!m) return Value::Bool(false);
    return Value::Bool(m->data.erase(args[1]) > 0);
}

Value mapSize_(std::vector<Value>& args) {
    if (args.empty()) return Value::Int(0);
    VMMap* m = dynamic_cast<VMMap*>(args[0].isUserData() ? args[0].obj : nullptr);
    if (!m) return Value::Int(0);
    return Value::Int(static_cast<Int64>(m->data.size()));
}

Value mapIsEmpty_(std::vector<Value>& args) {
    if (args.empty()) return Value::Bool(true);
    VMMap* m = dynamic_cast<VMMap*>(args[0].isUserData() ? args[0].obj : nullptr);
    if (!m) return Value::Bool(true);
    return Value::Bool(m->data.empty());
}

Value mapClear_(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    VMMap* m = dynamic_cast<VMMap*>(args[0].isUserData() ? args[0].obj : nullptr);
    if (m) m->data.clear();
    return Value::nil();
}

Value mapLowerBound_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::nil();
    VMMap* m = dynamic_cast<VMMap*>(args[0].isUserData() ? args[0].obj : nullptr);
    if (!m) return Value::nil();
    auto it = m->data.lower_bound(args[1]);
    if (it != m->data.end()) return it->first;
    return Value::nil();
}

Value mapUpperBound_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::nil();
    VMMap* m = dynamic_cast<VMMap*>(args[0].isUserData() ? args[0].obj : nullptr);
    if (!m) return Value::nil();
    auto it = m->data.upper_bound(args[1]);
    if (it != m->data.end()) return it->first;
    return Value::nil();
}

Value mapKeys_(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    VMMap* m = dynamic_cast<VMMap*>(args[0].isUserData() ? args[0].obj : nullptr);
    if (!m) return Value::nil();
    VMArray* arr = VMArray::create();
    for (auto& kv : m->data) arr->data.push_back(kv.first);
    return makeArrayVal(arr);
}

Value mapValues_(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    VMMap* m = dynamic_cast<VMMap*>(args[0].isUserData() ? args[0].obj : nullptr);
    if (!m) return Value::nil();
    VMArray* arr = VMArray::create();
    for (auto& kv : m->data) arr->data.push_back(kv.second);
    return makeArrayVal(arr);
}

Value mapIter_(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    VMMap* m = dynamic_cast<VMMap*>(args[0].isUserData() ? args[0].obj : nullptr);
    if (!m) return Value::nil();
    VMArray* arr = VMArray::create();
    for (auto& kv : m->data) {
        VMTable* pair = VMTable::create();
        pair->set(makeStringVal(VMString::create("key")), kv.first);
        pair->set(makeStringVal(VMString::create("value")), kv.second);
        arr->data.push_back(makeTableVal(pair));
    }
    return makeArrayVal(arr);
}

} // namespace map_ns

void StdLib::registerMap(VM* vm) {
    using namespace map_ns;
    registerFunction(vm, "mapNew",       mapNew_);
    registerFunction(vm, "mapInsert",    mapInsert_);
    registerFunction(vm, "mapFind",      mapFind_);
    registerFunction(vm, "mapContains",  mapContains_);
    registerFunction(vm, "mapErase",     mapErase_);
    registerFunction(vm, "mapSize",      mapSize_);
    registerFunction(vm, "mapIsEmpty",   mapIsEmpty_);
    registerFunction(vm, "mapClear",     mapClear_);
    registerFunction(vm, "mapLowerBound", mapLowerBound_);
    registerFunction(vm, "mapUpperBound", mapUpperBound_);
    registerFunction(vm, "mapKeys",      mapKeys_);
    registerFunction(vm, "mapValues",    mapValues_);
    registerFunction(vm, "mapIter",      mapIter_);
    registerAlias(vm, "有序映射",        "mapNew");
    registerAlias(vm, "有序插入",        "mapInsert");
    registerAlias(vm, "有序查找",        "mapFind");
    registerAlias(vm, "有序包含",        "mapContains");
    registerAlias(vm, "有序删除",        "mapErase");
    registerAlias(vm, "有序大小",        "mapSize");
    registerAlias(vm, "有序为空",        "mapIsEmpty");
    registerAlias(vm, "有序清空",        "mapClear");
    registerAlias(vm, "有序下界",        "mapLowerBound");
    registerAlias(vm, "有序上界",        "mapUpperBound");
    registerAlias(vm, "有序键列表",      "mapKeys");
    registerAlias(vm, "有序值列表",      "mapValues");
    registerAlias(vm, "有序迭代",        "mapIter");
}
