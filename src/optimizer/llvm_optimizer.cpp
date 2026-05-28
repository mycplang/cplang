// LLVM 优化 Pass 管理器实现

#include "optimizer/llvm_optimizer.hpp"
#include "core/verbose.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <regex>

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  构造与析构
// ═══════════════════════════════════════════════════════════════════

LLVMOptimizer::LLVMOptimizer() {
    // 默认配置
    config_.optLevel = 2;
    config_.targetTriple = "x86_64-pc-windows-msvc";
}

LLVMOptimizer::~LLVMOptimizer() = default;

// ═══════════════════════════════════════════════════════════════════
//  配置
// ═══════════════════════════════════════════════════════════════════

void LLVMOptimizer::setConfig(const LLVMOptConfig& config) {
    config_ = config;
}

// ═══════════════════════════════════════════════════════════════════
//  构建优化管线
// ═══════════════════════════════════════════════════════════════════

std::string LLVMOptimizer::buildOptPipeline() const {
    std::ostringstream pipeline;
    
    // 优化级别前缀
    switch (config_.optLevel) {
        case 0:
            // O0: 无优化
            return "";
        case 1:
            pipeline << "-O1 ";
            break;
        case 2:
            pipeline << "-O2 ";
            break;
        case 3:
            pipeline << "-O3 ";
            break;
        default:
            pipeline << "-O2 ";
            break;
    }
    
    // 添加特定 Pass
    std::vector<std::string> passes;
    
    if (config_.enableSROA) passes.push_back(LLVMPass::SROA);
    if (config_.enableEarlyCSE) passes.push_back(LLVMPass::EARLY_CSE);
    if (config_.enableMem2Reg) passes.push_back(LLVMPass::MEM2REG);
    if (config_.enableInstCombine) passes.push_back(LLVMPass::INST_COMBINE);
    if (config_.enableReassociate) passes.push_back(LLVMPass::REASSOCIATE);
    if (config_.enableInline) passes.push_back(LLVMPass::INLINE);
    if (config_.enableGVN) passes.push_back(LLVMPass::GVN);
    if (config_.enableLICM) passes.push_back(LLVMPass::LICM);
    if (config_.enableLoopUnroll) passes.push_back(LLVMPass::LOOP_UNROLL);
    if (config_.enableLoopVectorize) passes.push_back(LLVMPass::LOOP_VECTORIZE);
    if (config_.enableSLPVectorize) passes.push_back(LLVMPass::SLP_VECTORIZE);
    if (config_.enableDeadCodeElim) passes.push_back(LLVMPass::DCE);
    if (config_.enableAggressiveDCE) passes.push_back(LLVMPass::ADCE);
    if (config_.enableCFGSimplification) passes.push_back(LLVMPass::CFG_SIMPLIFY);
    
    if (!passes.empty()) {
        pipeline << "-passes=\"";
        for (size_t i = 0; i < passes.size(); i++) {
            if (i > 0) pipeline << ",";
            pipeline << passes[i];
        }
        pipeline << "\" ";
    }
    
    // 目标架构
    pipeline << "-target " << config_.targetTriple << " ";
    
    return pipeline.str();
}

// ═══════════════════════════════════════════════════════════════════
//  优化 IR 文件
// ═══════════════════════════════════════════════════════════════════

bool LLVMOptimizer::optimizeIR(const std::string& inputFile, const std::string& outputFile) {
    if (!isOptAvailable()) {
        std::cerr << "错误: LLVM opt 不可用" << std::endl;
        return false;
    }
    
    // 构建命令
    std::ostringstream cmd;
    cmd << getOptPath() << " ";
    cmd << buildOptPipeline();
    cmd << "-S ";  // 输出文本 IR
    cmd << inputFile << " ";
    cmd << "-o " << outputFile;
    
    if (config_.printStats) {
        cmd << " -stats";
    }
    
    // 执行命令
    std::string command = cmd.str();
    
    if (config_.printOptimizedIR) {
        std::cout << "执行命令: " << command << std::endl;
    }
    
    int result = std::system(command.c_str());
    
    if (result != 0) {
        std::cerr << "错误: opt 执行失败，返回码 " << result << std::endl;
        return false;
    }
    
    return true;
}

// ═══════════════════════════════════════════════════════════════════
//  运行特定 Pass
// ═══════════════════════════════════════════════════════════════════

bool LLVMOptimizer::runPass(const std::string& passName, 
                            const std::string& inputFile, 
                            const std::string& outputFile) {
    if (!isOptAvailable()) {
        std::cerr << "错误: LLVM opt 不可用" << std::endl;
        return false;
    }
    
    std::ostringstream cmd;
    cmd << getOptPath() << " ";
    cmd << "-passes=\"" << passName << "\" ";
    cmd << "-S ";
    cmd << inputFile << " ";
    cmd << "-o " << outputFile;
    
    std::string command = cmd.str();
    int result = std::system(command.c_str());
    
    return result == 0;
}

