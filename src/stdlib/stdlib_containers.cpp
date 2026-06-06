#include "stdlib/stdlib.hpp"

namespace cplang {

// Container functions (Set, Stack, Queue, Deque, PriorityQueue, LinkedList, etc.)
// #include'd from stdlib.cpp, already inside namespace cplang

void StdLib::registerSet(VM* vm) {
    registerFunction(vm, "setNew", set::create);
    registerFunction(vm, "setAdd", set::add);
    registerFunction(vm, "setHas", set::has);
    registerFunction(vm, "setRemove", set::remove);
    registerFunction(vm, "setSize", set::size);
    registerFunction(vm, "setClear", set::clear);
    registerFunction(vm, "setUnion", set::union_);
    registerFunction(vm, "setIntersect", set::intersect);
    registerFunction(vm, "setDiff", set::diff);
    
    // 中文别名
    registerAlias(vm, "集合新建", "setNew");
    registerAlias(vm, "集合添加", "setAdd");
    registerAlias(vm, "集合包含", "setHas");
    registerAlias(vm, "集合删除", "setRemove");
    registerAlias(vm, "集合大小", "setSize");
    registerAlias(vm, "集合清空", "setClear");
    registerAlias(vm, "集合并集", "setUnion");
    registerAlias(vm, "集合交集", "setIntersect");
    registerAlias(vm, "集合差集", "setDiff");
}

namespace set {
Value create(std::vector<Value>& args) {
    return Value::Set(VMSet::create());
}

Value add(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isSet()) return Value::Bool(false);
    return Value::Bool(args[0].asSet()->add(args[1]));
}

Value has(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isSet()) return Value::Bool(false);
    return Value::Bool(args[0].asSet()->has(args[1]));
}

Value remove(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isSet()) return Value::Bool(false);
    return Value::Bool(args[0].asSet()->remove(args[1]));
}

Value size(std::vector<Value>& args) {
    if (args.empty() || !args[0].isSet()) return Value::Int(0);
    return Value::Int(static_cast<Int64>(args[0].asSet()->size()));
}

Value clear(std::vector<Value>& args) {
    if (args.empty() || !args[0].isSet()) return Value::nil();
    args[0].asSet()->clear();
    return Value::nil();
}

Value union_(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isSet() || !args[1].isSet()) return Value::nil();
    VMSet* result = VMSet::create();
    for (auto& v : args[0].asSet()->data) result->add(v);
    for (auto& v : args[1].asSet()->data) result->add(v);
    return Value::Set(result);
}

Value intersect(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isSet() || !args[1].isSet()) return Value::nil();
    VMSet* result = VMSet::create();
    for (auto& v : args[0].asSet()->data) {
        if (args[1].asSet()->has(v)) result->add(v);
    }
    return Value::Set(result);
}

Value diff(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isSet() || !args[1].isSet()) return Value::nil();
    VMSet* result = VMSet::create();
    for (auto& v : args[0].asSet()->data) {
        if (!args[1].asSet()->has(v)) result->add(v);
    }
    return Value::Set(result);
}
} // namespace set

// ═══════════════════════════════════════════════════════════════════
//  栈实现
// ═══════════════════════════════════════════════════════════════════

void StdLib::registerStack(VM* vm) {
    registerFunction(vm, "stackNew", stack::create);
    registerFunction(vm, "stackPush", stack::push);
    registerFunction(vm, "stackPop", stack::pop);
    registerFunction(vm, "stackPeek", stack::peek);
    registerFunction(vm, "stackIsEmpty", stack::isEmpty);
    registerFunction(vm, "stackSize", stack::size);
    registerFunction(vm, "stackClear", stack::clear);
    
    // 中文别名
    registerAlias(vm, "栈新建", "stackNew");
    registerAlias(vm, "栈压入", "stackPush");
    registerAlias(vm, "栈弹出", "stackPop");
    registerAlias(vm, "栈顶", "stackPeek");
    registerAlias(vm, "栈为空", "stackIsEmpty");
    registerAlias(vm, "栈大小", "stackSize");
    registerAlias(vm, "栈清空", "stackClear");
}

namespace stack {
Value create(std::vector<Value>& args) {
    return Value::Stack(VMStack::create());
}

Value push(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isStack()) return Value::nil();
    args[0].asStack()->push(args[1]);
    return Value::nil();
}

