#pragma once
#include "vm/vm_fwd.hpp"
#include "vm/value.hpp"
#include <vector>
#include <deque>
#include <list>
#include <forward_list>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <shared_mutex>
#include <functional>
#include <string>

namespace cplang {

struct VMString : VMObject {
    VMString() { typeTag = TAG_STRING; data = nullptr; }
    ~VMString() { delete[] data; }
    UInt32 hash = 0;
    UInt32 length = 0;
    char*  data = nullptr;
    static VMString* create(const char* s, UInt32 len);
    static VMString* create(const std::string& s);
    const char* c_str() const { return data ? data : ""; }
};

struct VMArray : VMObject {
    VMArray() { typeTag = TAG_ARRAY; }
    std::vector<Value> data;
    static VMArray* create(UInt32 cap = 0);
    Value get(Int64 index);
    void set(Int64 index, const Value& v);
    Int64 length() const { return static_cast<Int64>(data.size()); }
};

struct VMTable : VMObject {
    VMTable() { typeTag = TAG_TABLE; }
    static VMTable* create();
    Value get(const Value& key);
    void set(const Value& key, const Value& val);
    bool has(const Value& key);
    bool remove(const Value& key);
    void clear();
    size_t size() const;
    std::vector<std::pair<Value, Value>> data;
    struct HashEntry { Value key; Value value; bool occupied = false; };
    std::vector<HashEntry> buckets;
    size_t count = 0;
    void rehash(size_t newSize);
    size_t findSlot(const Value& key) const;
    static size_t hashValue(const Value& v);
};

struct VMSet : VMObject {
    VMSet() { typeTag = TAG_SET; }
    static VMSet* create();
    bool add(const Value& key);
    bool has(const Value& key);
    bool remove(const Value& key);
    void clear();
    size_t size() const;
    std::vector<Value> data;
    struct HashEntry { Value key; bool occupied = false; };
    std::vector<HashEntry> buckets;
    size_t count = 0;
    void rehash(size_t newSize);
    size_t findSlot(const Value& key) const;
    static size_t hashValue(const Value& v);
};

struct VMStack : VMObject {
    VMStack() { typeTag = TAG_STACK; }
    std::vector<Value> data;
    static VMStack* create();
    void push(const Value& v);
    Value pop();
    Value peek() const;
    bool isEmpty() const;
    size_t size() const;
    void clear();
};

struct VMQueue : VMObject {
    VMQueue() { typeTag = TAG_QUEUE; }
    std::deque<Value> data;
    static VMQueue* create();
    void enqueue(const Value& v);
    Value dequeue();
    Value front() const;
    bool isEmpty() const;
    size_t size() const;
    void clear();
};

struct VMDeque : VMObject {
    VMDeque() { typeTag = TAG_DEQUE; }
    std::deque<Value> data;
    static VMDeque* create();
    void pushBack(const Value& v);
    void pushFront(const Value& v);
    Value popBack();
    Value popFront();
    Value back() const;
    Value front() const;
    Value at(Int64 index) const;
    void set(Int64 index, const Value& v);
    bool isEmpty() const;
    size_t size() const;
    void clear();
};

struct VMPriorityQueue : VMObject {
    VMPriorityQueue() { typeTag = TAG_PRIORITY_QUEUE; }
    struct Entry { Value value; double priority = 0.0;
        bool operator<(const Entry& other) const { return priority < other.priority; } };
    std::vector<Entry> heap;
    static VMPriorityQueue* create();
    void push(const Value& v, double priority = 0.0);
    Value pop();
    Value top() const;
    double topPriority() const;
    bool isEmpty() const;
    size_t size() const;
    void clear();
};

struct VMLinkedList : VMObject {
    VMLinkedList() { typeTag = TAG_LINKEDLIST; }
    std::list<Value> data;
    static VMLinkedList* create();
    void pushBack(const Value& v); void pushFront(const Value& v);
    Value popBack(); Value popFront();
    Value front() const; Value back() const;
    bool isEmpty() const; size_t size() const; void clear();
    void insert(Int64 pos, const Value& v); Value erase(Int64 pos);
    Value at(Int64 pos) const; void set(Int64 pos, const Value& v);
    Int64 find(const Value& v) const; void splice(Int64 pos, VMLinkedList* other);
};

struct VMSLinkedList : VMObject {
    VMSLinkedList() { typeTag = TAG_SLINKEDLIST; }
    std::forward_list<Value> data;
    size_t _size = 0;
    static VMSLinkedList* create();
    void pushFront(const Value& v); Value popFront(); Value front() const;
    bool isEmpty() const; size_t size() const; void clear();
    void insertAfter(Int64 pos, const Value& v); Value eraseAfter(Int64 pos);
    Int64 find(const Value& v) const; void reverse(); void sort(); void unique();
    Value at(Int64 pos) const; void spliceAfter(Int64 pos, VMSLinkedList* other);
};

struct VMMultiSet : VMObject {
    VMMultiSet() { typeTag = TAG_MULTISET; }
    std::multiset<Value, ValueLess> data;
    static VMMultiSet* create();
    void insert(const Value& v); Int64 count(const Value& v) const;
    Value find(const Value& v) const; bool contains(const Value& v) const;
    Int64 eraseOne(const Value& v); Int64 eraseAll(const Value& v);
    bool isEmpty() const; size_t size() const; void clear();
    Value lowerBound(const Value& v) const; Value upperBound(const Value& v) const;
};

struct VMMultiMap : VMObject {
    VMMultiMap() { typeTag = TAG_MULTIMAP; }
    std::multimap<Value, Value, ValueLess> data;
    static VMMultiMap* create();
    void insert(const Value& key, const Value& val); Int64 count(const Value& key) const;
    Value find(const Value& key) const; bool contains(const Value& key) const;
    Int64 eraseOne(const Value& key); Int64 eraseAll(const Value& key);
    bool isEmpty() const; size_t size() const; void clear();
    Value lowerBound(const Value& key) const; Value upperBound(const Value& key) const;
    Value equalRange(const Value& key) const;
};

struct VMOrderedSet : VMObject {
    VMOrderedSet() { typeTag = TAG_ORDERED_SET; }
    std::set<Value, ValueLess> data;
    static VMOrderedSet* create();
    void insert(const Value& v); Int64 count(const Value& v) const;
    Value find(const Value& v) const; bool contains(const Value& v) const;
    Int64 erase(const Value& v); bool isEmpty() const; size_t size() const; void clear();
    Value lowerBound(const Value& v) const; Value upperBound(const Value& v) const;
    Value min() const; Value max() const;
};

struct VMOrderedMap : VMObject {
    VMOrderedMap() { typeTag = TAG_ORDERED_MAP; }
    std::map<Value, Value, ValueLess> data;
    static VMOrderedMap* create();
    void insert(const Value& key, const Value& val); Int64 count(const Value& key) const;
    Value find(const Value& key) const; Value lookup(const Value& key) const;
    bool contains(const Value& key) const; Int64 erase(const Value& key);
    bool isEmpty() const; size_t size() const; void clear();
    Value lowerBound(const Value& key) const; Value upperBound(const Value& key) const;
    Value minKey() const; Value maxKey() const;
};

struct VMUnorderedSet : VMObject {
    VMUnorderedSet() { typeTag = TAG_UNORDERED_SET; }
    std::unordered_set<Value, ValueHash, ValueEqual> data;
    static VMUnorderedSet* create();
    void insert(const Value& v); Int64 count(const Value& v) const;
    Value find(const Value& v) const; bool contains(const Value& v) const;
    Int64 erase(const Value& v); bool isEmpty() const; size_t size() const; void clear();
};

struct VMUnorderedMultiSet : VMObject {
    VMUnorderedMultiSet() { typeTag = TAG_UNORDERED_MULTISET; }
    std::unordered_multiset<Value, ValueHash, ValueEqual> data;
    static VMUnorderedMultiSet* create();
    void insert(const Value& v); Int64 count(const Value& v) const;
    Value find(const Value& v) const; bool contains(const Value& v) const;
    Int64 eraseOne(const Value& v); Int64 eraseAll(const Value& v);
    bool isEmpty() const; size_t size() const; void clear();
};

struct VMUnorderedMap : VMObject {
    VMUnorderedMap() { typeTag = TAG_UNORDERED_MAP; }
    std::unordered_map<Value, Value, ValueHash, ValueEqual> data;
    static VMUnorderedMap* create();
    void insert(const Value& key, const Value& val); Int64 count(const Value& key) const;
    Value find(const Value& key) const; bool contains(const Value& key) const;
    Int64 erase(const Value& key); Value lookup(const Value& key);
    bool isEmpty() const; size_t size() const; void clear();
};

struct VMUnorderedMultiMap : VMObject {
    VMUnorderedMultiMap() { typeTag = TAG_UNORDERED_MULTIMAP; }
    std::unordered_multimap<Value, Value, ValueHash, ValueEqual> data;
    static VMUnorderedMultiMap* create();
    void insert(const Value& key, const Value& val); Int64 count(const Value& key) const;
    Value find(const Value& key) const; bool contains(const Value& key) const;
    Int64 eraseOne(const Value& key); Int64 eraseAll(const Value& key);
    Value equalRange(const Value& key) const; bool isEmpty() const; size_t size() const; void clear();
};

struct VMThread : VMObject {
    VMThread() { typeTag = TAG_THREAD; }
    ~VMThread() override { if (th.joinable()) th.detach(); }
    VMThread(const VMThread&) = delete;
    VMThread& operator=(const VMThread&) = delete;
    std::thread th;
    static VMThread* create();
};

struct VMMutex : VMObject {
    VMMutex() { typeTag = TAG_MUTEX; }
    std::mutex mtx;
    static VMMutex* create();
};

struct VMCondition : VMObject {
    VMCondition() { typeTag = TAG_CONDITION; }
    std::condition_variable cv;
    static VMCondition* create();
};

struct VMSemaphore : VMObject {
    VMSemaphore() { typeTag = TAG_SEMAPHORE; }
    std::mutex mtx; std::condition_variable cv; Int64 count = 0;
    static VMSemaphore* create();
};

struct VMAtomicInt : VMObject {
    VMAtomicInt() { typeTag = TAG_ATOMIC_INT; }
    std::atomic<Int64> value{0};
    static VMAtomicInt* create();
};

struct VMBarrier : VMObject {
    VMBarrier() { typeTag = TAG_BARRIER; }
    std::mutex mtx; std::condition_variable cv;
    Int64 target = 0, current = 0, generation = 0;
    static VMBarrier* create();
};

struct VMFuture : VMObject {
    VMFuture() { typeTag = TAG_FUTURE; }
    std::mutex mtx; std::condition_variable cv;
    Value result; bool ready = false; std::thread worker;
    static VMFuture* create();
};

struct VMChannel : VMObject {
    VMChannel() { typeTag = TAG_CHANNEL; }
    std::queue<Value> q; std::mutex mtx;
    std::condition_variable notEmpty, notFull;
    size_t capacity = 0; bool closed = false;
    static VMChannel* create();
};

struct VMRWLock : VMObject {
    VMRWLock() { typeTag = TAG_RWLOCK; }
    std::shared_mutex smtx;
    static VMRWLock* create();
};

struct VMWebSocket : VMObject {
    VMWebSocket() { typeTag = TAG_WEBSOCKET; }
    ~VMWebSocket() override;
    VMWebSocket(const VMWebSocket&) = delete;
    VMWebSocket& operator=(const VMWebSocket&) = delete;
    void* hWebSocket = nullptr; void* hSession = nullptr; void* hConnect = nullptr;
    bool closed = false; std::mutex mtx;
    static VMWebSocket* create();
};

struct VMTexture2D : VMObject {
    VMTexture2D() { typeTag = TAG_TEXTURE2D; }
    ~VMTexture2D() override;
    unsigned int id = 0; int width = 0, height = 0, mipmaps = 1, format = 0;
    static VMTexture2D* create(unsigned int id, int w, int h, int mipmaps, int fmt);
};

struct VMImage : VMObject {
    VMImage() { typeTag = TAG_IMAGE; }
    ~VMImage() override;
    void* data = nullptr; int width = 0, height = 0, mipmaps = 1, format = 0;
    static VMImage* create(void* data, int w, int h, int mipmaps, int fmt);
};

struct VMSound : VMObject {
    VMSound() { typeTag = TAG_SOUND; }
    ~VMSound() override;
    unsigned char rlData[64] = {};
    static VMSound* create();
};

struct VMMusic : VMObject {
    VMMusic() { typeTag = TAG_MUSIC; }
    ~VMMusic() override;
    unsigned char rlData[128] = {};
    static VMMusic* create();
};

struct VMFont : VMObject {
    VMFont() { typeTag = TAG_FONT; }
    ~VMFont() override;
    unsigned int baseSize = 0, glyphCount = 0, glyphPadding = 0;
    void* texture = nullptr; void* recs = nullptr; void* glyphs = nullptr;
    static VMFont* create();
};

struct VMMap : VMObject {
    struct KeyCompare {
        bool operator()(const Value& a, const Value& b) const {
            if (a.isInt() && b.isInt()) return a.asInt() < b.asInt();
            if (a.isFloat() && b.isFloat()) return a.asFloat() < b.asFloat();
            if (a.isString() && b.isString()) {
                auto* sa = a.asString(); auto* sb = b.asString();
                size_t ml = sa->length < sb->length ? sa->length : sb->length;
                int cmp = std::memcmp(sa->data, sb->data, ml);
                if (cmp != 0) return cmp < 0;
                return sa->length < sb->length;
            }
            return false;
        }
    };
    VMMap() { typeTag = TAG_MAP; }
    std::map<Value, Value, KeyCompare> data;
    static VMMap* create();
};

struct VMFunction : VMObject {
    VMFunction() { typeTag = TAG_FUNCTION; }
    VMString* name = nullptr;
    UInt32 maxStack = 256, numParams = 0, numLocals = 0;
    bool isVararg = false;
    Int32 upvalueCount = 0;
    bool hasSlots = false;
    bool isGenerator = false;  // 是否为生成器函数（P9.1）
    std::vector<UInt8> code;
    std::vector<Value> constants;
    std::vector<Int32> lineInfo;
    std::string sourceFile;
    void* jitEntry = nullptr;
    bool jitCompiled = false;
    bool isTyped = false;
    bool hasExplicitTypes = false;
    std::string disassemble() const;
};

struct VMClosure : VMObject {
    VMClosure() { typeTag = TAG_CLOSURE; }
    VMFunction* func = nullptr;
    std::vector<VMUpvalue*> upvalues;
    static VMClosure* create(VMFunction* f);
};

struct VMClass : VMObject {
    VMClass() { typeTag = TAG_CLASS; }
    VMString* name = nullptr;
    VMClass* base = nullptr;
    std::vector<VMString*> fieldNames;
    std::vector<VMFunction*> methods;
    static VMClass* create(VMString* name);
};

struct VMInstance : VMObject {
    VMInstance() { typeTag = TAG_INSTANCE; }
    VMClass* cls = nullptr;
    std::vector<Value> fields;
    static VMInstance* create(VMClass* c);
    Value getField(Int32 index);
    void setField(Int32 index, const Value& v);
};

struct VMStructDef : VMObject {
    VMStructDef() { typeTag = TAG_STRUCT; }
    VMString* name = nullptr;
    std::vector<VMString*> fieldNames;
    std::vector<Int32> fieldOffsets;
    size_t totalSize = 0;
    static VMStructDef* create(VMString* name);
    Int32 getFieldIndex(const char* name);
};

struct VMStruct : VMObject {
    VMStruct() { typeTag = TAG_STRUCT; }
    VMStructDef* def = nullptr;
    std::vector<Value> fields;
    static VMStruct* create(VMStructDef* def);
    Value getField(Int32 index);
    void setField(Int32 index, const Value& v);
};

struct VMUpvalue : VMObject {
    VMUpvalue() { typeTag = TAG_UPVALUE; }
    Value* location = nullptr;
    Value  closed = Value::nil();
    VMUpvalue* next = nullptr;
    static VMUpvalue* create(Value* slot);
};

// 生成器对象（P9.1）
struct VMGenerator : VMObject {
    VMGenerator() { typeTag = TAG_GENERATOR; }
    VMFunction* func = nullptr;
    VMClosure* closure = nullptr;
    std::vector<Value> stack;     // 保存的栈/寄存器数据
    Int32 pcOffset = 0;           // 当前执行位置（字节码偏移）
    Int32 baseOffset = 0;         // 基址偏移
    Int32 yieldReg = 0;           // yield 指令的目标寄存器（恢复时写入发送值）
    bool isDone = false;          // 是否执行完毕
    bool isClosed = false;        // 是否已关闭
    Value sentValue;              // 通过send发送的值
    bool hasSentValue = false;    // 是否有待处理的发送值
    std::vector<VMUpvalue*> upvalues;  // 闭包上值（如果是闭包生成器）
    
