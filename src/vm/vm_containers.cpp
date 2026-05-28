// CP语言 虚拟机实现 - 容器类型
#include "vm/vm.hpp"

namespace cplang {

// ========== VMString ==========
VMString* VMString::create(const char* s, UInt32 len) {
    // 使用 new 分配，与 VMNativeFunc 等其他对象统一
    VMString* str = new VMString();
    // 单独分配数据缓冲区（变长数据）
    char* buf = new char[len + 1];
    str->length = len;
    str->data = buf;
    std::memcpy(buf, s, len);
    buf[len] = '\0';
    return str;
}
VMString* VMString::create(const std::string& s) {
    return create(s.c_str(), static_cast<UInt32>(s.size()));
}

// ========== VMArray ==========
VMArray* VMArray::create(UInt32 cap) {
    VMArray* arr = new VMArray();
    arr->data.reserve(cap);
    return arr;
}
Value VMArray::get(Int64 index) {
    Int64 n = data.size();
    if (index < 0) index += n;
    if (index >= 0 && index < n) return data[index];
    return Value::nil();
}
void VMArray::set(Int64 index, const Value& v) {
    Int64 n = data.size();
    if (index < 0) index += n;
    if (index >= 0) {
        if (index >= static_cast<Int64>(n)) data.resize(index + 1);
        data[index] = v;
    }
}

// ========== VMTable ==========
VMTable* VMTable::create() { 
    VMTable* t = new VMTable();
    t->buckets.resize(16);  // 初始16个槽位
    return t;
}

size_t VMTable::hashValue(const Value& v) {
    if (v.isNil()) return 0;
    if (v.isBool()) return v.asBool() ? 1 : 2;
    if (v.isInt()) {
        Int64 iv = v.asInt();
        return std::hash<Int64>{}(iv);
    }
    if (v.isFloat()) {
        Float64 f = v.asFloat();
        if (f == 0.0) f = 0.0;
        return std::hash<Float64>{}(f);
    }
    if (v.isString()) {
        size_t hash = 14695981039346656037ull;
        const char* data = v.asString()->data;
        UInt32 len = v.asString()->length;
        for (UInt32 i = 0; i < len; i++) {
            hash ^= static_cast<size_t>(static_cast<unsigned char>(data[i]));
            hash *= 1099511628211ull;
        }
        return hash;
    }
    return std::hash<void*>{}(v.asPtr());
}

size_t VMTable::findSlot(const Value& key) const {
    if (buckets.empty()) return 0;
    size_t h = hashValue(key);
    size_t mask = buckets.size() - 1;
    size_t idx = h & mask;
    
    // 线性探测
    while (buckets[idx].occupied) {
        if (buckets[idx].key.equals(key)) return idx;
        idx = (idx + 1) & mask;
    }
    return idx;
}

void VMTable::rehash(size_t newSize) {
    std::vector<HashEntry> oldBuckets = std::move(buckets);
    buckets.resize(newSize);
    count = 0;
    
    for (auto& entry : oldBuckets) {
        if (entry.occupied) {
            size_t idx = findSlot(entry.key);
            buckets[idx] = entry;
            count++;
        }
    }
}

Value VMTable::get(const Value& key) {
    if (buckets.empty()) return Value::nil();
    size_t idx = findSlot(key);
    if (buckets[idx].occupied && buckets[idx].key.equals(key)) {
        return buckets[idx].value;
    }
    return Value::nil();
}

void VMTable::set(const Value& key, const Value& val) {
    if (buckets.empty()) buckets.resize(16);
    
    size_t idx = findSlot(key);
    if (buckets[idx].occupied && buckets[idx].key.equals(key)) {
        buckets[idx].value = val;
        return;
    }
    
    // 新键
    buckets[idx].key = key;
    buckets[idx].value = val;
    buckets[idx].occupied = true;
    count++;
    
    // 负载因子超过 0.75 时扩容
    if (count * 4 > buckets.size() * 3) {
        rehash(buckets.size() * 2);
    }
    
    // 同步到 data（保持兼容）
    bool found = false;
    for (auto& kv : data) {
        if (kv.first.equals(key)) { kv.second = val; found = true; break; }
    }
    if (!found) data.emplace_back(key, val);
}

bool VMTable::has(const Value& key) {
    if (buckets.empty()) return false;
    size_t idx = findSlot(key);
    return buckets[idx].occupied && buckets[idx].key.equals(key);
}

bool VMTable::remove(const Value& key) {
    if (buckets.empty()) return false;
    size_t idx = findSlot(key);
    if (buckets[idx].occupied && buckets[idx].key.equals(key)) {
        buckets[idx].occupied = false;
        count--;
        
        // 同步 data
        for (auto it = data.begin(); it != data.end(); ++it) {
            if (it->first.equals(key)) { data.erase(it); break; }
        }
        return true;
    }
    return false;
}

void VMTable::clear() {
    for (auto& entry : buckets) {
        entry.occupied = false;
    }
    count = 0;
    data.clear();
}

size_t VMTable::size() const {
    return count;
}

// ========== VMSet ==========
VMSet* VMSet::create() {
    VMSet* s = new VMSet();
    s->buckets.resize(16);
    return s;
}

size_t VMSet::hashValue(const Value& v) {
    return VMTable::hashValue(v);  // 复用VMTable的哈希函数
}

size_t VMSet::findSlot(const Value& key) const {
    if (buckets.empty()) return 0;
    size_t h = hashValue(key);
    size_t mask = buckets.size() - 1;
    size_t idx = h & mask;
    
    while (buckets[idx].occupied) {
        if (buckets[idx].key.equals(key)) return idx;
        idx = (idx + 1) & mask;
    }
    return idx;
}

void VMSet::rehash(size_t newSize) {
    std::vector<HashEntry> oldBuckets = std::move(buckets);
    buckets.resize(newSize);
    count = 0;
    
    for (auto& entry : oldBuckets) {
        if (entry.occupied) {
            size_t idx = findSlot(entry.key);
            buckets[idx] = entry;
            count++;
        }
    }
}

bool VMSet::add(const Value& key) {
    if (buckets.empty()) buckets.resize(16);
    
    size_t idx = findSlot(key);
    if (buckets[idx].occupied && buckets[idx].key.equals(key)) {
        return false;  // 已存在
    }
    
    buckets[idx].key = key;
    buckets[idx].occupied = true;
    count++;
    
    // 同步到 data
    data.push_back(key);
    
    // 负载因子超过 0.75 时扩容
    if (count * 4 > buckets.size() * 3) {
        rehash(buckets.size() * 2);
    }
    return true;
}

bool VMSet::has(const Value& key) {
    if (buckets.empty()) return false;
    size_t idx = findSlot(key);
    return buckets[idx].occupied && buckets[idx].key.equals(key);
}

bool VMSet::remove(const Value& key) {
    if (buckets.empty()) return false;
    size_t idx = findSlot(key);
    if (buckets[idx].occupied && buckets[idx].key.equals(key)) {
        buckets[idx].occupied = false;
        count--;
        
        // 同步 data
        for (auto it = data.begin(); it != data.end(); ++it) {
            if (it->equals(key)) { data.erase(it); break; }
        }
        return true;
    }
    return false;
}

void VMSet::clear() {
    for (auto& entry : buckets) {
        entry.occupied = false;
    }
    count = 0;
    data.clear();
}

size_t VMSet::size() const {
    return count;
}

// ========== VMStack ==========
VMStack* VMStack::create() {
    return new VMStack();
}

void VMStack::push(const Value& v) {
    data.push_back(v);
}

Value VMStack::pop() {
    if (data.empty()) return Value::nil();
    Value v = data.back();
    data.pop_back();
    return v;
}

Value VMStack::peek() const {
    if (data.empty()) return Value::nil();
    return data.back();
}

bool VMStack::isEmpty() const {
    return data.empty();
}

size_t VMStack::size() const {
    return data.size();
}

void VMStack::clear() {
    data.clear();
}

// ========== VMQueue ==========
VMQueue* VMQueue::create() {
    return new VMQueue();
}

void VMQueue::enqueue(const Value& v) {
    data.push_back(v);
}

Value VMQueue::dequeue() {
    if (data.empty()) return Value::nil();
    Value v = data.front();
    data.pop_front();
    return v;
}

Value VMQueue::front() const {
    if (data.empty()) return Value::nil();
    return data.front();
}

bool VMQueue::isEmpty() const {
    return data.empty();
}

size_t VMQueue::size() const {
    return data.size();
}

void VMQueue::clear() {
    data.clear();
}

// ========== VMDeque（双端队列）==========
VMDeque* VMDeque::create() {
    return new VMDeque();
}

void VMDeque::pushBack(const Value& v) {
    data.push_back(v);
}

void VMDeque::pushFront(const Value& v) {
    data.push_front(v);
}

Value VMDeque::popBack() {
    if (data.empty()) return Value::nil();
    Value v = data.back();
    data.pop_back();
    return v;
}

Value VMDeque::popFront() {
    if (data.empty()) return Value::nil();
    Value v = data.front();
    data.pop_front();
    return v;
}

Value VMDeque::back() const {
    if (data.empty()) return Value::nil();
    return data.back();
}

Value VMDeque::front() const {
    if (data.empty()) return Value::nil();
    return data.front();
}

Value VMDeque::at(Int64 index) const {
    Int64 n = static_cast<Int64>(data.size());
    if (index < 0) index += n;
    if (index >= 0 && index < n) return data[static_cast<size_t>(index)];
    return Value::nil();
}

void VMDeque::set(Int64 index, const Value& v) {
    Int64 n = static_cast<Int64>(data.size());
    if (index < 0) index += n;
    if (index >= 0 && index < n) data[static_cast<size_t>(index)] = v;
}

bool VMDeque::isEmpty() const {
    return data.empty();
}

size_t VMDeque::size() const {
    return data.size();
}

void VMDeque::clear() {
    data.clear();
}

// ========== VMPriorityQueue（优先队列）==========
VMPriorityQueue* VMPriorityQueue::create() {
    return new VMPriorityQueue();
}

void VMPriorityQueue::push(const Value& v, double priority) {
    heap.push_back({v, priority});
    std::push_heap(heap.begin(), heap.end());
}

Value VMPriorityQueue::pop() {
    if (heap.empty()) return Value::nil();
    std::pop_heap(heap.begin(), heap.end());
    Value v = heap.back().value;
    heap.pop_back();
    return v;
}

Value VMPriorityQueue::top() const {
    if (heap.empty()) return Value::nil();
    return heap.front().value;
}

double VMPriorityQueue::topPriority() const {
    if (heap.empty()) return 0.0;
    return heap.front().priority;
}

bool VMPriorityQueue::isEmpty() const {
    return heap.empty();
}

size_t VMPriorityQueue::size() const {
    return heap.size();
}

void VMPriorityQueue::clear() {
    heap.clear();
}

// ========== VMLinkedList（双向链表）==========
VMLinkedList* VMLinkedList::create() {
    return new VMLinkedList();
}

void VMLinkedList::pushBack(const Value& v) {
    data.push_back(v);
}

void VMLinkedList::pushFront(const Value& v) {
    data.push_front(v);
}

Value VMLinkedList::popBack() {
    if (data.empty()) return Value::nil();
    Value v = data.back();
    data.pop_back();
    return v;
}

Value VMLinkedList::popFront() {
    if (data.empty()) return Value::nil();
    Value v = data.front();
    data.pop_front();
    return v;
}

Value VMLinkedList::front() const {
    if (data.empty()) return Value::nil();
    return data.front();
}

Value VMLinkedList::back() const {
    if (data.empty()) return Value::nil();
    return data.back();
}

bool VMLinkedList::isEmpty() const {
    return data.empty();
}

size_t VMLinkedList::size() const {
    return data.size();
}

void VMLinkedList::clear() {
    data.clear();
}

void VMLinkedList::insert(Int64 pos, const Value& v) {
    if (pos < 0) pos = 0;
    if (static_cast<size_t>(pos) > data.size()) pos = static_cast<Int64>(data.size());
    auto it = data.begin();
    std::advance(it, pos);
    data.insert(it, v);
}

Value VMLinkedList::erase(Int64 pos) {
    if (pos < 0 || static_cast<size_t>(pos) >= data.size()) return Value::nil();
    auto it = data.begin();
    std::advance(it, pos);
    Value v = *it;
    data.erase(it);
    return v;
}

Value VMLinkedList::at(Int64 pos) const {
    if (pos < 0 || static_cast<size_t>(pos) >= data.size()) return Value::nil();
    auto it = data.begin();
    std::advance(it, pos);
    return *it;
}

void VMLinkedList::set(Int64 pos, const Value& v) {
    if (pos < 0 || static_cast<size_t>(pos) >= data.size()) return;
    auto it = data.begin();
    std::advance(it, pos);
    *it = v;
}

Int64 VMLinkedList::find(const Value& v) const {
    Int64 idx = 0;
    for (auto& item : data) {
        if (item.equals(v)) return idx;
        idx++;
    }
    return -1;
}

void VMLinkedList::splice(Int64 pos, VMLinkedList* other) {
    if (!other || other->data.empty()) return;
    if (pos < 0) pos = 0;
    if (static_cast<size_t>(pos) > data.size()) pos = static_cast<Int64>(data.size());
    auto it = data.begin();
    std::advance(it, pos);
    data.splice(it, other->data);
}

// ========== VMSLinkedList（单向链表）==========
VMSLinkedList* VMSLinkedList::create() {
    return new VMSLinkedList();
}

void VMSLinkedList::pushFront(const Value& v) {
    data.push_front(v);
    _size++;
}

Value VMSLinkedList::popFront() {
    if (data.empty()) return Value::nil();
    Value v = data.front();
    data.pop_front();
    _size--;
    return v;
}

Value VMSLinkedList::front() const {
    if (data.empty()) return Value::nil();
    return data.front();
}

bool VMSLinkedList::isEmpty() const {
    return data.empty();
}

size_t VMSLinkedList::size() const {
    return _size;
}

void VMSLinkedList::clear() {
    data.clear();
    _size = 0;
}

void VMSLinkedList::insertAfter(Int64 pos, const Value& v) {
    if (pos < 0) pos = 0;
    auto it = data.before_begin();
    for (Int64 i = 0; i <= pos && it != data.end(); i++) {
        auto next = std::next(it);
        if (i == pos) {
            data.insert_after(it, v);
            _size++;
            return;
        }
        if (next == data.end()) break;
        it = next;
    }
    data.insert_after(it, v);
    _size++;
}

Value VMSLinkedList::eraseAfter(Int64 pos) {
    if (pos < 0 || data.empty()) return Value::nil();
    auto it = data.before_begin();
    for (Int64 i = 0; i < pos && it != data.end(); i++) {
        it++;
    }
    auto next = std::next(it);
    if (next == data.end()) return Value::nil();
    Value v = *next;
    data.erase_after(it);
    _size--;
    return v;
}

Value VMSLinkedList::at(Int64 pos) const {
    if (pos < 0 || static_cast<size_t>(pos) >= _size) return Value::nil();
    auto it = data.begin();
    std::advance(it, pos);
    return *it;
}

Int64 VMSLinkedList::find(const Value& v) const {
    Int64 idx = 0;
    for (auto& item : data) {
        if (item.equals(v)) return idx;
        idx++;
    }
    return -1;
}

void VMSLinkedList::reverse() {
    data.reverse();
}

static double valToDoubleSList(const Value& v) {
    return v.isInt() ? static_cast<double>(v.asInt()) : v.asFloat();
}

void VMSLinkedList::sort() {
    data.sort([](const Value& a, const Value& b) {
        if (a.isNumber() && b.isNumber()) return valToDoubleSList(a) < valToDoubleSList(b);
        if (a.isString() && b.isString()) {
            return std::string(a.asString()->data, a.asString()->length) <
                   std::string(b.asString()->data, b.asString()->length);
        }
        return a.raw() < b.raw();
    });
}

void VMSLinkedList::unique() {
    data.unique([](const Value& a, const Value& b) { return a.equals(b); });
}

void VMSLinkedList::spliceAfter(Int64 pos, VMSLinkedList* other) {
    if (!other || other->data.empty()) return;
    if (pos < 0) pos = 0;
    auto it = data.before_begin();
    for (Int64 i = 0; i <= pos && it != data.end(); i++) {
        auto next = std::next(it);
        if (i == pos) {
            data.splice_after(it, other->data);
            _size += other->_size;
            other->_size = 0;
            return;
        }
        if (next == data.end()) break;
        it = next;
    }
    data.splice_after(it, other->data);
    _size += other->_size;
    other->_size = 0;
}

// ========== ValueLess/ValueHash/ValueEqual (现在统一在 vm/value.hpp 和 src/vm/value.cpp 中) ==========
// 此处不再有实现，避免与 value.hpp 中的定义冲突

// ========== VMMultiSet（多重集合）==========
VMMultiSet* VMMultiSet::create() {
    return new VMMultiSet();
}

void VMMultiSet::insert(const Value& v) {
    data.insert(v);
}

Int64 VMMultiSet::count(const Value& v) const {
    return static_cast<Int64>(data.count(v));
}

Value VMMultiSet::find(const Value& v) const {
    auto it = data.find(v);
    if (it == data.end()) return Value::nil();
    return *it;
}

bool VMMultiSet::contains(const Value& v) const {
    return data.find(v) != data.end();
}

Int64 VMMultiSet::eraseOne(const Value& v) {
    auto it = data.find(v);
    if (it == data.end()) return 0;
    data.erase(it);
    return 1;
}

Int64 VMMultiSet::eraseAll(const Value& v) {
    return static_cast<Int64>(data.erase(v));
}

bool VMMultiSet::isEmpty() const {
    return data.empty();
}

size_t VMMultiSet::size() const {
    return data.size();
}

void VMMultiSet::clear() {
    data.clear();
}

Value VMMultiSet::lowerBound(const Value& v) const {
    auto it = data.lower_bound(v);
    if (it == data.end()) return Value::nil();
    return *it;
}

Value VMMultiSet::upperBound(const Value& v) const {
    auto it = data.upper_bound(v);
    if (it == data.end()) return Value::nil();
    return *it;
}

// ========== VMMultiMap（多重映射）==========
VMMultiMap* VMMultiMap::create() {
    return new VMMultiMap();
}

void VMMultiMap::insert(const Value& key, const Value& val) {
    data.insert({key, val});
}

Int64 VMMultiMap::count(const Value& key) const {
    return static_cast<Int64>(data.count(key));
}

Value VMMultiMap::find(const Value& key) const {
    auto it = data.find(key);
    if (it == data.end()) return Value::nil();
    return it->second;
}

bool VMMultiMap::contains(const Value& key) const {
    return data.find(key) != data.end();
}

Int64 VMMultiMap::eraseOne(const Value& key) {
    auto it = data.find(key);
    if (it == data.end()) return 0;
    data.erase(it);
    return 1;
}

Int64 VMMultiMap::eraseAll(const Value& key) {
    return static_cast<Int64>(data.erase(key));
}

bool VMMultiMap::isEmpty() const {
    return data.empty();
}

size_t VMMultiMap::size() const {
    return data.size();
}

void VMMultiMap::clear() {
    data.clear();
}

Value VMMultiMap::lowerBound(const Value& key) const {
    auto it = data.lower_bound(key);
    if (it == data.end()) return Value::nil();
    return it->second;
}

Value VMMultiMap::upperBound(const Value& key) const {
    auto it = data.upper_bound(key);
    if (it == data.end()) return Value::nil();
    return it->second;
}

Value VMMultiMap::equalRange(const Value& key) const {
    auto range = data.equal_range(key);
    VMArray* arr = VMArray::create();
    for (auto it = range.first; it != range.second; ++it) {
        arr->data.push_back(it->second);
    }
    return makeArrayVal(arr);
}

// ========== VMOrderedSet ==========
VMOrderedSet* VMOrderedSet::create() { return new VMOrderedSet(); }
void VMOrderedSet::insert(const Value& v) { data.insert(v); }
Int64 VMOrderedSet::count(const Value& v) const { return static_cast<Int64>(data.count(v)); }
Value VMOrderedSet::find(const Value& v) const {
    auto it = data.find(v);
    if (it != data.end()) return *it;
    return Value::nil();
}
bool VMOrderedSet::contains(const Value& v) const { return data.find(v) != data.end(); }
Int64 VMOrderedSet::erase(const Value& v) { return static_cast<Int64>(data.erase(v)); }
bool VMOrderedSet::isEmpty() const { return data.empty(); }
size_t VMOrderedSet::size() const { return data.size(); }
void VMOrderedSet::clear() { data.clear(); }
Value VMOrderedSet::lowerBound(const Value& v) const {
    auto it = data.lower_bound(v);
    if (it != data.end()) return *it;
    return Value::nil();
}
Value VMOrderedSet::upperBound(const Value& v) const {
    auto it = data.upper_bound(v);
    if (it != data.end()) return *it;
    return Value::nil();
}
Value VMOrderedSet::min() const {
    if (data.empty()) return Value::nil();
    return *data.begin();
}
Value VMOrderedSet::max() const {
    if (data.empty()) return Value::nil();
    return *data.rbegin();
}

// ========== VMOrderedMap ==========
VMOrderedMap* VMOrderedMap::create() { return new VMOrderedMap(); }
void VMOrderedMap::insert(const Value& key, const Value& val) { data[key] = val; }
Int64 VMOrderedMap::count(const Value& key) const { return static_cast<Int64>(data.count(key)); }
Value VMOrderedMap::find(const Value& key) const {
    auto it = data.find(key);
    if (it != data.end()) return it->second;
    return Value::nil();
}
Value VMOrderedMap::lookup(const Value& key) const {
    auto it = data.find(key);
    if (it != data.end()) return it->second;
    return Value::nil();
}
bool VMOrderedMap::contains(const Value& key) const { return data.find(key) != data.end(); }
Int64 VMOrderedMap::erase(const Value& key) { return static_cast<Int64>(data.erase(key)); }
bool VMOrderedMap::isEmpty() const { return data.empty(); }
size_t VMOrderedMap::size() const { return data.size(); }
void VMOrderedMap::clear() { data.clear(); }
Value VMOrderedMap::lowerBound(const Value& key) const {
    auto it = data.lower_bound(key);
    if (it != data.end()) return it->first;
    return Value::nil();
}
Value VMOrderedMap::upperBound(const Value& key) const {
    auto it = data.upper_bound(key);
    if (it != data.end()) return it->first;
    return Value::nil();
}
Value VMOrderedMap::minKey() const {
    if (data.empty()) return Value::nil();
    return data.begin()->first;
}
Value VMOrderedMap::maxKey() const {
    if (data.empty()) return Value::nil();
    return data.rbegin()->first;
}

// ========== VMUnorderedSet ==========
VMUnorderedSet* VMUnorderedSet::create() { return new VMUnorderedSet(); }
void VMUnorderedSet::insert(const Value& v) { data.insert(v); }
Int64 VMUnorderedSet::count(const Value& v) const { return static_cast<Int64>(data.count(v)); }
Value VMUnorderedSet::find(const Value& v) const {
    auto it = data.find(v);
    if (it == data.end()) return Value::nil();
    return *it;
}
bool VMUnorderedSet::contains(const Value& v) const { return data.find(v) != data.end(); }
Int64 VMUnorderedSet::erase(const Value& v) { return static_cast<Int64>(data.erase(v)); }
bool VMUnorderedSet::isEmpty() const { return data.empty(); }
size_t VMUnorderedSet::size() const { return data.size(); }
void VMUnorderedSet::clear() { data.clear(); }

// ========== VMUnorderedMultiSet ==========
VMUnorderedMultiSet* VMUnorderedMultiSet::create() { return new VMUnorderedMultiSet(); }
void VMUnorderedMultiSet::insert(const Value& v) { data.insert(v); }
Int64 VMUnorderedMultiSet::count(const Value& v) const { return static_cast<Int64>(data.count(v)); }
Value VMUnorderedMultiSet::find(const Value& v) const {
    auto it = data.find(v);
    if (it == data.end()) return Value::nil();
    return *it;
}
bool VMUnorderedMultiSet::contains(const Value& v) const { return data.find(v) != data.end(); }
Int64 VMUnorderedMultiSet::eraseOne(const Value& v) {
    auto it = data.find(v);
    if (it == data.end()) return 0;
    data.erase(it);
    return 1;
}
Int64 VMUnorderedMultiSet::eraseAll(const Value& v) { return static_cast<Int64>(data.erase(v)); }
bool VMUnorderedMultiSet::isEmpty() const { return data.empty(); }
size_t VMUnorderedMultiSet::size() const { return data.size(); }
void VMUnorderedMultiSet::clear() { data.clear(); }

// ========== VMUnorderedMap ==========
VMUnorderedMap* VMUnorderedMap::create() { return new VMUnorderedMap(); }
void VMUnorderedMap::insert(const Value& key, const Value& val) { data[key] = val; }
Int64 VMUnorderedMap::count(const Value& key) const { return static_cast<Int64>(data.count(key)); }
Value VMUnorderedMap::find(const Value& key) const {
    auto it = data.find(key);
    if (it == data.end()) return Value::nil();
    return it->second;
}
bool VMUnorderedMap::contains(const Value& key) const { return data.find(key) != data.end(); }
Int64 VMUnorderedMap::erase(const Value& key) { return static_cast<Int64>(data.erase(key)); }
Value VMUnorderedMap::lookup(const Value& key) { return data[key]; }
bool VMUnorderedMap::isEmpty() const { return data.empty(); }
size_t VMUnorderedMap::size() const { return data.size(); }
void VMUnorderedMap::clear() { data.clear(); }

// ========== VMUnorderedMultiMap ==========
VMUnorderedMultiMap* VMUnorderedMultiMap::create() { return new VMUnorderedMultiMap(); }
void VMUnorderedMultiMap::insert(const Value& key, const Value& val) { data.insert({key, val}); }
Int64 VMUnorderedMultiMap::count(const Value& key) const { return static_cast<Int64>(data.count(key)); }
Value VMUnorderedMultiMap::find(const Value& key) const {
    auto it = data.find(key);
    if (it == data.end()) return Value::nil();
    return it->second;
}
bool VMUnorderedMultiMap::contains(const Value& key) const { return data.find(key) != data.end(); }
Int64 VMUnorderedMultiMap::eraseOne(const Value& key) {
    auto it = data.find(key);
    if (it == data.end()) return 0;
    data.erase(it);
    return 1;
}
Int64 VMUnorderedMultiMap::eraseAll(const Value& key) { return static_cast<Int64>(data.erase(key)); }
Value VMUnorderedMultiMap::equalRange(const Value& key) const {
    auto range = data.equal_range(key);
    VMArray* arr = VMArray::create();
    for (auto it = range.first; it != range.second; ++it) {
        arr->data.push_back(it->second);
    }
    return makeArrayVal(arr);
}
bool VMUnorderedMultiMap::isEmpty() const { return data.empty(); }
size_t VMUnorderedMultiMap::size() const { return data.size(); }
void VMUnorderedMultiMap::clear() { data.clear(); }

} // namespace cplang