Value pop(std::vector<Value>& args) {
    if (args.empty() || !args[0].isStack()) return Value::nil();
    return args[0].asStack()->pop();
}

Value peek(std::vector<Value>& args) {
    if (args.empty() || !args[0].isStack()) return Value::nil();
    return args[0].asStack()->peek();
}

Value isEmpty(std::vector<Value>& args) {
    if (args.empty() || !args[0].isStack()) return Value::Bool(true);
    return Value::Bool(args[0].asStack()->isEmpty());
}

Value size(std::vector<Value>& args) {
    if (args.empty() || !args[0].isStack()) return Value::Int(0);
    return Value::Int(static_cast<Int64>(args[0].asStack()->size()));
}

Value clear(std::vector<Value>& args) {
    if (args.empty() || !args[0].isStack()) return Value::nil();
    args[0].asStack()->clear();
    return Value::nil();
}
} // namespace stack

// ═══════════════════════════════════════════════════════════════════
//  队列实现
// ═══════════════════════════════════════════════════════════════════

void StdLib::registerQueue(VM* vm) {
    registerFunction(vm, "queueNew", queue::create);
    registerFunction(vm, "queueEnqueue", queue::enqueue);
    registerFunction(vm, "queueDequeue", queue::dequeue);
    registerFunction(vm, "queueFront", queue::front);
    registerFunction(vm, "queueIsEmpty", queue::isEmpty);
    registerFunction(vm, "queueSize", queue::size);
    registerFunction(vm, "queueClear", queue::clear);
    
    // 中文别名
    registerAlias(vm, "队列新建", "queueNew");
    registerAlias(vm, "队列入队", "queueEnqueue");
    registerAlias(vm, "队列出队", "queueDequeue");
    registerAlias(vm, "队首", "queueFront");
    registerAlias(vm, "队列为空", "queueIsEmpty");
    registerAlias(vm, "队列大小", "queueSize");
    registerAlias(vm, "队列清空", "queueClear");
}

namespace queue {
Value create(std::vector<Value>& args) {
    return Value::Queue(VMQueue::create());
}

Value enqueue(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isQueue()) return Value::nil();
    args[0].asQueue()->enqueue(args[1]);
    return Value::nil();
}

Value dequeue(std::vector<Value>& args) {
    if (args.empty() || !args[0].isQueue()) return Value::nil();
    return args[0].asQueue()->dequeue();
}

Value front(std::vector<Value>& args) {
    if (args.empty() || !args[0].isQueue()) return Value::nil();
    return args[0].asQueue()->front();
}

Value isEmpty(std::vector<Value>& args) {
    if (args.empty() || !args[0].isQueue()) return Value::Bool(true);
    return Value::Bool(args[0].asQueue()->isEmpty());
}

Value size(std::vector<Value>& args) {
    if (args.empty() || !args[0].isQueue()) return Value::Int(0);
    return Value::Int(static_cast<Int64>(args[0].asQueue()->size()));
}

Value clear(std::vector<Value>& args) {
    if (args.empty() || !args[0].isQueue()) return Value::nil();
    args[0].asQueue()->clear();
    return Value::nil();
}
} // namespace queue

// ═══════════════════════════════════════════════════════════════════
//  双端队列（对标 C++ std::deque）
// ═══════════════════════════════════════════════════════════════════

void StdLib::registerDeque(VM* vm) {
    registerFunction(vm, "dequeNew", deque::create);
    registerFunction(vm, "dequePushBack", deque::pushBack);
    registerFunction(vm, "dequePushFront", deque::pushFront);
    registerFunction(vm, "dequePopBack", deque::popBack);
    registerFunction(vm, "dequePopFront", deque::popFront);
    registerFunction(vm, "dequeBack", deque::back);
    registerFunction(vm, "dequeFront", deque::front);
    registerFunction(vm, "dequeAt", deque::at);
    registerFunction(vm, "dequeSet", deque::set);
    registerFunction(vm, "dequeIsEmpty", deque::isEmpty);
    registerFunction(vm, "dequeSize", deque::size);
    registerFunction(vm, "dequeClear", deque::clear);
    
    // 中文别名
    registerAlias(vm, "双端队列新建", "dequeNew");
    registerAlias(vm, "双端队列尾加", "dequePushBack");
    registerAlias(vm, "双端队列头加", "dequePushFront");
    registerAlias(vm, "双端队列尾取", "dequePopBack");
    registerAlias(vm, "双端队列头取", "dequePopFront");
    registerAlias(vm, "双端队列尾", "dequeBack");
    registerAlias(vm, "双端队列首", "dequeFront");
    registerAlias(vm, "双端队列取", "dequeAt");
    registerAlias(vm, "双端队列设", "dequeSet");
    registerAlias(vm, "双端队列为空", "dequeIsEmpty");
    registerAlias(vm, "双端队列大小", "dequeSize");
    registerAlias(vm, "双端队列清空", "dequeClear");
}

