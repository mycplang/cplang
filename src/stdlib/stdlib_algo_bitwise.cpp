#include "stdlib/stdlib.hpp"

namespace cplang {

// Bitwise, Algorithms, Random functions
// #include'd from stdlib.cpp, already inside namespace cplang

void StdLib::registerBitwise(VM* vm) {
    registerFunction(vm, "bitAnd", bitwise::bitAnd);
    registerFunction(vm, "bitOr", bitwise::bitOr);
    registerFunction(vm, "bitXor", bitwise::bitXor);
    registerFunction(vm, "bitNot", bitwise::bitNot);
    registerFunction(vm, "shiftLeft", bitwise::shiftLeft);
    registerFunction(vm, "shiftRight", bitwise::shiftRight);
    registerFunction(vm, "popcount", bitwise::popcount);
    registerFunction(vm, "bitCeil", bitwise::bitCeil);
    registerFunction(vm, "bitFloor", bitwise::bitFloor);
    registerFunction(vm, "bitWidth", bitwise::bitWidth);
    registerFunction(vm, "countLeadingZeros", bitwise::countLeadingZeros);
    registerFunction(vm, "countTrailingZeros", bitwise::countTrailingZeros);
    registerFunction(vm, "hasSingleBit", bitwise::hasSingleBit);
    registerFunction(vm, "rotateLeft", bitwise::rotateLeft);
    registerFunction(vm, "rotateRight", bitwise::rotateRight);
    registerFunction(vm, "bitSwap", bitwise::bitSwap);

    registerAlias(vm, "位与", "bitAnd");
    registerAlias(vm, "位或", "bitOr");
    registerAlias(vm, "位异或", "bitXor");
    registerAlias(vm, "位非", "bitNot");
    registerAlias(vm, "左移", "shiftLeft");
    registerAlias(vm, "右移", "shiftRight");
    registerAlias(vm, "位数", "popcount");
    registerAlias(vm, "向上2幂", "bitCeil");
    registerAlias(vm, "向下2幂", "bitFloor");
    registerAlias(vm, "位宽", "bitWidth");
    registerAlias(vm, "前导零个数", "countLeadingZeros");
    registerAlias(vm, "尾随零个数", "countTrailingZeros");
    registerAlias(vm, "是2的幂", "hasSingleBit");
    registerAlias(vm, "位左旋", "rotateLeft");
    registerAlias(vm, "位右旋", "rotateRight");
    registerAlias(vm, "位字节交换", "bitSwap");
}

namespace bitwise {
Value bitAnd(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Int(0);
    Int64 a = args[0].isInt() ? args[0].asInt() : static_cast<Int64>(args[0].asFloat());
    Int64 b = args[1].isInt() ? args[1].asInt() : static_cast<Int64>(args[1].asFloat());
    return Value::Int(a & b);
}

Value bitOr(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Int(0);
    Int64 a = args[0].isInt() ? args[0].asInt() : static_cast<Int64>(args[0].asFloat());
    Int64 b = args[1].isInt() ? args[1].asInt() : static_cast<Int64>(args[1].asFloat());
    return Value::Int(a | b);
}

Value bitXor(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Int(0);
    Int64 a = args[0].isInt() ? args[0].asInt() : static_cast<Int64>(args[0].asFloat());
    Int64 b = args[1].isInt() ? args[1].asInt() : static_cast<Int64>(args[1].asFloat());
    return Value::Int(a ^ b);
}

Value bitNot(std::vector<Value>& args) {
    if (args.empty()) return Value::Int(0);
    Int64 a = args[0].isInt() ? args[0].asInt() : static_cast<Int64>(args[0].asFloat());
    return Value::Int(~a);
}

Value shiftLeft(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Int(0);
    Int64 a = args[0].isInt() ? args[0].asInt() : static_cast<Int64>(args[0].asFloat());
    Int64 n = args[1].isInt() ? args[1].asInt() : static_cast<Int64>(args[1].asFloat());
    return Value::Int(a << n);
}

Value shiftRight(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Int(0);
    Int64 a = args[0].isInt() ? args[0].asInt() : static_cast<Int64>(args[0].asFloat());
    Int64 n = args[1].isInt() ? args[1].asInt() : static_cast<Int64>(args[1].asFloat());
    return Value::Int(a >> n);
}

Value popcount(std::vector<Value>& args) {
    if (args.empty()) return Value::Int(0);
    Int64 a = args[0].isInt() ? args[0].asInt() : static_cast<Int64>(args[0].asFloat());
    UInt64 v = static_cast<UInt64>(a);
    int count = 0;
    while (v) { count++; v &= v - 1; }
    return Value::Int(count);
}

Value bitCeil(std::vector<Value>& args) {
    if (args.empty()) return Value::Int(1);
    Int64 a = args[0].isInt() ? args[0].asInt() : static_cast<Int64>(args[0].asFloat());
    if (a <= 1) return Value::Int(1);
    UInt64 v = static_cast<UInt64>(a - 1);
    v |= v >> 1; v |= v >> 2; v |= v >> 4; v |= v >> 8; v |= v >> 16; v |= v >> 32;
    return Value::Int(static_cast<Int64>(v + 1));
}

Value bitFloor(std::vector<Value>& args) {
    if (args.empty()) return Value::Int(0);
    Int64 a = args[0].isInt() ? args[0].asInt() : static_cast<Int64>(args[0].asFloat());
    if (a <= 0) return Value::Int(0);
    UInt64 v = static_cast<UInt64>(a);
    v |= v >> 1; v |= v >> 2; v |= v >> 4; v |= v >> 8; v |= v >> 16; v |= v >> 32;
    return Value::Int(static_cast<Int64>((v + 1) >> 1));
}

Value bitWidth(std::vector<Value>& args) {
    if (args.empty()) return Value::Int(0);
    Int64 a = args[0].isInt() ? args[0].asInt() : static_cast<Int64>(args[0].asFloat());
    UInt64 v = static_cast<UInt64>(a);
    int width = 0;
    while (v) { width++; v >>= 1; }
    return Value::Int(width);
}
Value countLeadingZeros(std::vector<Value>& args) {
    if (args.empty()) return Value::Int(64);
    Int64 a = args[0].isInt() ? args[0].asInt() : static_cast<Int64>(args[0].asFloat());
    UInt64 v = static_cast<UInt64>(a);
    if (v == 0) return Value::Int(64);
    int c = 0;
    while ((v & (UInt64(1) << 63)) == 0) { v <<= 1; c++; }
    return Value::Int(c);
}

Value countTrailingZeros(std::vector<Value>& args) {
    if (args.empty()) return Value::Int(64);
    Int64 a = args[0].isInt() ? args[0].asInt() : static_cast<Int64>(args[0].asFloat());
    UInt64 v = static_cast<UInt64>(a);
    if (v == 0) return Value::Int(64);
    int c = 0;
    while ((v & 1) == 0) { v >>= 1; c++; }
    return Value::Int(c);
}

Value hasSingleBit(std::vector<Value>& args) {
    if (args.empty()) return Value::Bool(false);
    Int64 a = args[0].isInt() ? args[0].asInt() : static_cast<Int64>(args[0].asFloat());
    UInt64 v = static_cast<UInt64>(a);
    return Value::Bool(v != 0 && (v & (v - 1)) == 0);
}

Value rotateLeft(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Int(0);
    Int64 a = args[0].isInt() ? args[0].asInt() : static_cast<Int64>(args[0].asFloat());
    Int64 n = args[1].isInt() ? args[1].asInt() : 0;
    UInt64 v = static_cast<UInt64>(a);
    int s = static_cast<int>(n & 63);
    return Value::Int(static_cast<Int64>((v << s) | (v >> (64 - s))));
}

Value rotateRight(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Int(0);
    Int64 a = args[0].isInt() ? args[0].asInt() : static_cast<Int64>(args[0].asFloat());
    Int64 n = args[1].isInt() ? args[1].asInt() : 0;
    UInt64 v = static_cast<UInt64>(a);
    int s = static_cast<int>(n & 63);
    return Value::Int(static_cast<Int64>((v >> s) | (v << (64 - s))));
}

Value bitSwap(std::vector<Value>& args) {
    if (args.empty()) return Value::Int(0);
    Int64 a = args[0].isInt() ? args[0].asInt() : static_cast<Int64>(args[0].asFloat());
    UInt64 v = static_cast<UInt64>(a);
    v = ((v & 0xFF00FF00FF00FF00ULL) >> 8)  | ((v & 0x00FF00FF00FF00FFULL) << 8);
    v = ((v & 0xFFFF0000FFFF0000ULL) >> 16) | ((v & 0x0000FFFF0000FFFFULL) << 16);
    v = (v >> 32) | (v << 32);
    return Value::Int(static_cast<Int64>(v));
}

} // namespace bitwise

// ═══════════════════════════════════════════════════════════════════
//  算法实现（对标 C++ <algorithm> + <numeric>）
// ═══════════════════════════════════════════════════════════════════

void StdLib::registerAlgorithms(VM* vm) {
    registerFunction(vm, "arrCount", algo::count);
    registerFunction(vm, "binarySearch", algo::binarySearch);
    registerFunction(vm, "lowerBound", algo::lowerBound);
    registerFunction(vm, "upperBound", algo::upperBound);
    registerFunction(vm, "isSorted", algo::isSorted);
    registerFunction(vm, "nextPermutation", algo::nextPermutation);
    registerFunction(vm, "clamp", algo::clamp);
    registerFunction(vm, "gcd", algo::gcd);
    registerFunction(vm, "lcm", algo::lcm);
    registerFunction(vm, "innerProduct", algo::innerProduct);
    registerFunction(vm, "partialSum", algo::partialSum);
    registerFunction(vm, "adjacentDiff", algo::adjacentDiff);
    registerFunction(vm, "iota", algo::iota);
    registerFunction(vm, "isSortedUntil", algo::isSortedUntil);
    registerFunction(vm, "prevPermutation", algo::prevPermutation);
    registerFunction(vm, "partialSort", algo::partialSort);
    registerFunction(vm, "nthElement", algo::nthElement);
    registerFunction(vm, "mergeSorted", algo::merge);
    registerFunction(vm, "inplaceMerge", algo::inplaceMerge);
    registerFunction(vm, "setUnion", algo::setUnion);
    registerFunction(vm, "setIntersection", algo::setIntersection);
    registerFunction(vm, "setDifference", algo::setDifference);
    registerFunction(vm, "setSymmetricDiff", algo::setSymmetricDiff);
    registerFunction(vm, "minElement", algo::minElement);
    registerFunction(vm, "maxElement", algo::maxElement);
    registerFunction(vm, "unique", algo::unique);
    registerFunction(vm, "rotate", algo::rotate);

    registerAlias(vm, "计数", "arrCount");
    registerAlias(vm, "二分查找", "binarySearch");
    registerAlias(vm, "下界", "lowerBound");
    registerAlias(vm, "上界", "upperBound");
    registerAlias(vm, "已排序", "isSorted");
    registerAlias(vm, "下一排列", "nextPermutation");
    registerAlias(vm, "限幅", "clamp");
    registerAlias(vm, "最大公约数", "gcd");
    registerAlias(vm, "最小公倍数", "lcm");
    registerAlias(vm, "点积", "innerProduct");
    registerAlias(vm, "前缀和", "partialSum");
    registerAlias(vm, "相邻差分", "adjacentDiff");
    registerAlias(vm, "序列", "iota");
    registerAlias(vm, "排序直到", "isSortedUntil");
    registerAlias(vm, "上一排列", "prevPermutation");
    registerAlias(vm, "部分排序", "partialSort");
    registerAlias(vm, "第n元素", "nthElement");
    registerAlias(vm, "合并有序", "mergeSorted");
    registerAlias(vm, "原地合并", "inplaceMerge");
    registerAlias(vm, "集合并", "setUnion");
    registerAlias(vm, "集交集", "setIntersection");
    registerAlias(vm, "集差集", "setDifference");
    registerAlias(vm, "集对称差", "setSymmetricDiff");
    registerAlias(vm, "最小元素", "minElement");
    registerAlias(vm, "最大元素", "maxElement");
    registerAlias(vm, "去重", "unique");
    registerAlias(vm, "旋转", "rotate");
}

namespace algo {
Value count(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::Int(0);
    auto arr = args[0].asArray();
    int cnt = 0;
    for (auto& v : arr->data) {
        if (v.equals(args[1])) cnt++;
    }
    return Value::Int(cnt);
}

Value binarySearch(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::Bool(false);
    auto arr = args[0].asArray();
    const auto& data = arr->data;
    size_t lo = 0, hi = data.size();
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (data[mid].equals(args[1])) return Value::Bool(true);
        // 简单比较：仅支持同类型数值
        bool less = false;
        if (data[mid].isInt() && args[1].isInt()) less = data[mid].asInt() < args[1].asInt();
        else if (data[mid].isNumber() && args[1].isNumber()) {
            double a = data[mid].isInt() ? static_cast<double>(data[mid].asInt()) : data[mid].asFloat();
            double b = args[1].isInt() ? static_cast<double>(args[1].asInt()) : args[1].asFloat();
            less = a < b;
        }
        if (less) lo = mid + 1; else hi = mid;
    }
    return Value::Bool(false);
}

Value lowerBound(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::Int(0);
    auto arr = args[0].asArray();
    const auto& data = arr->data;
    size_t lo = 0, hi = data.size();
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        bool less = false;
        if (data[mid].isInt() && args[1].isInt()) less = data[mid].asInt() < args[1].asInt();
        else if (data[mid].isNumber() && args[1].isNumber()) {
            double a = data[mid].isInt() ? static_cast<double>(data[mid].asInt()) : data[mid].asFloat();
            double b = args[1].isInt() ? static_cast<double>(args[1].asInt()) : args[1].asFloat();
            less = a < b;
        }
        if (less) lo = mid + 1; else hi = mid;
    }
    return Value::Int(static_cast<Int64>(lo));
}

