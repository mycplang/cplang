
void StdLib::registerBitset(VM* vm) {
    registerFunction(vm, "bitsetSet", bitset_::set_);
    registerFunction(vm, "bitsetClear", bitset_::clear_);
    registerFunction(vm, "bitsetToggle", bitset_::toggle);
    registerFunction(vm, "bitsetTest", bitset_::test);
    registerFunction(vm, "bitsetCount", bitset_::count);
    registerFunction(vm, "bitsetAll", bitset_::all);
    registerFunction(vm, "bitsetAny", bitset_::any);
    registerFunction(vm, "bitsetNone", bitset_::none);
    registerFunction(vm, "bitsetFlip", bitset_::flip);
    registerFunction(vm, "bitsetToString", bitset_::toBinaryString);
    registerFunction(vm, "bitsetFromString", bitset_::fromBinaryString);

    registerAlias(vm, "位集置位", "bitsetSet");
    registerAlias(vm, "位集清零", "bitsetClear");
    registerAlias(vm, "位集翻转位", "bitsetToggle");
    registerAlias(vm, "位集测位", "bitsetTest");
    registerAlias(vm, "位集计数", "bitsetCount");
    registerAlias(vm, "位集全置", "bitsetAll");
    registerAlias(vm, "位集任一", "bitsetAny");
    registerAlias(vm, "位集无置", "bitsetNone");
    registerAlias(vm, "位集翻转", "bitsetFlip");
    registerAlias(vm, "位集转字符串", "bitsetToString");
    registerAlias(vm, "字符串转位集", "bitsetFromString");
}

namespace priority_queue {
Value create(std::vector<Value>& args) {
    return Value::PriorityQueue(VMPriorityQueue::create());
}

Value push(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isPriorityQueue()) return Value::nil();
    double pri = 0.0;
    if (args.size() >= 3) {
        pri = args[2].isInt() ? static_cast<double>(args[2].asInt()) : args[2].asFloat();
    } else {
        // 无优先级时用值本身作优先级（数字）
        if (args[1].isInt()) pri = static_cast<double>(args[1].asInt());
        else if (args[1].isFloat()) pri = args[1].asFloat();
    }
    args[0].asPriorityQueue()->push(args[1], pri);
    return Value::nil();
}

Value pop(std::vector<Value>& args) {
    if (args.empty() || !args[0].isPriorityQueue()) return Value::nil();
    return args[0].asPriorityQueue()->pop();
}

Value top(std::vector<Value>& args) {
    if (args.empty() || !args[0].isPriorityQueue()) return Value::nil();
    return args[0].asPriorityQueue()->top();
}

Value topPriority(std::vector<Value>& args) {
    if (args.empty() || !args[0].isPriorityQueue()) return Value::Float(0.0);
    return Value::Float(args[0].asPriorityQueue()->topPriority());
}

Value isEmpty(std::vector<Value>& args) {
    if (args.empty() || !args[0].isPriorityQueue()) return Value::Bool(true);
    return Value::Bool(args[0].asPriorityQueue()->isEmpty());
}

Value size(std::vector<Value>& args) {
    if (args.empty() || !args[0].isPriorityQueue()) return Value::Int(0);
    return Value::Int(static_cast<Int64>(args[0].asPriorityQueue()->size()));
}

Value clear(std::vector<Value>& args) {
    if (args.empty() || !args[0].isPriorityQueue()) return Value::nil();
    args[0].asPriorityQueue()->clear();
    return Value::nil();
}
} // namespace priority_queue

// ═══════════════════════════════════════════════════════════════════
//  双向链表（对标 C++ std::list）
// ═══════════════════════════════════════════════════════════════════

namespace linked_list {
Value create(std::vector<Value>& args) {
    return Value::LinkedList(VMLinkedList::create());
}

Value pushBack(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isLinkedList()) return Value::nil();
    args[0].asLinkedList()->pushBack(args[1]);
    return Value::nil();
}

Value pushFront(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isLinkedList()) return Value::nil();
    args[0].asLinkedList()->pushFront(args[1]);
    return Value::nil();
}

Value popBack(std::vector<Value>& args) {
    if (args.empty() || !args[0].isLinkedList()) return Value::nil();
    return args[0].asLinkedList()->popBack();
}

Value popFront(std::vector<Value>& args) {
    if (args.empty() || !args[0].isLinkedList()) return Value::nil();
    return args[0].asLinkedList()->popFront();
}

