#pragma once
#include "vm/value.hpp"
#include <vector>

namespace cplang {

namespace bitwise {
    Value bitAnd(std::vector<Value>& args);
    Value bitOr(std::vector<Value>& args);
    Value bitXor(std::vector<Value>& args);
    Value bitNot(std::vector<Value>& args);
    Value shiftLeft(std::vector<Value>& args);
    Value shiftRight(std::vector<Value>& args);
    Value popcount(std::vector<Value>& args);
    Value bitCeil(std::vector<Value>& args);
    Value bitFloor(std::vector<Value>& args);
    Value bitWidth(std::vector<Value>& args);
    Value countLeadingZeros(std::vector<Value>& args);
    Value countTrailingZeros(std::vector<Value>& args);
    Value hasSingleBit(std::vector<Value>& args);
    Value rotateLeft(std::vector<Value>& args);
    Value rotateRight(std::vector<Value>& args);
    Value bitSwap(std::vector<Value>& args);
}

namespace algo {
    Value count(std::vector<Value>& args);
    Value binarySearch(std::vector<Value>& args);
    Value lowerBound(std::vector<Value>& args);
    Value upperBound(std::vector<Value>& args);
    Value isSorted(std::vector<Value>& args);
    Value nextPermutation(std::vector<Value>& args);
    Value clamp(std::vector<Value>& args);
    Value gcd(std::vector<Value>& args);
    Value lcm(std::vector<Value>& args);
    Value innerProduct(std::vector<Value>& args);
    Value partialSum(std::vector<Value>& args);
    Value adjacentDiff(std::vector<Value>& args);
    Value iota(std::vector<Value>& args);
    Value isSortedUntil(std::vector<Value>& args);
    Value prevPermutation(std::vector<Value>& args);
    Value partialSort(std::vector<Value>& args);
    Value nthElement(std::vector<Value>& args);
    Value merge(std::vector<Value>& args);
    Value inplaceMerge(std::vector<Value>& args);
    Value setUnion(std::vector<Value>& args);
    Value setIntersection(std::vector<Value>& args);
    Value setDifference(std::vector<Value>& args);
    Value setSymmetricDiff(std::vector<Value>& args);
    Value minElement(std::vector<Value>& args);
    Value maxElement(std::vector<Value>& args);
    Value unique(std::vector<Value>& args);
    Value rotate(std::vector<Value>& args);
    Value stableSort(std::vector<Value>& args);
    Value partition(std::vector<Value>& args);
    Value anyOf(std::vector<Value>& args);
    Value allOf(std::vector<Value>& args);
    Value noneOf(std::vector<Value>& args);
    Value findIf(std::vector<Value>& args);
    Value accumulate(std::vector<Value>& args);
    Value forEach(std::vector<Value>& args);
    Value transform(std::vector<Value>& args);
    Value generate(std::vector<Value>& args);
}

} // namespace cplang