// ═══════════════════════════════════════════════════════════════════
//  Pass 列表
// ═══════════════════════════════════════════════════════════════════

std::vector<std::string> LLVMOptimizer::getPassList() const {
    std::vector<std::string> passes;
    
    passes.push_back(LLVMPass::MEM2REG);
    passes.push_back(LLVMPass::INLINE);
    passes.push_back(LLVMPass::GVN);
    passes.push_back(LLVMPass::LICM);
    passes.push_back(LLVMPass::LOOP_UNROLL);
    passes.push_back(LLVMPass::LOOP_VECTORIZE);
    passes.push_back(LLVMPass::SLP_VECTORIZE);
    passes.push_back(LLVMPass::DCE);
    passes.push_back(LLVMPass::ADCE);
    passes.push_back(LLVMPass::CFG_SIMPLIFY);
    passes.push_back(LLVMPass::REASSOCIATE);
    passes.push_back(LLVMPass::EARLY_CSE);
    passes.push_back(LLVMPass::INST_COMBINE);
    passes.push_back(LLVMPass::SROA);
    
    return passes;
}

// ═══════════════════════════════════════════════════════════════════
//  验证 IR
// ═══════════════════════════════════════════════════════════════════

bool LLVMOptimizer::verifyIR(const std::string& irFile) {
    if (!isOptAvailable()) {
        return false;
    }
    
    std::ostringstream cmd;
    cmd << getOptPath() << " ";
    cmd << "-passes=verify ";
    cmd << irFile << " ";
    cmd << "-o /dev/null";
    
    std::string command = cmd.str();
    int result = std::system(command.c_str());
    
    return result == 0;
}

// ═══════════════════════════════════════════════════════════════════
//  辅助函数
// ═══════════════════════════════════════════════════════════════════

bool LLVMOptimizer::isOptAvailable() const {
    std::string path = getOptPath();
    std::string command = path + " --version > /dev/null 2>&1";
    return std::system(command.c_str()) == 0;
}

std::string LLVMOptimizer::getOptPath() const {
    // Windows 上通常在 PATH 中
    #ifdef _WIN32
    return "opt";
    #else
    return "opt";
    #endif
}

void LLVMOptimizer::parseOptOutput(const std::string& output) {
    // 解析 opt 的统计输出
    // 格式示例: "12 instcombine - Number of insts combined"
    
    std::regex statRegex("(\\d+)\\s+(\\w+)\\s+-\\s+(.+)");
    std::istringstream iss(output);
    std::string line;
    
    while (std::getline(iss, line)) {
        std::smatch match;
        if (std::regex_match(line, match, statRegex)) {
            int count = std::stoi(match[1].str());
            std::string passName = match[2].str();
            
            if (passName == "inline") {
                stats_.functionsInlined += count;
            } else if (passName == "mem2reg") {
                stats_.mem2RegPromoted += count;
            } else if (passName == "dce" || passName == "adce") {
                stats_.deadCodeEliminated += count;
            } else if (passName == "loop-unroll") {
                stats_.loopsUnrolled += count;
            } else if (passName == "loop-vectorize") {
                stats_.loopsVectorized += count;
            } else if (passName == "instcombine") {
                stats_.instsCombined += count;
            } else if (passName == "gvn" || passName == "early-cse") {
                stats_.cseEliminated += count;
            } else if (passName == "simplifycfg") {
                stats_.cfgSimplified += count;
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
//  打印统计
// ═══════════════════════════════════════════════════════════════════

void LLVMOptimizer::printStats() const {
    VERBOSE(
        std::cout << "=== LLVM 优化统计 ===" << std::endl;
        std::cout << "函数内联: " << stats_.functionsInlined << " 次" << std::endl;
        std::cout << "内存提升到寄存器: " << stats_.mem2RegPromoted << " 个变量" << std::endl;
        std::cout << "死代码消除: " << stats_.deadCodeEliminated << " 条指令" << std::endl;
        std::cout << "循环展开: " << stats_.loopsUnrolled << " 个循环" << std::endl;
        std::cout << "循环向量化: " << stats_.loopsVectorized << " 个循环" << std::endl;
        std::cout << "指令合并: " << stats_.instsCombined << " 条指令" << std::endl;
        std::cout << "公共子表达式消除: " << stats_.cseEliminated << " 次" << std::endl;
        std::cout << "控制流图简化: " << stats_.cfgSimplified << " 次" << std::endl;
    );
}

} // namespace cplang