Value front(std::vector<Value>& args) {
    if (args.empty() || !args[0].isLinkedList()) return Value::nil();
    return args[0].asLinkedList()->front();
}

Value back(std::vector<Value>& args) {
    if (args.empty() || !args[0].isLinkedList()) return Value::nil();
    return args[0].asLinkedList()->back();
}

Value isEmpty(std::vector<Value>& args) {
    if (args.empty() || !args[0].isLinkedList()) return Value::Bool(true);
    return Value::Bool(args[0].asLinkedList()->isEmpty());
}

Value size(std::vector<Value>& args) {
    if (args.empty() || !args[0].isLinkedList()) return Value::Int(0);
    return Value::Int(static_cast<Int64>(args[0].asLinkedList()->size()));
}

Value clear(std::vector<Value>& args) {
    if (args.empty() || !args[0].isLinkedList()) return Value::nil();
    args[0].asLinkedList()->clear();
    return Value::nil();
}

Value insert(std::vector<Value>& args) {
    if (args.size() < 3 || !args[0].isLinkedList()) return Value::nil();
    Int64 pos = args[1].isInt() ? args[1].asInt() : 0;
    args[0].asLinkedList()->insert(pos, args[2]);
    return Value::nil();
}

Value erase(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isLinkedList()) return Value::nil();
    Int64 pos = args[1].isInt() ? args[1].asInt() : 0;
    return args[0].asLinkedList()->erase(pos);
}

Value at(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isLinkedList()) return Value::nil();
    Int64 pos = args[1].isInt() ? args[1].asInt() : 0;
    return args[0].asLinkedList()->at(pos);
}

Value set(std::vector<Value>& args) {
    if (args.size() < 3 || !args[0].isLinkedList()) return Value::nil();
    Int64 pos = args[1].isInt() ? args[1].asInt() : 0;
    args[0].asLinkedList()->set(pos, args[2]);
    return Value::nil();
}

Value find(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isLinkedList()) return Value::Int(-1);
    return Value::Int(args[0].asLinkedList()->find(args[1]));
}

Value splice(std::vector<Value>& args) {
    if (args.size() < 3 || !args[0].isLinkedList() || !args[2].isLinkedList()) return Value::nil();
    Int64 pos = args[1].isInt() ? args[1].asInt() : 0;
    args[0].asLinkedList()->splice(pos, args[2].asLinkedList());
    return Value::nil();
}
} // namespace linked_list

// ═══════════════════════════════════════════════════════════════════
//  单向链表（对标 C++ std::forward_list）
// ═══════════════════════════════════════════════════════════════════

namespace forward_list {
Value create(std::vector<Value>& args) {
    return Value::SLinkedList(VMSLinkedList::create());
}

Value pushFront(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isSLinkedList()) return Value::nil();
    args[0].asSLinkedList()->pushFront(args[1]);
    return Value::nil();
}

Value popFront(std::vector<Value>& args) {
    if (args.empty() || !args[0].isSLinkedList()) return Value::nil();
    return args[0].asSLinkedList()->popFront();
}

Value front(std::vector<Value>& args) {
    if (args.empty() || !args[0].isSLinkedList()) return Value::nil();
    return args[0].asSLinkedList()->front();
}

Value isEmpty(std::vector<Value>& args) {
    if (args.empty() || !args[0].isSLinkedList()) return Value::Bool(true);
    return Value::Bool(args[0].asSLinkedList()->isEmpty());
}

Value size(std::vector<Value>& args) {
    if (args.empty() || !args[0].isSLinkedList()) return Value::Int(0);
    return Value::Int(static_cast<Int64>(args[0].asSLinkedList()->size()));
}

Value clear(std::vector<Value>& args) {
    if (args.empty() || !args[0].isSLinkedList()) return Value::nil();
    args[0].asSLinkedList()->clear();
    return Value::nil();
}

Value insertAfter(std::vector<Value>& args) {
    if (args.size() < 3 || !args[0].isSLinkedList()) return Value::nil();
    Int64 pos = args[1].isInt() ? args[1].asInt() : 0;
    args[0].asSLinkedList()->insertAfter(pos, args[2]);
    return Value::nil();
}

Value eraseAfter(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isSLinkedList()) return Value::nil();
    Int64 pos = args[1].isInt() ? args[1].asInt() : 0;
    return args[0].asSLinkedList()->eraseAfter(pos);
}

Value at(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isSLinkedList()) return Value::nil();
    Int64 pos = args[1].isInt() ? args[1].asInt() : 0;
    return args[0].asSLinkedList()->at(pos);
}

