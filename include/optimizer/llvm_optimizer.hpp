#pragma once
#include <string>
#include <vector>
#include <memory>

namespace cplang {

// LLVM 优化 Pass 配置
struct LLVMOptConfig {
    // 优化级别 (0-3)
    int optLevel = 2;
    
    // 是否启用特定 Pass
    bool enableMem2Reg = true;       // 提升栈变量到寄存器
    bool enableInline = true;        // 函数内联
    bool enableGVN = true;           // 全局值编号（消除冗余）
    bool enableLICM = true;          // 循环不变量外提
    bool enableLoopUnroll = true;    // 循环展开
    bool enableLoopVectorize = true; // 循环向量化
    bool enableSLPVectorize = true;  // SLP 向量化
    bool enableDeadCodeElim = true;  // 死代码消除
    bool enableAggressiveDCE = true; // 激进死代码消除
    bool enableCFGSimplification = true; // 控制流图简化
    bool enablePromoteMemory = true; // 内存到寄存器
    bool enableReassociate = true;   // 重新关联表达式
    bool enableEarlyCSE = true;      // 早期公共子表达式消除
    bool enableInstCombine = true;   // 指令合并
    bool enableSROA = true;          // 标量替换聚合
    
    // 目标架构
    std::string targetTriple = "x86_64-pc-windows-msvc";
    
    // 是否打印优化后的 IR
    bool printOptimizedIR = false;
    
    // 是否打印优化统计
    bool printStats = false;
    
    // === PGO (Profile Guided Optimization) 配置 ===
    bool enablePGO = false;
    enum PGOMode {
        PGO_NONE = 0,
        PGO_GEN = 1,  // 生成 profile
        PGO_USE = 2   // 使用 profile 优化
    } pgoMode = PGO_NONE;
    std::string profileFile;  // profile 数据文件路径
    
    // === LTO (Link Time Optimization) 配置 ===
    bool enableLTO = false;
    enum LTOMode {
        LTO_NONE = 0,
        LTO_FULL = 1,  // 完全 LTO
        LTO_THIN = 2   // Thin LTO（推荐，速度更快）
    } ltoMode = LTO_NONE;
    int ltoPartitions = 0;  // Thin LTO 分区数（0=自动）
    
    // === 代码生成配置 ===
    bool generateDebugInfo = false;
    bool generateUnwindTables = true;
    bool optimizeForSize = false;  // -Oz (优化大小而非速度)
};

// LLVM 优化统计
struct LLVMOptStats {
    int functionsInlined = 0;
    int mem2RegPromoted = 0;
    int deadCodeEliminated = 0;
    int loopsUnrolled = 0;
    int loopsVectorized = 0;
    int instsCombined = 0;
    int cseEliminated = 0;
    int cfgSimplified = 0;
};

// LLVM 优化 Pass 管理器
// 封装 LLVM 的优化管线，提供高级配置接口
class LLVMOptimizer {
public:
    LLVMOptimizer();
    ~LLVMOptimizer();
    
    // 配置优化
    void setConfig(const LLVMOptConfig& config);
    const LLVMOptConfig& getConfig() const { return config_; }
    
    // 优化 LLVM IR 文件
    bool optimizeIR(const std::string& inputFile, const std::string& outputFile);
    
    // 优化 LLVM 模块（内存中）
    bool optimizeModule(void* llvmModule);
    
    // 获取优化统计
    const LLVMOptStats& getStats() const { return stats_; }
    
    // 打印统计
    void printStats() const;
    
    // 获取 Pass 列表
    std::vector<std::string> getPassList() const;
    
    // 验证 IR
    bool verifyIR(const std::string& irFile);
    
    // 运行特定 Pass
    bool runPass(const std::string& passName, const std::string& inputFile, const std::string& outputFile);
    
private:
    LLVMOptConfig config_;
    LLVMOptStats stats_;
    
    // 构建优化管线
    std::string buildOptPipeline() const;
    
    // 解析 opt 输出
    void parseOptOutput(const std::string& output);
    
    // 检查 opt 是否可用
    bool isOptAvailable() const;
    
    // 获取 opt 路径
    std::string getOptPath() const;
};

// LLVM Pass 名称常量
namespace LLVMPass {
    constexpr const char* MEM2REG = "mem2reg";
    constexpr const char* INLINE = "inline";
    constexpr const char* GVN = "gvn";
    constexpr const char* LICM = "licm";
    constexpr const char* LOOP_UNROLL = "loop-unroll";
    constexpr const char* LOOP_VECTORIZE = "loop-vectorize";
    constexpr const char* SLP_VECTORIZE = "slp-vectorizer";
    constexpr const char* DCE = "dce";
    constexpr const char* ADCE = "adce";
    constexpr const char* CFG_SIMPLIFY = "simplifycfg";
    constexpr const char* PROMOTE_MEM = "promote-memory-to-register";
    constexpr const char* REASSOCIATE = "reassociate";
    constexpr const char* EARLY_CSE = "early-cse";
    constexpr const char* INST_COMBINE = "instcombine";
    constexpr const char* SROA = "sroa";
    constexpr const char* DEAD_ARG_ELIM = "deadargelim";
    constexpr const char* GLOBAL_DCE = "globaldce";
    constexpr const char* STRIP_DEAD_PROTOS = "strip-dead-prototypes";
    constexpr const char* MERGE_FUNC = "mergefunc";
    constexpr const char* CONST_MERGE = "constmerge";
    constexpr const char* CONST_PROP = "constprop";
    constexpr const char* SCCP = "sccp";
    constexpr const char* BDCE = "bdce";
    constexpr const char* CORRECT_ESCAPING = "correct-escaping";
    
    // === PGO 相关 ===
    constexpr const char* PGO_INSTRUMENT = "pgo-instrumentation";
    constexpr const char* PGO_USE = "pgo-profile-use";
    
    // === LTO 相关 ===
    constexpr const char* LTO_INTERNALIZE = "lto-internalize";
    constexpr const char* LTO_PROMOTE = "lto-promote";
    constexpr const char* THINLTO = "thinlto-pre-link";
}

} // namespace cplang