    static VMGenerator* create(VMFunction* f, VMClosure* cl = nullptr);
    
    // 恢复执行，返回产出值；执行完毕返回nil
    Value resume(VM* vm, const Value& sendValue = Value::nil());
};

// 承诺/期约对象（P9.3 原生协程）
struct VMPromise : VMObject {
    enum State {
        PENDING = 0,
        FULFILLED = 1,
        REJECTED = 2
    };
    
    VMPromise() { typeTag = TAG_PROMISE; }
    State state = PENDING;
    Value result;           // 完成值或拒绝原因
    std::vector<Value> thenCallbacks;   // then 回调函数列表
    std::vector<Value> catchCallbacks;  // catch 回调函数列表
    std::vector<VMGenerator*> waitingCoroutines;  // 等待此承诺的协程
    
    static VMPromise* create();
    
    // 解决承诺（成功）
    void resolve(const Value& value);
    // 拒绝承诺（失败）
    void reject(const Value& reason);
};

// ═══════════════════════════════════════════════════════════════════
//  VMByteArray — 字节数组（二进制数据缓冲区）
// ═══════════════════════════════════════════════════════════════════
struct VMByteArray : VMObject {
    VMByteArray() { typeTag = TAG_BYTEARRAY; data = nullptr; }
    ~VMByteArray() { delete[] data; }
    