Value find(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isSLinkedList()) return Value::Int(-1);
    return Value::Int(args[0].asSLinkedList()->find(args[1]));
}

Value reverse(std::vector<Value>& args) {
    if (args.empty() || !args[0].isSLinkedList()) return Value::nil();
    args[0].asSLinkedList()->reverse();
    return Value::nil();
}

Value sort(std::vector<Value>& args) {
    if (args.empty() || !args[0].isSLinkedList()) return Value::nil();
    args[0].asSLinkedList()->sort();
    return Value::nil();
}

Value unique(std::vector<Value>& args) {
    if (args.empty() || !args[0].isSLinkedList()) return Value::nil();
    args[0].asSLinkedList()->unique();
    return Value::nil();
}

Value spliceAfter(std::vector<Value>& args) {
    if (args.size() < 3 || !args[0].isSLinkedList() || !args[2].isSLinkedList()) return Value::nil();
    Int64 pos = args[1].isInt() ? args[1].asInt() : 0;
    args[0].asSLinkedList()->spliceAfter(pos, args[2].asSLinkedList());
    return Value::nil();
}
} // namespace forward_list

// ═══════════════════════════════════════════════════════════════════
//  多重集合（对标 C++ std::multiset）
// ═══════════════════════════════════════════════════════════════════

namespace multiset_ {
Value create(std::vector<Value>& args) {
    return Value::MultiSet(VMMultiSet::create());
}

Value insert(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isMultiSet()) return Value::nil();
    args[0].asMultiSet()->insert(args[1]);
    return Value::nil();
}

Value count(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isMultiSet()) return Value::Int(0);
    return Value::Int(args[0].asMultiSet()->count(args[1]));
}

Value find(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isMultiSet()) return Value::nil();
    return args[0].asMultiSet()->find(args[1]);
}

Value eraseOne(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isMultiSet()) return Value::Int(0);
    return Value::Int(args[0].asMultiSet()->eraseOne(args[1]));
}

Value eraseAll(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isMultiSet()) return Value::Int(0);
    return Value::Int(args[0].asMultiSet()->eraseAll(args[1]));
}

Value contains(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isMultiSet()) return Value::Bool(false);
    return Value::Bool(args[0].asMultiSet()->contains(args[1]));
}

Value isEmpty(std::vector<Value>& args) {
    if (args.empty() || !args[0].isMultiSet()) return Value::Bool(true);
    return Value::Bool(args[0].asMultiSet()->isEmpty());
}

Value size(std::vector<Value>& args) {
    if (args.empty() || !args[0].isMultiSet()) return Value::Int(0);
    return Value::Int(static_cast<Int64>(args[0].asMultiSet()->size()));
}

Value clear(std::vector<Value>& args) {
    if (args.empty() || !args[0].isMultiSet()) return Value::nil();
    args[0].asMultiSet()->clear();
    return Value::nil();
}

Value lowerBound(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isMultiSet()) return Value::nil();
    return args[0].asMultiSet()->lowerBound(args[1]);
}

Value upperBound(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isMultiSet()) return Value::nil();
    return args[0].asMultiSet()->upperBound(args[1]);
}
} // namespace multiset_

// ═══════════════════════════════════════════════════════════════════
//  多重映射（对标 C++ std::multimap）
// ═══════════════════════════════════════════════════════════════════

namespace multimap_ {
Value create(std::vector<Value>& args) {
    return Value::MultiMap(VMMultiMap::create());
}

Value insert(std::vector<Value>& args) {
    if (args.size() < 3 || !args[0].isMultiMap()) return Value::nil();
    args[0].asMultiMap()->insert(args[1], args[2]);
    return Value::nil();
}

Value count(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isMultiMap()) return Value::Int(0);
    return Value::Int(args[0].asMultiMap()->count(args[1]));
}

Value find(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isMultiMap()) return Value::nil();
    return args[0].asMultiMap()->find(args[1]);
}

Value contains(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isMultiMap()) return Value::Bool(false);
    return Value::Bool(args[0].asMultiMap()->contains(args[1]));
}

Value eraseOne(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isMultiMap()) return Value::Int(0);
    return Value::Int(args[0].asMultiMap()->eraseOne(args[1]));
}

Value eraseAll(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isMultiMap()) return Value::Int(0);
    return Value::Int(args[0].asMultiMap()->eraseAll(args[1]));
}