Value upperBound(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::Int(0);
    auto arr = args[0].asArray();
    const auto& data = arr->data;
    size_t lo = 0, hi = data.size();
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        bool lessOrEq = false;
        if (data[mid].isInt() && args[1].isInt()) lessOrEq = data[mid].asInt() <= args[1].asInt();
        else if (data[mid].isNumber() && args[1].isNumber()) {
            double a = data[mid].isInt() ? static_cast<double>(data[mid].asInt()) : data[mid].asFloat();
            double b = args[1].isInt() ? static_cast<double>(args[1].asInt()) : args[1].asFloat();
            lessOrEq = a <= b;
        }
        if (lessOrEq) lo = mid + 1; else hi = mid;
    }
    return Value::Int(static_cast<Int64>(lo));
}

Value isSorted(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::Bool(true);
    auto arr = args[0].asArray();
    const auto& data = arr->data;
    for (size_t i = 1; i < data.size(); i++) {
        bool less = false;
        if (data[i].isInt() && data[i-1].isInt()) less = data[i-1].asInt() > data[i].asInt();
        else if (data[i].isNumber() && data[i-1].isNumber()) {
            double a = data[i-1].isInt() ? static_cast<double>(data[i-1].asInt()) : data[i-1].asFloat();
            double b = data[i].isInt() ? static_cast<double>(data[i].asInt()) : data[i].asFloat();
            less = a > b;
        } else return Value::Bool(false);
        if (less) return Value::Bool(false);
    }
    return Value::Bool(true);
}

