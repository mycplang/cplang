// LLVM 优化演示
// 展示 LLVM opt 的优化效果

#include <iostream>
#include <chrono>
#include <fstream>
#include <sstream>
#include "optimizer/llvm_optimizer.hpp"

using namespace cplang;

// 性能测试函数
void benchmarkOptimization(const std::string& irFile) {
    std::cout << "\n╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║        性能对比                                      ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n\n";
    
    LLVMOptimizer optimizer;
    
    // 测试不同优化级别
    for (int level = 0; level <= 3; level++) {
        LLVMOptConfig config;
        config.optLevel = level;
        config.printStats = true;
        optimizer.setConfig(config);
        
        std::string outputFile = "benchmark_O" + std::to_string(level) + ".ll";
        
        std::cout << "O" << level << " 优化:\n";
        
        auto start = std::chrono::high_resolution_clock::now();
        bool success = optimizer.optimizeIR(irFile, outputFile);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        if (success) {
            std::cout << "  时间: " << time << " ms\n";
            
            // 统计 IR 大小
            std::ifstream in(outputFile);
            std::stringstream buffer;
            buffer << in.rdbuf();
            std::string content = buffer.str();
            
            std::cout << "  IR 大小: " << content.size() << " 字节\n";
            
            // 统计指令数
            int instCount = 0;
            size_t pos = 0;
            while ((pos = content.find('\n', pos)) != std::string::npos) {
                instCount++;
                pos++;
            }
            std::cout << "  行数: " << instCount << "\n";
        } else {
            std::cout << "  优化失败\n";
        }
        
        std::cout << "\n";
    }
}

// Pass 分析
void analyzePasses(const std::string& irFile) {
    std::cout << "\n╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║        Pass 分析                                     ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n\n";
    
    LLVMOptimizer optimizer;
    auto passes = optimizer.getPassList();
    
    std::cout << "可用 Pass 列表:\n";
    for (const auto& pass : passes) {
        std::cout << "  - " << pass << "\n";
    }
    
    std::cout << "\n逐 Pass 优化效果:\n";
    
    for (const auto& pass : passes) {
        std::string outputFile = "pass_" + pass + ".ll";
        
        auto start = std::chrono::high_resolution_clock::now();
        bool success = optimizer.runPass(pass, irFile, outputFile);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        
        if (success) {
            std::cout << "  " << pass << ": " << time << " μs\n";
        }
    }
}

int main(int argc, char* argv[]) {
    std::cout << "╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║        LLVM 优化演示                                 ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n\n";
    
    // 默认 IR 文件
    std::string irFile = "benchmark.ll";
    if (argc > 1) {
        irFile = argv[1];
    }
    
    // 检查文件是否存在
    std::ifstream test(irFile);
    if (!test.good()) {
        std::cout << "错误: 找不到文件 " << irFile << "\n";
        std::cout << "用法: " << argv[0] << " [IR文件]\n";
        return 1;
    }
    test.close();
    
    // 创建优化器
    LLVMOptimizer optimizer;
    
    // 配置优化
    LLVMOptConfig config;
    config.optLevel = 2;
    config.enableMem2Reg = true;
    config.enableInline = true;
    config.enableGVN = true;
    config.enableLICM = true;
    config.enableLoopUnroll = true;
    config.enableLoopVectorize = true;
    config.enableDeadCodeElim = true;
    config.enableInstCombine = true;
    config.printStats = true;
    config.printOptimizedIR = false;
    
    optimizer.setConfig(config);
    
    std::cout << "优化配置:\n";
    std::cout << "  优化级别: O" << config.optLevel << "\n";
    std::cout << "  目标架构: " << config.targetTriple << "\n";
    std::cout << "\n启用的 Pass:\n";
    std::cout << "  - mem2reg (内存到寄存器)\n";
    std::cout << "  - inline (函数内联)\n";
    std::cout << "  - gvn (全局值编号)\n";
    std::cout << "  - licm (循环不变量外提)\n";
    std::cout << "  - loop-unroll (循环展开)\n";
    std::cout << "  - loop-vectorize (循环向量化)\n";
    std::cout << "  - dce (死代码消除)\n";
    std::cout << "  - instcombine (指令合并)\n";
    
    // 优化 IR
    std::string outputFile = "optimized.ll";
    
    std::cout << "\n优化中...\n";
    
    auto start = std::chrono::high_resolution_clock::now();
    bool success = optimizer.optimizeIR(irFile, outputFile);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    if (success) {
        std::cout << "优化成功！\n";
        std::cout << "时间: " << time << " ms\n";
        std::cout << "输出: " << outputFile << "\n";
        
        // 打印统计
        optimizer.printStats();
        
        // 验证 IR
        if (optimizer.verifyIR(outputFile)) {
            std::cout << "\n✓ IR 验证通过\n";
        } else {
            std::cout << "\n✗ IR 验证失败\n";
        }
    } else {
        std::cout << "优化失败！\n";
    }
    
    // Pass 分析
    analyzePasses(irFile);
    
    // 性能对比
    benchmarkOptimization(irFile);
    
    // Pass 说明
    std::cout << "\n\n╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║        Pass 说明                                     ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "mem2reg:\n";
    std::cout << "  将栈变量提升到 SSA 寄存器\n";
    std::cout << "  消除 alloca/load/store 指令\n\n";
    
    std::cout << "inline:\n";
    std::cout << "  函数内联\n";
    std::cout << "  消除函数调用开销\n\n";
    
    std::cout << "gvn:\n";
    std::cout << "  全局值编号\n";
    std::cout << "  消除冗余计算\n\n";
    
    std::cout << "licm:\n";
    std::cout << "  循环不变量外提\n";
    std::cout << "  将循环内不变的计算移到循环外\n\n";
    
    std::cout << "loop-unroll:\n";
    std::cout << "  循环展开\n";
    std::cout << "  减少循环控制开销\n\n";
    
    std::cout << "loop-vectorize:\n";
    std::cout << "  循环向量化\n";
    std::cout << "  使用 SIMD 指令\n\n";
    
    std::cout << "instcombine:\n";
    std::cout << "  指令合并\n";
    std::cout << "  简化指令序列\n\n";
    
    std::cout << "dce/adce:\n";
    std::cout << "  死代码消除\n";
    std::cout << "  移除无用指令\n";
    
    std::cout << "\n完成！\n";
    return 0;
}