Value isEmpty(std::vector<Value>& args) {
    if (args.empty() || !args[0].isMultiMap()) return Value::Bool(true);
    return Value::Bool(args[0].asMultiMap()->isEmpty());
}

Value size(std::vector<Value>& args) {
    if (args.empty() || !args[0].isMultiMap()) return Value::Int(0);
    return Value::Int(static_cast<Int64>(args[0].asMultiMap()->size()));
}

Value clear(std::vector<Value>& args) {
    if (args.empty() || !args[0].isMultiMap()) return Value::nil();
    args[0].asMultiMap()->clear();
    return Value::nil();
}

Value lowerBound(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isMultiMap()) return Value::nil();
    return args[0].asMultiMap()->lowerBound(args[1]);
}

Value upperBound(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isMultiMap()) return Value::nil();
    return args[0].asMultiMap()->upperBound(args[1]);
}

Value equalRange(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isMultiMap()) return Value::Array(VMArray::create());
    return args[0].asMultiMap()->equalRange(args[1]);
}
} // namespace multimap_

// ═══════════════════════════════════════════════════════════════════
//  无序集合（对标 C++ std::unordered_set）
// ═══════════════════════════════════════════════════════════════════

namespace unordered_set_ {
Value create(std::vector<Value>& args) {
    return Value::UnorderedSet(VMUnorderedSet::create());
}
Value insert(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isUnorderedSet()) return Value::nil();
    args[0].asUnorderedSet()->insert(args[1]);
    return Value::nil();
}
Value count(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isUnorderedSet()) return Value::Int(0);
    return Value::Int(args[0].asUnorderedSet()->count(args[1]));
}
Value find(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isUnorderedSet()) return Value::nil();
    return args[0].asUnorderedSet()->find(args[1]);
}
Value contains(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isUnorderedSet()) return Value::Bool(false);
    return Value::Bool(args[0].asUnorderedSet()->contains(args[1]));
}
Value erase(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isUnorderedSet()) return Value::Int(0);
    return Value::Int(args[0].asUnorderedSet()->erase(args[1]));
}
Value isEmpty(std::vector<Value>& args) {
    if (args.empty() || !args[0].isUnorderedSet()) return Value::Bool(true);
    return Value::Bool(args[0].asUnorderedSet()->isEmpty());
}
Value size(std::vector<Value>& args) {
    if (args.empty() || !args[0].isUnorderedSet()) return Value::Int(0);
    return Value::Int(static_cast<Int64>(args[0].asUnorderedSet()->size()));
}
Value clear(std::vector<Value>& args) {
    if (args.empty() || !args[0].isUnorderedSet()) return Value::nil();
    args[0].asUnorderedSet()->clear();
    return Value::nil();
}
} // namespace unordered_set_

// ═══════════════════════════════════════════════════════════════════
//  无序多重集合（对标 C++ std::unordered_multiset）
// ═══════════════════════════════════════════════════════════════════

namespace unordered_mset_ {
Value create(std::vector<Value>& args) {
    return Value::UnorderedMultiSet(VMUnorderedMultiSet::create());
}
Value insert(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isUnorderedMultiSet()) return Value::nil();
    args[0].asUnorderedMultiSet()->insert(args[1]);
    return Value::nil();
}
Value count(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isUnorderedMultiSet()) return Value::Int(0);
    return Value::Int(args[0].asUnorderedMultiSet()->count(args[1]));
}
Value find(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isUnorderedMultiSet()) return Value::nil();
    return args[0].asUnorderedMultiSet()->find(args[1]);
}
Value contains(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isUnorderedMultiSet()) return Value::Bool(false);
    return Value::Bool(args[0].asUnorderedMultiSet()->contains(args[1]));
}
Value eraseOne(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isUnorderedMultiSet()) return Value::Int(0);
    return Value::Int(args[0].asUnorderedMultiSet()->eraseOne(args[1]));
}
Value eraseAll(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isUnorderedMultiSet()) return Value::Int(0);
    return Value::Int(args[0].asUnorderedMultiSet()->eraseAll(args[1]));
}
Value isEmpty(std::vector<Value>& args) {
    if (args.empty() || !args[0].isUnorderedMultiSet()) return Value::Bool(true);
    return Value::Bool(args[0].asUnorderedMultiSet()->isEmpty());
}
Value size(std::vector<Value>& args) {
    if (args.empty() || !args[0].isUnorderedMultiSet()) return Value::Int(0);
    return Value::Int(static_cast<Int64>(args[0].asUnorderedMultiSet()->size()));
}
Value clear(std::vector<Value>& args) {
    if (args.empty() || !args[0].isUnorderedMultiSet()) return Value::nil();
    args[0].asUnorderedMultiSet()->clear();
    return Value::nil();
}
} // namespace unordered_mset_