Value nextPermutation(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::Bool(false);
    auto arr = args[0].asArray();
    auto& data = arr->data;
    if (data.size() < 2) return Value::Bool(false);
    // 找到从右往左第一个降序对
    int i = static_cast<int>(data.size()) - 2;
    while (i >= 0) {
        bool less = false;
        if (data[i].isInt() && data[i+1].isInt()) less = data[i].asInt() < data[i+1].asInt();
        else if (data[i].isNumber() && data[i+1].isNumber()) {
            double a = data[i].isInt() ? static_cast<double>(data[i].asInt()) : data[i].asFloat();
            double b = data[i+1].isInt() ? static_cast<double>(data[i+1].asInt()) : data[i+1].asFloat();
            less = a < b;
        }
        if (less) break;
        i--;
    }
    if (i < 0) {
        std::reverse(data.begin(), data.end());
        return Value::Bool(false);
    }
    int j = static_cast<int>(data.size()) - 1;
    while (j > i) {
        bool greater = false;
        if (data[j].isInt() && data[i].isInt()) greater = data[j].asInt() > data[i].asInt();
        else if (data[j].isNumber() && data[i].isNumber()) {
            double a = data[j].isInt() ? static_cast<double>(data[j].asInt()) : data[j].asFloat();
            double b = data[i].isInt() ? static_cast<double>(data[i].asInt()) : data[i].asFloat();
            greater = a > b;
        }
        if (greater) break;
        j--;
    }
    std::swap(data[i], data[j]);
    std::reverse(data.begin() + i + 1, data.end());
    return Value::Bool(true);
}