namespace deque {
Value create(std::vector<Value>& args) {
    return Value::Deque(VMDeque::create());
}

Value pushBack(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isDeque()) return Value::nil();
    args[0].asDeque()->pushBack(args[1]);
    return Value::nil();
}

Value pushFront(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isDeque()) return Value::nil();
    args[0].asDeque()->pushFront(args[1]);
    return Value::nil();
}

Value popBack(std::vector<Value>& args) {
    if (args.empty() || !args[0].isDeque()) return Value::nil();
    return args[0].asDeque()->popBack();
}

Value popFront(std::vector<Value>& args) {
    if (args.empty() || !args[0].isDeque()) return Value::nil();
    return args[0].asDeque()->popFront();
}

Value back(std::vector<Value>& args) {
    if (args.empty() || !args[0].isDeque()) return Value::nil();
    return args[0].asDeque()->back();
}

Value front(std::vector<Value>& args) {
    if (args.empty() || !args[0].isDeque()) return Value::nil();
    return args[0].asDeque()->front();
}

Value at(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isDeque()) return Value::nil();
    return args[0].asDeque()->at(args[1].asInt());
}

Value set(std::vector<Value>& args) {
    if (args.size() < 3 || !args[0].isDeque()) return Value::nil();
    args[0].asDeque()->set(args[1].asInt(), args[2]);
    return Value::nil();
}

Value isEmpty(std::vector<Value>& args) {
    if (args.empty() || !args[0].isDeque()) return Value::Bool(true);
    return Value::Bool(args[0].asDeque()->isEmpty());
}

Value size(std::vector<Value>& args) {
    if (args.empty() || !args[0].isDeque()) return Value::Int(0);
    return Value::Int(static_cast<Int64>(args[0].asDeque()->size()));
}

Value clear(std::vector<Value>& args) {
    if (args.empty() || !args[0].isDeque()) return Value::nil();
    args[0].asDeque()->clear();
    return Value::nil();
}
} // namespace deque

// ═══════════════════════════════════════════════════════════════════
//  优先队列（对标 C++ std::priority_queue）
// ═══════════════════════════════════════════════════════════════════

void StdLib::registerPriorityQueue(VM* vm) {
    registerFunction(vm, "pqNew", priority_queue::create);
    registerFunction(vm, "pqPush", priority_queue::push);
    registerFunction(vm, "pqPop", priority_queue::pop);
    registerFunction(vm, "pqTop", priority_queue::top);
    registerFunction(vm, "pqTopPriority", priority_queue::topPriority);
    registerFunction(vm, "pqIsEmpty", priority_queue::isEmpty);
    registerFunction(vm, "pqSize", priority_queue::size);
    registerFunction(vm, "pqClear", priority_queue::clear);
    
    // 中文别名
    registerAlias(vm, "优先队列新建", "pqNew");
    registerAlias(vm, "优先队列入队", "pqPush");
    registerAlias(vm, "优先队列出队", "pqPop");
    registerAlias(vm, "优先队列顶", "pqTop");
    registerAlias(vm, "优先队列顶优先", "pqTopPriority");
    registerAlias(vm, "优先队列为空", "pqIsEmpty");
    registerAlias(vm, "优先队列大小", "pqSize");
    registerAlias(vm, "优先队列清空", "pqClear");
}