    UInt8*  data     = nullptr;  // 堆分配的原始字节缓冲区
    UInt32  length   = 0;        // 已用长度（逻辑视图）
    UInt32  capacity = 0;        // 分配容量
    
    // 切片支持（零拷贝视图）
    VMByteArray* parent = nullptr; // 如果是切片，指向父对象
    UInt32       offset = 0;       // 切片在父对象中的偏移
    
    /// 创建指定大小的零填充字节数组
    static VMByteArray* create(UInt32 size);
    /// 创建切片（零拷贝，指向父对象的数据）
    static VMByteArray* createSlice(VMByteArray* parent, UInt32 offset, UInt32 len);
    
    /// 确保容量至少为 newCap
    void ensure(UInt32 newCap);
    /// 调整大小（保留已有数据，新空间未初始化）
    void resize(UInt32 newSize);
    /// 获取实际数据指针（考虑切片偏移）
    UInt8* ptr() { return parent ? (parent->data + offset) : data; }
    const UInt8* ptr() const { return parent ? (parent->data + offset) : data; }
    
    UInt8  get(UInt32 index) const;
    void   set(UInt32 index, UInt8 value);
    /// 纯数据视图（不含GC可达Value子对象）
    bool hasValueChildren() const { return parent != nullptr; }
};

struct VMNativeFunc : VMObject {
    using Fn = std::function<Value(std::vector<Value>&)>;
    VMNativeFunc() { typeTag = TAG_NATIVE; }
    Fn fn;
    VMString* name = nullptr;
};

} // namespace cplang