Value clamp(std::vector<Value>& args) {
    if (args.size() < 3) return Value::nil();
    double v = args[0].isInt() ? static_cast<double>(args[0].asInt()) : args[0].asFloat();
    double lo = args[1].isInt() ? static_cast<double>(args[1].asInt()) : args[1].asFloat();
    double hi = args[2].isInt() ? static_cast<double>(args[2].asInt()) : args[2].asFloat();
    if (v < lo) return args[1];
    if (v > hi) return args[2];
    return args[0];
}

Value gcd(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Int(0);
    Int64 a = std::llabs(args[0].isInt() ? args[0].asInt() : static_cast<Int64>(args[0].asFloat()));
    Int64 b = std::llabs(args[1].isInt() ? args[1].asInt() : static_cast<Int64>(args[1].asFloat()));
    while (b != 0) { Int64 t = b; b = a % b; a = t; }
    return Value::Int(a);
}

Value lcm(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Int(0);
    Value g = gcd(args);
    if (g.asInt() == 0) return Value::Int(0);
    Int64 a = args[0].isInt() ? args[0].asInt() : static_cast<Int64>(args[0].asFloat());
    Int64 b = args[1].isInt() ? args[1].asInt() : static_cast<Int64>(args[1].asFloat());
    return Value::Int(std::llabs(a / g.asInt() * b));
}