void StdLib::registerLinkedList(VM* vm) {
    registerFunction(vm, "listNew", linked_list::create);
    registerFunction(vm, "listPushBack", linked_list::pushBack);
    registerFunction(vm, "listPushFront", linked_list::pushFront);
    registerFunction(vm, "listPopBack", linked_list::popBack);
    registerFunction(vm, "listPopFront", linked_list::popFront);
    registerFunction(vm, "listFront", linked_list::front);
    registerFunction(vm, "listBack", linked_list::back);
    registerFunction(vm, "listIsEmpty", linked_list::isEmpty);
    registerFunction(vm, "listSize", linked_list::size);
    registerFunction(vm, "listClear", linked_list::clear);
    registerFunction(vm, "listInsert", linked_list::insert);
    registerFunction(vm, "listErase", linked_list::erase);
    registerFunction(vm, "listAt", linked_list::at);
    registerFunction(vm, "listSet", linked_list::set);
    registerFunction(vm, "listFind", linked_list::find);
    registerFunction(vm, "listSplice", linked_list::splice);

    // 中文别名
    registerAlias(vm, "链表新建", "listNew");
    registerAlias(vm, "链表尾加", "listPushBack");
    registerAlias(vm, "链表头加", "listPushFront");
    registerAlias(vm, "链表尾取", "listPopBack");
    registerAlias(vm, "链表头取", "listPopFront");
    registerAlias(vm, "链表首", "listFront");
    registerAlias(vm, "链表尾", "listBack");
    registerAlias(vm, "链表为空", "listIsEmpty");
    registerAlias(vm, "链表大小", "listSize");
    registerAlias(vm, "链表清空", "listClear");
    registerAlias(vm, "链表插入", "listInsert");
    registerAlias(vm, "链表删除", "listErase");
    registerAlias(vm, "链表取", "listAt");
    registerAlias(vm, "链表置", "listSet");
    registerAlias(vm, "链表查找", "listFind");
    registerAlias(vm, "链表拼接", "listSplice");
}

void StdLib::registerSLinkedList(VM* vm) {
    registerFunction(vm, "slNew", forward_list::create);
    registerFunction(vm, "slPushFront", forward_list::pushFront);
    registerFunction(vm, "slPopFront", forward_list::popFront);
    registerFunction(vm, "slFront", forward_list::front);
    registerFunction(vm, "slIsEmpty", forward_list::isEmpty);
    registerFunction(vm, "slSize", forward_list::size);
    registerFunction(vm, "slClear", forward_list::clear);
    registerFunction(vm, "slInsertAfter", forward_list::insertAfter);
    registerFunction(vm, "slEraseAfter", forward_list::eraseAfter);
    registerFunction(vm, "slAt", forward_list::at);
    registerFunction(vm, "slFind", forward_list::find);
    registerFunction(vm, "slReverse", forward_list::reverse);
    registerFunction(vm, "slSort", forward_list::sort);
    registerFunction(vm, "slUnique", forward_list::unique);
    registerFunction(vm, "slSpliceAfter", forward_list::spliceAfter);

    registerAlias(vm, "单向链表新建", "slNew");
    registerAlias(vm, "单向链表头加", "slPushFront");
    registerAlias(vm, "单向链表头取", "slPopFront");
    registerAlias(vm, "单向链表首", "slFront");
    registerAlias(vm, "单向链表为空", "slIsEmpty");
    registerAlias(vm, "单向链表大小", "slSize");
    registerAlias(vm, "单向链表清空", "slClear");
    registerAlias(vm, "单向链表后插", "slInsertAfter");
    registerAlias(vm, "单向链表后删", "slEraseAfter");
    registerAlias(vm, "单向链表取", "slAt");
    registerAlias(vm, "单向链表查找", "slFind");
    registerAlias(vm, "单向链表反转", "slReverse");
    registerAlias(vm, "单向链表排序", "slSort");
    registerAlias(vm, "单向链表去重", "slUnique");
    registerAlias(vm, "单向链表后拼", "slSpliceAfter");
}

