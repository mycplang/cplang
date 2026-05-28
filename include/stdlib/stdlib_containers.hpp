#pragma once
#include "vm/value.hpp"
#include <vector>

namespace cplang {

namespace set {
    Value create(std::vector<Value>& args);
    Value add(std::vector<Value>& args);
    Value has(std::vector<Value>& args);
    Value remove(std::vector<Value>& args);
    Value size(std::vector<Value>& args);
    Value clear(std::vector<Value>& args);
    Value union_(std::vector<Value>& args);
    Value intersect(std::vector<Value>& args);
    Value diff(std::vector<Value>& args);
}

namespace stack {
    Value create(std::vector<Value>& args);
    Value push(std::vector<Value>& args);
    Value pop(std::vector<Value>& args);
    Value peek(std::vector<Value>& args);
    Value isEmpty(std::vector<Value>& args);
    Value size(std::vector<Value>& args);
    Value clear(std::vector<Value>& args);
}

namespace queue {
    Value create(std::vector<Value>& args);
    Value enqueue(std::vector<Value>& args);
    Value dequeue(std::vector<Value>& args);
    Value front(std::vector<Value>& args);
    Value isEmpty(std::vector<Value>& args);
    Value size(std::vector<Value>& args);
    Value clear(std::vector<Value>& args);
}

namespace deque {
    Value create(std::vector<Value>& args);
    Value pushBack(std::vector<Value>& args);
    Value pushFront(std::vector<Value>& args);
    Value popBack(std::vector<Value>& args);
    Value popFront(std::vector<Value>& args);
    Value back(std::vector<Value>& args);
    Value front(std::vector<Value>& args);
    Value at(std::vector<Value>& args);
    Value set(std::vector<Value>& args);
    Value isEmpty(std::vector<Value>& args);
    Value size(std::vector<Value>& args);
    Value clear(std::vector<Value>& args);
}

namespace priority_queue {
    Value create(std::vector<Value>& args);
    Value push(std::vector<Value>& args);
    Value pop(std::vector<Value>& args);
    Value top(std::vector<Value>& args);
    Value topPriority(std::vector<Value>& args);
    Value isEmpty(std::vector<Value>& args);
    Value size(std::vector<Value>& args);
    Value clear(std::vector<Value>& args);
}

namespace linked_list {
    Value create(std::vector<Value>& args);
    Value pushBack(std::vector<Value>& args);
    Value pushFront(std::vector<Value>& args);
    Value popBack(std::vector<Value>& args);
    Value popFront(std::vector<Value>& args);
    Value front(std::vector<Value>& args);
    Value back(std::vector<Value>& args);
    Value isEmpty(std::vector<Value>& args);
    Value size(std::vector<Value>& args);
    Value clear(std::vector<Value>& args);
    Value insert(std::vector<Value>& args);
    Value erase(std::vector<Value>& args);
    Value at(std::vector<Value>& args);
    Value set(std::vector<Value>& args);
    Value find(std::vector<Value>& args);
    Value splice(std::vector<Value>& args);
}

namespace forward_list {
    Value create(std::vector<Value>& args);
    Value pushFront(std::vector<Value>& args);
    Value popFront(std::vector<Value>& args);
    Value front(std::vector<Value>& args);
    Value isEmpty(std::vector<Value>& args);
    Value size(std::vector<Value>& args);
    Value clear(std::vector<Value>& args);
    Value insertAfter(std::vector<Value>& args);
    Value eraseAfter(std::vector<Value>& args);
    Value at(std::vector<Value>& args);
    Value find(std::vector<Value>& args);
    Value reverse(std::vector<Value>& args);
    Value sort(std::vector<Value>& args);
    Value unique(std::vector<Value>& args);
    Value spliceAfter(std::vector<Value>& args);
}

namespace multiset_ {
    Value create(std::vector<Value>& args);
    Value insert(std::vector<Value>& args);
    Value count(std::vector<Value>& args);
    Value find(std::vector<Value>& args);
    Value eraseOne(std::vector<Value>& args);
    Value eraseAll(std::vector<Value>& args);
    Value contains(std::vector<Value>& args);
    Value isEmpty(std::vector<Value>& args);
    Value size(std::vector<Value>& args);
    Value clear(std::vector<Value>& args);
    Value lowerBound(std::vector<Value>& args);
    Value upperBound(std::vector<Value>& args);
}

namespace multimap_ {
    Value create(std::vector<Value>& args);
    Value insert(std::vector<Value>& args);
    Value count(std::vector<Value>& args);
    Value find(std::vector<Value>& args);
    Value contains(std::vector<Value>& args);
    Value eraseOne(std::vector<Value>& args);
    Value eraseAll(std::vector<Value>& args);
    Value isEmpty(std::vector<Value>& args);
    Value size(std::vector<Value>& args);
    Value clear(std::vector<Value>& args);
    Value lowerBound(std::vector<Value>& args);
    Value upperBound(std::vector<Value>& args);
    Value equalRange(std::vector<Value>& args);
}

namespace unordered_set_ {
    Value create(std::vector<Value>& args);
    Value insert(std::vector<Value>& args);
    Value count(std::vector<Value>& args);
    Value find(std::vector<Value>& args);
    Value contains(std::vector<Value>& args);
    Value erase(std::vector<Value>& args);
    Value isEmpty(std::vector<Value>& args);
    Value size(std::vector<Value>& args);
    Value clear(std::vector<Value>& args);
}

namespace unordered_mset_ {
    Value create(std::vector<Value>& args);
    Value insert(std::vector<Value>& args);
    Value count(std::vector<Value>& args);
    Value find(std::vector<Value>& args);
    Value contains(std::vector<Value>& args);
    Value eraseOne(std::vector<Value>& args);
    Value eraseAll(std::vector<Value>& args);
    Value isEmpty(std::vector<Value>& args);
    Value size(std::vector<Value>& args);
    Value clear(std::vector<Value>& args);
}

namespace unordered_map_ {
    Value create(std::vector<Value>& args);
    Value insert(std::vector<Value>& args);
    Value count(std::vector<Value>& args);
    Value find(std::vector<Value>& args);
    Value lookup(std::vector<Value>& args);
    Value contains(std::vector<Value>& args);
    Value erase(std::vector<Value>& args);
    Value isEmpty(std::vector<Value>& args);
    Value size(std::vector<Value>& args);
    Value clear(std::vector<Value>& args);
}

namespace unordered_mmap_ {
    Value create(std::vector<Value>& args);
    Value insert(std::vector<Value>& args);
    Value count(std::vector<Value>& args);
    Value find(std::vector<Value>& args);
    Value contains(std::vector<Value>& args);
    Value eraseOne(std::vector<Value>& args);
    Value eraseAll(std::vector<Value>& args);
    Value equalRange(std::vector<Value>& args);
    Value isEmpty(std::vector<Value>& args);
    Value size(std::vector<Value>& args);
    Value clear(std::vector<Value>& args);
}

namespace bitset_ {
    Value set_(std::vector<Value>& args);
    Value clear_(std::vector<Value>& args);
    Value toggle(std::vector<Value>& args);
    Value test(std::vector<Value>& args);
    Value count(std::vector<Value>& args);
    Value all(std::vector<Value>& args);
    Value any(std::vector<Value>& args);
    Value none(std::vector<Value>& args);
    Value flip(std::vector<Value>& args);
    Value toBinaryString(std::vector<Value>& args);
    Value fromBinaryString(std::vector<Value>& args);
}

namespace complex {
    Value create(std::vector<Value>& args);
    Value add(std::vector<Value>& args);
    Value sub(std::vector<Value>& args);
    Value mul(std::vector<Value>& args);
    Value div(std::vector<Value>& args);
    Value abs_(std::vector<Value>& args);
    Value arg(std::vector<Value>& args);
    Value conj(std::vector<Value>& args);
    Value real_(std::vector<Value>& args);
    Value imag(std::vector<Value>& args);
}

namespace pair_ {
    Value create(std::vector<Value>& args);
    Value first(std::vector<Value>& args);
    Value second(std::vector<Value>& args);
    Value swap_(std::vector<Value>& args);
}

} // namespace cplang
