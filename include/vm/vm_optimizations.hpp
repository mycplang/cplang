// CP语言 VM渐进式优化方案
// 目标：在原VM基础上做最小改动，获得最大性能提升
//
// 优化1: 全局变量Slot化（预计提速3-5倍）
// 优化2: 指令预取+边界检查消除（预计提速20%）
// 优化3: 热点函数内联（预计提速10%）
//
// 此文件作为优化指南和patch参考

#ifndef VM_OPTIMIZATIONS_HPP
#define VM_OPTIMIZATIONS_HPP

#include "vm/vm.hpp"
#include <vector>

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  优化1: 全局变量Slot化
// ═══════════════════════════════════════════════════════════════════
// 
// 当前问题：
//   globals_[key] 使用 std::unordered_map<std::string, Value>
//   每次访问都要：构造std::string → 计算hash → 查找bucket → 比较key
//
// 优化方案：
//   1. 编译期为每个全局变量名分配一个slot索引（UInt16）
//   2. VM维护一个固定大小的全局slot数组：Value globalSlots_[65536]
//   3. 运行时直接用索引访问：globalSlots_[slot]（O(1)，无哈希）
//
// 实现步骤：
//   A. VM添加：
//      - std::vector<Value> globalSlots_
//      - std::unordered_map<std::string, UInt16> globalNameToSlot_
//      - UInt16 nextGlobalSlot_ = 0
//      - Int32 getOrCreateGlobalSlot(const char* name)
//
//   B. Codegen修改：
//      - compileIdentifier() 中，如果是全局变量，调用vm->getOrCreateGlobalSlot()
//      - 将slot索引存入VMFunction的globalSlots数组
//      - 生成新的OP_LOADGLOBAL_S/OP_STOREGLOBAL_S指令（带slot参数）
//
//   C. VM修改OP_LOADGLOBAL/OP_STOREGLOBAL：
//      - 从nameIdx查找改为直接slot访问
//      - 消除std::string构造

class GlobalSlotOptimizer {
public:
    static constexpr int MAX_GLOBAL_SLOTS = 65536;
    
    // 获取或创建全局slot
    UInt16 getOrCreateSlot(const std::string& name) {
        auto it = nameToSlot_.find(name);
        if (it != nameToSlot_.end()) return it->second;
        
        UInt16 slot = nextSlot_++;
        nameToSlot_[name] = slot;
        if (slot >= slots_.size()) slots_.resize(slot + 1);
        return slot;
    }
    
    Value* getSlot(UInt16 slot) {
        if (slot < slots_.size()) return &slots_[slot];
        return nullptr;
    }
    
private:
    std::vector<Value> slots_;
    std::unordered_map<std::string, UInt16> nameToSlot_;
    UInt16 nextSlot_ = 0;
};

// ═══════════════════════════════════════════════════════════════════
//  优化2: 指令预取+边界检查消除
// ═══════════════════════════════════════════════════════════════════
//
// 当前问题：
//   while (ctx->pc < ctx->codeSize) {  // 每条指令都检查边界
//       UInt8 op = ctx->code[ctx->pc++];
//       ...
//   }
//
// 优化方案：
//   A. 在函数开始处检查一次：确保codeSize > 0
//   B. 使用sentinel指令：在code末尾添加OP_HALT
//   C. 跳转指令统一处理边界（只有跳转可能越界）
//
// 伪代码：
//   // 函数入口检查
//   if (ctx->codeSize == 0) return true;
//   
//   // 确保末尾有HALT
//   if (ctx->code[ctx->codeSize-1] != OP_HALT) {
//       ctx->code.push_back(OP_HALT);
//   }
//   
//   // 主循环（无边界检查）
//   for (;;) {
//       UInt8 op = *pc++;
//       switch (op) {
//           case OP_RETURN: return true;
//           case OP_HALT: return true;
//           case OP_JUMP: {
//               Int32 offset = ...;
//               pc += offset;
//               // 可选：检查pc是否在有效范围
//               continue;
//           }
//           ...
//       }
//   }

// ═══════════════════════════════════════════════════════════════════
//  优化3: 热点函数内联
// ═══════════════════════════════════════════════════════════════════
//
// 当前问题：
//   Value::asInt(), Value::isTrue() 等简单函数没有内联
//
// 优化方案：
//   A. 在vm.hpp中将这些函数标记为inline
//   B. 在vm.cpp中确保编译器能看到定义（或移到hpp）
//
// 例如：
//   inline bool isTrue() const {  // 加inline关键字
//       if (tag == T_BOOL) return i != 0;
//       ...
//   }

// ═══════════════════════════════════════════════════════════════════
//  优化4: 数组操作优化
// ═══════════════════════════════════════════════════════════════════
//
// 当前问题：
//   VMArray::get/set 有边界检查，且返回Value（拷贝）
//
// 优化方案：
//   A. 提供不检查边界的版本（VM确保不越界）
//   B. 返回Value&引用而非Value拷贝
//
//   Value& fastGet(Int64 index) { return data[index]; }
//   void fastSet(Int64 index, const Value& v) { data[index] = v; }

// ═══════════════════════════════════════════════════════════════════
//  优化5: 字符串驻留优化
// ═══════════════════════════════════════════════════════════════════
//
// 当前问题：
//   每次创建字符串都分配内存，即使内容相同
//
// 已有优化：
//   stringTable_ 实现了字符串驻留
//
// 可改进：
//   A. 使用更高效的hash表（如robin_hood_hashing）
//   B. 短字符串优化（SSO）：<15字符直接存对象内

// ═══════════════════════════════════════════════════════════════════
//  优化6: 寄存器分配优化
// ═══════════════════════════════════════════════════════════════════
//
// 当前问题：
//   allocReg()只递增nextReg_，不回收临时寄存器
//   长程序会导致寄存器溢出
//
// 已有优化：
//   releaseTempRegs()在语句边界回收
//
// 可改进：
//   A. 基本块级别的寄存器分配
//   B. 图着色寄存器分配（复杂但高效）

// ═══════════════════════════════════════════════════════════════════
//  优化7: 内联缓存（Inline Cache）
// ═══════════════════════════════════════════════════════════════════
//
// 对于动态类型语言，属性访问可以用内联缓存：
//
//   struct InlineCache {
//       VMClass* cachedClass;  // 上次访问的类
//       int cachedOffset;      // 字段偏移
//   };
//
//   OP_GETFIELD: {
//       auto* obj = ...;
//       auto* cache = &inlineCaches[pc - code.begin()];
//       if (obj->cls == cache->cachedClass) {
//           *ra = obj->fields[cache->cachedOffset];  // 快速路径
//       } else {
//           // 慢路径：查找字段并更新缓存
//       }
//   }

// ═══════════════════════════════════════════════════════════════════
//  优化8: JIT编译（终极方案）
// ═══════════════════════════════════════════════════════════════════
//
// 对于热点代码，编译为机器码：
//
//   1. 追踪热点（如执行超过1000次的循环）
//   2. 将字节码编译为x86机器码
//   3. 使用libgccjit或手写汇编生成器
//
// 简单JIT示例（伪代码）：
//   void* jitCode = mmap(PROT_EXEC);
//   emit_mov_r64_imm(jitCode, RAX, (uint64_t)&globalSlots_[0]);
//   emit_mov_r64_mem(jitCode, RBX, RAX, slot * sizeof(Value));
//   emit_ret(jitCode);

} // namespace cplang

#endif // VM_OPTIMIZATIONS_HPP