Value innerProduct(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray() || !args[1].isArray()) return Value::Int(0);
    auto a = args[0].asArray();
    auto b = args[1].asArray();
    size_t n = std::min(a->data.size(), b->data.size());
    double sum = 0;
    for (size_t i = 0; i < n; i++) {
        double av = a->data[i].isInt() ? static_cast<double>(a->data[i].asInt()) : a->data[i].asFloat();
        double bv = b->data[i].isInt() ? static_cast<double>(b->data[i].asInt()) : b->data[i].asFloat();
        sum += av * bv;
    }
    return Value::Float(sum);
}

Value partialSum(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::Array(VMArray::create());
    auto arr = args[0].asArray();
    auto result = VMArray::create();
    double sum = 0;
    for (auto& v : arr->data) {
        sum += v.isInt() ? static_cast<double>(v.asInt()) : v.asFloat();
        result->data.push_back(Value::Float(sum));
    }
    return Value::Array(result);
}

Value adjacentDiff(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::Array(VMArray::create());
    auto arr = args[0].asArray();
    auto result = VMArray::create();
    if (arr->data.empty()) return Value::Array(result);
    double prev = arr->data[0].isInt() ? static_cast<double>(arr->data[0].asInt()) : arr->data[0].asFloat();
    result->data.push_back(Value::Float(prev));
    for (size_t i = 1; i < arr->data.size(); i++) {
        double curr = arr->data[i].isInt() ? static_cast<double>(arr->data[i].asInt()) : arr->data[i].asFloat();
        result->data.push_back(Value::Float(curr - prev));
        prev = curr;
    }
    return Value::Array(result);
}

Value iota(std::vector<Value>& args) {
    Int64 start = (args.size() > 0 && args[0].isInt()) ? args[0].asInt() : 0;
    Int64 count = (args.size() > 1 && args[1].isInt()) ? args[1].asInt() : 10;
    auto result = VMArray::create();
    for (Int64 i = 0; i < count; i++) {
        result->data.push_back(Value::Int(start + i));
    }
    return Value::Array(result);
}

// 是否已排序直到何处（返回第一个非升序位置）
Value isSortedUntil(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::Int(0);
    auto arr = args[0].asArray();
    const auto& data = arr->data;
    for (size_t i = 1; i < data.size(); i++) {
        bool less = false;
        if (data[i].isInt() && data[i-1].isInt()) less = data[i-1].asInt() > data[i].asInt();
        else if (data[i].isNumber() && data[i-1].isNumber()) {
            double a = data[i-1].isInt() ? static_cast<double>(data[i-1].asInt()) : data[i-1].asFloat();
            double b = data[i].isInt() ? static_cast<double>(data[i].asInt()) : data[i].asFloat();
            less = a > b;
        } else return Value::Int(static_cast<Int64>(i));
        if (less) return Value::Int(static_cast<Int64>(i));
    }
    return Value::Int(static_cast<Int64>(data.size()));
}

