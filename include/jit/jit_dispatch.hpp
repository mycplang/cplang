// JIT 分派接口 — VM → JIT 的桥梁
// 包含 JIT 信息注册表（将 JIT 字段从 VMFunction 解耦到外部映射）
#pragma once
#include "vm/vm_fwd.hpp"
#include "vm/value.hpp"
#include <unordered_map>

namespace cplang {

// ── JIT 元数据（从 VMFunction 结构体中分离） ──
struct JITInfo {
    void* entry = nullptr;
    bool  compiled = false;
};

// ── JIT 信息注册表（外部映射表，不污染 VMFunction） ──
class JITRegistry {
public:
    JITInfo& get(VMFunction* func) {
        auto it = map_.find(func);
        if (it != map_.end()) return it->second;
        return map_.emplace(func, JITInfo{}).first->second;
    }

    bool has(VMFunction* func) const {
        return map_.count(func) > 0;
    }

    void erase(VMFunction* func) {
        map_.erase(func);
    }

    void clear() { map_.clear(); }
    size_t size() const { return map_.size(); }

private:
    std::unordered_map<VMFunction*, JITInfo> map_;
};

// 尝试通过 JIT 执行函数调用
// 返回 true 表示 JIT 已处理并设置了 result，false 表示需回退到 VM 字节码
bool jitTryCallDispatch(VM* vm, VMFunction* func, int argc, Value* args, Value& result);

} // namespace cplang