void StdLib::registerMultiSet(VM* vm) {
    registerFunction(vm, "msNew", multiset_::create);
    registerFunction(vm, "msInsert", multiset_::insert);
    registerFunction(vm, "msCount", multiset_::count);
    registerFunction(vm, "msFind", multiset_::find);
    registerFunction(vm, "msEraseOne", multiset_::eraseOne);
    registerFunction(vm, "msEraseAll", multiset_::eraseAll);
    registerFunction(vm, "msContains", multiset_::contains);
    registerFunction(vm, "msIsEmpty", multiset_::isEmpty);
    registerFunction(vm, "msSize", multiset_::size);
    registerFunction(vm, "msClear", multiset_::clear);
    registerFunction(vm, "msLowerBound", multiset_::lowerBound);
    registerFunction(vm, "msUpperBound", multiset_::upperBound);

    registerAlias(vm, "多重集新建", "msNew");
    registerAlias(vm, "多重集插入", "msInsert");
    registerAlias(vm, "多重集计数", "msCount");
    registerAlias(vm, "多重集查找", "msFind");
    registerAlias(vm, "多重集删一", "msEraseOne");
    registerAlias(vm, "多重集删全", "msEraseAll");
    registerAlias(vm, "多重集包含", "msContains");
    registerAlias(vm, "多重集为空", "msIsEmpty");
    registerAlias(vm, "多重集大小", "msSize");
    registerAlias(vm, "多重集清空", "msClear");
    registerAlias(vm, "多重集下界", "msLowerBound");
    registerAlias(vm, "多重集上界", "msUpperBound");
}

void StdLib::registerMultiMap(VM* vm) {
    registerFunction(vm, "mmNew", multimap_::create);
    registerFunction(vm, "mmInsert", multimap_::insert);
    registerFunction(vm, "mmCount", multimap_::count);
    registerFunction(vm, "mmFind", multimap_::find);
    registerFunction(vm, "mmContains", multimap_::contains);
    registerFunction(vm, "mmEraseOne", multimap_::eraseOne);
    registerFunction(vm, "mmEraseAll", multimap_::eraseAll);
    registerFunction(vm, "mmIsEmpty", multimap_::isEmpty);
    registerFunction(vm, "mmSize", multimap_::size);
    registerFunction(vm, "mmClear", multimap_::clear);
    registerFunction(vm, "mmLowerBound", multimap_::lowerBound);
    registerFunction(vm, "mmUpperBound", multimap_::upperBound);
    registerFunction(vm, "mmEqualRange", multimap_::equalRange);

    registerAlias(vm, "多重映射新建", "mmNew");
    registerAlias(vm, "多重映射插入", "mmInsert");
    registerAlias(vm, "多重映射计数", "mmCount");
    registerAlias(vm, "多重映射查找", "mmFind");
    registerAlias(vm, "多重映射包含", "mmContains");
    registerAlias(vm, "多重映射删一", "mmEraseOne");
    registerAlias(vm, "多重映射删全", "mmEraseAll");
    registerAlias(vm, "多重映射为空", "mmIsEmpty");
    registerAlias(vm, "多重映射大小", "mmSize");
    registerAlias(vm, "多重映射清空", "mmClear");
    registerAlias(vm, "多重映射下界", "mmLowerBound");
    registerAlias(vm, "多重映射上界", "mmUpperBound");
    registerAlias(vm, "多重映射范围", "mmEqualRange");
}

void StdLib::registerUnorderedSet(VM* vm) {
    registerFunction(vm, "usNew", unordered_set_::create);
    registerFunction(vm, "usInsert", unordered_set_::insert);
    registerFunction(vm, "usCount", unordered_set_::count);
    registerFunction(vm, "usFind", unordered_set_::find);
    registerFunction(vm, "usContains", unordered_set_::contains);
    registerFunction(vm, "usErase", unordered_set_::erase);
    registerFunction(vm, "usIsEmpty", unordered_set_::isEmpty);
    registerFunction(vm, "usSize", unordered_set_::size);
    registerFunction(vm, "usClear", unordered_set_::clear);

    registerAlias(vm, "无序集合新建", "usNew");
    registerAlias(vm, "无序集合插入", "usInsert");
    registerAlias(vm, "无序集合计数", "usCount");
    registerAlias(vm, "无序集合查找", "usFind");
    registerAlias(vm, "无序集合包含", "usContains");
    registerAlias(vm, "无序集合删除", "usErase");
    registerAlias(vm, "无序集合为空", "usIsEmpty");
    registerAlias(vm, "无序集合大小", "usSize");
    registerAlias(vm, "无序集合清空", "usClear");
}