static double valToDouble(const Value& v) {
    return v.isInt() ? static_cast<double>(v.asInt()) : v.asFloat();
}

// 前一个排列（降序 -> 升序）
Value prevPermutation(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::Bool(false);
    auto arr = args[0].asArray();
    auto& data = arr->data;
    if (data.size() < 2) return Value::Bool(false);
    int i = static_cast<int>(data.size()) - 2;
    while (i >= 0) {
        bool greater = false;
        if (data[i].isNumber() && data[i+1].isNumber()) {
            greater = valToDouble(data[i]) > valToDouble(data[i+1]);
        }
        if (greater) break;
        i--;
    }
    if (i < 0) { std::reverse(data.begin(), data.end()); return Value::Bool(false); }
    int j = static_cast<int>(data.size()) - 1;
    while (j > i) {
        if (data[j].isNumber() && data[i].isNumber()) {
            if (valToDouble(data[j]) < valToDouble(data[i])) break;
        }
        j--;
    }
    std::swap(data[i], data[j]);
    std::reverse(data.begin() + i + 1, data.end());
    return Value::Bool(true);
}

// 部分排序：只排前k个
Value partialSort(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::nil();
    auto arr = args[0].asArray();
    auto& data = arr->data;
    Int64 k = args[1].isInt() ? args[1].asInt() : 0;
    if (k <= 0 || static_cast<size_t>(k) > data.size()) return Value::nil();
    std::partial_sort(data.begin(), data.begin() + k, data.end(),
        [](const Value& a, const Value& b) {
            return valToDouble(a) < valToDouble(b);
        });
    return Value::nil();
}

// 第n个元素
Value nthElement(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::nil();
    auto arr = args[0].asArray();
    auto& data = arr->data;
    Int64 n = args[1].isInt() ? args[1].asInt() : 0;
    if (n < 0 || static_cast<size_t>(n) >= data.size()) return Value::nil();
    std::nth_element(data.begin(), data.begin() + n, data.end(),
        [](const Value& a, const Value& b) {
            return valToDouble(a) < valToDouble(b);
        });
    return data[n];
}

// 合并两个有序数组
Value merge(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray() || !args[1].isArray()) return Value::Array(VMArray::create());
    auto a = args[0].asArray();
    auto b = args[1].asArray();
    auto result = VMArray::create();
    auto cmp = [](const Value& x, const Value& y) { return valToDouble(x) < valToDouble(y); };
    std::merge(a->data.begin(), a->data.end(), b->data.begin(), b->data.end(),
        std::back_inserter(result->data), cmp);
    return Value::Array(result);
}

// 原地合并：将数组分为 [lo..mid) 和 [mid..hi) 两部分合并
Value inplaceMerge(std::vector<Value>& args) {
    if (args.size() < 3 || !args[0].isArray()) return Value::nil();
    auto arr = args[0].asArray();
    auto& data = arr->data;
    Int64 lo = args[1].isInt() ? args[1].asInt() : 0;
    Int64 mid = args[2].isInt() ? args[2].asInt() : 0;
    Int64 hi = args.size() > 3 && args[3].isInt() ? args[3].asInt() : static_cast<Int64>(data.size());
    if (lo < 0 || mid < lo || hi > static_cast<Int64>(data.size()) || hi < mid) return Value::nil();
    auto cmp = [](const Value& x, const Value& y) { return valToDouble(x) < valToDouble(y); };
    std::inplace_merge(data.begin() + lo, data.begin() + mid, data.begin() + hi, cmp);
    return Value::nil();
}

// 集合运算（要求有序数组）
Value setUnion(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray() || !args[1].isArray()) return Value::Array(VMArray::create());
    auto a = args[0].asArray();
    auto b = args[1].asArray();
    auto result = VMArray::create();
    std::set_union(a->data.begin(), a->data.end(), b->data.begin(), b->data.end(),
        std::back_inserter(result->data),
        [](const Value& x, const Value& y) { return valToDouble(x) < valToDouble(y); });
    return Value::Array(result);
}

