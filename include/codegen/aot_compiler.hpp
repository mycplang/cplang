// CP语言 AOT 编译器 - 将CP代码直接编译为原生可执行文件
#pragma once
#include "common/types.hpp"
#include "ast/ast.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace cplang {

// AOT 编译器配置
struct AOTConfig {
    OptLevel optLevel = OptLevel::O2;
    String outputFile = "a.out";
    bool emitLLVM = false;  // 只输出LLVM IR
    bool emitAssembly = false;  // 输出汇编
    bool emitObject = true;  // 输出目标文件
    String targetTriple = "";  // 目标三元组（空为默认）
    bool debugInfo = false;
    bool pureMath = false;  // 纯数学模式：跳过 NaN-boxing 分派和运行时调用
    String llvmToolsDir = "";  // LLVM工具链目录(空=自动检测)
    String msvcToolsDir = "";  // MSVC工具链目录(空=自动检测)
    String winKitsDir = "";    // Windows Kits目录(空=自动检测)
};

// AOT 编译结果
struct AOTResult {
    bool success = false;
    String errorMessage;
    String outputFile;
    size_t codeSize = 0;
};

// AOT 编译器类
class AOTCompiler {
public:
    AOTCompiler();
    ~AOTCompiler();
    
    // 从源文件编译
    AOTResult compileFile(const String& filename, const AOTConfig& config = AOTConfig());
    
    // 从源代码字符串编译
    AOTResult compileSource(const String& source, const AOTConfig& config = AOTConfig());
    
    // 从AST编译
    AOTResult compileAST(Shared<Program> ast, const AOTConfig& config = AOTConfig());
    
private:
    // 内部编译步骤
    bool generateLLVMIR(Shared<Program> ast, String& ir, const AOTConfig& config);
    bool compileIRToObject(const String& ir, const String& objFile, const AOTConfig& config);
    bool linkToExecutable(const std::vector<String>& objFiles, const String& exeFile, const AOTConfig& config);
    
    // 平台特定的链接
    String getDefaultOutputExtension();
    String getDefaultTargetTriple();
    std::vector<String> getDefaultLinkLibraries();
    
    // MSVC 路径检测
    struct MSVCPaths {
        String msvcLib;
        String msvcBin;
        String msvcInclude;
        String ucrtLib;
        String umLib;
        String ucrtInclude;
        String umInclude;
        String sharedInclude;
    };
    static MSVCPaths detectMSVCPaths();
    
    // 使用预编译的 build/jit_runtime.lib（由 build_msvc.bat 生成）
    
    // 生成 main 包装器
    bool generateMainWrapper(const String& wrapperLl, const String& wrapperObj, 
                             const String& irContent, const AOTConfig& config);
    
    // 工具函数
    String getTempFile(const String& suffix);
    void cleanupTempFiles();
    
    // 临时文件列表
    std::vector<String> tempFiles_;
};

} // namespace cplang