// ═══════════════════════════════════════════════════════════════════
//  无序映射（对标 C++ std::unordered_map）
// ═══════════════════════════════════════════════════════════════════

namespace unordered_map_ {
Value create(std::vector<Value>& args) {
    return Value::UnorderedMap(VMUnorderedMap::create());
}
Value insert(std::vector<Value>& args) {
    if (args.size() < 3 || !args[0].isUnorderedMap()) return Value::nil();
    args[0].asUnorderedMap()->insert(args[1], args[2]);
    return Value::nil();
}
Value count(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isUnorderedMap()) return Value::Int(0);
    return Value::Int(args[0].asUnorderedMap()->count(args[1]));
}
Value find(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isUnorderedMap()) return Value::nil();
    return args[0].asUnorderedMap()->find(args[1]);
}
Value lookup(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isUnorderedMap()) return Value::nil();
    return args[0].asUnorderedMap()->lookup(args[1]);
}
Value contains(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isUnorderedMap()) return Value::Bool(false);
    return Value::Bool(args[0].asUnorderedMap()->contains(args[1]));
}
Value erase(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isUnorderedMap()) return Value::Int(0);
    return Value::Int(args[0].asUnorderedMap()->erase(args[1]));
}
Value isEmpty(std::vector<Value>& args) {
    if (args.empty() || !args[0].isUnorderedMap()) return Value::Bool(true);
    return Value::Bool(args[0].asUnorderedMap()->isEmpty());
}
Value size(std::vector<Value>& args) {
    if (args.empty() || !args[0].isUnorderedMap()) return Value::Int(0);
    return Value::Int(static_cast<Int64>(args[0].asUnorderedMap()->size()));
}
Value clear(std::vector<Value>& args) {
    if (args.empty() || !args[0].isUnorderedMap()) return Value::nil();
    args[0].asUnorderedMap()->clear();
    return Value::nil();
}
} // namespace unordered_map_

// ═══════════════════════════════════════════════════════════════════
//  无序多重映射（对标 C++ std::unordered_multimap）
// ═══════════════════════════════════════════════════════════════════

namespace unordered_mmap_ {
Value create(std::vector<Value>& args) {
    return Value::UnorderedMultiMap(VMUnorderedMultiMap::create());
}
Value insert(std::vector<Value>& args) {
    if (args.size() < 3 || !args[0].isUnorderedMultiMap()) return Value::nil();
    args[0].asUnorderedMultiMap()->insert(args[1], args[2]);
    return Value::nil();
}
Value count(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isUnorderedMultiMap()) return Value::Int(0);
    return Value::Int(args[0].asUnorderedMultiMap()->count(args[1]));
}
Value find(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isUnorderedMultiMap()) return Value::nil();
    return args[0].asUnorderedMultiMap()->find(args[1]);
}
Value contains(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isUnorderedMultiMap()) return Value::Bool(false);
    return Value::Bool(args[0].asUnorderedMultiMap()->contains(args[1]));
}
Value eraseOne(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isUnorderedMultiMap()) return Value::Int(0);
    return Value::Int(args[0].asUnorderedMultiMap()->eraseOne(args[1]));
}
Value eraseAll(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isUnorderedMultiMap()) return Value::Int(0);
    return Value::Int(args[0].asUnorderedMultiMap()->eraseAll(args[1]));
}
Value equalRange(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isUnorderedMultiMap()) return Value::Array(VMArray::create());
    return args[0].asUnorderedMultiMap()->equalRange(args[1]);
}
Value isEmpty(std::vector<Value>& args) {
    if (args.empty() || !args[0].isUnorderedMultiMap()) return Value::Bool(true);
    return Value::Bool(args[0].asUnorderedMultiMap()->isEmpty());
}
Value size(std::vector<Value>& args) {
    if (args.empty() || !args[0].isUnorderedMultiMap()) return Value::Int(0);
    return Value::Int(static_cast<Int64>(args[0].asUnorderedMultiMap()->size()));
}
Value clear(std::vector<Value>& args) {
    if (args.empty() || !args[0].isUnorderedMultiMap()) return Value::nil();
    args[0].asUnorderedMultiMap()->clear();
    return Value::nil();
}
} // namespace unordered_mmap_