Value setIntersection(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray() || !args[1].isArray()) return Value::Array(VMArray::create());
    auto a = args[0].asArray();
    auto b = args[1].asArray();
    auto result = VMArray::create();
    std::set_intersection(a->data.begin(), a->data.end(), b->data.begin(), b->data.end(),
        std::back_inserter(result->data),
        [](const Value& x, const Value& y) { return valToDouble(x) < valToDouble(y); });
    return Value::Array(result);
}

Value setDifference(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray() || !args[1].isArray()) return Value::Array(VMArray::create());
    auto a = args[0].asArray();
    auto b = args[1].asArray();
    auto result = VMArray::create();
    std::set_difference(a->data.begin(), a->data.end(), b->data.begin(), b->data.end(),
        std::back_inserter(result->data),
        [](const Value& x, const Value& y) { return valToDouble(x) < valToDouble(y); });
    return Value::Array(result);
}

Value setSymmetricDiff(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray() || !args[1].isArray()) return Value::Array(VMArray::create());
    auto a = args[0].asArray();
    auto b = args[1].asArray();
    auto result = VMArray::create();
    std::set_symmetric_difference(a->data.begin(), a->data.end(), b->data.begin(), b->data.end(),
        std::back_inserter(result->data),
        [](const Value& x, const Value& y) { return valToDouble(x) < valToDouble(y); });
    return Value::Array(result);
}

// 最小元素索引
Value minElement(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::Int(-1);
    auto arr = args[0].asArray();
    if (arr->data.empty()) return Value::Int(-1);
    size_t best = 0;
    for (size_t i = 1; i < arr->data.size(); i++) {
        if (arr->data[i].isNumber() && arr->data[best].isNumber()) {
            if (valToDouble(arr->data[i]) < valToDouble(arr->data[best])) best = i;
        }
    }
    return Value::Int(static_cast<Int64>(best));
}

// 最大元素索引
Value maxElement(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::Int(-1);
    auto arr = args[0].asArray();
    if (arr->data.empty()) return Value::Int(-1);
    size_t best = 0;
    for (size_t i = 1; i < arr->data.size(); i++) {
        if (arr->data[i].isNumber() && arr->data[best].isNumber()) {
            if (valToDouble(arr->data[i]) > valToDouble(arr->data[best])) best = i;
        }
    }
    return Value::Int(static_cast<Int64>(best));
}

// 去重（连续重复移除）
Value unique(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::Int(0);
    auto arr = args[0].asArray();
    auto& data = arr->data;
    auto it = std::unique(data.begin(), data.end(),
        [](const Value& a, const Value& b) { return a.equals(b); });
    size_t n = std::distance(data.begin(), it);
    data.resize(n);
    return Value::Int(static_cast<Int64>(n));
}

// 左旋转
Value rotate(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray()) return Value::nil();
    auto arr = args[0].asArray();
    auto& data = arr->data;
    if (data.empty()) return Value::nil();
    Int64 n = args[1].isInt() ? args[1].asInt() : 0;
    // n为正表示左移，为负表示右移
    size_t mid;
    if (n >= 0) {
        mid = static_cast<size_t>(n) % data.size();
    } else {
        mid = data.size() - (static_cast<size_t>(-n) % data.size());
    }
    if (mid == 0) return Value::nil();
    std::rotate(data.begin(), data.begin() + mid, data.end());
    return Value::nil();
}
} // namespace algo

// ═══════════════════════════════════════════════════════════════════
//  随机数增强（对标 C++ <random>）
// ═══════════════════════════════════════════════════════════════════

