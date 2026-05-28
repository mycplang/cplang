// 字节码优化器
// 在字节码层面进行优化，提升虚拟机执行效率

#pragma once
#include "common/types.hpp"
#include "vm/vm.hpp"
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace cplang {

// 字节码优化统计
struct BytecodeOptStats {
    int64_t peepholesApplied = 0;       // 窥孔优化次数
    int64_t deadInstructionsRemoved = 0;  // 死指令移除数
    int64_t constantPropagations = 0;     // 常量传播次数
    int64_t registersAllocated = 0;       // 寄存器分配数
    int64_t totalBytesSaved = 0;          // 节省的字节数
};

// 字节码优化器
class BytecodeOptimizer {
public:
    BytecodeOptimizer(OptLevel level = OptLevel::O2) : level_(level) {}
    
    // 优化字节码函数
    VMFunction* optimize(VMFunction* func);
    
    // 获取统计
    const BytecodeOptStats& getStats() const { return stats_; }
    
    // 打印统计
    void printStats() const;
    
private:
    OptLevel level_;
    BytecodeOptStats stats_;
    
    // === 优化通道 ===
    
    // 1. 窥孔优化（Peephole Optimization）
    // 查找并替换常见指令模式
    void peepholeOptimize(VMFunction* func);
    
    // 2. 死指令消除
    // 移除不会被执行的指令
    void eliminateDeadInstructions(VMFunction* func);
    
    // 3. 常量传播
    // 传播已知的常量值
    void propagateConstants(VMFunction* func);
    
    // 4. 基本块重排
    // 优化指令缓存局部性
    void reorderBasicBlocks(VMFunction* func);
    
    // 5. 寄存器分配（为未来寄存器VM预留）
    void allocateRegisters(VMFunction* func);
    
    // === 辅助函数 ===
    
    // 分析控制流图
    struct BasicBlock {
        size_t start;        // 起始偏移
        size_t end;          // 结束偏移
        std::vector<size_t> predecessors;  // 前驱块
        std::vector<size_t> successors;    // 后继块
    };
    std::vector<BasicBlock> buildCFG(VMFunction* func);
    
    // 标记可达代码
    std::unordered_set<size_t> findReachableCode(VMFunction* func, const std::vector<BasicBlock>& cfg);
    
    // 合并连续LOAD
    void mergeLoads(VMFunction* func);
    
    // 移除冗余STORE
    void removeRedundantStores(VMFunction* func);
};

} // namespace cplang