// ═══════════════════════════════════════════════════════════════════
//  位集（对标 C++ std::bitset）
// ═══════════════════════════════════════════════════════════════════

namespace bitset_ {
Value set_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Int(0);
    Int64 n = args[0].isInt() ? args[0].asInt() : 0;
    Int64 bit = args[1].isInt() ? args[1].asInt() : 0;
    if (bit < 0 || bit >= 64) return Value::Int(n);
    return Value::Int(n | (1LL << bit));
}

Value clear_(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Int(0);
    Int64 n = args[0].isInt() ? args[0].asInt() : 0;
    Int64 bit = args[1].isInt() ? args[1].asInt() : 0;
    if (bit < 0 || bit >= 64) return Value::Int(n);
    return Value::Int(n & ~(1ULL << bit));
}

Value toggle(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Int(0);
    Int64 n = args[0].isInt() ? args[0].asInt() : 0;
    Int64 bit = args[1].isInt() ? args[1].asInt() : 0;
    if (bit < 0 || bit >= 64) return Value::Int(n);
    return Value::Int(n ^ (1ULL << bit));
}

Value test(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Bool(false);
    Int64 n = args[0].isInt() ? args[0].asInt() : 0;
    Int64 bit = args[1].isInt() ? args[1].asInt() : 0;
    if (bit < 0 || bit >= 64) return Value::Bool(false);
    return Value::Bool((n >> bit) & 1);
}

Value count(std::vector<Value>& args) {
    if (args.empty()) return Value::Int(0);
    UInt64 n = args[0].isInt() ? static_cast<UInt64>(args[0].asInt()) : 0;
    int cnt = 0;
    while (n) { cnt++; n &= n - 1; }
    return Value::Int(cnt);
}

Value all(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Bool(false);
    Int64 n = args[0].isInt() ? args[0].asInt() : 0;
    Int64 bits = args[1].isInt() ? args[1].asInt() : 0;
    if (bits <= 0 || bits > 64) return Value::Bool(false);
    UInt64 mask = (bits >= 64) ? ~0ULL : ((1ULL << bits) - 1);
    return Value::Bool((static_cast<UInt64>(n) & mask) == mask);
}

Value any(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Bool(false);
    Int64 n = args[0].isInt() ? args[0].asInt() : 0;
    Int64 bits = args[1].isInt() ? args[1].asInt() : 0;
    if (bits <= 0 || bits > 64) return Value::Bool(n != 0);
    UInt64 mask = (bits >= 64) ? ~0ULL : ((1ULL << bits) - 1);
    return Value::Bool((static_cast<UInt64>(n) & mask) != 0);
}

Value none(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Bool(true);
    Int64 n = args[0].isInt() ? args[0].asInt() : 0;
    Int64 bits = args[1].isInt() ? args[1].asInt() : 0;
    if (bits <= 0 || bits > 64) return Value::Bool(n == 0);
    UInt64 mask = (bits >= 64) ? ~0ULL : ((1ULL << bits) - 1);
    return Value::Bool((static_cast<UInt64>(n) & mask) == 0);
}

Value flip(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Int(0);
    Int64 n = args[0].isInt() ? args[0].asInt() : 0;
    Int64 bits = args[1].isInt() ? args[1].asInt() : 0;
    if (bits <= 0 || bits > 64) return Value::Int(~n);
    UInt64 mask = (bits >= 64) ? ~0ULL : ((1ULL << bits) - 1);
    return Value::Int(static_cast<Int64>(static_cast<UInt64>(n) ^ mask));
}

Value toBinaryString(std::vector<Value>& args) {
    if (args.size() < 2) return Value::String(VMString::create("0"));
    Int64 n = args[0].isInt() ? args[0].asInt() : 0;
    Int64 bits = args[1].isInt() ? args[1].asInt() : 0;
    if (bits <= 0 || bits > 64) bits = 8;
    UInt64 u = static_cast<UInt64>(n);
    std::string s(bits, '0');
    for (Int64 i = 0; i < bits && i < 64; i++) {
        s[bits - 1 - i] = ((u >> i) & 1) ? '1' : '0';
    }
    return Value::String(VMString::create(s));
}

Value fromBinaryString(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Int(0);
    std::string s(args[0].asString()->data, args[0].asString()->length);
    UInt64 n = 0;
    for (char c : s) {
        n <<= 1;
        if (c == '1') n |= 1;
    }
    return Value::Int(static_cast<Int64>(n));
}
} // namespace bitset_