void StdLib::registerRandom(VM* vm) {
    registerFunction(vm, "randomFloat", random::randomFloat);
    registerFunction(vm, "randomNormal", random::randomNormal);
    registerFunction(vm, "randomSeed", random::randomSeed);
    registerFunction(vm, "shuffle", random::shuffle);
    registerFunction(vm, "randomUniformInt", random::randomUniformInt);
    registerFunction(vm, "randomExponential", random::randomExponential);
    registerFunction(vm, "randomBernoulli", random::randomBernoulli);
    registerFunction(vm, "randomPoisson", random::randomPoisson);

    registerAlias(vm, "随机浮点", "randomFloat");
    registerAlias(vm, "随机正态", "randomNormal");
    registerAlias(vm, "随机种子", "randomSeed");
    registerAlias(vm, "打乱", "shuffle");
    registerAlias(vm, "随机整数", "randomUniformInt");
    registerAlias(vm, "随机指数", "randomExponential");
    registerAlias(vm, "随机伯努利", "randomBernoulli");
    registerAlias(vm, "随机泊松", "randomPoisson");
}

namespace random {
static std::mt19937& getGen() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return gen;
}

Value randomFloat(std::vector<Value>& args) {
    auto& gen = getGen();
    if (args.size() < 2) {
        std::uniform_real_distribution<> dis(0.0, 1.0);
        return Value::Float(dis(gen));
    }
    double lo = args[0].isInt() ? static_cast<double>(args[0].asInt()) : args[0].asFloat();
    double hi = args[1].isInt() ? static_cast<double>(args[1].asInt()) : args[1].asFloat();
    std::uniform_real_distribution<> dis(lo, hi);
    return Value::Float(dis(gen));
}

Value randomNormal(std::vector<Value>& args) {
    auto& gen = getGen();
    double mean = (args.size() > 0) ? (args[0].isInt() ? static_cast<double>(args[0].asInt()) : args[0].asFloat()) : 0.0;
    double stddev = (args.size() > 1) ? (args[1].isInt() ? static_cast<double>(args[1].asInt()) : args[1].asFloat()) : 1.0;
    std::normal_distribution<> dis(mean, stddev);
    return Value::Float(dis(gen));
}

Value randomSeed(std::vector<Value>& args) {
    if (!args.empty() && args[0].isInt()) {
        getGen().seed(static_cast<unsigned long>(args[0].asInt()));
    }
    return Value::nil();
}

Value shuffle(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::nil();
    auto arr = args[0].asArray();
    auto& gen = getGen();
    std::shuffle(arr->data.begin(), arr->data.end(), gen);
    return Value::nil();
}

// 均匀整数 [min, max]
Value randomUniformInt(std::vector<Value>& args) {
    auto& gen = getGen();
    Int64 lo = (args.size() > 0 && args[0].isInt()) ? args[0].asInt() : 0;
    Int64 hi = (args.size() > 1 && args[1].isInt()) ? args[1].asInt() : 100;
    if (lo > hi) std::swap(lo, hi);
    std::uniform_int_distribution<Int64> dis(lo, hi);
    return Value::Int(dis(gen));
}

// 指数分布
Value randomExponential(std::vector<Value>& args) {
    auto& gen = getGen();
    double lambda = (args.size() > 0) ? (args[0].isNumber() ? 
        (args[0].isInt() ? static_cast<double>(args[0].asInt()) : args[0].asFloat()) : 1.0) : 1.0;
    if (lambda <= 0.0) lambda = 1.0;
    std::exponential_distribution<> dis(lambda);
    return Value::Float(dis(gen));
}

// 伯努利分布
Value randomBernoulli(std::vector<Value>& args) {
    auto& gen = getGen();
    double p = 0.5;
    if (args.size() > 0 && args[0].isNumber()) {
        p = args[0].isInt() ? static_cast<double>(args[0].asInt()) : args[0].asFloat();
    }
    if (p < 0.0) p = 0.0;
    if (p > 1.0) p = 1.0;
    std::bernoulli_distribution dis(p);
    return Value::Bool(dis(gen));
}

// 泊松分布
Value randomPoisson(std::vector<Value>& args) {
    auto& gen = getGen();
    double lambda = 1.0;
    if (args.size() > 0 && args[0].isNumber()) {
        lambda = args[0].isInt() ? static_cast<double>(args[0].asInt()) : args[0].asFloat();
    }
    if (lambda <= 0.0) lambda = 1.0;
    std::poisson_distribution<Int64> dis(lambda);
    return Value::Int(dis(gen));
}
} // namespace random

// ═══════════════════════════════════════════════════════════════════
//  正则表达式实现（对标 C++ <regex>）
// ═══════════════════════════════════════════════════════════════════

} // namespace cplang
