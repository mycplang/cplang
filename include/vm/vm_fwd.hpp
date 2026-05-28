#pragma once
#include "common/types.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace cplang {

class HybridJIT;

struct VMObject;
struct VMString;
struct VMArray;
struct VMTable;
struct VMSet;
struct VMStack;
struct VMQueue;
struct VMDeque;
struct VMPriorityQueue;
struct VMLinkedList;
struct VMSLinkedList;
struct VMMultiSet;
struct VMMultiMap;
struct VMOrderedSet;
struct VMOrderedMap;
struct VMUnorderedSet;
struct VMUnorderedMultiSet;
struct VMUnorderedMap;
struct VMUnorderedMultiMap;
struct VMFunction;
struct VMClosure;
struct VMClass;
struct VMInstance;
struct VMUpvalue;
struct VMNativeFunc;
class Value;
class VM;

enum class GCColor : UInt8 { WHITE = 0, GRAY = 1, BLACK = 2 };

struct ExecContext {
    VMFunction* func = nullptr;
    UInt8* code = nullptr;
    size_t codeSize = 0;
    size_t pc = 0;
    Value* base = nullptr;
    size_t baseOffset = 0;
};

struct ObjectHeader {
    VMObject* next = nullptr;
    UInt32    size = 0;
    GCColor   color = GCColor::WHITE;
    UInt8     typeTag = 0;
    UInt16    flags = 0;
    enum Tag : UInt8 {
        TAG_STRING=0, TAG_ARRAY=1, TAG_TABLE=2, TAG_SET=3,
        TAG_STACK=4, TAG_QUEUE=5, TAG_DEQUE=6, TAG_PRIORITY_QUEUE=7,
        TAG_FUNCTION=8, TAG_CLOSURE=9, TAG_CLASS=10, TAG_INSTANCE=11,
        TAG_UPVALUE=12, TAG_NATIVE=13, TAG_STRUCT=14, TAG_USERDATA=15,
        TAG_LINKEDLIST=16, TAG_SLINKEDLIST=17, TAG_MULTISET=18,
        TAG_MULTIMAP=19,
        TAG_ORDERED_SET=40, TAG_ORDERED_MAP=41,
        TAG_UNORDERED_SET=20, TAG_UNORDERED_MULTISET=21,
        TAG_UNORDERED_MAP=22, TAG_UNORDERED_MULTIMAP=23,
        TAG_THREAD=24, TAG_MUTEX=25, TAG_CONDITION=26,
        TAG_SEMAPHORE=27, TAG_ATOMIC_INT=28, TAG_BARRIER=29, TAG_FUTURE=30,
        TAG_CHANNEL=31, TAG_RWLOCK=32, TAG_WEBSOCKET=33, TAG_MAP=34,
        TAG_TEXTURE2D=35, TAG_IMAGE=36, TAG_SOUND=37, TAG_MUSIC=38, TAG_FONT=39,
        TAG_BOXED_INT64=42, TAG_BOXED_FLOAT=43
    };
    bool isWhite() const { return color == GCColor::WHITE; }
    void setGray()  { color = GCColor::GRAY; }
    void setBlack() { color = GCColor::BLACK; }
    void setWhite() { color = GCColor::WHITE; }
};

struct VMObject : ObjectHeader {
    virtual ~VMObject() {}
};

} // namespace cplang