void StdLib::registerUnorderedMultiSet(VM* vm) {
    registerFunction(vm, "umsNew", unordered_mset_::create);
    registerFunction(vm, "umsInsert", unordered_mset_::insert);
    registerFunction(vm, "umsCount", unordered_mset_::count);
    registerFunction(vm, "umsFind", unordered_mset_::find);
    registerFunction(vm, "umsContains", unordered_mset_::contains);
    registerFunction(vm, "umsEraseOne", unordered_mset_::eraseOne);
    registerFunction(vm, "umsEraseAll", unordered_mset_::eraseAll);
    registerFunction(vm, "umsIsEmpty", unordered_mset_::isEmpty);
    registerFunction(vm, "umsSize", unordered_mset_::size);
    registerFunction(vm, "umsClear", unordered_mset_::clear);

    registerAlias(vm, "无序多重集新建", "umsNew");
    registerAlias(vm, "无序多重集插入", "umsInsert");
    registerAlias(vm, "无序多重集计数", "umsCount");
    registerAlias(vm, "无序多重集查找", "umsFind");
    registerAlias(vm, "无序多重集包含", "umsContains");
    registerAlias(vm, "无序多重集删一", "umsEraseOne");
    registerAlias(vm, "无序多重集删全", "umsEraseAll");
    registerAlias(vm, "无序多重集为空", "umsIsEmpty");
    registerAlias(vm, "无序多重集大小", "umsSize");
    registerAlias(vm, "无序多重集清空", "umsClear");
}

void StdLib::registerUnorderedMap(VM* vm) {
    registerFunction(vm, "umNew", unordered_map_::create);
    registerFunction(vm, "umInsert", unordered_map_::insert);
    registerFunction(vm, "umCount", unordered_map_::count);
    registerFunction(vm, "umFind", unordered_map_::find);
    registerFunction(vm, "umLookup", unordered_map_::lookup);
    registerFunction(vm, "umContains", unordered_map_::contains);
    registerFunction(vm, "umErase", unordered_map_::erase);
    registerFunction(vm, "umIsEmpty", unordered_map_::isEmpty);
    registerFunction(vm, "umSize", unordered_map_::size);
    registerFunction(vm, "umClear", unordered_map_::clear);

    registerAlias(vm, "无序映射新建", "umNew");
    registerAlias(vm, "无序映射插入", "umInsert");
    registerAlias(vm, "无序映射计数", "umCount");
    registerAlias(vm, "无序映射查找", "umFind");
    registerAlias(vm, "无序映射取值", "umLookup");
    registerAlias(vm, "无序映射包含", "umContains");
    registerAlias(vm, "无序映射删除", "umErase");
    registerAlias(vm, "无序映射为空", "umIsEmpty");
    registerAlias(vm, "无序映射大小", "umSize");
    registerAlias(vm, "无序映射清空", "umClear");
}

void StdLib::registerUnorderedMultiMap(VM* vm) {
    registerFunction(vm, "ummNew", unordered_mmap_::create);
    registerFunction(vm, "ummInsert", unordered_mmap_::insert);
    registerFunction(vm, "ummCount", unordered_mmap_::count);
    registerFunction(vm, "ummFind", unordered_mmap_::find);
    registerFunction(vm, "ummContains", unordered_mmap_::contains);
    registerFunction(vm, "ummEraseOne", unordered_mmap_::eraseOne);
    registerFunction(vm, "ummEraseAll", unordered_mmap_::eraseAll);
    registerFunction(vm, "ummEqualRange", unordered_mmap_::equalRange);
    registerFunction(vm, "ummIsEmpty", unordered_mmap_::isEmpty);
    registerFunction(vm, "ummSize", unordered_mmap_::size);
    registerFunction(vm, "ummClear", unordered_mmap_::clear);

    registerAlias(vm, "无序多重映射新建", "ummNew");
    registerAlias(vm, "无序多重映射插入", "ummInsert");
    registerAlias(vm, "无序多重映射计数", "ummCount");
    registerAlias(vm, "无序多重映射查找", "ummFind");
    registerAlias(vm, "无序多重映射包含", "ummContains");
    registerAlias(vm, "无序多重映射删一", "ummEraseOne");
    registerAlias(vm, "无序多重映射删全", "ummEraseAll");
    registerAlias(vm, "无序多重映射范围", "ummEqualRange");
    registerAlias(vm, "无序多重映射为空", "ummIsEmpty");
    registerAlias(vm, "无序多重映射大小", "ummSize");
    registerAlias(vm, "无序多重映射清空", "ummClear");
}

} // namespace cplang
