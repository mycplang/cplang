#!/bin/sed -f
# CP语言 NaN-boxing 迁移: vm.cpp 旧API→新API

# === Tag comparisons ===
s/\.tag == Value::T_NIL/.isNil()/g
s/\.tag == Value::T_BOOL/.isBool()/g
s/\.tag == Value::T_INT/.isInt()/g
s/\.tag == Value::T_FLOAT/.isFloat()/g
s/\.tag == Value::T_STRING/.isString()/g
s/\.tag == Value::T_ARRAY/.isArray()/g
s/\.tag == Value::T_TABLE/.isTable()/g
s/\.tag == Value::T_SET/.isSet()/g
s/\.tag == Value::T_STACK/.isStack()/g
s/\.tag == Value::T_QUEUE/.isQueue()/g
s/\.tag == Value::T_DEQUE/.isDeque()/g
s/\.tag == Value::T_PRIORITY_QUEUE/.isPriorityQueue()/g
s/\.tag == Value::T_LINKEDLIST/.isLinkedList()/g
s/\.tag == Value::T_SLINKEDLIST/.isSLinkedList()/g
s/\.tag == Value::T_MULTISET/.isMultiSet()/g
s/\.tag == Value::T_MULTIMAP/.isMultiMap()/g
s/\.tag == Value::T_UNORDERED_SET/.isUnorderedSet()/g
s/\.tag == Value::T_UNORDERED_MULTISET/.isUnorderedMultiSet()/g
s/\.tag == Value::T_UNORDERED_MAP/.isUnorderedMap()/g
s/\.tag == Value::T_UNORDERED_MULTIMAP/.isUnorderedMultiMap()/g
s/\.tag == Value::T_FUNCTION/.isFunction()/g
s/\.tag == Value::T_CFUNCTION/.isCFunction()/g
s/\.tag == Value::T_CLOSURE/.isClosure()/g
s/\.tag == Value::T_CLASS/.isClass()/g
s/\.tag == Value::T_INSTANCE/.isInstance()/g
s/\.tag == Value::T_UPVALUE/.isUpvalue()/g
s/\.tag == Value::T_USERDATA/.isUserData()/g
s/\.tag == Value::T_RAYLIB/.isRaylib()/g

# v.tag >= Value::T_STRING
s/v\.tag >= Value::T_STRING/v.isObject()/g

# === Factory methods ===
s/Value::String(/makeStringVal(/g
s/Value::Array(/makeArrayVal(/g
s/Value::Table(/makeTableVal(/g
s/Value::Function(/makeFunctionVal(/g
s/Value::Set(/makePtrVal(reinterpret_cast<VMObject*>(/g
s/Value::Stack(/makePtrVal(reinterpret_cast<VMObject*>(/g
s/Value::Queue(/makePtrVal(reinterpret_cast<VMObject*>(/g
s/Value::Deque(/makePtrVal(reinterpret_cast<VMObject*>(/g
s/Value::PriorityQueue(/makePtrVal(reinterpret_cast<VMObject*>(/g
s/Value::LinkedList(/makePtrVal(reinterpret_cast<VMObject*>(/g
s/Value::SLinkedList(/makePtrVal(reinterpret_cast<VMObject*>(/g
s/Value::MultiSet(/makePtrVal(reinterpret_cast<VMObject*>(/g
s/Value::MultiMap(/makePtrVal(reinterpret_cast<VMObject*>(/g
s/Value::UnorderedSet(/makePtrVal(reinterpret_cast<VMObject*>(/g
s/Value::UnorderedMultiSet(/makePtrVal(reinterpret_cast<VMObject*>(/g
s/Value::UnorderedMap(/makePtrVal(reinterpret_cast<VMObject*>(/g
s/Value::UnorderedMultiMap(/makePtrVal(reinterpret_cast<VMObject*>(/g
s/Value::Object(/makePtrVal(/g
s/Value::UserData(/makePtrVal(/g
s/Value::RaylibObj(/makePtrVal(/g

# === Direct member access → method calls ===
# These need to be careful: only replace when they're Value accesses, not member names
# For now we target common patterns:
s/\([^a-zA-Z_]\)v\.i\([^a-zA-Z_]\)/\1v.asInt()\2/g
s/\([^a-zA-Z_]\)v\.f\([^a-zA-Z_]\)/\1v.asFloat()\2/g
s/\([^a-zA-Z_]\)v\.obj\([^a-zA-Z_]\)/\1v.asPtr()\2/g
s/\([^a-zA-Z_]\)v\.str\([^a-zA-Z_]\)/\1v.asString()\2/g
s/\([^a-zA-Z_]\)v\.arr\([^a-zA-Z_]\)/\1v.asArray()\2/g
s/\([^a-zA-Z_]\)v\.func\([^a-zA-Z_]\)/\1v.asFunction()\2/g
