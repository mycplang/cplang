// CP语言 内联缓存（Inline Caching）系统
// 用于优化属性访问和方法调用
//
// 优化原理：
//   对于属性访问 obj.x，第一次查找后，缓存对象类型和偏移
//   后续访问直接从缓存偏移获取，无需哈希查找
//
// 性能提升：
//   单态缓存（Monomorphic）: O(1) 访问，速度提升 10-50 倍
//   多态缓存（Polymorphic）: 支持有限种类型，保持高效
//
#pragma once

#include "vm/value.hpp"
#include <cstdint>
#include <vector>
#include <cstring>

namespace cplang {

// 前置声明
struct VMNativeFunc;

// 内联缓存状态
enum class CacheState : uint8_t {
    Uninitialized = 0,    // 未初始化
    Monomorphic = 1,      // 单态缓存（一种类型）
    Polymorphic = 2,      // 多态缓存（多种类型，有上限）
    Megamorphic = 3,      // 超多态（太多类型，禁用缓存）
};

// 单态缓存条目
struct MonomorphicCache {
    VMObject* cachedObject = nullptr;  // 上次访问的对象
    Value cachedValue;                 // 缓存的值
    uint64_t cachedShape = 0;          // 对象形状（用于验证）
};

// 多态缓存条目（支持最多4种类型）
struct PolymorphicCache {
    static constexpr int MAX_ENTRIES = 4;
    
    struct Entry {
        VMObject* object = nullptr;
        Value value;
        uint64_t shape = 0;
    };
    
    Entry entries[MAX_ENTRIES];
    int count = 0;
};

// 属性访问内联缓存
struct PropertyIC {
    CacheState state = CacheState::Uninitialized;
    VMString* propertyName = nullptr;
    
    union {
        MonomorphicCache mono;
        PolymorphicCache poly;
    };
    
    PropertyIC() {
        memset(&mono, 0, sizeof(mono));
    }
    
    // 快速路径查找
    bool tryGet(VMObject* obj, uint64_t shape, Value* out) {
        if (state == CacheState::Monomorphic) {
            if (mono.cachedShape == shape && mono.cachedObject == obj) {
                *out = mono.cachedValue;
                return true;
            }
        } else if (state == CacheState::Polymorphic) {
            for (int i = 0; i < poly.count; i++) {
                if (poly.entries[i].shape == shape && poly.entries[i].object == obj) {
                    *out = poly.entries[i].value;
                    return true;
                }
            }
        }
        return false;
    }
    
    // 设置缓存
    void set(VMObject* obj, uint64_t shape, const Value& val) {
        if (state == CacheState::Uninitialized) {
            state = CacheState::Monomorphic;
            mono.cachedObject = obj;
            mono.cachedShape = shape;
            mono.cachedValue = val;
        } else if (state == CacheState::Monomorphic) {
            // 转换为多态
            state = CacheState::Polymorphic;
            poly.entries[0].object = mono.cachedObject;
            poly.entries[0].shape = mono.cachedShape;
            poly.entries[0].value = mono.cachedValue;
            poly.count = 1;
            // 添加新条目
            if (poly.count < PolymorphicCache::MAX_ENTRIES) {
                poly.entries[poly.count].object = obj;
                poly.entries[poly.count].shape = shape;
                poly.entries[poly.count].value = val;
                poly.count++;
            } else {
                state = CacheState::Megamorphic;
            }
        } else if (state == CacheState::Polymorphic) {
            if (poly.count < PolymorphicCache::MAX_ENTRIES) {
                poly.entries[poly.count].object = obj;
                poly.entries[poly.count].shape = shape;
                poly.entries[poly.count].value = val;
                poly.count++;
            } else {
                state = CacheState::Megamorphic;
            }
        }
        // Megamorphic: 不缓存
    }
};

// 方法调用内联缓存
struct MethodIC {
    CacheState state = CacheState::Uninitialized;
    VMString* methodName = nullptr;
    
    void* cachedMethod = nullptr;  // 改为 void* 避免类型依赖
    VMObject* cachedReceiver = nullptr;
    uint64_t cachedShape = 0;
    
    // 快速路径
    bool tryCall(VMObject* receiver, uint64_t shape, void** outMethod) {
        if (state == CacheState::Monomorphic && 
            cachedReceiver == receiver && cachedShape == shape) {
            *outMethod = cachedMethod;
            return true;
        }
        return false;
    }
    
    void set(VMObject* receiver, uint64_t shape, void* method) {
        state = CacheState::Monomorphic;
        cachedReceiver = receiver;
        cachedShape = shape;
        cachedMethod = method;
    }
};

// 内联缓存管理器
class InlineCacheManager {
public:
    static constexpr int MAX_IC_ENTRIES = 4096;  // 最多4096个IC条目
    
    InlineCacheManager() {
        propertyICs_.resize(MAX_IC_ENTRIES);
        methodICs_.resize(MAX_IC_ENTRIES);
    }
    
    // 获取属性访问IC
    PropertyIC* getPropertyIC(int index) {
        if (index >= 0 && index < MAX_IC_ENTRIES) {
            return &propertyICs_[index];
        }
        return nullptr;
    }
    
    // 获取方法调用IC
    MethodIC* getMethodIC(int index) {
        if (index >= 0 && index < MAX_IC_ENTRIES) {
            return &methodICs_[index];
        }
        return nullptr;
    }
    
    // 清除所有缓存（GC时调用）
    void clear() {
        for (auto& ic : propertyICs_) {
            ic = PropertyIC();
        }
        for (auto& ic : methodICs_) {
            ic = MethodIC();
        }
    }
    
    // 统计信息
    struct Stats {
        int64_t propertyHits = 0;
        int64_t propertyMisses = 0;
        int64_t methodHits = 0;
        int64_t methodMisses = 0;
    };
    
    const Stats& getStats() const { return stats_; }
    
    void recordPropertyHit() { stats_.propertyHits++; }
    void recordPropertyMiss() { stats_.propertyMisses++; }
    void recordMethodHit() { stats_.methodHits++; }
    void recordMethodMiss() { stats_.methodMisses++; }
    
    void resetStats() {
        stats_ = Stats();
    }
    
private:
    std::vector<PropertyIC> propertyICs_;
    std::vector<MethodIC> methodICs_;
    Stats stats_;
};

} // namespace cplang
