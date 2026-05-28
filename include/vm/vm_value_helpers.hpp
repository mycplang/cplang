#pragma once
#include "vm/value.hpp"
#include "vm/vm_object.hpp"

namespace cplang {

// 向后兼容内联包装
inline Value makeStringVal(VMString* s) { return Value::Ptr(reinterpret_cast<VMObject*>(s)); }
inline Value makeArrayVal(VMArray* a)   { return Value::Ptr(reinterpret_cast<VMObject*>(a)); }
inline Value makeTableVal(VMTable* t)   { return Value::Ptr(reinterpret_cast<VMObject*>(t)); }
inline Value makeFunctionVal(VMFunction* f) { return Value::Ptr(reinterpret_cast<VMObject*>(f)); }
inline Value makePtrVal(VMObject* o)    { return Value::Ptr(o); }

// 对象类型检测
inline bool isStringVal(const Value& v)  { return v.isPtr() && v.asPtr() && v.asPtr()->typeTag == ObjectHeader::TAG_STRING; }
inline bool isArrayVal(const Value& v)   { return v.isPtr() && v.asPtr() && v.asPtr()->typeTag == ObjectHeader::TAG_ARRAY; }
inline bool isTableVal(const Value& v)   { return v.isPtr() && v.asPtr() && v.asPtr()->typeTag == ObjectHeader::TAG_TABLE; }
inline bool isFunctionVal(const Value& v){ return v.isPtr() && v.asPtr() && v.asPtr()->typeTag == ObjectHeader::TAG_FUNCTION; }
inline bool isSetVal(const Value& v)     { return v.isPtr() && v.asPtr() && v.asPtr()->typeTag == ObjectHeader::TAG_SET; }
inline bool isStackVal(const Value& v)   { return v.isPtr() && v.asPtr() && v.asPtr()->typeTag == ObjectHeader::TAG_STACK; }
inline bool isQueueVal(const Value& v)   { return v.isPtr() && v.asPtr() && v.asPtr()->typeTag == ObjectHeader::TAG_QUEUE; }

inline VMString*  asStringVal(const Value& v)  { return reinterpret_cast<VMString*>(v.asPtr()); }
inline VMArray*   asArrayVal(const Value& v)   { return reinterpret_cast<VMArray*>(v.asPtr()); }
inline VMTable*   asTableVal(const Value& v)   { return reinterpret_cast<VMTable*>(v.asPtr()); }
inline VMFunction* asFunctionVal(const Value& v){ return reinterpret_cast<VMFunction*>(v.asPtr()); }
inline VMSet*     asSetVal(const Value& v)     { return reinterpret_cast<VMSet*>(v.asPtr()); }
inline VMStack*   asStackVal(const Value& v)   { return reinterpret_cast<VMStack*>(v.asPtr()); }
inline VMQueue*   asQueueVal(const Value& v)   { return reinterpret_cast<VMQueue*>(v.asPtr()); }
inline VMObject*  asObjectVal(const Value& v)  { return v.asPtr(); }

// 宏定义 Value::is*() 方法
#define DEF_VALUE_IS(NAME, TAG_CONST) \
    inline bool Value::is##NAME() const { return isPtr() && asPtr() && asPtr()->typeTag == ObjectHeader::TAG_CONST; }

DEF_VALUE_IS(String,     TAG_STRING)
DEF_VALUE_IS(Array,      TAG_ARRAY)
DEF_VALUE_IS(Table,      TAG_TABLE)
DEF_VALUE_IS(Set,        TAG_SET)
DEF_VALUE_IS(Stack,      TAG_STACK)
DEF_VALUE_IS(Queue,      TAG_QUEUE)
DEF_VALUE_IS(Deque,      TAG_DEQUE)
DEF_VALUE_IS(PriorityQueue, TAG_PRIORITY_QUEUE)
DEF_VALUE_IS(LinkedList,    TAG_LINKEDLIST)
DEF_VALUE_IS(SLinkedList,   TAG_SLINKEDLIST)
DEF_VALUE_IS(MultiSet,      TAG_MULTISET)
DEF_VALUE_IS(MultiMap,      TAG_MULTIMAP)
DEF_VALUE_IS(OrderedSet,    TAG_ORDERED_SET)
DEF_VALUE_IS(OrderedMap,    TAG_ORDERED_MAP)
DEF_VALUE_IS(UnorderedSet,        TAG_UNORDERED_SET)
DEF_VALUE_IS(UnorderedMultiSet,   TAG_UNORDERED_MULTISET)
DEF_VALUE_IS(UnorderedMap,        TAG_UNORDERED_MAP)
DEF_VALUE_IS(UnorderedMultiMap,   TAG_UNORDERED_MULTIMAP)
DEF_VALUE_IS(Function,    TAG_FUNCTION)
DEF_VALUE_IS(Closure,     TAG_CLOSURE)
DEF_VALUE_IS(Class,       TAG_CLASS)
DEF_VALUE_IS(Instance,    TAG_INSTANCE)
DEF_VALUE_IS(Upvalue,     TAG_UPVALUE)
DEF_VALUE_IS(CFunction,   TAG_NATIVE)
DEF_VALUE_IS(UserData,    TAG_USERDATA)
DEF_VALUE_IS(WebSocket,   TAG_WEBSOCKET)

#undef DEF_VALUE_IS

// Raylib 特殊
inline bool Value::isRaylib() const {
    if (!isPtr() || !asPtr()) return false;
    auto t = asPtr()->typeTag;
    return t == ObjectHeader::TAG_TEXTURE2D || t == ObjectHeader::TAG_IMAGE
        || t == ObjectHeader::TAG_SOUND || t == ObjectHeader::TAG_MUSIC || t == ObjectHeader::TAG_FONT;
}

// 线程/同步类型
#define DEF_VALUE_IS2(NAME, TAG_CONST) \
    inline bool Value::is##NAME() const { return isPtr() && asPtr() && asPtr()->typeTag == ObjectHeader::TAG_CONST; }
DEF_VALUE_IS2(Thread,      TAG_THREAD)
DEF_VALUE_IS2(Mutex,       TAG_MUTEX)
DEF_VALUE_IS2(Condition,   TAG_CONDITION)
DEF_VALUE_IS2(Semaphore,   TAG_SEMAPHORE)
DEF_VALUE_IS2(AtomicInt,   TAG_ATOMIC_INT)
DEF_VALUE_IS2(Channel,     TAG_CHANNEL)
#undef DEF_VALUE_IS2

// 对象类型 as*() 访问器
#define DEF_VALUE_AS(NAME, TYPE) \
    inline TYPE* Value::as##NAME() const { return reinterpret_cast<TYPE*>(asPtr()); }

DEF_VALUE_AS(String,   VMString)
DEF_VALUE_AS(Array,    VMArray)
DEF_VALUE_AS(Table,    VMTable)
DEF_VALUE_AS(Set,      VMSet)
DEF_VALUE_AS(Stack,    VMStack)
DEF_VALUE_AS(Queue,    VMQueue)
DEF_VALUE_AS(Deque,    VMDeque)
DEF_VALUE_AS(PriorityQueue, VMPriorityQueue)
DEF_VALUE_AS(LinkedList,    VMLinkedList)
DEF_VALUE_AS(SLinkedList,   VMSLinkedList)
DEF_VALUE_AS(MultiSet,      VMMultiSet)
DEF_VALUE_AS(MultiMap,      VMMultiMap)
DEF_VALUE_AS(OrderedSet,   VMOrderedSet)
DEF_VALUE_AS(OrderedMap,   VMOrderedMap)
DEF_VALUE_AS(UnorderedSet,        VMUnorderedSet)
DEF_VALUE_AS(UnorderedMultiSet,   VMUnorderedMultiSet)
DEF_VALUE_AS(UnorderedMap,        VMUnorderedMap)
DEF_VALUE_AS(UnorderedMultiMap,   VMUnorderedMultiMap)
DEF_VALUE_AS(Function,  VMFunction)
DEF_VALUE_AS(Closure,   VMClosure)
DEF_VALUE_AS(UserData,  VMObject)

#undef DEF_VALUE_AS

inline bool Value::isObject() const { return isPtr(); }

} // namespace cplang
